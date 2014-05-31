/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#ifndef MESSAGES_H
#define MESSAGES_H

#include <string>

const std::string MSG_DEV_BADID = "Incorrect device ID";
const std::string MSG_DEV_NOID = "Expected a device ID as an argument";
const std::string MSG_OHAI = "URY playslave at your service";
const std::string MSG_TTFN = "Sleep now";
const std::string MSG_CMD_ARGN = "Expecting no argument, got one";
const std::string MSG_CMD_ARGU = "Expecting an argument, didn't get one";
const std::string MSG_CMD_HITEND = "Hit end of commands list without stopping";
const std::string MSG_CMD_NOPROP =
                "Command type is PROPAGATE, but propagate stream is NULL";
const std::string MSG_CMD_NOSUCH = "Command not recognised";
const std::string MSG_CMD_NOWORD = "Need at least a command word";
const std::string MSG_ERR_NOMEM = "(ran out of memory to write error!)";

#endif /* !MESSAGES_H  */
