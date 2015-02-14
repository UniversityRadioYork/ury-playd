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

#include "audio/audio_system.hpp"
#include "io.hpp"
#include "response.hpp"
#include "player.hpp"
#include "messages.h"

#ifdef WITH_FLAC
#include "audio/sources/flac.hpp"
#endif // WITH_FLAC
#ifdef WITH_MP3
#include "audio/sources/mp3.hpp"
#endif // WITH_MP3
#ifdef WITH_SNDFILE
#include "audio/sources/sndfile.hpp"
#endif // WITH_SNDFILE

/// The default IP hostname on which playd will bind.
static const std::string DEFAULT_HOST = "0.0.0.0";

/// The default TCP port on which playd will bind.
static const std::string DEFAULT_PORT = "1350";

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
 * Sets up the audio system with the desired sources and sinks.
 * @param audio The audio system to configure.
 */
void SetupAudioSystem(PipeAudioSystem &audio)
{
	audio.SetSink(&SdlAudioSink::Build);

// Now set up the available sources.
#ifdef WITH_FLAC
	audio.AddSource("flac", &FlacAudioSource::Build);
#endif // WITH_FLAC

#ifdef WITH_MP3
	mpg123_init();
	atexit(mpg123_exit);
	audio.AddSource("mp3", &Mp3AudioSource::Build);
#endif // WITH_MP3

#ifdef WITH_SNDFILE
	audio.AddSource("flac", &SndfileAudioSource::Build);
	audio.AddSource("ogg", &SndfileAudioSource::Build);
	audio.AddSource("wav", &SndfileAudioSource::Build);
#endif // WITH_SNDFILE
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
// If we don't ignore SIGPIPE, certain classes of connection droppage
// will crash our program with it.
// TODO(CaptainHayashi): a more rigorous ifndef here.
#ifndef _MSC_VER
	signal(SIGPIPE, SIG_IGN);
#endif

	// This call needs to happen before GetDeviceID, otherwise no device
	// IDs will be recognised.  (This is why it's here, and not in
	// SetupAudioSystem.)
	SdlAudioSink::InitLibrary();
	atexit(SdlAudioSink::CleanupLibrary);

	auto args = MakeArgVector(argc, argv);

	auto device_id = GetDeviceID(args);
	if (device_id < 0) ExitWithUsage(args.at(0));

	// Set up all of the components of playd in one fell swoop.
	PipeAudioSystem audio(device_id);
	SetupAudioSystem(audio);
	Player player(audio);
	IoCore io(player);

	// Make sure the player broadcasts its responses back to the IoCore.
	player.SetSink(io);

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
