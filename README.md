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

### Requirements

* [libuv] 1.9.1+;
* [SDL2] 2.0.3;
* A C++14 compiler (recent versions of [clang], [gcc], and Visual Studio
  work);
* [CMake] 2.8+, for building [optional, but recommended].

The following dependencies are used for file format support, and you'll need at
least one of them:

* [libmpg123] 1.20.1+, for MP3 support;
* [libsndfile] 1.0.25+, for Ogg Vorbis, WAV, and FLAC support.

Certain operating systems may need additional dependencies; see the OS-specific
build instructions below.

You can build using the newer **CMake** scripts (recommended), or the older **`config.sh`**.


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

#### OS X

All dependencies are available in [homebrew] - it is highly recommended that
you use it!

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

#### POSIX (GNU/Linux, BSD, OS X)

Create a directory for the build, for cleanliness, e.g. `cbuild/`. From this directory, run `cmake .. [options..]`.
See `cmake --help` for additional options.

You can override the default C and C++ compilers by setting the `CC` and `CXX` environment variables.

	CC=gcc CXX=g++ cmake ..
	CC=clang CXX=clang++ cmake ..

You can then run `make`, `make test`, or `sudo make install`.

#### Windows (Visual Studio 2015+)

playd can be built with Visual Studio (tested with 2015 Community). See [README.VisualStudio.md].

#### Windows (Cygwin, MSYS2)

These environments aren't currently supported, but it shouldn't be much work to make that happen.


### Build without CMake

#### POSIX (GNU/Linux, BSD, OS X)

**Warning**: This method of building `playd` is liable to be removed in
favour of CMake in future.

`playd` comes with `config.sh`, a Bourne shell script that will generate a
GNU-compatible Makefile that can be used both to make and install.

To use the Makefile, you'll need [GNU Make] and `pkg-config` (or equivalent),
and pkg-config packages for SDL2, libuv, and any needed decoder libraries.
We've tested building playd on Gentoo, FreeBSD 10, and OS X, but other
POSIX-style operating systems should work.

Using the Makefile is straightforward:

* Ensure you have the dependencies above;
* Run `config.sh` (optionally, read it first to see if any variables need to be
  overriden for your environment);
* Optionally read the generated `Makefile`, to make sure it's ok;
* Run `make` (or whatever GNU Make is called on your OS; in FreeBSD, for
  example, it'd be `gmake`), and, optionally, `sudo make install`.
  The latter will globally install playd and its man page.

#### FreeBSD (10+)

FreeBSD 10 and above come with `clang` 3.3 as standard, which should be able to
compile `playd`.  `gcc` is available through the FreeBSD Ports Collection
and package repositories.

You will need `gmake`, as `Makefile` is incompatible with BSD make.  Sorry!

Then, run `gmake` (__not__ `make`), and, optionally, `gmake install` to install
`playd` (as root):

    user@freebsd:~/ % gmake
    root@freebsd:~/ # gmake install

#### Windows

You should use CMake.


## Contributing

We appreciate any and all pull requests made in accordance with our
philosophy.


## Legal

All original code is licenced under the [MIT licence] (see LICENSE.txt).
Some code is taken from the [PortAudio] project (see LICENSE.portaudio),
as well as [CATCH] (see LICENSE.catch).  The various CMake scripts come
with their licence information attached.


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
[netcat]:                 http://nc110.sourceforge.net
[SDL2]:                   https://www.libsdl.org
[PuTTY]:                  http://www.chiark.greenend.org.uk/~sgtatham/putty/
[University Radio York]:  http://ury.org.uk

[CMAKE_PREFIX_PATH]:      https://cmake.org/cmake/help/latest/variable/CMAKE_PREFIX_PATH.html
[README.VisualStudio.md]: README.VisualStudio.md
