// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Main entry point and implementation of the playd class.
 * @see main.cpp
 */

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <tuple>

#include "io.h"
#include "response.h"
#include "player.h"
#include "messages.h"

#ifdef WITH_MP3
#include "audio/sources/mp3.h"
#endif // WITH_MP3
#ifdef WITH_SNDFILE
#include "audio/sources/sndfile.h"
#endif // WITH_SNDFILE

/// The default IP hostname on which playd will bind.
static const std::string DEFAULT_HOST = "0.0.0.0";

/// The default TCP port on which playd will bind.
static const std::string DEFAULT_PORT = "1350";

/// Map from file extensions to AudioSource builder functions.
static const std::map<std::string, Player::SourceFn> SOURCES = {
#ifdef WITH_MP3
        {"mp3", &std::make_unique<Mp3AudioSource, const std::string &>},
#endif // WITH_MP3

#ifdef WITH_SNDFILE
        {"flac", &std::make_unique<SndfileAudioSource, const std::string &>},
        {"ogg", &std::make_unique<SndfileAudioSource, const std::string &>},
        {"wav", &std::make_unique<SndfileAudioSource, const std::string &>},
#endif // WITH_SNDFILE
};

/**
 * Creates a vector of strings from a C-style argument vector.
 * @param argc Program argument count.
 * @param argv Program argument vector.
 * @return The argument vector, as a C++ string vector.
 */
std::vector<std::string> MakeArgVector(int argc, char *argv[])
{
	std::vector<std::string> args;
	for (int i = 0; i < argc; i++) args.emplace_back(argv[i]);
	return args;
}

/**
 * Tries to get the output device ID from program arguments.
 * @param args The program argument vector.
 * @return The device ID, -1 if invalid selection (or none).
 */
int GetDeviceID(const std::vector<std::string> &args)
{
	// Did the user provide an ID at all?
	if (args.size() < 2) return -1;

	// Only accept valid numbers (stoi will throw for invalid ones).
	int id;
	try {
		id = std::stoi(args.at(1));
	} catch (...) {
		// Only std::{invalid_argument,out_of_range} are thrown here.
		return -1;
	}

	// Only allow valid, outputtable devices; reject input-only devices.
	if (!SdlAudioSink::IsOutputDevice(id)) return -1;

	return id;
}

/**
 * Reports usage information and exits.
 * @param progname The name of the program as executed.
 */
void ExitWithUsage(const std::string &progname)
{
	std::cerr << "usage: " << progname << " ID [HOST] [PORT]\n";
	std::cerr << "where ID is one of the following numbers:\n";

	// Show the user the valid device IDs they can use.
	auto device_list = SdlAudioSink::GetDevicesInfo();
	for (const auto &device : device_list) {
		std::cerr << "\t" << device.first << ": " << device.second
		          << "\n";
	}

	std::cerr << "default HOST: " << DEFAULT_HOST << "\n";
	std::cerr << "default PORT: " << DEFAULT_PORT << "\n";

	exit(EXIT_FAILURE);
}

/**
 * Gets the host and port from the program arguments.
 * The default arguments are used if the host and/or port are not supplied.
 * @param args The program argument vector.
 * @return A pair of strings representing the hostname and port.
 */
std::pair<std::string, std::string> GetHostAndPort(
        const std::vector<std::string> &args)
{
	auto size = args.size();
	return std::make_pair(size > 2 ? args.at(2) : DEFAULT_HOST,
	                      size > 3 ? args.at(3) : DEFAULT_PORT);
}

/**
 * Exits with an error message for a network error.
 * @param host The IP host to which playd tried to bind.
 * @param port The TCP port to which playd tried to bind.
 * @param msg The exception's error message.
 */
void ExitWithNetError(const std::string &host, const std::string &port,
                      const std::string &msg)
{
	std::cerr << "Network error: " << msg << "\n";
	std::cerr << "Is " << host << ":" << port << " available?\n";
	exit(EXIT_FAILURE);
}

/**
 * Exits with an error message for an unhandled exception.
 * @param msg The exception's error message.
 */
void ExitWithError(const std::string &msg)
{
	std::cerr << "Unhandled exception in main loop: " << msg << std::endl;
	exit(EXIT_FAILURE);
}

/**
 * The main entry point.
 * @param argc Program argument count.
 * @param argv Program argument vector.
 * @return The exit code (zero for success; non-zero otherwise).
 */
int main(int argc, char *argv[])
{
#ifndef _WIN32
	// If we don't ignore SIGPIPE, certain classes of connection droppage
	// will crash our program with it.
	// TODO(CaptainHayashi): a more rigorous ifndef here.
	signal(SIGPIPE, SIG_IGN);
#endif

	// SDL requires some cleanup and teardown.
	// This call needs to happen before GetDeviceID, otherwise no device
	// IDs will be recognised.  (This is why it's here, and not in
	// SetupAudioSystem.)
	SdlAudioSink::InitLibrary();
	atexit(SdlAudioSink::CleanupLibrary);

#ifdef WITH_MP3
	// mpg123 insists on us running its init and exit functions, too.
	mpg123_init();
	atexit(mpg123_exit);
#endif // WITH_MP3

	auto args = MakeArgVector(argc, argv);

	auto device_id = GetDeviceID(args);
	if (device_id < 0) ExitWithUsage(args.at(0));

	Player player(device_id,
	              &std::make_unique<SdlAudioSink, const AudioSource &, int>,
	              SOURCES);

	// Set up the IO now (to avoid a circular dependency).
	// Make sure the player broadcasts its responses back to the IoCore.
	IoCore io(player);
	player.SetIo(io);

	// Now, actually run the IO loop.
	std::string host;
	std::string port;
	std::tie(host, port) = GetHostAndPort(args);
	try {
		io.Run(host, port);
	} catch (NetError &e) {
		ExitWithNetError(host, port, e.Message());
	} catch (Error &e) {
		ExitWithError(e.Message());
	}

	return EXIT_SUCCESS;
}
