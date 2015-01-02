// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the playd class.
 * @see main.cpp
 */

#ifndef PLAYD_MAIN_HPP
#define PLAYD_MAIN_HPP

#include "audio/audio_system.hpp"
#include "cmd.hpp"
#include "io/io_core.hpp"
#include "io/io_response.hpp"
#include "player/player.hpp"
#include "player/player_position.hpp"

/**
 * The playd application.
 *
 * This class contains all the state required by playd, with the exception
 * of that introduced by external C libraries.  It is a RAII class, so
 * constructing Playd will load playd's library dependencies, and
 * destructing it will unload them.  It is probably not safe to create more than
 * one Playd.
 */
class Playd : public ResponseSink
{
public:
	/**
	 * Constructs an application instance, initialising its libraries.
	 * @param argc The argument count from the main function.
	 * @param argv The argument vector from the main function.
	 */
	Playd(int argc, char *argv[]);

	/**
	 * Runs playd.
	 * @return The exit code, which may be returned by the program.
	 */
	int Run();

private:
	/**
	 * The period between position announcements from the Player object.
	 * This is given in microseconds, so eg. 500000 = 0.5sec = 2Hz.
	 */
	static const std::uint64_t POSITION_PERIOD = 500000;

	/// The ID returned by GetDeviceID if something goes wrong.
	static const int INVALID_ID = -1;

	std::vector<std::string> argv; ///< The argument vector.
	PaAudioSystem audio;           ///< The audio subsystem.

	PlayerPosition pposition; ///< The player-position subsystem.
	Player player;            ///< The player subsystem.

	CommandHandler handler;     ///< The command handler.
	std::unique_ptr<IoCore> io; ///< The I/O handler.

	void Respond(const Response &response) const override;

	/**
	 * Tries to get the output device ID from program arguments.
	 * @return The device ID, INVALID_ID if invalid selection (or none).
	 */
	int GetDeviceID();
};

#endif // PLAYD_MAIN_HPP
