## Welcome

**URY playd** is a minimalist audio player written in [C++11].  It
uses [libsox], [PortAudio], and [libuv] to decode audio, play it
to any output device, and take commands via a simple, line-based
TCP protocol.

playd is part of the <strong>BAPS3</strong> project at [University
Radio York].  It is under _active development_, and we can't yet
guarantee it won't try to strangle your cat.

The latest release is [v0.2.0] (__Edgware__); the documentation
hosted here refers to it.

## Features

playd doesn't have many features, as it's intended to be a minimal
player core for use in bigger things.  Here are some features it
_does_ have:

  - Plays any file format supported by libsox;
  - Selectable output device at launch-time;
  - Play/Stop/Seek/Eject/Load commands;
  - Text-based TCP/IP interface;
  - Cross-platform: we've built it on Windows, FreeBSD, OS X, and
    some GNU/Linux distributions.

## Legal

playd itself is licensed under the [MIT licence].  However, it
includes, and uses, code under other licences, including the LGPL.
A full build of playd is likely to come under the GPL, due to
optional sound library dependencies.

## Installation

You can find the latest code, and instructions on how to use it,
at playd's [GitHub] page.

## Usage

See the [man page] for usage instructions.

## Resources

### playd

  - [Man page] -
    provides a concise description of how to run playd.
  - [Documentation] -
    contains the READMEs included in the playd distribution, as
    well as auto-generated source code documentation.
  - [GitHub] -
    source code for playd, as well as an issue tracker and resources
    for contributing.
  - [BAPS3 Spec] -
    a formal(ish) document on the playout system for which playd
    was developed.

### Similar projects

playd is not the first attempt at a minimalist audio player.  Here
are resources on similar projects which may better suit your needs.

  - [Music Player Daemon]
    has a similar, minimalist philosophy, but a wider scope.
  - [aplay][] (and similar programs)
    are examples of lightweight audio players, but lack playd's
    network interface.

### Further reading

Articles that are relevant, but not directly related, to playd.

  - [A Plea for Lean Software][] (Wirth, 1995) -
    dated, but an early article about the perils of bloated software.
  - [The Art of Unix Programming][] (Raymond, 2003) -
    a very good resource on the UNIX philosophy and its influences on
    software development.

[C++11]:                       https://isocpp.org/
[libsox]:                      http://sox.sourceforge.net/libsox.html
[PortAudio]:                   http://www.portaudio.com/
[libuv]:                       https://github.com/libuv/libuv
[University Radio York]:       http://ury.org.uk
[v0.2.0]:                      https://github.com/UniversityRadioYork/ury-playd/releases/tag/v0.2.0
[MIT licence]:                 https://raw.githubusercontent.com/UniversityRadioYork/ury-playd/master/license.txt
[Man page]:                    https://universityradioyork.github.io/ury-playd/man.html
[Documentation]:               https://universityradioyork.github.io/ury-playd/doxygen
[GitHub]:                      https://github.com/UniversityRadioYork/ury-playd
[BAPS3 Spec]:                  https://github.com/UniversityRadioYork/baps3-spec
[Music Player Daemon]:         http://www.musicpd.org
[aplay]:                       http://linux.die.net/man/1/aplay
[The Art of Unix Programming]: http://www.catb.org/esr/writings/taoup/
[A Plea for Lean Software]:    http://cr.yp.to/bib/1995/wirth.pdf