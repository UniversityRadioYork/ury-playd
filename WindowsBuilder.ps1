$project="$pwd"
$deps="$project\deps"
$build="$project\build"
$libdir="$project\lib"
$includedir="$project\include"
mkdir -Force "$deps"
mkdir -Force "$build"
mkdir -Force "$libdir"
mkdir -Force "$includedir"

function BuildDeps
{
	# These screw up libuv searching for MSVC.
	Remove-Item Env:\VCINSTALLDIR -ErrorAction SilentlyContinue
	Remove-Item Env:\WindowsSDKDir -ErrorAction SilentlyContinue

	echo "cmake on AppVeyor"
	cmake -version

	cd "$deps"
	Invoke-WebRequest "http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.27-w64.zip" -OutFile "libsndfile-1.0.27-w64.zip"
	7z e -o"$libdir" "libsndfile-1.0.27-w64.zip" "lib/*.lib" -r
	7z e -o"$includedir" "libsndfile-1.0.27-w64.zip" "include/*" -r

	Invoke-WebRequest "https://www.libsdl.org/release/SDL2-devel-2.0.4-VC.zip" -OutFile "SDL2-devel-2.0.4-VC.zip"
	7z e -o"$libdir" "SDL2-devel-2.0.4-VC.zip" "SDL2-2.0.4/lib/x64/*.lib" -r
	7z e -o"$includedir" "SDL2-devel-2.0.4-VC.zip" "SDL2-2.0.4/include/*" -r

	git clone "https://github.com/libuv/libuv.git"
	cd "libuv"
	cmd /c "vcbuild.bat" "x64" "release" "shared"
	cp "Release/*.lib" "$libdir"
	cp "include/*" "$includedir"
	cd "$project"
}

function BuildPlayd
{
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

	cmake "$project" -G "Visual Studio 14 2015 Win64"
	msbuild playd.sln /p:Configuration="Release" /toolsversion:14.0 /p:Platform="x64" /p:PlatformToolset=v140
	cd "$project"
}

# Main
if (!$args[0].CompareTo("deps")) {
	BuildDeps
} elseif (!$args[0].CompareTo("playd")) {
	BuildPlayd
} else {
	BuildDeps
	BuildPlayd
}