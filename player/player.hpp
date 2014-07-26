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

#include "../audio/audio.hpp"
#include "../time_parser.hpp"
#include "../io/io_response.hpp"

#include "player_position.hpp"
#include "player_state.hpp"

class AudioSystem;

/**
 * A player contains a loaded audio file and the state of its playback.
 * Player connects to the audio system via PortAudio, given a device handle.
 */
class Player {
public:
	/// The type of TimeParser the Player expects.
	using TP = TimeParser<std::chrono::microseconds>;

private:
	const AudioSystem &audio_system;
	const TP &time_parser;

	std::unique_ptr<Audio> audio;

	PlayerPosition position;
	PlayerState state;

public:
	/**
	 * Constructs a Player.
	 * @param audio_system The audio system object.
	 * @param time_parser The parser used to interpret Seek commands.
	 */
	Player(const AudioSystem &audio_system, const TP &time_parser);

	/// Deleted copy constructor.
	Player(const Player &) = delete;

	/// Deleted copy-assignment constructor.
	Player &operator=(const Player &) = delete;

	/**
	 * Returns whether this Player is still running.
	 * @return True if this player is not in the QUITTING state; false
	 *   otherwise.
	 */
	bool IsRunning() const;

	/**
	 * Ejects the current loaded song, if any.
	 * @return Whether the ejection succeeded.
	 */
	bool Eject();

	/**
	 * Plays the current loaded song, if any.
	 * @return Whether the starting of playback succeeded.
	 */
	bool Play();

	/**
	 * Quits Playslave++.
	 * @return Whether the quit succeeded.
	 */
	bool Quit();

	/**
	 * Stops the currently playing track, if any.
	 * This behaves like a pause in other audio players: to reset the track
	 * to its start, issue a seek command afterwards.
	 * @return Whether the stop succeeded.
	 */
	bool Stop();

	/**
	 * Loads a track.
	 * @param path The absolute path to a track to load.
	 * @return Whether the load succeeded.
	 */
	bool Load(const std::string &path);

	/**
	 * Seeks to a given position in the current track.
	 * @param time_str A string containing a timestamp, followed by the
	 *   shorthand for the units of time in which the timestamp is measured
	 *   relative to the start of the track.  If the latter is omitted,
	 *   microseconds are assumed.
	 * @return Whether the seek succeeded.
	 */
	bool Seek(const std::string &time_str);

	/**
	 * A human-readable string representation of the current state.
	 * @return The current state, as a human-readable string..
	 */
	const std::string &CurrentStateString() const;

	/**
	 * Instructs the Player to perform a cycle of work.
	 * This includes decoding the next frame and responding to commands.
	 * @return Whether the player has more cycles of work to do.
	 */
	bool Update();

	/**
	 * Registers a responder with the position and state subsystems.
	 * This responder is sent information on position and state changes
	 * periodically.
	 * @param responder The Responder to register with the Player.
	 */
	void SetResponder(Responder &listener);

	/**
	 * Sets the period between position responses.
	 * @param period The period to wait between responses.
	 */
	void SetPositionResponseCodePeriod(PlayerPosition::Unit period);

	/**
	 * Sends welcome/current status information to a new client.
	 * @param client An IO Responder to which messages to the client
	 *   should be sent.
	 */
	void WelcomeClient(Responder &client);

private:
	/**
	 * Executes a closure iff the current state is one of the given states.
	 * @param states The initialiser list of states.
	 * @param f The closure to execute if in the correct state.
	 * @return False if the state was not valid, or the result of the
	 *   closure otherwise.
	 */
	bool IfCurrentStateIn(PlayerState::List states,
		              PlayerState::StateRestrictedFunction f);

	/**
	 * Checks to see if the current state is one of the given states.
	 * @param states The initialiser list of states.
	 * @return Whether the current state is not in the states given by the
	 *   initializer_list.
	 */
	bool CurrentStateIn(PlayerState::List states) const;

	/**
	 * Sets the current player state.
	 * @param state The new state.
	 */
	void SetState(PlayerState::State state);

	/**
	 * Parses a time string into a pair of unit prefix and timestamp.
	 * @param time_str The time string to parse.
	 * @return A pair of unit prefix and timestamp.
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
	 *   seek has happened, or a new file has been loaded).
	 * @see UpdatePosition
	 */
	void ResetPosition();

	/**
	 * Performs player updates necessary while the player is playing.
	 * @return True (so that the function can be used in an IfPlayerStateIn
	 *   call).
	 */
	bool PlaybackUpdate();

	/**
	 * Opens a file, setting this->audio to the resulting file.
	 * Generally, you should use Load instead.
	 * @param path The absolute path to a track to load.
	 */
	void OpenFile(const std::string &path);
};

#endif // PS_PLAYER_HPP
