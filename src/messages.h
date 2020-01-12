// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Constant human-readable messages used within playd.
 */

#ifndef PLAYD_MESSAGES_H
#define PLAYD_MESSAGES_H

#include <string>

//
// Client communications
//

#ifndef PD_VERSION
#define PD_VERSION "0.0.0"
#endif
/// The playd name and version.
constexpr std::string_view MSG_OHAI_PLAYD { "playd-" PD_VERSION };

/// The protocol name and version.
constexpr std::string_view MSG_OHAI_BIFROST { "bifrost-0.3.0" };

//
// Command failure messages
//

/// Message shown when the CommandHandler receives an under-length command.
constexpr std::string_view MSG_CMD_SHORT {
        "Command must contain at least a tag and one command word" };

/// Message shown when the CommandHandler receives an invalid command.
constexpr std::string_view MSG_CMD_INVALID { "Bad command or file name" };

/**
 * Message shown when a command that works only when a file is loaded is fired
 * when there isn't anything loaded.
 */
constexpr std::string_view MSG_CMD_NEEDS_LOADED { "Command requires a loaded file" };

/**
 * Message shown when a command that works only when a file is playing is fired
 * when there isn't anything playing.
 */
constexpr std::string_view MSG_CMD_NEEDS_PLAYING { "Command requires a playing file" };

/**
 * Message shown when a command that works only when a file is stopped is fired
 * when there isn't anything stopped.
 */
constexpr std::string_view MSG_CMD_NEEDS_STOPPED { "Command requires a stopped file" };

/// Message shown when a command is sent to a closing Player.
constexpr std::string_view MSG_CMD_PLAYER_CLOSING { "Server is closing" };

//
// Decoder failures
//

/// Message shown when a bad sample rate is found.
constexpr std::string_view MSG_DECODE_BADRATE { "Unsupported or invalid sample rate" };

//
// Device ID failures
//

/// Message shown when an incorrect device ID is provided.
constexpr std::string_view MSG_DEV_BADID { "Incorrect device ID" };

//
// Load failures
//

/// Message shown when one tries to Load an empty path.
constexpr std::string_view MSG_LOAD_EMPTY_PATH { "Empty file path given" };

//
// Audio output failures
//

/// Message shown when there is an error writing to the ring buffer.
constexpr std::string_view MSG_OUTPUT_RINGWRITE { "Ring buffer write error" };

/// Message shown when there is an error initialising the ring buffer.
constexpr std::string_view MSG_OUTPUT_RINGINIT { "Ring buffer init error" };

//
// Seek failures
//

/// Message shown when an attempt to seek fails.
constexpr std::string_view MSG_SEEK_FAIL { "Seek failed" };

/// Message shown when a seek command has an invalid time value.
constexpr std::string_view MSG_SEEK_INVALID_VALUE { "Invalid time: try integer" };

//
// General command failures
//

/// Message shown when a command is called with a missing resource.
constexpr std::string_view MSG_NOT_FOUND { "not found" };

/// Message shown when an invalid payload is supplied for writing.
constexpr std::string_view MSG_INVALID_PAYLOAD { "cannot write this value" };

/// Message shown when we try to write/delete to something we can't.
constexpr std::string_view MSG_INVALID_ACTION { "cannot perform this action" };

//
// IO failures
//

/// Message shown when allocating an IO object fails.
constexpr std::string_view MSG_IO_CANNOT_ALLOC { "cannot allocate I/O object" };

/// Message shown when too many simultaneous connections are launched.
constexpr std::string_view MSG_TOO_MANY_CONNS { "too many simultaneous connections" };

#endif // PLAYD_MESSAGES_H
