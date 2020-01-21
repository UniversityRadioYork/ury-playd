<#
.SYNOPSIS
	Script for building ury-playd and its dependencies on Windows.
.DESCRIPTION
	This script automates all the steps for building ury-playd on Windows.
	It downloads the dependencies and compiles them where necessary.
	It builds ury-playd using the downloaded import libraries and headers, and puts the built exes, library DLLs, and licenses in the same folder.

	All files for a build go in a folder named after the target architecture, e.g. x86.
	The directory containing exes is $arch\build\$configuration, e.g. x86\build\Release.

	Currently, only the Release configuration is supported (and hard-coded), because we are not building debug libraries.

	DEPENDENCIES:

	Python 2.7 is required for building libuv.
	  Set the PYTHON environment variable to the path to python.exe, or add the path to the PATH environment vairable.
	  PS C:\>$env:PYTHON = "C:\Python27\python.exe"
	Git
	  A copy of git needs to be in your PATH environment variable.
	7-Zip
	  7z.exe needs to be in your PATH environment variable.

.PARAMETER arch
	Architecture to build for: x86 or x64.
.PARAMETER deps
	Enable to build ury-playd's dependencies.
.PARAMETER playd
	Enable to build ury-playd.
.PARAMETER tests
	Enable to build ury-playd and its tests.
.PARAMETER check
	Enable to build ury-playd and its tests, and run the tests.

.EXAMPLE
	.\WindowsBuilder.ps1 -arch x64

	Builds ury-playd and its dependencies for x64.
.EXAMPLE
	.\WindowsBuilder.ps1 -arch x86 -deps

	Builds the dependencies for ury-playd for x86.
.EXAMPLE
	.\WindowsBuilder.ps1 -arch x86 -playd

	Builds ury-playd for x86. Requires the
	dependencies to have already been built.
.EXAMPLE
	.\WindowsBuilder.ps1 -arch x64 -tests

	Builds ury-playd and its tests for x64.
	Requires the dependencies to have already been built.
.EXAMPLE
	.\WindowsBuilder.ps1 -arch x64 -check

	Builds ury-playd and its tests, and runs the tests.
	Requires the dependencies to have already been built.
.EXAMPLE
	.\WindowsBuilder.ps1 -arch x64 -check -deps

	Builds ury-playd, its tests, and its dependencies, and runs the tests.
.INPUTS
	None. You cannot pipe objects to WindowsBuilder.
.OUTPUTS
	The build log.

.LINK
	https://github.com/UniversityRadioYork/ury-playd
#>

[CmdletBinding(PositionalBinding = $False)]
param([switch]$deps,
      [switch]$playd,
      [switch]$tests,
      [switch]$check,
      [Parameter(Mandatory = $True)]
      [ValidateSet('x86', 'x64')]
      [string]$arch)


function Write-Yellow ($message) {
    Write-Host "`n$message`n" -ForegroundColor Yellow
}


