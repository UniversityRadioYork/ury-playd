# playd

`playd` (_player daemon_) is a C++ audio
player developed by [University Radio York].  It's designed to be
minimal, hackable, and composable into bigger systems (such as our
ongoing BAPS3 project to build a new radio playout system).


## Usage

`playd DEVICE-ID [ADDRESS] [PORT]`

* Invoking `playd` with no arguments lists the various device IDs
  available to it.
* Full protocol information is available on the GitHub wiki.
* On POSIX systems, see the enclosed man page.

For command usage, see `README.commands.md`.

### Sending commands manually

To connect directly to `playd` and issue commands to it, you can use
[netcat]:

    # If you specified [ADDRESS] or [PORT], replace localhost and 1350 respectively.
    $ nc localhost 1350

On Windows, using [PuTTY] in _raw mode_ (__not__ Telnet mode) with
_Implicit CR in every LF_ switched on in the _Terminal_ options should
work.

__Do _not_ use a Telnet client (or PuTTY in telnet mode)!__  `playd` will
do weird things in the presence of Telnet-isms.


## Features

* Plays MP3s, Ogg Vorbis, FLACs and WAV files;
* Seek;
* Frequently announces the current position;
* TCP/IP interface with text protocol;
* Deliberately not much else.


## Philosophy

`playd` is developed using the following guidelines:

* Do one thing and do it well;
* Be hackable;
* Favour simplicity over performance;
* Favour simplicity over features;
* Let other programs handle the shinies.


## Compilation

You can build using the newer **CMake** scripts (recommended), or the older **`config.sh`**.

Some [ports](#ports) are provided for automatically building packages on certain operating systems and platforms.

### Requirements

* [libuv] 1.9.1+;
* [SDL2] 2.0.3;
* A C++14 compiler (recent versions of [clang], [gcc], and Visual Studio
  work);
* [CMake] 2.8+, for building (optional, but recommended).

The following dependencies are used for file format support, and you'll need at
least one of them:

* [libmpg123] 1.20.1+, for MP3 support;
* [libsndfile] 1.0.25+, for Ogg Vorbis, WAV, and FLAC support.

Certain operating systems may need additional dependencies; see the OS-specific
build instructions below.

### Acquire Dependencies

If you have a package manager, use it to install the dependencies.

#### Ubuntu (14.04+) / Debian (8+)

	root@ubuntu:/ # apt-get install build-essential libmpg123-dev libsndfile-dev libuv1-dev libsdl2-dev

Additional dependencies for building without CMake:

	root@ubuntu:/ # apt-get install pkg-config

#### FreeBSD (10+)

	root@freebsd:/ # pkg install libmpg123 libsndfile libuv sdl2
    
Additional dependencies for building without CMake:

	root@freebsd:/ # pkg install gmake pkgconf

#### macOS

All dependencies are available in [homebrew].

	URYs-Mac:ury-playd ury$ homebrew install cmake mpg123 libsndfile libuv sdl2

_Note: URY does not actually have a Mac._

#### Windows (Visual Studio 2015+)

See [README.VisualStudio.md].


### Build with CMake

This is the easiest way to build `playd`.

CMake should automatically detect which generator to use (e.g. `Unix Makefiles` or `Visual Studio 14 2015`),
but you can see a full list, and specify it manually, with the `-G` option.

To manually specify a prefix directory for searching for library directories (`lib` and `include`), use the CMake option [CMAKE_PREFIX_PATH].
The paths must be absolute.
Newer versions of CMake support this being a ;-separated list.

For example:

	cmake . -DCMAKE_PREFIX_PATH="/path/to/prefix/;/another/prefix/"

#### POSIX (GNU/Linux, BSD, macOS)

Create a directory for the build, for cleanliness, e.g. `cbuild/`. From this directory, run `cmake .. [options..]`.
See `cmake --help` for additional options.

You can override the default C and C++ compilers by setting the `CC` and `CXX` environment variables.

	CC=gcc CXX=g++ cmake ..
	CC=clang CXX=clang++ cmake ..

You can then run `make`, `make test`, `make check`, or `sudo make install`.

On macOS, you can even use Xcode! Just add the `-G Xcode` option to `cmake`.

#### Windows (Visual Studio 2015+)

playd can be built with Visual Studio (tested with 2015 Community). See [README.VisualStudio.md].

#### Windows (MSYS2)

Using the [PKGBUILD](#msys2-pkgbuild) is an easy way to install the dependencies and build a package from a git branch.
Building your working copy is just like on any other POSIX system. The [PKGBUILD](#msys2-pkgbuild) is very easy to read.

#### Windows (Cygwin)

This environment isn't currently supported, but it shouldn't be much work to make that happen.

### Ports

The `ports/` directory contains files for performing fully automated
builds on certain operating systems and platforms.

#### [MSYS2] and [Arch Linux] `PKGBUILD`s

These build from the `HEAD` of the remote `master` branch.

Run the following in a MinGW32 or MinGW64 shell to build for each platform:
```
cd ports/msys2
makepkg -si
```

Or on Arch:
```
cd ports/arch
makepkg -si
```

See `man makepkg` for more information.

#### FreeBSD `Makefile`

This builds from a remote tag specified in the `Makefile` by `GH_TAGNAME`. If you
change this, you should change `DISTVERSION` as appropriate.

You need a copy of the ports tree, e.g. in `/usr/ports`.

NOTE: You must run `make makesum` (probably as su) to generate the
distinfo file, which contains the SHA256 and filesize of the code.
```
mkdir /usr/ports/audio/playd
cp ports/freebsd/* /usr/ports/audio/playd
cd /usr/ports/audio/playd
sudo make makesum
```

Then build it like any other port.
```
cd /usr/ports/audio/playd
make
sudo make install
```

or
`portmaster audio/playd`


## Contributing

We appreciate any and all pull requests made in accordance with our
philosophy.


## Legal

All original code is licenced under the [MIT licence] (see LICENSE.txt).
Some code is taken from the [CATCH] project (see LICENSE.catch).
The various CMake scripts come with their licence information attached.


[Arch Linux]:             https://www.archlinux.org/
[CMake]:                  https://cmake.org/
[CATCH]:                  http://catch-lib.net
[clang]:                  http://clang.llvm.org
[gcc]:                    https://gcc.gnu.org
[GNU Make]:               https://www.gnu.org/software/make/
[Homebrew]:               http://brew.sh
[libmpg123]:              http://www.mpg123.de
[libsndfile]:             http://www.mega-nerd.com/libsndfile/
[libsox]:                 http://sox.sourceforge.net
[libuv]:                  https://github.com/joyent/libuv
[MIT licence]:            http://opensource.org/licenses/MIT
[MSYS2]:                  https://msys2.github.io/
[netcat]:                 http://nc110.sourceforge.net
[SDL2]:                   https://www.libsdl.org
[PuTTY]:                  http://www.chiark.greenend.org.uk/~sgtatham/putty/
[University Radio York]:  http://ury.org.uk

[CMAKE_PREFIX_PATH]:      https://cmake.org/cmake/help/latest/variable/CMAKE_PREFIX_PATH.html
[README.VisualStudio.md]: README.VisualStudio.md
