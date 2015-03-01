// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the Player class, and associated types.
 * @see player.cpp
 */

#ifndef PLAYD_PLAYER_HPP
#define PLAYD_PLAYER_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "audio/audio_system.hpp"
#include "audio/audio.hpp"
#include "response.hpp"
#include "cmd_result.hpp"

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
	 * @param audio The AudioSystem to be used by the player.
	 */
	Player(AudioSystem &audio);

	/// Deleted copy constructor.
	Player(const Player &) = delete;

	/// Deleted copy-assignment constructor.
	Player &operator=(const Player &) = delete;

	/**
	 * Sets the sink to which this Player shall send responses.
	 * @param sink The response sink.
	 */
	void SetSink(ResponseSink &sink);

	//
	// Commands
	//

	/**
	 * Handles a command line.
	 * @param cmd A reference to the list of words in the command.
	 * @return Whether the command succeeded.
	 */
	CommandResult RunCommand(const std::vector<std::string> &words);

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
	 * @param id The ID of the new client inside the IO system.
	 */
	void WelcomeClient(size_t id) const;

private:
	//
	// Commands
	//

	/**
	 * Runs a nullary (0-argument) command.
	 * @param word The command word.
	 * @return True if the command was successfully found and executed;
	 *   false otherwise.
	 */
	CommandResult RunNullaryCommand(const std::string &word);

	/**
	 * Runs a unary (1-argument) command.
	 * @param word The command word.
	 * @param arg The argument to the command.
	 * @return True if the command was successfully found and executed;
	 *   false otherwise.
	 */
	CommandResult RunUnaryCommand(const std::string &word,
	                              const std::string &arg);

	//
	// Playback control
	//

	/**
	 * Tells the audio file to start or stop playing.
	 * @param playing True if playing; false otherwise.
	 * @see Play
	 * @see Stop
	 */
	CommandResult SetPlaying(bool playing);

	/**
	 * Ejects the current loaded song, if any.
	 * @return Whether the ejection succeeded.
	 */
	CommandResult Eject();

	/**
	 * Loads a track.
	 * @param path The absolute path to a track to load.
	 * @return Whether the load succeeded.
	 */
	CommandResult Load(const std::string &path);

	/// Handles ending a file (stopping and rewinding).
	void End();

	//
	// Seeking
	//

	/**
	 * Seeks to a given position in the current track.
	 * @param time_str A string containing a timestamp, followed by the
	 *   shorthand for the units of time in which the timestamp is measured
	 *   relative to the start of the track.  If the latter is omitted,
	 *   microseconds are assumed.
	 * @return Whether the seek succeeded.
	 */
	CommandResult Seek(const std::string &time_str);

	/**
	 * Parses time_str as a seek timestamp.
	 * @param time_str The time string to be parsed.
	 * @return The parsed time.
	 * @exception std::out_of_range
	 *   See http://www.cplusplus.com/reference/string/stoull/#exceptions
	 * @exception std::invalid_argument
	 *   See http://www.cplusplus.com/reference/string/stoull/#exceptions
	 * @exception SeekError
	 *   Raised if checks beyond those done by stoull fail.
	 */
	static std::uint64_t SeekParse(const std::string &time_str);

	/**
	 * Performs an actual seek.
	 * This does not do any EOF handling.
	 * @param pos The new position, in microseconds.
	 * @exception SeekError
	 *   Raised if the seek is out of range (usually EOF).
	 * @see Player::Seek
	 */
	void SeekRaw(std::uint64_t pos);

	//
	// Other
	//

	/**
	 * Quits playd.
	 * @return Whether the quit succeeded.
	 */
	CommandResult Quit();

	/**
	 * Asks the current file to dump all of its state to the connection
	 * with the given ID.
	 * @param id The ID of the connection to receive the dump.
	 * @note This is a pointer, not a reference, so as to allow nullptr
	 *   (which means no sink is assigned).  When `optional` becomes
	 *   standard, perhaps use that.
	 */
	void EmitAllAudioState(size_t id) const;
};

#endif // PLAYD_PLAYER_HPP
