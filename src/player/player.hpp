// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the Player class, and associated types.
 * @see player/player.cpp
 * @see player/player_position.hpp
 * @see player/player_position.cpp
 * @see player/player_state.cpp
 */

#ifndef PLAYD_PLAYER_HPP
#define PLAYD_PLAYER_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../audio/audio.hpp"
#include "../io/io_response.hpp"
#include "../cmd_result.hpp"
#include "../time_parser.hpp"

#include "player_file.hpp"
#include "player_position.hpp"
#include "player_state.hpp"

/**
 * A Player contains a loaded audio file and the state of its playback.
 * @see PlayerPosition
 * @see PlayerState
 */
class Player
{
private:
	PlayerFile file;         ///< The file subcomponent of the Player.
	PlayerPosition position; ///< The position subcomponent of the Player.
	PlayerState state;       ///< The state subcomponent of the Player.

	/// The time parser used to parse seek commands.
	const TimeParser &time_parser;

	/// The sink to which END responses shall be sent.
	ResponseSink *end_sink;

	/// The set of features playd implements.
	const static std::vector<std::string> FEATURES;

public:
	/**
	 * Constructs a Player.
	 * @param audio_system The audio system object.
	 * @param time_parser The parser used to interpret Seek commands.
	 */
	Player(const AudioSystem &audio_system, const TimeParser &time_parser);

	/// Deleted copy constructor.
	Player(const Player &) = delete;

	/// Deleted copy-assignment constructor.
	Player &operator=(const Player &) = delete;

	//
	// Commands
	//

	/**
	 * Ejects the current loaded song, if any.
	 * @return Whether the ejection succeeded.
	 */
	CommandResult Eject();

	/**
	 * Plays the current loaded song, if any.
	 * @return Whether the starting of playback succeeded.
	 */
	CommandResult Play();

	/**
	 * Quits playd.
	 * @return Whether the quit succeeded.
	 */
	CommandResult Quit();

	/**
	 * Stops the currently playing track, if any.
	 * This behaves like a pause in other audio players: to reset the track
	 * to its start, issue a seek command afterwards.
	 * @return Whether the stop succeeded.
	 */
	CommandResult Stop();

	/**
	 * Loads a track.
	 * @param path The absolute path to a track to load.
	 * @return Whether the load succeeded.
	 */
	CommandResult Load(const std::string &path);

	/**
	 * Seeks to a given position in the current track.
	 * @param time_str A string containing a timestamp, followed by the
	 *   shorthand for the units of time in which the timestamp is measured
	 *   relative to the start of the track.  If the latter is omitted,
	 *   microseconds are assumed.
	 * @return Whether the seek succeeded.
	 */
	CommandResult Seek(const std::string &time_str);

	//
	// Other methods
	//

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
	 * Registers a response sink with the Player.
	 * The sink is sent information on position, file and state changes
	 * periodically, as well as being notified when a file ends.
	 * @param sink The ResponseSink to register with the Player.
	 */
	void SetResponseSink(ResponseSink &sink);

	/**
	 * Sets the period between position responses.
	 * @param period The period to wait between responses.
	 */
	void SetPositionResponsePeriod(TimeParser::MicrosecondPosition period);

	/**
	 * Sends welcome/current status information to a new client.
	 * @param client An IO ResponseSink to which messages to the client
	 *   should be sent.
	 */
	void WelcomeClient(ResponseSink &client) const;

private:
	/**
	 * Parses a time string into a pair of unit prefix and timestamp.
	 * @param time_str The time string to parse.
	 * @return A pair of unit prefix and timestamp.
	 */
	std::pair<std::string, std::uint64_t> ParseSeekTime(
	                const std::string &time_str) const;

	/**
	 * Performs an actual seek.
	 * This does not do any EOF handling.
	 * @param pos The new position, in microseconds.
	 * @exception SeekError
	 *   Raised if the seek is out of range (usually EOF).
	 * @see Player::Seek
	 */
	void SeekRaw(TimeParser::MicrosecondPosition pos);

	/// Handles ending a file (stopping and rewinding).
	void End();
};

#endif // PLAYD_PLAYER_HPP
