/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#include <memory>
#include <sstream>
#include <vector>

#include <cstdarg>  /* CurrentStateIn */
#include <cstdbool> /* bool */
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <thread>
#include <chrono>

#ifdef WIN32
struct timespec {
	time_t tv_sec;
	long tv_nsec;
};
#include <Winsock2.h>
#else
#include <time.h> /* struct timespec */
#endif

#include "cmd.h" /* struct cmd, check_commands */
#include "io.hpp"

#include "audio.h"
#include "constants.h"
#include "messages.h"
#include "player.h"

Player::Player(const std::string &device) : device(device)
{
	this->current_state = State::EJECTED;
	this->au = nullptr;

	this->position_listener = nullptr;
	this->position_period = std::chrono::microseconds(0);
	this->position_last = std::chrono::microseconds(0);
	this->position_last_invalid = true;
}

bool Player::Eject()
{
	bool valid = CurrentStateIn({State::STOPPED, State::PLAYING});
	if (valid) {
		this->au = nullptr;
		SetState(State::EJECTED);
		this->position_last = std::chrono::microseconds(0);
	}
	return valid;
}

bool Player::Play()
{
	bool valid = CurrentStateIn({State::STOPPED}) && (this->au != nullptr);
	if (valid) {
		this->au->Start();
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
	bool valid = CurrentStateIn({State::PLAYING});
	if (valid) {
		this->au->Stop();
		SetState(State::STOPPED);
	}
	return valid;
}

bool Player::Load(const std::string &filename)
{
	if (filename.length() == 0) return false;

	try
	{
		this->au = std::unique_ptr<AudioOutput>(
		                new AudioOutput(filename, this->device));
		this->position_last_invalid = true;
		Debug("Loaded ", filename);
		SetState(State::STOPPED);
	}
	catch (Error &error)
	{
		error.ToResponse();
		Eject();
	}

	return true; // Always a valid command.
}

/**
 * A template for converting a uint64_t giving a duration in terms of T1 into
 * a duration expressed as a T2.
 *
 * For example, MkTime<std::chrono::seconds> takes in a uint64_t of seconds
 * and returns a std::chrono::microseconds.
 */
template <typename T1, typename T2 = std::chrono::microseconds>
T2 MkTime(uint64_t raw_time)
{
	return std::chrono::duration_cast<T2>(T1(raw_time));
}

typedef std::map<std::string,
                 std::function<std::chrono::microseconds(uint64_t)>>
                TimeSuffixMap;

/**
 * Mapping from unit suffixes for seek command times to functions converting
 * time integers to the appropriate duration in microseconds.
 */
static const TimeSuffixMap time_suffixes = {
                {"s", MkTime<std::chrono::seconds>},
                {"sec", MkTime<std::chrono::seconds>},
                {"secs", MkTime<std::chrono::seconds>},
                {"m", MkTime<std::chrono::minutes>},
                {"min", MkTime<std::chrono::minutes>},
                {"mins", MkTime<std::chrono::minutes>},
                {"h", MkTime<std::chrono::hours>},
                {"hour", MkTime<std::chrono::hours>},
                {"hours", MkTime<std::chrono::hours>},
                // Default when there is no unit
                {"", MkTime<std::chrono::microseconds>}};

std::pair<std::string, uint64_t> Player::ParseSeekTime(
                const std::string &time_str) const
{
	std::istringstream is(time_str);
	uint64_t raw_time;
	std::string rest;

	is >> raw_time >> rest;
	return std::make_pair(rest, raw_time);
}

bool Player::Seek(const std::string &time_str)
{
	bool valid = CurrentStateIn({State::PLAYING, State::STOPPED});
	if (valid) {
		auto seek = ParseSeekTime(time_str);
		std::chrono::microseconds position(0);

		try
		{
			position = time_suffixes.at(seek.first)(seek.second);
		}
		catch (std::out_of_range)
		{
			valid = false;
		}

		if (valid) {
			this->au->SeekToPosition(position);
			this->position_last_invalid = true;
		}
	}

	return valid;
}

State Player::CurrentState()
{
	return this->current_state;
}

void Player::Update()
{
	if (this->current_state == State::PLAYING) {
		if (this->au->IsHalted()) {
			Eject();
		} else {
			SendPositionIfReady();
		}
	}
	if (CurrentStateIn({State::PLAYING, State::STOPPED})) {
		if (!this->au->Update()) {
			Eject();
		}
	}
}

bool Player::CurrentStateIn(std::initializer_list<State> states)
{
	return std::find(states.begin(), states.end(), this->current_state) !=
	       states.end();
}

void Player::SetState(State state)
{
	State last_state = this->current_state;

	this->current_state = state;

	if (this->state_listener != nullptr) {
		this->state_listener(last_state, state);
	}
}

void Player::RegisterStateListener(StateListener listener)
{
	this->state_listener = listener;
}

void Player::SendPositionIfReady()
{
	auto position = this->au->CurrentPosition<std::chrono::microseconds>();
	if (IsReadyToSendPosition(position)) {
		this->position_listener(position);
		this->position_last = position;
		this->position_last_invalid = false;
	}
}

bool Player::IsReadyToSendPosition(std::chrono::microseconds current_position)
{
	bool ready = false;

	if (this->position_last_invalid) {
		ready = true;
	} else if (this->position_listener != nullptr) {
		auto difference = current_position - this->position_last;
		ready = difference >= this->position_period;
	}

	return ready;
}

void Player::RegisterPositionListener(PositionListener listener,
                                      const std::chrono::microseconds period)
{
	this->position_listener = listener;
	this->position_period = period;
}
