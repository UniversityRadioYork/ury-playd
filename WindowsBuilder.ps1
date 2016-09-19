param (
	[switch][alias("deps")]$arg_deps,
	[switch][alias("playd")]$arg_playd,
	[string][alias("arch")]$arg_arch
)

function Write-Yellow($message) {
	write-host "`n$message`n" -ForegroundColor Yellow
}

function Path-Windows-to-Cygwin($path) {
	return $path.Replace("\", "\\\")
}

function BuildDeps ($arch, $downloads, $libdir, $includedir, $build, $sh)
{
	Write-Yellow "Building dependencies for ury-playd on $arch..."
	$oldpwd = $pwd
	cd "$downloads"

	# These screw up libuv searching for MSVC.
	Remove-Item Env:\VCINSTALLDIR -ErrorAction SilentlyContinue
	Remove-Item Env:\WindowsSDKDir -ErrorAction SilentlyContinue

	echo "cmake on AppVeyor"
	cmake -version

	switch($arch) {
		"x86" {
			$url_libsndfile = "http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.27-w32.zip"
			$url_mpg123 = "https://www.mpg123.de/download/win32/mpg123-1.23.6-x86.zip"
		}
		"x64" {
			$url_libsndfile = "http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.27-w64.zip"
			$url_mpg123 = "https://www.mpg123.de/download/win64/mpg123-1.23.6-x86-64.zip"
		}
	}
	$url_sdl2 = "https://www.libsdl.org/release/SDL2-devel-2.0.4-VC.zip"
	$url_libuv = "https://github.com/libuv/libuv.git"
	$url_mingw = "https://sourceforge.net/projects/mingw-w64/files/mingw-w64/mingw-w64-release/mingw-w64-v4.0.6.zip"

	$releasedir = "$build\Release"
	mkdir -Force "$releasedir"

	$wc = New-Object System.Net.WebClient

	Write-Yellow "Downloading MPG123 DLL..."
	$f = "$([System.IO.Path]::GetFileName($url_mpg123))"
	$wc.DownloadFile("$url_mpg123", "$downloads\$f")
	7z e -o"$releasedir" "$f" "mpg123*/libmpg123*.dll"
	7z e -o"$releasedir" "$f" "mpg123*/libmpg123*.def"
	7z e -o"$includedir" "$f" "mpg123*/*.h"
	7z e -o"$releasedir" "$f" "mpg123*/COPYING.txt"
	cp "$releasedir\COPYING.txt" "$releasedir\LICENSE.MPG123"
	rm -Force "$releasedir\COPYING.txt"

	Write-Yellow "Creating MPG123 lib..."
	cd "$releasedir"
	mv "libmpg123-0.dll.def" "libmpg123-0.def"
	lib /def:"libmpg123-0.def" /out:"$libdir\libmpg123-0.lib" /machine:"$arch"
	rm "libmpg123-0.def"

	Write-Yellow "Downloading sndfile..."
	cd "$downloads"
	$f = "$([System.IO.Path]::GetFileName($url_libsndfile))"
	$wc.DownloadFile("$url_libsndfile", "$downloads\$f")
	7z e -o"$libdir" "$f" "lib/*.lib" -r
	7z e -o"$includedir" "$f" "include/*" -r
	7z e -o"$releasedir" "$f" "bin/*.dll"

	Write-Yellow "Downloading SDL2..."
	$f = "$([System.IO.Path]::GetFileName($url_sdl2))"
	$wc.DownloadFile("$url_sdl2", "$downloads\$f")
	7z e -o"$libdir" "$f" "SDL2-*/lib/$arch/*.lib" -r
	7z e -o"$includedir" "$f" "SDL2-*/include/*" -r
	7z e -o"$releasedir" "$f" "SDL2-*/COPYING.txt" "SDL2-*/lib/$arch/*.dll"
	cp "$releasedir\COPYING.txt" "$releasedir\LICENSE.SDL2"
	rm -Force "$releasedir\COPYING.txt"

	Write-Yellow "Downloading libuv..."
	git clone "$url_libuv"
	Write-Yellow "Compiling libuv..."
	cd "libuv"
	cmd /c "vcbuild.bat" "$arch" "release" "shared"
	cp "Release/*.lib" "$libdir/"
	cp "include/*" "$includedir/"
	cp "Release/*.dll" "$releasedir/"
	cp "LICENSE" "$releasedir/LICENSE.libuv"
	cd "$oldpwd"
}

function BuildPlayd ($arch, $archdir, $build)
{
	Write-Yellow "Building ury-playd on $arch..."
	$oldpwd = $pwd
	cd "$build"

	switch($arch) {
		"x86" { $cmake_generator = "Visual Studio 14 2015"; $msbuild_platform = "Win32" }
		"x64" { $cmake_generator = "Visual Studio 14 2015 Win64"; $msbuild_platform = "x64" }
	}
	Write-Yellow "Running cmake..."
	cmake "$project" -G "$cmake_generator" -DCMAKE_PREFIX_PATH="$archdir"
	Write-Yellow "Running msbuild..."
	msbuild playd.sln /p:Configuration="Release" /toolsversion:14.0 /p:Platform="$msbuild_platform" /p:PlatformToolset=v140
	cd "$oldpwd"
}

function Load-MSVC-Vars
{
	#Set environment variables for Visual Studio Command Prompt
	pushd "$env:VS140COMNTOOLS"
	cmd /c "vsvars32.bat&set" |
	foreach {
	  if ($_ -match "=") {
		$v = $_.split("="); set-item -force -path "ENV:\$($v[0])"  -value "$($v[1])"
	  }
	}
	popd
	Write-Yellow "Visual Studio 2015 Command Prompt variables set."
}

# Main
switch($arg_arch) {
	"" { $arg_arch = "x86" }
	"x86" {}
	"x64" {}
	default { throw "Invalid architecture '$arg_arch'" }
}

Load-MSVC-Vars

$project = "$pwd"
$archdir = "$project\$arg_arch"
$deps = "$archdir\deps"
$build = "$archdir\build"
$libdir = "$archdir\lib"
$includedir = "$archdir\include"
$patch = "$project\patch"

Write-Yellow "Making directories..."
mkdir -Force "$deps"
mkdir -Force "$build"
mkdir -Force "$libdir"
mkdir -Force "$includedir"

if ($arg_deps) {
	BuildDeps $arg_arch $deps $libdir $includedir $build $arg_sh
}
if ($arg_playd) {
	BuildPlayd $arg_arch $archdir $build
}
if (!($arg_deps -or $arg_playd)) {
	BuildDeps $arg_arch $deps $libdir $includedir $build $arg_sh
	BuildPlayd $arg_arch $archdir $build
}