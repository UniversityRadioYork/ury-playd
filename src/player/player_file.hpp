// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the PlayerFile class, and associated types.
 * @see player/player_file.cpp
 */

#ifndef PLAYD_PLAYER_FILE_HPP
#define PLAYD_PLAYER_FILE_HPP

#include <memory>

#include "../audio/audio_system.hpp"
#include "../audio/audio.hpp"
#include "../io/io_response.hpp"
#include "../time_parser.hpp"
#include "player_position.hpp"

/**
 * The subcomponent of a Player that stores the audio file.
 * This itself adheres to the Audio interface.
 * @see Audio
 * @see Player
 */
class PlayerFile : public ResponseSource, public Audio
{
public:
	/**
	 * Constructs a new PlayerFile.
	 * @param file_sink The ResponseSink to which FILE notifications are sent.
	 * @param audio_system The audio system to use when loading
	 *   Audio.
	 */
	PlayerFile(const ResponseSink *file_sink, const AudioSystem &audio_system);

	//
	// File operations
	//

	/**
	 * Ejects the current file.
	 * @see Load
	 */
	void Eject();

	/**
	 * Opens a file and stores it in this PlayerFile.
	 * @param path The absolute path to a track to load.
	 * @see Eject
	 */
	void Load(const std::string &path);

	//
	// Audio interface
	//

	void Start() override;
	void Stop() override;
	void Emit(const ResponseSink &sink) const override;
	Audio::State Update() override;
	TimeParser::MicrosecondPosition Position() const override;
	void Seek(TimeParser::MicrosecondPosition position) override;

private:
	/// The audio file.
	std::unique_ptr<Audio> audio;

	/// The audio system responsible for creating audio files.
	const AudioSystem &audio_system;
};

#endif // PLAYD_PLAYER_FILE_HPP
