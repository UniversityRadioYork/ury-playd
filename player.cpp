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

Player::Player(const std::string &device) : device(device)
{
	this->current_state = State::EJECTED;
	this->au = nullptr;

	this->position_listener = nullptr;
	this->position_period = 0;
	this->position_last = 0;
}

bool Player::Eject()
{
	bool valid = CurrentStateIn({ State::STOPPED, State::PLAYING });
	if (valid) {
		this->au = nullptr;
		SetState(State::EJECTED);
		this->position_last = 0;
	}
	return valid;
}

bool Player::Play()
{
	bool valid = CurrentStateIn({ State::STOPPED }) && (this->au != nullptr);
	if (valid) {
		this->au->start();
		SetState(State::PLAYING);
	}
	return valid;
}

bool Player::Quit()
{
	Eject();
	SetState(State::QUITTING);

	return true; // Always a valid command.
}

bool Player::Stop()
{
	bool valid = CurrentStateIn({ State::PLAYING });
	if (valid) {
		this->au->stop();
		SetState(State::STOPPED);
	}
	return valid;
}

bool Player::Load(const std::string &filename)
{
	try {
		this->au = std::unique_ptr<audio>(new audio(filename, this->device));
		dbug("loaded %s", filename);
		SetState(State::STOPPED);
	}
	catch (enum error) {
		Eject();
	}

	return true; // Always a valid command.
}

bool Player::Seek(const std::string &time_str)
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
	bool valid = CurrentStateIn({ State::PLAYING, State::STOPPED });
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

State Player::CurrentState()
{
	return this->current_state;
}

/* Performs an iteration of the player update loop. */
void Player::Update()
{
	if (this->current_state == State::PLAYING) {
		if (this->au->halted()) {
			Eject();
		} else {
			SendPositionIfReady();
		}
	}
	if (CurrentStateIn({ State::PLAYING, State::STOPPED }))	{
		bool more = this->au->decode();
		if (!more) {
			Eject();
		}
	}
}

/* Throws an error if the current state is not in the state set provided by
 * the initializer_list.
 */
bool Player::CurrentStateIn(std::initializer_list<State> states)
{
	bool in_state = false;
	for (State state : states) {
		if (this->current_state == state) {
			in_state = true;
		}
	}
	return in_state;
}

/* Sets the player state and honks accordingly. */
void Player::SetState(State state)
{
	State last_state = this->current_state;

	this->current_state = state;

	if (this->state_listener != nullptr) {
		this->state_listener(last_state, state);
	}
}

/**
 * Registers a listener for state changes.
 * @param listener The function to which state change signals shall be sent.
 */
void Player::RegisterStateListener(StateListener listener)
{
	this->state_listener = listener;
}

/**
 * Sends a position signal to the outside environment, if ready to send one.
 * This only sends a signal if the requested amount of time has passed since the last one.
 * It requires a handler to have been registered via SetTimeSignalHandler.
 */
void Player::SendPositionIfReady()
{
	uint64_t position = this->au->usec();
	if (IsReadyToSendPosition(position)) {
		this->position_listener(position);
		this->position_last = position;
	}
}

/**
 * Figures out whether it's time to send a position signal.
 * @param current_time The current position in the song.
 * @return True if enough time has elapsed for a signal to be sent; false otherwise.
 */
bool Player::IsReadyToSendPosition(uint64_t current_position)
{
	bool ready = false;

	if (this->position_listener != nullptr) {
		ready = (current_position - this->position_last) > this->position_period;
	}

	return ready;
}

/**
 * Registers a listener for position signals.
 * @param listener The function to which position signals shall be sent.
 * @param period_usecs The approximate period, in microseconds, between position signals.
 */
void Player::RegisterPositionListener(PositionListener listener, uint64_t period_usecs)
{
	this->position_listener = listener;
	this->position_period = period_usecs;
}
