// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Main entry point and implementation of the playd class.
 * @see main.cpp
 */

#include <algorithm>
#include <chrono>
#include <iostream>

#include "audio/audio_system.hpp"
#include "cmd.hpp"
#include "io/io_reactor.hpp"
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
	playd ps(argc, argv);
	return ps.Run();
}

//
// playd
//

const std::chrono::microseconds playd::POSITION_PERIOD(500000);

int playd::GetDeviceID()
{
	if (this->arguments.size() < 2) return -1;

	/* Only accept valid numbers. */
	int id;
	try
	{
		id = std::stoi(this->arguments[1]);
	}
	catch (...)
	{
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

/**
 * UNITS defines the unit suffixes that playd understands when parsing
 * positions in seek commands.
 *
 * The suffixes are defined using the Player::TP::MkTime template, in terms
 * of the std::chrono duration units.
 *
 * If no suffix is given, it is treated as the string "".  Therefore, the
 * member of UNITS with suffix "" is the default unit.
 *
 * The members of UNITS are documented by example.
 *
 * @see Player::TP::MkTime
 */
const Player::TP::UnitMap UNITS = {
	/// 1us = 1 microsecond.
	{ "us", Player::TP::MkTime<std::chrono::microseconds> },
	/// 1usec = 1 microsecond.
	{ "usec", Player::TP::MkTime<std::chrono::microseconds> },
	/// 2usecs = 2 microseconds.
	{ "usecs", Player::TP::MkTime<std::chrono::microseconds> },
	/// 1ms = 1 millisecond.
	{ "ms", Player::TP::MkTime<std::chrono::milliseconds> },
	/// 1msec = 1 millisecond.
	{ "msec", Player::TP::MkTime<std::chrono::milliseconds> },
	/// 2msecs = 2 milliseconds.
	{ "msecs", Player::TP::MkTime<std::chrono::milliseconds> },
	/// 1s = 1 second.
	{ "s", Player::TP::MkTime<std::chrono::seconds> },
	/// 1sec = 1 second.
	{ "sec", Player::TP::MkTime<std::chrono::seconds> },
	/// 2secs = 2 seconds.
	{ "secs", Player::TP::MkTime<std::chrono::seconds> },
	/// 1m = 1 minute.
	{ "m", Player::TP::MkTime<std::chrono::minutes> },
	/// 1min = 1 minute.
	{ "min", Player::TP::MkTime<std::chrono::minutes> },
	/// 2mins = 2 minutes.
	{ "mins", Player::TP::MkTime<std::chrono::minutes> },
	/// 1h = 1 hour.
	{ "h", Player::TP::MkTime<std::chrono::hours> },
	/// 1hour = 1 hour.
	{ "hour", Player::TP::MkTime<std::chrono::hours> },
	/// 2hours = 2 hours.
	{ "hours", Player::TP::MkTime<std::chrono::hours> },
	/// When no unit is provided, we assume microseconds.
	{ "", Player::TP::MkTime<std::chrono::microseconds> }
};

playd::playd(int argc, char *argv[])
    : audio(), player(audio, time_parser), handler(player), time_parser(UNITS)
{
	for (int i = 0; i < argc; i++) {
		this->arguments.push_back(std::string(argv[i]));
	}

	auto size = this->arguments.size();

	std::string addr = size > 2 ? this->arguments.at(2) : "0.0.0.0";
	std::string port = size > 3 ? this->arguments.at(3) : "1350";
	this->io = decltype(this->io)(
	                new IoReactor(this->player, this->handler, addr, port));
}

int playd::Run()
{
	try
	{
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
	catch (Error &error)
	{
		io->RespondWithError(error);
		Debug() << "Unhandled exception caught, going away now."
		        << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
