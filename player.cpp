/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
 */

#define _POSIX_C_SOURCE 200809

#include <memory>
#include <sstream>
#include <vector>

#include <cstdarg>		/* gate_state */
#include <cstdbool>		/* bool */
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <thread>
#include <chrono>

#ifdef WIN32
struct timespec
{
	time_t tv_sec;
	long tv_nsec;
};
#include <Winsock2.h>
#else
#include <time.h>		/* struct timespec */
#endif

#include "cuppa/cmd.h"		/* struct cmd, check_commands */
#include "cuppa/io.h"           /* response */

#include "audio.h"
#include "constants.h"
#include "cuppa/constants.h"
#include "messages.h"
#include "player.h"

/* Names of the states in enum state. */
const char	STATES[NUM_STATES][WORD_LEN] = {
	"Void",
	"Ejct",
	"Stop",
	"Play",
	"Quit",
};

player::player(int device)
{
	this->cstate = S_EJCT;
	this->device = device;
	this->au = nullptr;
}

void
player::main_loop()
{
	enum error	err = E_OK;

	/* Set of commands that can be performed on the player. */
	command_set PLAYER_CMDS = {
		/* Nullary commands */
		{ "play", [&](const cmd_words &) { return this->cmd_play(); } },
		{ "stop", [&](const cmd_words &) { return this->cmd_stop(); } },
		{ "ejct", [&](const cmd_words &) { return this->cmd_ejct(); } },
		{ "quit", [&](const cmd_words &) { return this->cmd_quit(); } },
		/* Unary commands */
		{ "load", [&](const cmd_words &words) { return this->cmd_load(words[1]); } },
		{ "seek", [&](const cmd_words &words) { return this->cmd_seek(words[1]); } }
	};

	response(R_OHAI, "%s", MSG_OHAI);	/* Say hello */
	while (state() != S_QUIT) {
		/*
		 * Possible Improvement: separate command checking and player
		 * updating into two threads.  Player updating is quite
		 * intensive and thus impairs the command checking latency.
		 * Do this if it doesn't make the code too complex.
		 */
		err = check_commands(PLAYER_CMDS);
		/* TODO: Check to see if err was fatal */
		loop_iter();

		std::this_thread::sleep_for(std::chrono::nanoseconds(LOOP_NSECS));
	}
	response(R_TTFN, "%s", MSG_TTFN);	/* Wave goodbye */
}

bool
player::cmd_ejct()
{
	bool valid = gate_state({ S_STOP, S_PLAY });
	if (valid) {
		this->au = nullptr;
		set_state(S_EJCT);
		this->ptime = 0;
	}
	return valid;
}

bool
player::cmd_play()
{
	bool valid = gate_state({ S_STOP }) && (this->au != nullptr);
	if (valid) {
		this->au->start();
		set_state(S_PLAY);
	}
	return valid;
}

bool
player::cmd_quit()
{
	cmd_ejct();
	set_state(S_QUIT);

	return true; // Always a valid command.
}

bool
player::cmd_stop()
{
	bool valid = gate_state({ S_PLAY });
	if (valid) {
		this->au->stop();
		set_state(S_STOP);
	}
	return valid;
}

bool
player::cmd_load(const std::string &filename)
{
	try {
		this->au = std::unique_ptr<audio>(new audio(filename, this->device));
		dbug("loaded %s", filename);
		set_state(S_STOP);
	}
	catch (enum error) {
		cmd_ejct();
	}

	return true; // Always a valid command.
}

bool
player::cmd_seek(const std::string &time_str)
{
	/* TODO: proper overflow checking */

	std::istringstream is(time_str);
	uint64_t time;
	std::string rest;
	is >> time >> rest;

	if (rest == "s" || rest == "sec") {
		time *= USECS_IN_SEC;
	}

	/* Weed out any unwanted states */
	bool valid = gate_state({ S_PLAY, S_STOP });
	if (valid) {
		//enum state current_state = this->cstate;

		//cmd_stop(); // We need the player engine stopped in order to seek
		this->au->seek_usec(time);
		//if (current_state == S_PLAY) {
			// If we were playing before we'd ideally like to resume
			//cmd_play();
		//}
	}

	return valid;
}

enum state
player::state()
{
	return this->cstate;
}

/* Performs an iteration of the player update loop. */
void
player::loop_iter()
{
	if (this->cstate == S_PLAY) {
		if (this->au->halted()) {
			cmd_ejct();
		} else {
			/* Send a time pulse upstream every TIME_USECS usecs */
			uint64_t time = this->au->usec();
			if (time / TIME_USECS > this->ptime / TIME_USECS) {
				response(R_TIME, "%u", time);
			}
			this->ptime = time;
		}
	}
	if (this->cstate == S_PLAY || this->cstate == S_STOP) {
		this->au->decode();
	}
}

/* Throws an error if the current state is not in the state set provided by
 * the initializer_list.
 */
bool
player::gate_state(std::initializer_list<enum state> states)
{
	bool		in_state = false;
	for (enum state state : states) {
		if (this->cstate == state) {
			in_state = true;
		}
	}
	return in_state;
}

/* Sets the player state and honks accordingly. */
void
player::set_state(enum state state)
{
	enum state pstate = this->cstate;

	this->cstate = state;

	response(R_STAT, "%s %s", STATES[pstate], STATES[state]);
}
