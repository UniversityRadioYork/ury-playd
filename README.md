# playslave++

`playslave++` is a very minimal C++ audio player using ffmpeg and
portaudio.

It is a C++ refactoring/port of `playslave`, and is developed
primarily by CaptainHayashi and LordAro.  It is licenced under the MIT licence
(see LICENCE.txt).  Some code is taken from the PortAudio project
(see LICENCE.portaudio).

## Usage

`playslave++ DEVICE-ID`

* Invoking `playslave++` with no arguments lists the various device IDs
  available to it.
* Full protocol information is available on the GitHub wiki.

## Features

* Theoretically plays anything ffmpeg can play
* Seek (microseconds, seconds, minutes etc)
* Announces the current position via stdout
* Unix-style stdin/stdout interface with text protocol
* Deliberately not much else

### Planned

Anything not on this list is likely not going to be a playslave++ feature.

* Duration report on song load
* Configurable/optional time announcements
* Possible move to sockets
* Possible better support of more esoteric sample formats

## Philosophy

* Do one thing and do it well
* Be hackable
* Favour simplicity over performance
* Favour simplicity over features
* Let other programs handle the shinies

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
