// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
* @file
* Declaration of the PlayerState class, and associated types.
* @see player/player_state.cpp
*/

#ifndef PS_PLAYER_STATE_HPP
#define PS_PLAYER_STATE_HPP

#include <cstdint>
#include <initializer_list>
#include <functional>

#include "../io/io_response.hpp"

/**
 * An object holding and representing the current state of a Player.
 * @see Player
 * @see PlayerPosition
 */
class PlayerState : public ResponseSource {
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

	/// A list of states, used for IfIn.
	using List = std::initializer_list<State>;

	/// List of states in which the player is playing something.
	const static List AUDIO_PLAYING_STATES;

	/// List of states in which some audio is loaded.
	const static List AUDIO_LOADED_STATES;

	/// Constructs a PlayerState.
	PlayerState();

	/**
	 * Announces the current state to a ResponseSink.
	 * @param sink The ResponseSink to which the current state shall be
	 *   emitted.
	 */
	void Emit(ResponseSink &sink) const override;

	/**
	 * Checks whether the current state is in a given list of states.
	 * @param states The initialiser list of states.
	 * @return True if the state is in the List; false otherwise.
	 */
	bool In(List states) const;

	/**
	 * Returns whether the player is still running.
	 * @return True if the player is not in the QUITTING state; false
	 *   otherwise.
	 */
	bool IsRunning() const;

	/**
	 * Sets the player's current state.
	 * This will automatically emit the new state to any responder that has
	 * been registered with this PlayerState using SetResponseSink.
	 * @param state The new state.
	 */
	void Set(State state);

private:
	/// A mapping between states and their human-readable names.
	const static std::map<State, std::string> STRINGS;

	/// The current state.
	State current;
};

#endif // PS_PLAYER_STATE_HPP
