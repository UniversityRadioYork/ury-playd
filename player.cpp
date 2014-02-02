/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
 */

#define _POSIX_C_SOURCE 200809

#include <memory>
#include <sstream>
#include <vector>

#include <cstdarg>		/* CurrentStateIn */
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

#include "cmd.h"		/* struct cmd, check_commands */
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
	this->ptime = 0;
}

bool
player::Eject()
{
	bool valid = CurrentStateIn({ S_STOP, S_PLAY });
	if (valid) {
		this->au = nullptr;
		SetState(S_EJCT);
		this->ptime = 0;
	}
	return valid;
}

bool
player::Play()
{
	bool valid = CurrentStateIn({ S_STOP }) && (this->au != nullptr);
	if (valid) {
		this->au->start();
		SetState(S_PLAY);
	}
	return valid;
}

bool
player::Quit()
{
	Eject();
	SetState(S_QUIT);

	return true; // Always a valid command.
}

bool
player::Stop()
{
	bool valid = CurrentStateIn({ S_PLAY });
	if (valid) {
		this->au->stop();
		SetState(S_STOP);
	}
	return valid;
}

bool
player::Load(const std::string &filename)
{
	try {
		this->au = std::unique_ptr<audio>(new audio(filename, this->device));
		dbug("loaded %s", filename);
		SetState(S_STOP);
	}
	catch (enum error) {
		Eject();
	}

	return true; // Always a valid command.
}

bool
player::Seek(const std::string &time_str)
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
	bool valid = CurrentStateIn({ S_PLAY, S_STOP });
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
player::Update()
{
	if (this->cstate == S_PLAY) {
		if (this->au->halted()) {
			Eject();
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
		bool more = this->au->decode();
		if (!more) {
			Eject();
		}
	}
}

/* Throws an error if the current state is not in the state set provided by
 * the initializer_list.
 */
bool
player::CurrentStateIn(std::initializer_list<enum state> states)
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
player::SetState(enum state state)
{
	enum state pstate = this->cstate;

	this->cstate = state;

	response(R_STAT, "%s %s", STATES[pstate], STATES[state]);
}
