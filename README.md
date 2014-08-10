# playslave++

`playslave++` is a very minimal C++ audio player using ffmpeg and
portaudio.

It is a C++ refactoring/port of `playslave`, and is developed
primarily by CaptainHayashi and LordAro.  It is licenced under the MIT licence
(see LICENCE.txt).  Some code is taken from the PortAudio project
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

## Features

* Theoretically plays anything ffmpeg can play
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

## Requirements

* [PortAudio](http://www.portaudio.com/) V19
* [FFmpeg](https://www.ffmpeg.org/) (latest version)
* [Boost](http://boost.org/) 1.55.0 or newer

playslave++ probably doesn't work with libav, due to its dependency on
libswresample.

To use the Makefile, you'll need GNU Make and pkg-config (or equivalent), and
pkg-config packages for PortAudio and FFmpeg.  We've tested building playslave++
on Gentoo and FreeBSD 10, but other POSIX-style operating systems should work.

playslave++ **can** be built with Visual Studio (tested with 2013 Premium), but
you will need to source and configure the dependencies manually.

If you have the PortAudio C++ bindings available, those may be used in place
of the bundled bindings.  This will happen automatically when using the
Makefile, if the C++ bindings are installed as a pkg-config package.

## Installation on POSIX-style operating systems

* Ensure you have the dependencies above;
* Read the `Makefile`, to see if any variables need to be overridden for your
  environment;
* Run `make`, and, optionally, `sudo make install`.  The latter will install
  a man page for playslave++, in addition to playslave++ itself.

## Q/A

### Why does this exist?

It was originally written as an experiment when coming up with a new playout
system for [University Radio York](http://ury.org.uk).

### Why is it named playslave++?

The name is meant to be short, snappy, and descriptive of what the program does
(it's intended to be used by a driver program, in a master/slave configuration).

### Can I contribute?

Certainly!  We appreciate any and all pull requests that further the playslave++
philosophy.
