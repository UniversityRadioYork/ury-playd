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
#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <memory>

#include "audio.h"

#include "cuppa/errors.h" /* enum error */

/* Enumeration of states that the player can be in.
 *
 * The player is effectively a finite-state machine whose behaviour
 * at any given time is dictated by the current state, which is
 * represented by an instance of 'enum state'.
 */
enum state {
	S_VOID,			/* No state (usually when player starts up) */
	S_EJCT, 		/* No file loaded */
	S_STOP, 		/* File loaded but not playing */
	S_PLAY, 		/* File loaded and playing */
	S_QUIT, 		/* Player about to quit */
        /*--------------------------------------------------------------------*/
	NUM_STATES              /* Number of items in enum */
};

/* The player structure contains all persistent state in the program.
 *
 */
class player {
public:
	player(int driver);

	void main_loop();

	bool cmd_ejct();
	bool cmd_play();
	bool cmd_quit();
	bool cmd_stop();

	bool cmd_load(const std::string &path);
	bool cmd_seek(const std::string &time_str);

	enum state state();

private:
	int device;
	uint64_t ptime;
	enum state cstate;
	std::unique_ptr<audio> au;

	bool gate_state(std::initializer_list<enum state> states);
	void set_state(enum state state);
	void loop_iter();
};


#endif				/* not PLAYER_H */
