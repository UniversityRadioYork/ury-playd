# Visual Studio Build Notes

Here are some field notes for building `playd` with Microsoft Visual Studio.

## Visual Studio Versions

At time of writing, `playd` has been tested with Visual Studio 2013 (version
12).  As `playd` needs a C++11 compiler, earlier versions will likely fail;
newer versions may work, but this is not guaranteed.

## Assembling Libraries and Includes

The Visual Studio project provided (`playd.vcxproj`) expects import libraries
(`.lib` files) and library headers (`.h`, `.hpp`, `.hxx`) to be collected in
the `lib` and `include` subdirectories respectively.

The `lib` directory should include:

* `libsox.lib`, from building libuv _as a shared library_ (see below for a
  somewhat hacky way to do this with MSVC);
* `libuv.lib`, from building libuv as below;
* `portaudio_x86.lib`, from building PortAudio from `cmake` as below;
* `portaudiocpp-vc7_1-d.lib`, from building the PortAudio C++ bindings as
  instructed below.

The `include` directory should include:

* `portaudio.h` from PortAudio's `include` directory;
* The `portaudiocpp` directory (the _whole directory_, _not_ its contents) from
  PortAudio's `bindings\cpp\include` directory;
* `sox.h` from libsox's `src` directory;
* These headers from libuv's `include` directory:
  * `tree.h`
  * `uv.h`
  * `uv-errno.h`
  * `uv-threadpool.h`
  * `uv-version.h`
  * `uv-win.h`

## PortAudio

Use cmake, and the shared library (`portaudio-x86.lib`).

The C++ bindings are _not_ built by the cmake-generated Visual Studio
solution.  However, the Visual Studio 7.1 solution in
`bindings\cpp\build\vc7_1\static_library.sln` should work once upgraded to
modern Visual Studio.

## LibUV

This _must_ be built as a shared library (`vcbuild.bat shared`).  Otherwise,
this should work fine.

## SoX

Some persuasion of the dated sources recommended by the SoX build instructions
is necessary to get them to build with modern Visual Studio.  The following
'hacks' work on 32-bit VS2013:

* libflac: Replace all `ftello`/`fseeko` with `ftell`/`fseek`;
* libsndfile: Replace all lrint with _lrint.
* In the LibSox project, change the following properties:
  * __General/General/Target Extension__ to __.dll__;
  * __General/Project Defaults/Configuration Type__ to
    __Dynamic Library (.dll)__;
  * __Linker/Input/Additional Dependencies__ should contain:
    * `winmm.lib`
    * `libflac.lib`
    * `libgsm.lib`
    * `libid3tag.lib`
    * `liblpc10.lib`
    * `libmad.lib`
    * `libmp3lame.lib`
    * `libogg.lib`
    * `libpng.lib`
    * `libsndfileg72x.lib`
    * `libsndfile-1.lib`
    * `libsndfilegsm610.lib`
    * `libspeex.lib`
    * `libvorbis.lib`
    * `libwavpack.lib`
    * `libzlib.lib`
  * __Linker/Input/Module Definition File__ should point to a valid module
    definition file (see below);
  * __Linker/General/Link Library Dependencies__ may need to be set to __No__.

### Module definition file

This is needed to make LibSoX build a `.lib` file for dynamic linking.  An
example is given in the source bundle as `libsox.def`.
