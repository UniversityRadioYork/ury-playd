# playslave++

`playslave++` is a very minimal C++ audio player using [libsox][] and
[PortAudio][], developed primarily by CaptainHayashi and LordAro and designed to
be composable into bigger systems.

All code developed for `playslave++` is licenced under the [MIT licence][]
(see LICENCE.txt).  Some code is taken from the [PortAudio][] project
(see LICENCE.portaudio).

## Usage

`playslave++ DEVICE-ID [ADDRESS] [PORT]`

* Invoking `playslave++` with no arguments lists the various device IDs
  available to it.
* Full protocol information is available on the GitHub wiki.
* On POSIX systems, see the enclosed man page.

`playslave++` understands the following commands via its TCP/IP interface:

* `load "/full/path/to/file"` — Loads /full/path/to/file for playback;
* `eject` — Unloads the current file;
* `play` — Starts playback;
* `stop` — Stops (pauses) playback;
* `seek 1m` — Seeks one minute into the current file.  Units supported include
  `h`, `m`, `s`, `ms`, `us` (micros), with `us` assumed if no unit is given.
* `quit` — Closes `playslave++`.

### Sending commands manually

To connect directly to `playslave++` and issue commands to it, you can use
[netcat][]:

```sh
# If you specified [ADDRESS] or [PORT], replace localhost and 1350 respectively.
$ nc localhost 1350
```

On Windows, using [PuTTY][] in _raw mode_ (__not__ Telnet mode)
with _Implicit CR in every LF_ switched on in the _Terminal_ options should
work.

__Do _not_ use a Telnet client (or PuTTY in telnet mode)!__  `playslave++` will
do weird things in the presence of Telnet-isms.

## Features

* Plays anything [libsox][] can play (in practice, more esoteric
  formats might not work)
* Seek (microseconds, seconds, minutes etc)
* Frequently announces the current position
* TCP/IP interface with text protocol
* Deliberately not much else

## Philosophy

* Do one thing and do it well
* Be hackable
* Favour simplicity over performance
* Favour simplicity over features
* Let other programs handle the shinies

## Compilation

### Requirements

* [libsox][] (latest version)
* [libuv][] v0.11.28
* [PortAudio][] V19
* A C++11 compiler (recent versions of [clang][], [gcc][], and Visual Studio
  work)

`playslave++` probably doesn't work with libav, due to its dependency on
libswresample.

### POSIX (GNU/Linux, BSD)

`playslave++` comes with a GNU-compatible Makefile that can be used both to make
and install.

To use the Makefile, you'll need [GNU Make][] and `pkg-config` (or equivalent),
and pkg-config packages for PortAudio, libsox and libuv.
We've tested building playslave++ on Gentoo and FreeBSD 10, but other
POSIX-style operating systems should work.

Using the Makefile is straightforward:

* Ensure you have the dependencies above;
* Read the `Makefile`, to see if any variables need to be overridden for your
  environment;
* Run `make`, and, optionally, `sudo make install`.  The latter will install
  a man page for playslave++, in addition to playslave++ itself.

#### OS X

All dependencies are available in [homebrew](http://brew.sh) - it is highly recommended that you use it!

### Windows

#### Visual Studio

playslave++ **can** be built with Visual Studio (tested with 2013 Premium), but
you will need to source and configure the dependencies manually.  A Visual
Studio project is provided, but will need tweaking for your environment.

#### MinGW

Not yet thoroughly tested, but should work.

### PortAudio C++ Bindings

If you have the PortAudio C++ bindings available, those may be used in place
of the bundled bindings.  This will happen automatically when using the
Makefile, if the C++ bindings are installed as a pkg-config package.

__Visual Studio users:__ The Visual Studio 7.1 project supplied in the
PortAudio source distribution for building the C++ bindings
(`\bindings\cpp\build\vc7_1\static_library.vcproj`) should work.  If
not, then use the bundled bindings.

## Q&A

### Why does this exist?

It was originally written as an experiment when coming up with a new playout
system for [University Radio York](http://ury.org.uk).

### Why is it named playslave++?

The name is meant to be short, snappy, and descriptive of what the program does
(it's intended to be used by a driver program, in a master/slave configuration).

### Can I contribute?

Certainly!  We appreciate any and all pull requests that further the playslave++
philosophy.

[clang]:                 http://clang.llvm.org/
[libsox]:                http://sox.sourceforge.net/
[gcc]:                   https://gcc.gnu.org/
[GNU Make]:              https://www.gnu.org/software/make/
[libuv]:                 https://github.com/joyent/libuv
[MIT licence]:           http://opensource.org/licenses/MIT
[netcat]:                http://nc110.sourceforge.net/
[PortAudio]:             http://www.portaudio.com/
[PuTTY]:                 http://www.chiark.greenend.org.uk/~sgtatham/putty/
[University Radio York]: http://ury.org.uk
