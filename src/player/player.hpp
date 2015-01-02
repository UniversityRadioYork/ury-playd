// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the Player class, and associated types.
 * @see player/player.cpp
 */

#ifndef PLAYD_PLAYER_HPP
#define PLAYD_PLAYER_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../audio/audio_system.hpp"
#include "../audio/audio.hpp"
#include "../io/io_response.hpp"
#include "../cmd_result.hpp"

/**
 * A Player contains a loaded audio file and a command API for manipulating it.
 * @see Audio
 * @see AudioSystem
 */
class Player
{
private:
	AudioSystem &audio;          ///< The system used for loading audio.
	std::unique_ptr<Audio> file; ///< The currently loaded Audio.
	bool is_running;             ///< Whether the Player is running.
	const ResponseSink *sink;    ///< The sink for audio responses.

	/// The set of features playd implements.
	const static std::vector<std::string> FEATURES;

public:
	/**
	 * Constructs a Player.
	 * @param sink The sink to which audio notifications are sent.
	 * @param audio The AudioSystem to be used by the player.
	 */
	Player(const ResponseSink *sink, AudioSystem &audio);

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
	void SetPositionResponsePeriod(std::uint64_t period);

	/**
	 * Sends welcome/current status information to a new client.
	 * @param client An IO ResponseSink to which messages to the client
	 *   should be sent.
	 */
	void WelcomeClient(ResponseSink &client) const;

private:
	/**
	 * Performs an actual seek.
	 * This does not do any EOF handling.
	 * @param pos The new position, in microseconds.
	 * @exception SeekError
	 *   Raised if the seek is out of range (usually EOF).
	 * @see Player::Seek
	 */
	void SeekRaw(std::uint64_t pos);

	/// Handles ending a file (stopping and rewinding).
	void End();
};

#endif // PLAYD_PLAYER_HPP
