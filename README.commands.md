# Command Reference

`playd` implements the BAPS3 Internal API (specifically the `End`, `FileLoad`,
`PlayStop`, `Seek`, and `TimeReport` features), as described in the [BAPS3
specification][].  The specification provides more formal definitions of the
commands supported by `playd`.

## Sending Commands Manually

To talk to `playd`, open a raw TCP/IP session on the host and port on which
`playd` was started.  For example, using [netcat][]:

    nc ${ADDRESS} ${PORT}

On Windows, use [PuTTY][] in _raw mode_ with an _Implicit CR for every LF_.

## Initial Responses

When connecting to `playd`, it will send a few initial responses to inform
the client of its current state.  This will look something like:

    OHAI playd
    FEATURES End FileLoad PlayStop Seek TimeReport
    TIME 0
    STATE Ejected

This tells the client:

* The name of the server program (`playd`);
* The BAPS3 features implemented by `playd`;
* The current position, in microseconds, into the current song (in this example,
  it is `0` because there is no song)
* The current state (`Ejected`, which means no song is loaded).

## Command Format

`playd` uses _shell-style_ commands:

* Each command is a linefeed-delimited _line_ of whitespace-delimited _words_;
* Whitespace and quotes can be _escaped_ by using backslash;
* Words can be _quoted_ to avoid needing to backslash-escape large amounts of
  whitespace: either using _double quotes_, which allows backslash escaping, or
  _single quotes_, which doesn't;
* The first word represents the _command_, and each subsequent word is an
  _argument_ to that command.

## Requests

These are the requests that clients can send to `playd`.  Request commands are
always in _lowercase_.

### quit

Terminates `playd`.

### load _file_

Loads _file_, which is an _absolute_ path to an audio file.

### eject

Unloads the current file, stopping it if it is currently playing.

### play

Starts playing the currently loaded file.

### stop

Stops playing the currently loaded file.  This does not alter the position;
use `seek 0` after `stop` to rewind the file.

### seek _position_

Seeks to _position_, which may either be an integer (interpreted as microseconds
since the beginning of the file), or an integer followed by a suffix denoting
the unit.  Units supported include `s` (seconds), `m` (minutes), `h` (hours),
`ms` (milliseconds), and `us` (microseconds).

## Responses

These are the responses sent to clients by `playd`.  Response commands are
always in _uppercase_.

### OHAI _name_

Identifies the server program.

### FEATURES _feat1_ _feat2..._

Lists the BAPS3 feature flags supported by `playd`.

### END

Marks the end of a file.  This is sent if a file finishes playing of its own
accord.

### TIME _position_

Announces the current position in the file, in microseconds.

### STATE _state_

Announces a state change.  The _state_ will be one of:

* `Playing` - the current file is now being played;
* `Stopped` - the current file is now stopped;
* `Ejected` - the current file has just been ejected;
* `Quitting` - `playd` is now quitting.

### FILE _file_

Announces that _file_ has just been loaded.

### OKAY _command_

Announces that a valid command has just been received and processed.

### WHAT _error_

Announces that a bad command was just received.

[BAPS3 specification]: https://UniversityRadioYork.github.io/baps3-spec
[PuTTY]:               http://www.chiark.greenend.org.uk/~sgtatham/putty/
[netcat]:              http://nc110.sourceforge.net/
