/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
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
