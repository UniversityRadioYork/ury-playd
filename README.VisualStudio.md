# Visual Studio Build Notes

Here are some field notes for building `playd` with Microsoft Visual Studio.

Visual Studio builds used to be much harder to pull off, but nowadays most
of the dependencies are nicely packaged.  _Most_.

## Visual Studio Versions

At time of writing, `playd` has been tested with Visual Studio 2015 Update 3.
As `playd` needs a C++11 compiler, earlier versions will likely fail;
newer versions may work, but this is not guaranteed.

## Assembling Libraries and Includes

The following instructions assume you are building an x86 _Release_ build.
If you are building a _Debug_ build, or for x64, you will probably have to
compile the following yourself, or do other weird hoop-jumping to get things
to work.

You will need:

* The 32-bit MSVC distribution of SDL2 (_only if_ you are building a Release
  build; for Debug builds, you will need to compile SDL2 yourself);
* The 32-bit MSVC distribution of `libsndfile`;
* The 32-bit MSVC distribution of `libuv`;
* A reverse-engineered LIB from the `libmpg123` 32-bit binary distribution,
  as well as the includes from its source distribution.  See below for tips.

The Visual Studio project provided (`playd.vcxproj`) expects import libraries
(`.lib` files) and library headers (`.h`, `.hpp`, `.hxx`) to be collected in
the `lib` and `include` subdirectories respectively.

The `lib` directory should include:

* `libmpg123-0.lib`, from doing the below dark arts to `libmpg123-0.dll`;
* `libsndfile-1.lib`, from the `libsndfile` Windows distribution;
* `libuv.lib`, from the libuv `Windows` distribution;
* `SDL2.lib` and `SDLmain.lib` from SDL2 (if you are building a `Debug` version
  of playd, you **will** need to build SDL2 from source--the Visual Studio
  pre-packaged lib is built for `Release` only and **will** give you linker
  errors!)

The `include` directory should include:

* `mpg123.h` from libmpg123;
* The contents of SDL2's `include` directory (better safe than sorry);
* `sndfile.h` and `sndfile.hh` from libsndfile;
* These headers from `libuv`'s `include` directory:
  * `tree.h`
  * `uv.h`
  * `uv-errno.h`
  * `uv-threadpool.h`
  * `uv-version.h`
  * `uv-win.h`

## `libmpg123`

Trying to compile modern versions of `libmpg123` using its old MSVC2010
port is hopeless due to code rot.  Instead, a technique discovered by
combining [this](http://zdoom.org/wiki/Compile_ZDoom_on_Windows) and
[this](https://adrianhenke.wordpress.com/2008/12/05/create-lib-file-from-dll/)
seems to work:

* Get into a Visual Studio Command Prompt;
* Change to the `mpg123` distribution directory;
* Run `dumpbin /exports libmpg123-0.dll`;
* Copy the output into your favourite text editor;
* Strip down to the function names (`mpg123_`*) and add `EXPORTS` to the top
  of the file;
* Save as `libmpg123-0.def`;
* Run `lib /def:libmpg123-0.def /OUT:libmpg123-0.lib`;
* Copy to `\lib` as above.

## `libuv`

If you're building `libuv` from scratch, it _must_ be built as a shared library
(`vcbuild.bat shared`).  Otherwise, it should work fine.
