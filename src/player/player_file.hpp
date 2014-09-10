// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the PlayerFile class, and associated types.
 * @see player/player_file.cpp
 */

#ifndef PS_PLAYER_FILE_HPP
#define PS_PLAYER_FILE_HPP

#include <memory>

#include "../audio/audio_system.hpp"
#include "../io/io_response.hpp"
#include "player_position.hpp"

class Audio;

/**
 * The subcomponent of a Player that stores the audio file.
 * @see Player
 */
class PlayerFile : public ResponseSource {
public:
	/**
	 * Constructs a new PlayerFile.
	 * @param audio_system The audio system to use when loading
	 *   Audio.
	 */
	PlayerFile(const AudioSystem &audio_system);

	/**
	 * Opens a file and stores it in this PlayerFile.
	 * @param path The absolute path to a track to load.
	 */
	void Load(const std::string &path);

	/// Starts the current file.
	void Start();

	/// Stops the current file.
	void Stop();

	/// Ejects the current file.
	void Eject();

	/**
	 * Emits the current filename to a ResponseSink.
	 * @param sink The ResponseSink to which a FILE response shall be
	 *   sent.
	 */
	void Emit(ResponseSink &sink) const;

	/**
	 * Checks to see if the file has stopped playing.
	 * @return True if the file has stopped; false otherwise.
	 * @see Start
	 * @see Stop
	 */
	bool IsStopped();

	/// Updates the file (performing a round of decoding on it, etc.).
	void Update();

	/**
	 * Return the current position.
	 * @return The current position in the audio, in PlayerPosition units.
	 */
	PlayerPosition::Unit CurrentPosition();

	/**
	 * Seek to a position.
	 * @param position The position to seek to in the audio,
	 *   in PlayerPosition units.
	 */
	void SeekToPosition(PlayerPosition::Unit position);

private:
	/// The audio file.
	std::unique_ptr<Audio> audio;

	/// The audio system responsible for creating audio files.
	const AudioSystem &audio_system;
};

#endif // PS_PLAYER_FILE_HPP
