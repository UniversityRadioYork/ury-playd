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
enum class State {
	STARTING,
	EJECTED,
	STOPPED,
	PLAYING,
	QUITTING
};

typedef std::function<void(uint64_t)> PositionListener;
typedef std::function<void(State, State)> StateListener;

/* The player structure contains all persistent state in the program.
 *
 */
class player {
public:
	player(int driver);

	bool Eject();
	bool Play();
	bool Quit();
	bool Stop();

	bool Load(const std::string &path);
	bool Seek(const std::string &time_str);

	State CurrentState();

	void Update();

	void RegisterPositionListener(PositionListener listener, uint64_t position_usecs);
	void RegisterStateListener(StateListener listener);


private:
	int device;

	std::unique_ptr<audio> au;

	PositionListener position_listener;
	uint64_t position_period;
	uint64_t position_last;

	StateListener state_listener;
	State current_state;

	bool CurrentStateIn(std::initializer_list<State> states);
	void SetState(State state);

	void SendPositionIfReady();
	bool IsReadyToSendPosition(uint64_t current_position);
};


#endif				/* not PLAYER_H */
