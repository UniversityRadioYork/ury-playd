# Visual Studio Build Notes

Here are some field notes for building `playd` with Microsoft Visual Studio.

**NOTE**: Much of this is from memory and is in need of filling out with
more details.

## Visual Studio Versions

At time of writing, `playd` has been tested with Visual Studio 2015 (version
14).  As `playd` needs a C++11 compiler, earlier versions will likely fail;
newer versions may work, but this is not guaranteed.

## Assembling Libraries and Includes

The Visual Studio project provided (`playd.vcxproj`) expects import libraries
(`.lib` files) and library headers (`.h`, `.hpp`, `.hxx`) to be collected in
the `lib` and `include` subdirectories respectively.

The `lib` directory should include:

* `libmpg123-0.lib`, from building libmpg123 (details TBA);
* `libsndfile-1.lib`, from the libsndfile Windows distribution;
* `libuv.lib`, from building libuv as below;
* `SDL2.lib` and `SDLmain.lib` from SDL2 (if you are building a `Debug` version
  of playd, you **will** need to build SDL2 from source--the Visual Studio
  pre-packaged lib is built for `Release` only and **will** give you linker
  errors!)

The `include` directory should include:

* `mpg123.h` from libmpg123;
* The contents of SDL2's `include` directory (better safe than sorry);
* `sndfile.h` and `sndfile.hh` from libsndfile;
* These headers from libuv's `include` directory:
  * `tree.h`
  * `uv.h`
  * `uv-errno.h`
  * `uv-threadpool.h`
  * `uv-version.h`
  * `uv-win.h`

## LibUV

This _must_ be built as a shared library (`vcbuild.bat shared`).  Otherwise,
this should work fine.
