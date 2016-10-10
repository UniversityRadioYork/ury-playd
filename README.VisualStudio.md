# Visual Studio Build Notes

Here are some field notes for building `playd` with Microsoft Visual Studio.

Visual Studio builds used to be much harder to pull off, but nowadays all
of the dependencies are nicely packaged for both x86 and x64.

**Note**: The x64 build currently has a few warnings, but it appears to work!


## Visual Studio Versions

At time of writing, `playd` has been tested with Visual Studio 2015 Update 3.
As `playd` needs a C++14 compiler, earlier versions will likely fail;
newer versions may work, but this is not guaranteed.


## The Easy Way: WindowsBuilder.ps1

Now you can sit back and relax whilst this script downloads the binary versions of all the libraries.

### Requirements

* [CMake], for generating the MSVC project;
* [Python 2.7], for building libuv;
* PowerShell (compatibility unknown, but works on 5.0)

Set the PYTHON environment variable to the path to python.exe, or add the path to the PATH environment vairable.

`C:\>$env:PYTHON = "C:\Python27\python.exe"`

### Usage

`.\WindowsBuilder.ps1 [-deps] [-playd] -arch x86|x64 [<CommonParameters>]`

Full help text: `Get-Help .\Windows-Builder.ps1`

The build will be in a directory like: `x86\build\Release\` along with all the necessary DLLs.

**Note**: Only the **Release** configuration is currently supported.
Debug will eventually be supported, but until then, you're on your own. Use Linux or something.


## The Manual Way

### Directories

Create a directory structure like this. `build\` and `cbuild\` can be named however you like.

```
build\
├── cbuild\
├── include\
└── lib\
```

### Assembling Libraries and Includes

The following instructions assume you are building an x86 _Release_ build.
If you are building a _Debug_ build, or for x64, you will probably have to
compile the following yourself, or do other weird hoop-jumping to get things
to work.

You will need:

* The 32-bit MSVC [distribution][SDL2] of SDL2 (_only if_ you are building a Release
  build; for Debug builds, you will need to compile SDL2 yourself);
* The 32-bit MSVC [distribution][libsndfile] of `libsndfile`;
* The 32-bit MSVC [distribution][libuv] of `libuv`;
* The includes, and a self-generated import library, from the `libmpg123` 32-bit binary [distribution][libmpg123].
  See below for tips.

The `lib` directory should include:

* `libmpg123-0.lib`, see [below](#libmpg123);
* `libsndfile-1.lib`, from the `libsndfile` Windows distribution;
* `libuv.lib`, from the libuv `Windows` distribution;
* `SDL2.lib` and `SDLmain.lib` from SDL2 (if you are building a `Debug` version
  of playd, you **will** need to build SDL2 from source--the Visual Studio
  pre-packaged lib is built for `Release` only and **will** give you linker
  errors!)

The `include` directory should include:

* `mpg123.h` and `fmt123.h` from libmpg123;
* The contents of SDL2's `include` directory (better safe than sorry);
* `sndfile.h` and `sndfile.hh` from libsndfile;
* These headers from `libuv`'s `include` directory:
  * `tree.h`
  * `uv.h`
  * `uv-errno.h`
  * `uv-threadpool.h`
  * `uv-version.h`
  * `uv-win.h`

#### `libmpg123`

The MSVC 2010 port of libmpg123 is in need of updating to make it work, but you can use the binary distribution.
You need to make an import library for the DLL, using the included DEF file.

* Get into a Visual Studio Command Prompt;
* Change to the `mpg123` distribution directory;
* Rename `libmpg123-0.dll.def` to `libmpg123-0.def`;
* Run `lib /def:libmpg123-0.def /OUT:libmpg123-0.lib`;
* Copy to `\lib` as above.

#### `libuv`

To get the files from the binary distribution, you can install the exe, or extract it with [7-Zip].

To compile `libuv` from [source][libuv] instead, see the official [readme][libuv-gh].
You will need [Python 2.7].

To compile the shared x86 release binary: `vcbuild.bat shared x86 release`

#### `libsndfile`

You must install the exe. Extracting it doesn't work at the time of writing, on `libsndfile v1.0.27`.

### Run CMake

In the `cbuild\` directory:

`cmake ..\..\ -G "Visual Studio 15 2015" -DCMAKE_PREFIX_PATH="absolute\path\to\build\"`

### Run MSVC

Open `build\cbuild\playd.sln`, ensure that the **Release Win32** configuration is selected, and have fun building!

[CMake]: https://cmake.org/download/
[Python 2.7]: https://www.python.org/downloads/
[libuv]: http://dist.libuv.org/dist/
[libuv-gh]: https://github.com/libuv/libuv
[SDL2]: https://www.libsdl.org/download-2.0.php
[libsndfile]: http://www.mega-nerd.com/libsndfile/#Download
[libmpg123]: https://www.mpg123.de/download/win32/?V=1&O=D
[7-Zip]: http://www.7-zip.org/download.html
