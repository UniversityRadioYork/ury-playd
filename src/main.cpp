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

#include "audio/audio_system.hpp"
#include "io/io_core.hpp"
#include "io/io_response.hpp"
#include "player/player.hpp"
#include "cmd.hpp"
#include "messages.h"

#include "main.hpp"

#ifdef WITH_FLAC
#include "audio/sources/flac.hpp"
#endif // WITH_FLAC
#ifdef WITH_MP3
#include "audio/sources/mp3.hpp"
#endif // WITH_MP3
#ifdef WITH_SNDFILE
#include "audio/sources/sndfile.hpp"
#endif // WITH_SNDFILE

/**
 * The main entry point.
 * @param argc Program argument count.
 * @param argv Program argument vector.
 * @return The exit code (zero for success; non-zero otherwise).
 */
int main(int argc, char *argv[])
{
	SdlAudioSink::InitLibrary();

	Playd ps(argc, argv);
	auto res = ps.Run();

	SdlAudioSink::CleanupLibrary();
	return res;
}

//
// Playd
//

int Playd::GetDeviceID()
{
	// Did the user provide an ID at all?
	if (this->argv.size() < 2) return this->INVALID_ID;

	// Only accept valid numbers (stoi will throw for invalid ones).
	int id;
	try {
		id = std::stoi(this->argv.at(1));
	} catch (...) {
		// Only std::{invalid_argument,out_of_range} are thrown here.
		return this->INVALID_ID;
	}

	// Only allow valid, outputtable devices; reject input-only devices.
	if (!SdlAudioSink::IsOutputDevice(id)) return this->INVALID_ID;

	return id;
}

Playd::Playd(int argc, char *argv[]) : audio(), player(this, audio), handler(player)
{
	for (int i = 0; i < argc; i++) this->argv.emplace_back(argv[i]);
}

int Playd::Run()
{
	// Fill in some default arguments.
	// Note that we don't have a default device ID; if the user doesn't
	// supply an ID, we treat it as if they had supplied an invalid one.
	auto size = this->argv.size();
	std::string addr = size > 2 ? this->argv.at(2) : "0.0.0.0";
	std::string port = size > 3 ? this->argv.at(3) : "1350";

	// Now set up the device ID.
	// Do this now, so that an invalid ID is caught before we start trying
	// to acquire the network socket.
	int id = this->GetDeviceID();
	if (id == INVALID_ID) {
		// Show the user the valid device IDs they can use.
		auto device_list = SdlAudioSink::GetDevicesInfo();
		for (const auto &device : device_list) {
			std::cout << device.first << ": " << device.second
			          << std::endl;
		}
		return EXIT_FAILURE;
	}
	this->audio.SetSink(&SdlAudioSink::Build, id);

	// Now set up the available sources.
#ifdef WITH_FLAC
	this->audio.AddSource({ "flac" }, &FlacAudioSource::Build);
#endif // WITH_FLAC

#ifdef WITH_MP3
	this->audio.AddSource({ "mp3" }, &Mp3AudioSource::Build);
#endif // WITH_MP3

#ifdef WITH_SNDFILE
	this->audio.AddSource({ "flac", "ogg", "wav" }, &SndfileAudioSource::Build);
#endif // WITH_SNDFILE

	// Now set up all the IO (network socket and event loop).
	try {
		this->io = std::unique_ptr<IoCore>(new IoCore(
		                this->player, this->handler, addr, port));
	} catch (NetError &e) {
		std::cerr << "Network error: " << e.Message() << std::endl;
		std::cerr << "Is " << addr << ":" << port << " available?"
		          << std::endl;
		return EXIT_FAILURE;
	}

	try {
		this->io->Run();
	} catch (Error &error) {
		std::cerr << "Unhandled exception in main loop: "
		          << error.Message() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void Playd::Respond(const Response &response) const
{
	if (this->io != nullptr) this->io->Respond(response);
}
