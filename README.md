# URY playd

URY playd (or just `playd`; short for _player daemon_) is a C++ audio
player developed by [University Radio York].  It's designed to be
minimal, hackable, and composable into bigger systems (such as our
ongoing BAPS3 project to build a new radio playout system).


## Usage

`playd DEVICE-ID [ADDRESS] [PORT]`

* Invoking `playd` with no arguments lists the various device IDs
  available to it.
* Full protocol information is available on the GitHub wiki.
* On POSIX systems, see the enclosed man page.

`playd` understands the following commands via its TCP/IP interface:

* `load "/full/path/to/file"` — Loads /full/path/to/file for playback;
* `eject` — Unloads the current file;
* `play` — Starts playback;
* `stop` — Stops (pauses) playback;
* `seek 1000` — Seeks to 1,000 microseconds after the start of the current file.
* `quit` — Closes `playd`.

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

* Plays virtually anything [libsox] can play (notably, MP3s, OGGs, and FLACs)
* Seek (microseconds, seconds, minutes etc)
* Frequently announces the current position
* TCP/IP interface with text protocol
* Deliberately not much else


## Philosophy

`playd` is developed using the following guidelines:

* Do one thing and do it well
* Be hackable
* Favour simplicity over performance
* Favour simplicity over features
* Let other programs handle the shinies


## Compilation

### Requirements

* [libsox] (1.14.1)
* [libuv] (0.11.29)
* [PortAudio] (19_20140130)
* A C++11 compiler (recent versions of [clang], [gcc], and Visual Studio
  work)

Certain operating systems may need additional dependencies; see the OS-specific
build instructions below.

### POSIX (GNU/Linux, BSD, OS X)

`playd` comes with a GNU-compatible Makefile that can be used both to
make and install.

To use the Makefile, you'll need [GNU Make] and `pkg-config` (or equivalent),
and pkg-config packages for PortAudio, libsox and libuv.  We've tested building
playd on Gentoo, FreeBSD 10, and OS X, but other POSIX-style operating systems
should work.

Using the Makefile is straightforward:

* Ensure you have the dependencies above;
* Read the `Makefile`, to see if any variables need to be overridden for your
  environment;
* Run `make` (or whatever GNU Make is called on your OS; in FreeBSD, for
  example, it'd be `gmake`), and, optionally, `sudo make install`.
  The latter will globally install playd and its man page.

#### OS X

All dependencies are available in [homebrew] - it is highly recommended that
you use it!

#### FreeBSD (10+)

FreeBSD 10 and above come with `clang` 3.3 as standard, which should be able to
compile `playd`.  `gcc` is available through the FreeBSD Ports Collection
and package repositories.

You will need `gmake`, as `Makefile` is incompatible with BSD make.  Sorry!

All of `playd`'s dependencies are available through both the FreeBSD Ports
Collection and standard package repository.  (The FreeBSD port for PortAudio
doesn't build C++ bindings, but we bundle them anyway.)  To install them as
packages:

    root@freebsd:/ # pkg install gmake sox libuv portaudio2 pkgconf

Then, run `gmake` (__not__ `make`), and, optionally, `gmake install` to install
`playd` (as root):

    user@freebsd:~/ % gmake
    root@freebsd:~/ # gmake install

### Windows

#### Visual Studio

_For more information, see `README.VisualStudio.md`._

playd **can** be built with Visual Studio (tested with 2013 Premium), but
you will need to source and configure the dependencies manually.  A Visual
Studio project is provided, but will need tweaking for your environment.

#### MinGW

We haven't managed ourselves, but assuming you can build all the dependencies,
(libsox is the difficult one), it should work fine.

### PortAudio C++ Bindings

If you have the PortAudio C++ bindings available, those may be used in place of
the bundled bindings.  This will happen automatically when using the Makefile,
if the C++ bindings are installed as a pkg-config package.

__Visual Studio users:__ The Visual Studio 7.1 project supplied in the
PortAudio source distribution for building the C++ bindings
(`\bindings\cpp\build\vc7_1\static_library.vcproj`) should work.  If not, then
use the bundled bindings.


## Contributing

We appreciate any and all pull requests made in accordance with our
philosophy.


## Legal

All original code is licenced under the [MIT licence] (see LICENSE.txt).
Some code is taken from the [PortAudio] project (see LICENSE.portaudio),
as well as [CATCH] (see LICENSE.catch).


[CATCH]:                 http://catch-lib.net
[clang]:                 http://clang.llvm.org
[gcc]:                   https://gcc.gnu.org
[GNU Make]:              https://www.gnu.org/software/make/
[Homebrew]:              http://brew.sh
[libsox]:                http://sox.sourceforge.net
[libuv]:                 https://github.com/joyent/libuv
[MIT licence]:           http://opensource.org/licenses/MIT
[netcat]:                http://nc110.sourceforge.net
[PortAudio]:             http://www.portaudio.com
[PuTTY]:                 http://www.chiark.greenend.org.uk/~sgtatham/putty/
[University Radio York]: http://ury.org.uk
