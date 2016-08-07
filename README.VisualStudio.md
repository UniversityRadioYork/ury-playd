# Build notes for MSVC 2013

Congratulations!  You have decided to embark on the wondrous and fantastic
journey of building your own copy of _URY playd_ using Visual Studio 2013.
We hope you enjoy your freshly built _playd_, but let's not get too far ahead
of ourselves--you need to build it first.

Unfortunately, Windows is an incredibly difficult platform to build for, due
to the dependencies being myriad, there being no easy way of getting them
pre-packaged for development, and there being a bit of bit-rot and niggles and
tweaks standing between you and your _playd_ build.  These build notes should
help you on the track.

## Before you start

### Conventions

* When we talk about changing directory to `\foo\bar`, this is relative to
  whichever directory you're building `playd` in.  For example, if you were
  building in `H:\playd`, `\foo\bar` means `H:\playd\foo\bar`.
* When we talk about `msbuild`, feel free to use the equivalent Visual Studio
  IDE invocation instead.
* This file is in Markdown format.  The practical upshot is that, when you see
  something in square brackets, it's a reference to a URL at the end of the
  file.

### Caveats

* This is a very laborious build that could (and should) eventually be
  automated.  One day, someone might!
* These notes assume you want a _Debug_ version of _playd_.  If and when we
  discover what needs to change to make a _Release_ version, these notes will
  be updated accordingly.
* These notes were typed out as a build of _playd_ was attempted, but there
  may be some inaccuracies.  Please file an issue if you spot any!

### Required Tools

`playd` requires several tools to build:

You will need:
  - [Visual Studio 2013 Community]: other VS2013en may work, but are untested;
  - [Python 2.7.9]: __not__ 3; other 2.7 versions are probably ok;
  - [CMake 3.1.0-rc3]: again, other versions may work;
  - [git 1.9.4]: any git will do, in likelihood.

Some general pointers as to how to get these available in a PATH:

  - Git will be in eg. `C:\Program Files (x86)\Git\bin` for Windows Git.
  - To enforce python 2, put its directory first in the path.
  - To ensure VS2013 is in the path, launch the
    _VS2013 x86 Native Tools Command Prompt_ (in the _Visual Studio Tools_
    submenu in the Visual Studio Start menu entry).
  - CMake usually puts itself in the system `PATH` during install.

