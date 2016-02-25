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

    ! OHAI 1 bifrost-0.3.0 playd-0.3.0
    ! IAMA player/file
    ! PLAY
    ! FLOAD 'C:\Users\mattbw\Music\07 - Games Without Frontiers.mp3'
    ! POS 72399818
    ! DUMP

This tells the client:

* Its client ID is 1 (mostly used for debug purposes);
* This server speaks the Bifrost protocol, version 0.3.0;
* This server runs `playd` version 0.3.0;
* This server is a file player;
* There is a file loaded and it is playing;
* The file loaded is 'C:\Users\mattbw\Music\07 - Games Without Frontiers.mp3'
  (a brilliant song from Peter Gabriel)
* We are 72,399,818 microseconds, or 72.4 seconds, into the song;
* This is all the information we need to begin sending commands.

## Command Format

`playd` uses _shell-style_ commands:

* Each command is a linefeed-delimited _line_ of whitespace-delimited _words_;
* Whitespace and quotes can be _escaped_ by using backslash;
* Words can be _quoted_ to avoid needing to backslash-escape large amounts of
  whitespace: either using _double quotes_, which allows backslash escaping, or
  _single quotes_, which doesn't;
* The first word is a _tag_, which _should_ uniquely identify the command among
  any other commands `playd` receives.  If you're confident you're the only
  user of a `playd`, feel free to use anything here, as long as it contains
  neither `!` nor `:`.  If you're sharing a `playd` with something else, you
  will need to make sure your tags don't clash with theirs (eg, using GUIDs, or
  hashes including your hostname/MAC address/etc).
* The second word represents the _command_, and each subsequent word is an
  _argument_ to that command.

## Requests

These are the requests that clients can send to `playd`.  Request commands are
always in _lowercase_.

### quit

Terminates `playd`.  (May be removed in future versions.)

### fload _file_

Loads _file_, which is an _absolute_ path to an audio file.

### eject

Unloads the current file, stopping it if it is currently playing.

### play

Starts playing the currently loaded file.

### stop

Stops playing the currently loaded file.  This does not alter the position;
use `seek 0` after `stop` to rewind the file.

### pos _position_

Seeks to _position_ microseconds since the beginning of the file.

### end

Causes the song to jump right to the end; this is useful for skipping to the
next file if you're using a playlist manager like `listd` with `playd`.

### dump

Dumps all of the current state, as if you had just connected (except we don't
show you the `OHAI` or `IAMA` again).

## Responses

These are the responses sent to clients by `playd`.  Response commands are
always in _uppercase_.

### OHAI _id_ _protocol-ver_ _server-ver_

Identifies the server program and version, the protocol server and version, and
the client ID.

### IAMA _role_

States the Bifrost role of `playd`, ie `player/file`.

### END

Marks the end of a file.  This is sent if a file finishes playing of its own
accord, or if `end` is sent.

### POS _position_

Announces the current position in the file, in microseconds.

### PLAY

Announces that the file is now being played.

### STOP

Announces that the file is now stopped.

### EJECT

Announces that no file is loaded.

### QUIT

Announces that the server is quitting (may be removed in future versions).

### FLOAD _file_

Announces that _file_ has just been loaded.

### ACK _status_ _message_ _command..._

_The format of this response may change in future versions._

Announces that a _command_ has just been received and processed.

The result of the command is given by _status_:

* `WHAT`: command is syntactically invalid.
* `FAIL`: command unsuccessfully completed.
* `OK`:  command successfully completed.

[BAPS3 specification]: https://UniversityRadioYork.github.io/baps3-spec
[PuTTY]:               http://www.chiark.greenend.org.uk/~sgtatham/putty/
[netcat]:              http://nc110.sourceforge.net/