function Path-Windows-to-Cygwin ($path) {
    return $path.Replace("\","\\\")
}


function BuildDeps ($arch, $downloads, $libdir, $includedir, $build) {
    Write-Yellow "Building dependencies for ury-playd on $arch..."
    $oldpwd = $pwd
    cd "$downloads"

    # Check for Python 2.7, for building libuv.
    $vstring = "Python 2.7"
    Write-Host "`nChecking for $vstring..." -ForegroundColor Yellow -NoNewline
    Check-Python $vstring
    Write-Host " found!`n" -ForegroundColor Yellow

    cmake --version

    switch ($arch) {
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
    $wc.DownloadFile("$url_mpg123","$downloads\$f")
    7z e -o"$releasedir" "$f" "mpg123*/libmpg123*.dll"
    7z e -o"$releasedir" "$f" "mpg123*/libmpg123*.def"
    7z e -o"$includedir" "$f" "mpg123*/*.h"
    7z e -o"$releasedir" "$f" "mpg123*/COPYING.txt"
    cp "$releasedir\COPYING.txt" "$releasedir\LICENSE.MPG123"
    rm -Force "$releasedir\COPYING.txt"

    Write-Yellow "Creating MPG123 lib..."
    cd "$releasedir"
    mv "libmpg123-0.dll.def" "libmpg123-0.def"
    Write-Yellow "VCINSTALLDIR: $env:VCINSTALLDIR"
    cmd /c "$env:VCINSTALLDIR\Auxiliary\Build\vcvarsall.bat" "$arch" "&" lib /def:libmpg123-0.def /out:"$libdir\libmpg123-0.lib" /machine:"$arch"
    rm "libmpg123-0.def"

    Write-Yellow "Downloading sndfile..."
    cd "$downloads"
    $f = "$([System.IO.Path]::GetFileName($url_libsndfile))"
    $wc.DownloadFile("$url_libsndfile","$downloads\$f")
    7z e -o"$libdir" "$f" "lib/*.lib" -r
    7z e -o"$includedir" "$f" "include/*" -r
    7z e -o"$releasedir" "$f" "bin/*.dll"

    Write-Yellow "Downloading SDL2..."
    $f = "$([System.IO.Path]::GetFileName($url_sdl2))"
    $wc.DownloadFile("$url_sdl2","$downloads\$f")
    7z e -o"$libdir" "$f" "SDL2-*/lib/$arch/*.lib" -r
    7z e -o"$includedir" "$f" "SDL2-*/include/*" -r
    7z e -o"$releasedir" "$f" "SDL2-*/COPYING.txt" "SDL2-*/lib/$arch/*.dll"
    cp "$releasedir\COPYING.txt" "$releasedir\LICENSE.SDL2"
    rm -Force "$releasedir\COPYING.txt"

    Write-Yellow "Downloading libuv..."
    git clone "$url_libuv"
    Write-Yellow "Compiling libuv..."
    cd "libuv"

    # Disable building libuv tests (can't get the VS upgrade to work headlessly)
    (Get-Content "vcbuild.bat") | 
    Foreach-Object {
        if ($_ -match "msbuild test\\test.sln") 
        {
            #Add Lines after the selected pattern 
            "if `"%run%`"==`"`" goto exit"
        }
        $_ # send the current line to output
    } | Set-Content "vcbuild.bat"

    Write-Yellow "Generating libuv VS2017 project..."
    cmd /c "$env:VCINSTALLDIR\Auxiliary\Build\vcvarsall.bat" "$arch" "&" "vcbuild.bat" "vs2017" "$arch" "release" "shared" "nobuild"
    Write-Yellow "Upgrading libuv project to VS2019..."
    cmd /c "$env:VCINSTALLDIR\Auxiliary\Build\vcvarsall.bat" "$arch" "&" "devenv" "uv.sln" "/Upgrade"
    Write-Yellow "Building libuv..."
    cmd /c "$env:VCINSTALLDIR\Auxiliary\Build\vcvarsall.bat" "$arch" "&" "vcbuild.bat" "vs2017" "$arch" "release" "shared" "noprojgen"
    Write-Yellow "Copying libuv libs and headers..."
    cp "Release/*.lib" "$libdir/"
    cp -r "include/*" "$includedir/"
    cp "Release/*.dll" "$releasedir/"
    cp "LICENSE" "$releasedir/LICENSE.libuv"
    cd "$oldpwd"
}


function BuildPlayd ($arch, $archdir, $build, $tests, $check) {
    Write-Yellow "Building ury-playd on $arch..."
    $oldpwd = $pwd
    cd "$build"

    $cmake_generator = "Visual Studio 16 2019";
    switch ($arch) {
        "x86" { $msbuild_platform = "Win32" }
        "x64" { $msbuild_platform = "x64" }
    }
    $targets = "playd"
    if ($tests -Or $check) {
        $targets="$targets;playd_tests"
    }

    Write-Yellow "Running cmake..."
    cmd /c "`"$env:VCINSTALLDIR\Auxiliary\Build\vcvarsall.bat`" $arch & cmake `"$project`" -G `"$cmake_generator`" -A `"$msbuild_platform`" -DCMAKE_PREFIX_PATH=`"$archdir`""
    Write-Yellow "Running msbuild..."
    cmd /c "$env:VCINSTALLDIR\Auxiliary\Build\vcvarsall.bat" $arch "&" msbuild playd.sln /p:Configuration="Release" /p:Platform="$msbuild_platform" /t:"$targets" /m
    if ($check) {
        ctest --force-new-ctest-process -C Release
    }
    cd "$oldpwd"
}


function Load-MSVC-Vars {
    #Set environment variables for Visual Studio Command Prompt
    cmd /c "vswhere_usability_wrapper.cmd&set" |
    ForEach-Object {
        if ($_ -match "=") {
            $v = $_.split("="); Set-Item -Force -Path "ENV:\$($v[0])" -Value "$($v[1])"
        }
    }
    Write-Yellow "Visual Studio 2019 Command Prompt variables set."
}


# Check if an exe is Python 2.7.
function Check-Python-Version ($pypath, $vstring) {
    $python_version = & "$pypath" "--version" 2>&1
    return $python_version -match $vstring
}


# Check for Python 2.7, and throw an exception if not successful.
function Check-Python ($vstring) {
    if (Test-Path Env:\PYTHON) {
        if (!(Test-Path $env:PYTHON)) {
            throw "Invalid path to Python '$env:PYTHON' in `$env:PYTHON."
        }

        if (Check-Python-Version $env:PYTHON $vstring) {
            return
        }
        else {
            throw "Invalid Python version for '$env:PYTHON' in `$env:PYTHON. We require 2.7."
        }
    }
    if ((Get-Command "python.exe" -ErrorAction SilentlyContinue) -eq $null) {
        throw "Python not found in `$env:PATH or `$env:PYTHON."
    }
    else {
        $pypath = Get-Command "python.exe"
        if (!(Check-Python-Version $pypath $vstring)) {
            throw "Invalid Python version in `$env:PATH. We require 2.7."
        }
    }
}


# Main
Load-MSVC-Vars

$project = "$pwd"
$archdir = "$project\$arch"
$depsdir = "$archdir\deps"
$build = "$archdir\build"
$libdir = "$archdir\lib"
$includedir = "$archdir\include"
$patch = "$project\patch"

Write-Yellow "Making directories..."
mkdir -Force "$depsdir"
mkdir -Force "$build"
mkdir -Force "$libdir"
mkdir -Force "$includedir"

if ($deps) {
    BuildDeps $arch $depsdir $libdir $includedir $build
}
if ($playd -Or $tests -Or $check) {
    BuildPlayd $arch $archdir $build $tests $check
}
if (!($deps -Or $playd -Or $check -Or $tests)) {
    BuildDeps $arch $depsdir $libdir $includedir $build
    BuildPlayd $arch $archdir $build $tests $check
}
