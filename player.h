/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
 */

/**
 * @file
 * Definition of Player and associated types.
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <memory>

#include "audio.h"

/**
 * Enumeration of states that the player can be in.
 * The player is effectively a finite-state machine whose behaviour
 * at any given time is dictated by the current state, which is
 * represented by an instance of State.
 */
enum class State {
	STARTING,
	EJECTED,
	STOPPED,
	PLAYING,
	QUITTING
};


/**
 * A player contains a loaded audio file and the state of its playback.
 * Player connects to the audio system via PortAudio, given a device handle.
 */
class Player {
public:
	typedef std::function<void(std::chrono::microseconds)> PositionListener;
	typedef std::function<void(State, State)> StateListener;
private:
	const std::string &device;

	std::unique_ptr<AudioOutput> au;

	PositionListener position_listener;
	std::chrono::microseconds position_period;
	std::chrono::microseconds position_last;
	bool position_last_invalid;

	StateListener state_listener;
	State current_state;

public:
	Player(const std::string &device);

	bool Eject();
	bool Play();
	bool Quit();
	bool Stop();
	bool Load(const std::string &path);
	bool Seek(const std::string &time_str);

	State CurrentState();

	void Update();

	void Player::RegisterPositionListener(PositionListener listener, const std::chrono::microseconds period);
	void RegisterStateListener(StateListener listener);
private:
	bool CurrentStateIn(std::initializer_list<State> states);
	void SetState(State state);

	void SendPositionIfReady();
	bool IsReadyToSendPosition(std::chrono::microseconds current_position);
};

#endif /* !PLAYER_H */
