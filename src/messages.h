// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Constant human-readable messages used within playd.
 */

#ifndef PS_MESSAGES_H
#define PS_MESSAGES_H

#include <string>


//
// Client communications
//

/// Message shown when a client connects to playd.
#ifndef PD_VERSION
#define PD_VERSION "0.0.0"
#endif
const std::string MSG_OHAI = "playd " PD_VERSION;


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


//
// Decoder failures
//

/// Message shown when a bad sample rate is found.
const std::string MSG_DECODE_BADRATE = "Unsupported or invalid sample rate";


//
// Device ID failures
//

/// Message shown when an incorrect device ID is provided.
const std::string MSG_DEV_BADID = "Incorrect device ID";


//
// Load failures
//

/// Message shown when one tries to Load an empty path.
const std::string MSG_LOAD_EMPTY_PATH = "Empty file path given";

//
// Audio output failures
//

/// Message shown when there is an error writing to the ring buffer.
const std::string MSG_OUTPUT_RINGWRITE = "Ring buffer write error";

/// Message shown when there is an error initialising the ring buffer.
const std::string MSG_OUTPUT_RINGINIT = "Ring buffer init error";


//
// Seek failures
//

/// Message shown when an attempt to seek fails.
const std::string MSG_SEEK_FAIL = "Seek failed";

/// Message shown when a seek command has an invalid time unit.
const std::string MSG_SEEK_INVALID_UNIT = "Invalid time unit: try us, s, m, h";

/// Message shown when a seek command has an invalid time value.
const std::string MSG_SEEK_INVALID_VALUE = "Invalid time: try integer[unit]";


#endif // PS_MESSAGES_H
