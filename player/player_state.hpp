// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
* @file
* Declaration of the PlayerState class, and associated types.
* @see player/player_state.cpp
* @see player/player.hpp
* @see player/player.cpp
* @see player/player_position.hpp
* @see player/player_position.cpp
*/

#ifndef PS_PLAYER_STATE_HPP
#define PS_PLAYER_STATE_HPP

#include <cstdint>
#include <initializer_list>
#include <functional>
#include <boost/optional.hpp>

#include "../io/io_response.hpp"

/**
 * An object holding and representing the current state of a Player.
 * @see Player
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

	/// A function that is executed by IfIn if the current state is in
	/// the given List.
	using StateRestrictedFunction = std::function<bool()>;

	/// List of states in which the player is playing something.
	const static List AUDIO_PLAYING_STATES;

	/// List of states in which some audio is loaded.
	const static List AUDIO_LOADED_STATES;

	/// Constructs a PlayerState.
	PlayerState();

	/**
	 * Announces the current state to a Responder.
	 * @param responder The responder to which the current state shall be
	 *   emitted.
	 */
	const void Emit(Responder &target) const override;

	/**
	 * Executes a closure iff the current state is one of the given states.
	 * @param states The initialiser list of states.
	 * @param f The closure to execute if in the correct state.
	 * @return False if the state was not valid, or the result of the
	 *   closure otherwise.
	 */
	const bool IfIn(List states, StateRestrictedFunction f) const;

	/**
	 * Returns whether the player is still running.
	 * @return True if the player is not in the QUITTING state; false
	 *   otherwise.
	 */
	const bool PlayerState::IsRunning() const;

	/**
	 * Sets the player's current state.
	 * This will automatically emit the new state to any responder that has
	 * been registered with this PlayerState using SetResponder.
	 * @param state The new state.
	 */
	void Set(State state);

private:
	/// A mapping between states and their human-readable names.
	const static std::map<State, std::string> STRINGS;

	/// The current state.
	State current;

	const bool In(List states) const;
};

#endif // PS_PLAYER_STATE_HPP