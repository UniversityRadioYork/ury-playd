// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Main entry point and implementation of the playd class.
 * @see main.cpp
 */

#include <algorithm>
#include <iostream>

#include "audio/audio_system.hpp"
#include "cmd.hpp"
#include "io/io_core.hpp"
#include "io/io_response.hpp"
#include "main.hpp"
#include "messages.h"
#include "player/player.hpp"

/**
 * The main entry point.
 * @param argc Program argument count.
 * @param argv Program argument vector.
 * @return The exit code (zero for success; non-zero otherwise).
 */
int main(int argc, char *argv[])
{
	Playd ps(argc, argv);
	return ps.Run();
}

//
// Playd
//

const AudioSource::MicrosecondPosition Playd::POSITION_PERIOD(500000);

int Playd::GetDeviceID()
{
	if (this->arguments.size() < 2) return -1;

	/* Only accept valid numbers. */
	int id;
	try {
		id = std::stoi(this->arguments[1]);
	}
	catch (...) {
		/* Only std::invalid_argument and std::out_of_range are thrown
		 * here. */
		return -1;
	}

	/* Only allow valid (output) devices. */
	auto device_list = this->audio.GetDevicesInfo();
	if (this->audio.IsOutputDevice(id)) {
		return id;
	}

	return -1;
}

// Conversion rates from various position units to microseconds.
const AudioSource::MicrosecondPosition US_RATE = 1;
const AudioSource::MicrosecondPosition MS_RATE = 1000;
const AudioSource::MicrosecondPosition S_RATE = 1000000;
const AudioSource::MicrosecondPosition M_RATE = 60000000;
const AudioSource::MicrosecondPosition H_RATE = 3600000000;

/**
 * UNITS defines the unit suffixes that playd understands when parsing
 * positions in seek commands.
 *
 * If no suffix is given, it is treated as the string "".  Therefore, the
 * member of UNITS with suffix "" is the default unit.
 */
const Player::TP::UnitMap UNITS = {
	/// Microseconds; 1us = 1us.
	{ "us", US_RATE },
	{ "usec", US_RATE },
	{ "usecs", US_RATE },
	/// Milliseconds; 1ms = 1,000us.
	{ "ms", MS_RATE },
	{ "msec", MS_RATE },
	{ "msecs", MS_RATE },
	/// Seconds; 1s = 1,000ms = 1,000,000us.
	{ "s", S_RATE },
	{ "sec", S_RATE },
	{ "secs", S_RATE },
	/// Minutes; 1m = 60s = 60,000ms = 60,000,000us.
	{ "m", M_RATE },
	{ "min", M_RATE },
	{ "mins", M_RATE },
	/// Hours; 1h = 60m = 3,600s = 3,600,000ms = 3,600,000,000us.
	{ "h", H_RATE },
	{ "hour", H_RATE },
	{ "hours", H_RATE },
	/// When no unit is provided, we assume microseconds.
	{ "", US_RATE }
};

Playd::Playd(int argc, char *argv[])
    : audio(), player(audio, time_parser), handler(player), time_parser(UNITS)
{
	for (int i = 0; i < argc; i++) {
		this->arguments.push_back(std::string(argv[i]));
	}

	auto size = this->arguments.size();

	std::string addr = size > 2 ? this->arguments.at(2) : "0.0.0.0";
	std::string port = size > 3 ? this->arguments.at(3) : "1350";
	this->io = decltype(this->io)(
	                new IoCore(this->player, this->handler, addr, port));
}

int Playd::Run()
{
	try {
		// Don't roll this into the constructor: it'll go out of scope!
		int id = this->GetDeviceID();
		if (id == -1) {
			/* Oops, user entered an invalid sound device. */
			auto device_list = this->audio.GetDevicesInfo();
			for (const auto &device : device_list) {
				std::cout << device.first << ": "
				          << device.second << std::endl;
			}
			return EXIT_FAILURE;
		}
		this->audio.SetDeviceID(id);

		this->player.SetPositionResponsePeriod(POSITION_PERIOD);
		this->player.SetResponseSink(*this->io);
		this->io->Run();
	}
	catch (Error &error) {
		io->RespondWithError(error);
		Debug() << "Unhandled exception caught, going away now."
		        << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