Something like the following might suffice to put Git/Python2 in the path
(assuming you're using the VS2013 command prompt):

```
SET PATH=C:\Python27;C:\Program Files(x86)\Git\bin;%PATH%
```

### ksguid.lib

To build PortAudio, you will also need `ksguid.lib`.  This is available in the
[Windows SDK].

When installed, it will most likely be in
`C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Lib`.

## Building

### Sourcing _playd_ and its libraries

These are versions that are known to build properly--feel free to play around
with newer/older versions, but caveat emptor.  For the most part, you can
call the directories whatever you want, but these are the names we use to
refer to them later.

You will need:

* [playd 0.2.0]: save in `\ury-playd-0.2.0`;
* [PortAudio v19-20140130]: save in `\portaudio`;
* [libuv 1.0.2]: save in `\libuv-1.0.2`;
* [A LibSox fork] maintained for playd: save in `\sox-master`.

#### LibSoX meta-dependencies

LibSox pulls in a lot of dependency decoder libraries.  These _must_ be
saved to the given directories for the LibSox build to find them.

   - [flac 1.2.1]: save in `\flac`;
   - [libid3tag 0.15.1b]: save in `\libid3tag`;
   - [libmad 0.15.1b]: save in `\libmad`;
   - [lame 3.99.5]: save in `\lame`;
   - [libogg 1.3.2]: save in `\libogg`;
   - [libpng 1.6.15]: save in `\libpng`;
   - [libsndfile 1.0.25]: save in `\libsndfile`;
   - [speex 1.2rc1]: save in `\speex`;
   - [libvorbis 1.3.4]: save in `\libvorbis`;
   - [wavpack 4.70.0]: save in `\wavpack`;
   - [zlib 1.2.8]: save in `\zlib`.


### Make directories

Make the following directories:

1. `\ury-playd-0.2.0\Debug`;
2. `\ury-playd-0.2.0\include`;
3. `\ury-playd-0.2.0\lib`.

These will hold the `.dll` files, headers (`.h` and `.hxx`), and `.lib` files,
respectively, from the prerequisite libraries.

### LibSoX

LibSoX is the trickiest dependency to build as a shared library, which is
one of the reasons why we fork it.

#### Patch things

You may get issues relating to missing functions `ftello` and `fseeko`.
A very quick hack to fix these problems is to change some `ifdefs` in the
following locations in SoX's `LibFlac` project so that they are always true:

1. `metadata_iterators.c`;
2. `stream_decoder.c`;
3. `stream_encoder.c`.

The `ifdefs` will look something like:

```C++
#if _MSC_VER <= 1600 || defined __BORLANDC__ /* @@@ [2G limit] */
#define fseeko fseek
#define ftello ftell
#endif
```

At time of writing, it currently isn't clear why this is necessary.

You may also have to change a setting on `LibFlac` so that warnings are
not treated as errors.  In the IDE, the setting in question is
_Properties > Configuration Properties > C/C++ > Treat Warnings As Errors_.

#### Compile sox

1. Change to `\sox-master\msvc12-shared`
2. `msbuild LibSoX-shared.vcxproj /p:Configuration=Debug /p:Platform=Win32`

#### Copy SoX to playd

1. Copy `\sox-master\msvc12-shared\Debug\LibSox-shared.lib`
   to   `\ury-playd-0.2.0\lib\libsox.lib` (note the destination)
2. Copy `\sox-master\msvc12-shared\Debug\*.dll`
   to   `\ury-playd-0.2.0\Debug`
3. Copy `\sox-master\src\*.h`
   to   `\ury-playd-0.2.0\src`

### libuv

This is probably the easiest dependency.

Ensure Python is version 2 (NOT 3) and `GYP_MSVS_VERSION` is set (eg. as below.)

```
SET GYP_MSVS_VERSION=2013
```

1. Change to `\libuv-1.0.2`
2. `vcbuild.bat shared`
3. Copy `\libuv-1.0.2\Debug\libuv.lib`
   to   `\ury-playd-0.2.0\lib`
4. Copy `\libuv-1.0.2\Debug\libuv.dll`
   to   `\ury-playd-0.2.0\Debug`
5. Copy `\libuv-1.0.2\include\*.h`
   to   `\ury-playd-0.2.0\include`

### PortAudio

This one requires a bit of working around to get the C++ bindings.

1. Change to `\portaudio`
2. `cmake -G "Visual Studio 12 2013"`
3. Copy `ksguid.lib` from Windows SDK to `\portaudio` (this may be unnecessary)
4. `msbuild portaudio.sln`
5. Copy `\portaudio\Debug\portaudio_x86.lib`
   to   `\ury-playd-0.2.0\lib`
6. Copy `\portaudio\Debug\portaudio_x86.dll`
   to   `\ury-playd-0.2.0\Debug`
7. Copy `\portaudio\include\*.h`
   to   `\ury-playd-0.2.0\include`

#### C++ bindings

These aren't built by the CMake-generated solution, so we have to fish them out
of an ancient Visual Studio 7.1 project.

1. Change to `\portaudio\bindings\cpp\build\vc7_1`
2. Convert `static_library.sln` to Visual Studio 2013.
3. `msbuild static_library.sln`
4. Copy `\portaudio\bindings\cpp\portaudiocpp-vc_1-d.lib`
   to   `\ury-playd-0.2.0\lib`
5. Copy `\portaudio\bindings\cpp\include\portaudiocpp` (a directory)
   to   `\ury-playd-0.2.0\include` (preserving the directory)

## playd

Hopefully, we can now build _playd_.  If you did everything correctly above,
this should be straightforward.

1. Change to `\ury-playd-0.2.0`
2. `msbuild playd.sln`

To test, run `\ury-playd-0.2.0\Debug\playd.exe`.

[Visual Studio 2013 Community]: http://go.microsoft.com/fwlink/?LinkId=517284
                                "Download for Visual Studio 2013 Community".
[Python 2.7.9]:                 https://www.python.org/ftp/python/2.7.9/python-2.7.9.msi
                                "Download for Python 2.7.9."
[CMake 3.1.0-rc3]:              http://www.cmake.org/files/v3.1/cmake-3.1.0-rc3-win32-x86.exe
[git 1.9.4]:                   https://github.com/msysgit/msysgit/releases/download/Git-1.9.4-preview20140929/Git-1.9.4-preview20140929.exe
                                "Download for git 1.9.4."
[Windows SDK]:                  http://www.microsoft.com/en-gb/download/confirmation.aspx?id=8279
                                "Download for Windows SDK for Windows 7."
[playd 0.2.0]:                  https://github.com/UniversityRadioYork/ury-playd/archive/v0.2.0.zip
                                "Download for playd 0.2.0."
[PortAudio v19-20140130]:       http://www.portaudio.com/archives/pa_stable_v19_20140130.tgz
                                "Download for PortAudio v19-20140130."
[libuv 1.0.2]:                  https://github.com/libuv/libuv/archive/v1.0.2.zip
                                "Download for libuv 1.0.2."
[A LibSoX fork]:                https://github.com/CaptainHayashi/sox/archive/master.zip
                                "Download for a dynamically buildable distribution of LibSoX."
[flac 1.2.1]:                   http://downloads.xiph.org/releases/flac/flac-1.2.1.tar.gz
                                "Download for flac 1.2.1."
[libid3tag 0.15.1b]:            http://sourceforge.net/projects/mad/files/libid3tag/0.15.1b/libid3tag-0.15.1b.tar.gz/download
                                "Download for libid3tag 0.15.1b."
[libmad 0.15.1b]:               http://sourceforge.net/projects/mad/files/libmad/0.15.1b/libmad-0.15.1b.tar.gz/download
                                "Download for libmad 0.15.1b."
[lame 3.99.5]:                  http://sourceforge.net/projects/lame/files/lame/3.99/lame-3.99.5.tar.gz/download
                                "Download for libmp3lame 3.99.5."
[libogg 1.3.2]:                 http://downloads.xiph.org/releases/ogg/libogg-1.3.2.zip
                                "Download for libogg 1.3.2."
[libpng 1.6.15]:                http://sourceforge.net/projects/libpng/files/libpng16/1.6.15/lpng1615.zip/download
                                "Download for libpng 1.6.15."
[libsndfile 1.0.25]:            http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.25.tar.gz
                                "Download for libsndfile 1.0.25."
[speex 1.2rc1]:                 http://downloads.xiph.org/releases/speex/speex-1.2rc1.tar.gz
                                "Download for speex 1.2rc1."
[libvorbis 1.3.4]:              http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.4.zip
                                "Download for libvorbis 1.3.4."
[wavpack 4.70.0]:               http://www.wavpack.com/wavpack-4.70.0.tar.bz2
                                "Download for wavpack 4.70.0."
[zlib 1.2.8]:                   http://sourceforge.net/projects/libpng/files/zlib/1.2.8/zlib128.zip/download
                                "Download for zlib 1.2.8."
