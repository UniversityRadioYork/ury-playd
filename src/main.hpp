// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the playd class.
 * @see main.cpp
 */

#ifndef PS_MAIN_HPP
#define PS_MAIN_HPP

#include "audio/audio_system.hpp"
#include "cmd.hpp"
#include "io/io_core.hpp"
#include "player/player.hpp"
#include "time_parser.hpp"

/**
 * The playd application.
 *
 * This class contains all the state required by playd, with the exception
 * of that introduced by external C libraries.  It is a RAII class, so
 * constructing Playd will load playd's library dependencies, and
 * destructing it will unload them.  It is probably not safe to create more than
 * one Playd.
 */
class Playd {
public:
	/**
	 * Constructs a playd, initialising its libraries.
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
	/// The period between position announcements from the Player object.
	static const TimeParser::MicrosecondPosition POSITION_PERIOD;

	/// The ID returned by GetDeviceID if something goes wrong.
	static const int INVALID_ID;

	std::vector<std::string> arguments; ///< The argument vector.
	AudioSystem audio;                  ///< The audio subsystem.
	Player player;                      ///< The player subsystem.
	CommandHandler handler;             ///< The command handler.
	TimeParser time_parser;             ///< The seek time parser.
	std::unique_ptr<IoCore> io;         ///< The I/O handler.

	/**
	 * Tries to get the output device ID from program arguments.
	 * @return The device ID, INVALID_ID if invalid selection (or none).
	 */
	int GetDeviceID();

	/**
	 * Lists on stdout all sound devices to which the audio output may
	 * connect.
	 * This is mainly for the benefit of the end user.
	 */
	void ListOutputDevices();

	/**
	 * Registers the playd command set on the given Player.
	 * @param p The Player on which the commands will act.
	 */
	void RegisterCommands(Player *p);
};

#endif // PS_MAIN_HPP
