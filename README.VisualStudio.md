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

Usage: `.\WindowsBuilder.ps1 [-deps] [-playd] -arch x86|x64 [<CommonParameters>]`

Full help text: `Get-Help .\Windows-Builder.ps1`

The build will be in a directory like: `x86\build\Release\` along with all the necessary DLLs.

**Note**: Only the **Release** configuration is currently supported.
Until that's supported, you're on your own. Use Linux or something.

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

* The 32-bit MSVC distribution of SDL2 (_only if_ you are building a Release
  build; for Debug builds, you will need to compile SDL2 yourself);
* The 32-bit MSVC distribution of `libsndfile`;
* The 32-bit MSVC distribution of `libuv`;
* The includes, and a self-generated import library, from the `libmpg123` 32-bit binary distribution.
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

If you're building `libuv` from scratch, it _must_ be built as a shared library
(`vcbuild.bat shared`).  Otherwise, it should work fine.

### Run CMake

In the `cbuild\` directory:

`cmake ..\..\ -G "Visual Studio 15 2015" -DCMAKE_PREFIX_PATH="absolute\path\to\build\"`

### Run MSVC

Open `build\cbuild\playd.sln`, ensure that the **Release Win32** configuration is selected, and have fun building!
