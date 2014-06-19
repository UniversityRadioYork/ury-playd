/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

/**
 * @file
 * Definition of Player and associated types.
 */

#ifndef PS_PLAYER_HPP
#define PS_PLAYER_HPP

#include <string>
#include <memory>

#include "audio.hpp"

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
	/**
	 * Type for position listeners.
	 * @see RegisterPositionListener
	 */
	typedef std::function<void(std::chrono::microseconds)> PositionListener;

	/**
	 * Type for state listeners.
	 * @see RegisterStateListener
	 */
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
	/**
	 * Constructs a Player.
	 * @param device  A string uniquely identifying the target audio device
	 *                to the audio backend.  For example, for PortAudio this
	 *                will be the device ID.
	 */
	Player(const std::string &device);

	/**
	 * Ejects the current loaded song, if any.
	 * @return  Whether the ejection succeeded.
	 */
	bool Eject();

	/**
	 * Plays the current loaded song, if any.
	 * @return  Whether the starting of playback succeeded.
	 */
	bool Play();

	/**
	 * Quits Playslave++.
	 * @return  Whether the quit succeeded.
	 */
	bool Quit();

	/**
	 * Stops the currently playing track, if any.
	 *
	 * This behaves like a pause in other audio players: to reset the track
	 * to its start, issue a seek command afterwards.
	 * @return  Whether the stop succeeded.
	 */
	bool Stop();

	/**
	 * Loads a track.
	 *
	 * @param path  The absolute path to a track to load.
	 * @return      Whether the load succeeded.
	 */
	bool Load(const std::string &path);

	/**
	 * Seeks to a given position in the current track.
	 *
	 * @param time_str  A string containing a timestamp, followed by the
	 *                  shorthand for the units of time in which the
	 *                  timestamp is measured relative to the start of the
	 *                  track.  If the latter is omitted, microseconds are
	 *                  assumed.
	 * @return      Whether the seek succeeded.
	 */
	bool Seek(const std::string &time_str);

	/**
	 * The current state of the Player.
	 * @return The current state.
	 */
	State CurrentState();

	/**
	 * Instructs the Player to perform a cycle of work.
	 *
	 * This includes decoding the next frame and responding to commands.
	 */
	void Update();

	/**
	 * Registers a position listener.
	 *
	 * This listener is sent the current song position, in microseconds,
	 * roughly every @a period microseconds.
	 * @param listener  The listener callback.
	 * @param period    The period between callback executions.
	 */
	void RegisterPositionListener(PositionListener listener,
	                              const std::chrono::microseconds period);

	/**
	 * Registers a position listener.
	 *
	 * This listener is notified on state changes.
	 * @param listener  The listener callback.
	 */
	void RegisterStateListener(StateListener listener);

private:
	/**
	 * Checks to see if the current state is one of the given states.
	 * @return  Whether the current state is not in the states given by
	 *          the initializer_list.
	 */
	bool CurrentStateIn(std::initializer_list<State> states);

	/**
	 * Sets the current player state.
	 * @param state  The new state.
	 */
	void SetState(State state);

	/**
	 * Parses a time string into a pair of unit prefix and timestamp.
	 * @param time_str  The time string to parse.
	 * @return          A pair of unit prefix and timestamp.
	 */
	std::pair<std::string, uint64_t> ParseSeekTime(const std::string &time_str) const;

	/**
	 * Sends a position signal to the outside environment, if ready.
	 * This only sends a signal if the requested amount of time has passed
	 * since the last one.
	 * It requires a handler to have been registered via SetTimeSignalHandler.
	 */
	void SendPositionIfReady();

	/**
	 * Figures out whether it's time to send a position signal.
	 * @param current_position The current position in the song.
	 * @return True if enough time has elapsed for a signal to be sent; false
	 * otherwise.
	 */
	bool IsReadyToSendPosition(std::chrono::microseconds current_position);
};

#endif // PS_PLAYER_HPP
