/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
 */

const char     *MSG_DEV_BADID = "Incorrect device ID";
const char     *MSG_DEV_NOID = "Expected a device ID as an argument";
const char     *MSG_OHAI = "URY playslave at your service";
const char     *MSG_TTFN = "Sleep now";

#define MSG(name, contents) const char * name = contents

MSG(MSG_CMD_ARGN,
	"Expecting no argument, got one");
MSG(MSG_CMD_ARGU,
	"Expecting an argument, didn't get one");
MSG(MSG_CMD_HITEND,
	"Hit end of commands list without stopping");
MSG(MSG_CMD_NOPROP,
	"Command type is PROPAGATE, but propagate stream is NULL");
MSG(MSG_CMD_NOSUCH,
	"Command not recognised");
MSG(MSG_CMD_NOWORD,
	"Need at least a command word");
MSG(MSG_ERR_NOMEM,
	"(ran out of memory to write error!)");
