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

#include "../audio.hpp"
#include "../cmd.hpp"

#include "player_position.hpp"

/**
 * A player contains a loaded audio file and the state of its playback.
 * Player connects to the audio system via PortAudio, given a device handle.
 */
class Player {
public:
	/**
	 * Enumeration of states that the player can be in.
	 * The player is effectively a finite-state machine whose behaviour
	 * at any given time is dictated by the current state, which is
	 * represented by an instance of State.
	 */
	enum class State {
		STARTING, ///< The player has just initialised.
		EJECTED,  ///< The player has no song loaded.
		STOPPED,  ///< The player has a song loaded and not playing.
		PLAYING,  ///< The player has a song loaded and playing.
		QUITTING  ///< The player is about to terminate.
	};

	/**
	 * Type for state listeners.
	 * @see RegisterStateListener
	 */
	using StateListener = std::function<void(State, State)>;

	/**
	 * A list of states.
	 */
	using StateList = std::initializer_list<State>;

private:
	const std::string &device;

	std::unique_ptr<AudioOutput> audio;

	PlayerPosition position;

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
	 * Returns whether this Player is still running.
	 * @return  True if this player is not in the QUITTING state; false
	 *          otherwise.
	 */
	bool IsRunning() const;

	/**
	 * Creates a function object that ejects this player.
	 * @return A function binding Eject.
	 */
	CommandHandler::NullAction EjectAction();

	/**
	 * Ejects the current loaded song, if any.
	 * @return  Whether the ejection succeeded.
	 */
	bool Eject();

	/**
	 * Creates a function object that starts this player.
	 * @return A function binding Play.
	 */
	CommandHandler::NullAction PlayAction();

	/**
	 * Plays the current loaded song, if any.
	 * @return  Whether the starting of playback succeeded.
	 */
	bool Play();

	/**
	 * Creates a function object that closes this player.
	 * @return A function binding Quit.
	 */
	CommandHandler::NullAction QuitAction();

	/**
	 * Quits Playslave++.
	 * @return  Whether the quit succeeded.
	 */
	bool Quit();

	/**
	 * Creates a function object that stops this player.
	 * @return A function binding Stop.
	 */
	CommandHandler::NullAction StopAction();

	/**
	 * Stops the currently playing track, if any.
	 *
	 * This behaves like a pause in other audio players: to reset the track
	 * to its start, issue a seek command afterwards.
	 * @return  Whether the stop succeeded.
	 */
	bool Stop();

	/**
	 * Creates a function object that loads into this player.
	 * @return A function binding Load.
	 */
	CommandHandler::SingleRequiredWordAction LoadAction();

	/**
	 * Loads a track.
	 *
	 * @param path  The absolute path to a track to load.
	 * @return      Whether the load succeeded.
	 */
	bool Load(const std::string &path);

	/**
	 * Creates a function object that seeks this player.
	 * @return A function binding Seek.
	 */
	CommandHandler::SingleRequiredWordAction SeekAction();

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
	 * A human-readable string representation of the current state.
	 * @return The current state, as a human-readable string..
	 */
	const std::string &CurrentStateString() const;

	/**
	 * The current state of the Player.
	 * @return The current state.
	 */
	State CurrentState() const;

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
	 */
	void RegisterPositionListener(PlayerPosition::Listener listener);

	/**
	 * Sets the period between position signals.
	 * This is shared across all listeners.
	 * @param period  The period to wait between listener callbacks.
	 */
	void SetPositionListenerPeriod(PlayerPosition::Unit period);

	/**
	 * Registers a position listener.
	 *
	 * This listener is notified on state changes.
	 * @param listener  The listener callback.
	 */
	void RegisterStateListener(StateListener listener);

	/**
	 * The human-readable name of the given player state.
	 * @param state  The state whose name is to be returned.
	 * @return       The human-readable name of @a state.
	 */
	static const std::string &StateString(State state);

private:
	/**
	 * A mapping between states and their human-readable names.
	 */
	const static std::map<State, std::string> STATE_STRINGS;

	/**
	 * Executes a closure iff the current state is one of the given states.
	 * @param states  The initialiser list of states.
	 * @param f       The closure to execute if in the correct state.
	 * @return        False if the state was not valid, or the result of the
	 *                closure otherwise.
	 */
	bool IfCurrentStateIn(StateList states, std::function<bool()> f);

	/**
	 * Executes a closure iff the current state is one of the given states.
	 * @param states  The initialiser list of states.
	 * @param f       The closure to execute if in the correct state.
	 * @return        False if the state was not valid, or true otherwise.
	 */
	bool IfCurrentStateIn(StateList states, std::function<void()> f);

	/**
	 * Checks to see if the current state is one of the given states.
	 * @param states  The initialiser list of states.
	 * @return        Whether the current state is not in the states given
	 *                by the initializer_list.
	 */
	bool CurrentStateIn(StateList states) const;

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
	std::pair<std::string, uint64_t> ParseSeekTime(
	                const std::string &time_str) const;

	/**
	 * Updates the player position to reflect changes in the audio system.
	 * Call this whenever the audio position has changed.
	 * @see ResetPosition
	 */
	void UpdatePosition();

	/**
	 * Resets the player position.
	 * Call this whenever the audio position has changed drastically (eg a
	 * seek has happened, or a new file has been loaded).
	 * @see UpdatePosition
	 */
	void ResetPosition();
};

#endif // PS_PLAYER_HPP
