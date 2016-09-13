param (
	[switch][alias("deps")]$arg_deps,
	[switch][alias("playd")]$arg_playd,
	[string][alias("arch")]$arg_arch
)

function BuildDeps ($arch, $downloads, $libdir, $includedir)
{
	$oldpwd = $pwd
	cd "$downloads"
	echo $downloads

	# These screw up libuv searching for MSVC.
	Remove-Item Env:\VCINSTALLDIR -ErrorAction SilentlyContinue
	Remove-Item Env:\WindowsSDKDir -ErrorAction SilentlyContinue

	echo "cmake on AppVeyor"
	cmake -version

	switch($arch) {
		"x86" {
			$url_libsndfile = "http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.27-w32.zip"
		}
		"x64" {
			$url_libsndfile = "http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.27-w64.zip"
		}
	}
	$url_sdl2 = "https://www.libsdl.org/release/SDL2-devel-2.0.4-VC.zip"
	$url_libuv = "https://github.com/libuv/libuv.git"

	$f = "$([System.IO.Path]::GetFileName($url_libsndfile))"
	Invoke-WebRequest "$url_libsndfile" -OutFile "$f"
	7z e -o"$libdir" "$f" "lib/*.lib" -r
	7z e -o"$includedir" "$f" "include/*" -r

	$f = "$([System.IO.Path]::GetFileName($url_sdl2))"
	Invoke-WebRequest "$url_sdl2" -OutFile "$f"
	7z e "-o$libdir" "$f" "SDL2-*/lib/$arch/*.lib" -r
	7z e "-o$includedir" "$f" "SDL2-*/include/*" -r

	git clone "$url_libuv"
	cd "libuv"
	cmd /c "vcbuild.bat" "$arch" "release" "shared"
	cp "Release/*.lib" "$libdir"
	cp "include/*" "$includedir"
	cd "$oldpwd"
}

function BuildPlayd ($arch, $archdir, $build)
{
	$oldpwd = $pwd
	cd "$build"

	#Set environment variables for Visual Studio Command Prompt
	pushd "$env:VS140COMNTOOLS"
	cmd /c "vsvars32.bat&set" |
	foreach {
	  if ($_ -match "=") {
		$v = $_.split("="); set-item -force -path "ENV:\$($v[0])"  -value "$($v[1])"
	  }
	}
	popd
	write-host "`nVisual Studio 2015 Command Prompt variables set." -ForegroundColor Yellow

	switch($arch) {
		"x86" { $cmake_generator = "Visual Studio 14 2015"; $msbuild_platform = "Win32" }
		"x64" { $cmake_generator = "Visual Studio 14 2015 Win64"; $msbuild_platform = "x64" }
	}
	cmake "$project" -G "$cmake_generator" -DCMAKE_PREFIX_PATH="$archdir"
	msbuild playd.sln /p:Configuration="Release" /toolsversion:14.0 /p:Platform="$msbuild_platform" /p:PlatformToolset=v140
	cd "$oldpwd"
}

# Main
switch($arg_arch) {
	"" { $arg_arch = "x86" }
	"x86" {}
	"x64" {}
	default { throw "Invalid architecture '$arg_arch'" }
}

$project = "$pwd"
$archdir = "$project\$arg_arch"
$deps = "$archdir\deps"
$build = "$archdir\build"
$libdir = "$archdir\lib"
$includedir = "$archdir\include"

mkdir -Force "$deps"
mkdir -Force "$build"
mkdir -Force "$libdir"
mkdir -Force "$includedir"

if ($arg_deps) {
	BuildDeps $arg_arch $deps $libdir $includedir
}
if ($arg_playd) {
	BuildPlayd $arg_arch $archdir $build
}
if (!($arg_deps -or $arg_playd)) {
	BuildDeps $arg_arch $deps $libdir $includedir
	BuildPlayd $arg_arch $archdir $build
}