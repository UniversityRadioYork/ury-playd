// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Constant human-readable messages used within playd.
 * TODO: sort these properly?
 */

#ifndef PS_MESSAGES_H
#define PS_MESSAGES_H

#include <string>

//
// Misc. failure messages
//

/// Message shown when the AudioSource fails to decode a file.
const std::string MSG_DECODE_FAIL = "Decoding failure";

/// Message shown when the AudioSource fails to find an audio stream.
const std::string MSG_DECODE_NOAUDIO = "This doesn't seem to be an audio file";

/// Message shown when the AudioSource fails to initialise a stream.
const std::string MSG_DECODE_NOSTREAM = "Couldn't acquire stream";

/// Message shown when the AudioSource fails to initialise a codec.
const std::string MSG_DECODE_NOCODEC = "Couldn't acquire codec";

/// Message shown when a bad sample rate is found.
const std::string MSG_DECODE_BADRATE = "Unsupported or invalid sample rate";

/// Message shown when an attempt to seek fails.
const std::string MSG_SEEK_FAIL = "Seek failed";

/// Message shown when an incorrect device ID is provided.
const std::string MSG_DEV_BADID = "Incorrect device ID";

/// Message shown when no device ID is provided.
const std::string MSG_DEV_NOID = "Expected a device ID as an argument";

/// Message shown when there is an error writing to the ring buffer.
const std::string MSG_OUTPUT_RINGWRITE = "Ring buffer write error";

/// Message shown when there is an error initialising the ring buffer.
const std::string MSG_OUTPUT_RINGINIT = "Ring buffer init error";

/// Message shown when a client connects to playd.
const std::string MSG_OHAI = "playd";

/// Message shown when a client disconnects from playd.
const std::string MSG_TTFN = "Sleep now";

//
// Command failure messages
//

/// Message shown when the CommandHandler receives an invalid command.
const std::string MSG_CMD_INVALID = "Bad command or file name";

/**
 * Message shown when a command that works only when a file is loaded is fired
 * when there isn't anything loaded.
 */
const std::string MSG_CMD_NEEDS_LOADED = "Command requires a loaded file";

/**
 * Message shown when a command that works only when a file is playing is fired
 * when there isn't anything playing.
 */
const std::string MSG_CMD_NEEDS_PLAYING = "Command requires a playing file";

/**
 * Message shown when a command that works only when a file is stopped is fired
 * when there isn't anything stopped.
 */
const std::string MSG_CMD_NEEDS_STOPPED = "Command requires a stopped file";

/// Message shown when a seek command has an invalid time unit.
const std::string MSG_SEEK_INVALID_UNIT = "Invalid time unit: try us, s, m, h";

/// Message shown when a seek command has an invalid time value.
const std::string MSG_SEEK_INVALID_VALUE = "Invalid time: try integer[unit]";

/// Message shown when one tries to Load an empty path.
const std::string MSG_LOAD_EMPTY_PATH = "Empty file path given";

#endif // PS_MESSAGES_H
