/*
 * =============================================================================
 *
 *       Filename:  messages.c
 *
 *    Description:  Messages.  (Does exactly what it says on the tin...)
 *
 *        Version:  1.0
 *        Created:  26/12/2012 01:36:25
 *       Revision:  none
 *       Compiler:  clang
 *
 *         Author:  Matt Windsor (CaptainHayashi), matt.windsor@ury.org.uk
 *        Company:  University Radio York Computing Team
 *
 * =============================================================================
 */
/*-
 * Copyright (C) 2012  University Radio York Computing Team
 *
 * This file is a part of playslave.
 *
 * playslave is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * playslave is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * playslave; if not, write to the Free Software Foundation, Inc., 51 Franklin
 * Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


/**  GLOBAL VARIABLES  ********************************************************/

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
