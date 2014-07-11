// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the Player class, and associated types.
 * @see player/player.cpp
 * @see player/player_position.hpp
 * @see player/player_position.cpp
 * @see player/player_state.cpp
 */

#ifndef PS_PLAYER_HPP
#define PS_PLAYER_HPP

#include <chrono>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "../audio/audio_output.hpp"
#include "../time_parser.hpp"

#include "player_position.hpp"

class AudioSystem;

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
	enum class State : std::uint8_t {
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

	/// The type of TimeParser the Player expects.
	using TP = TimeParser<std::chrono::microseconds>;

private:
	const AudioSystem &audio_system;
	const TP &time_parser;

	std::unique_ptr<AudioOutput> audio;

	PlayerPosition position;

	StateListener state_listener;
	State current_state;

public:
	/**
	 * Constructs a Player.
	 * @param audio_system  The audio system object.
	 * @param time_parser   The parser used to interpret Seek commands.
	 */
	Player(const AudioSystem &audio_system, const TP &time_parser);

	/// Deleted copy constructor.
	Player(const Player &) = delete;

	/// Deleted copy-assignment constructor.
	Player &operator=(const Player &) = delete;

	/**
	 * Returns whether this Player is still running.
	 * @return  True if this player is not in the QUITTING state; false
	 *          otherwise.
	 */
	bool IsRunning() const;

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
	/// A mapping between states and their human-readable names.
	const static std::map<State, std::string> STATE_STRINGS;

	/// Shorthand for {State::PLAYING, State::STOPPED}.
	const static StateList AUDIO_LOADED_STATES;

	/**
	 * Executes a closure iff the current state is one of the given states.
	 * @param states  The initialiser list of states.
	 * @param f       The closure to execute if in the correct state.
	 * @return        False if the state was not valid, or the result of the
	 *                closure otherwise.
	 */
	bool IfCurrentStateIn(StateList states, std::function<bool()> f);

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
	std::pair<std::string, std::uint64_t> ParseSeekTime(
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

	/**
	 * Opens a file, setting this->audio to the resulting file.
	 * Generally, you should use Load instead.
	 * @param path  The absolute path to a track to load.
	 */
	void OpenFile(const std::string &path);
};

#endif // PS_PLAYER_HPP
