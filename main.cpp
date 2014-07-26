// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Main entry point and implementation of the Playslave class.
 * @see main.cpp
 */

#include <chrono>
#include <iostream>

#include "cmd.hpp"
#include "io/io_response.hpp"
#include "io/io_reactor.hpp"
#include "messages.h"
#include "player/player.hpp"
#include "audio/audio_system.hpp"
#include "main.hpp"

/**
 * The main entry point.
 * @param argc Program argument count.
 * @param argv Program argument vector.
 */
int main(int argc, char *argv[])
{
	Playslave ps(argc, argv);
	return ps.Run();
}

//
// Playslave
//

const std::chrono::microseconds Playslave::POSITION_PERIOD(500000);

void Playslave::ListOutputDevices()
{
	this->audio.OnDevices([](const AudioSystem::Device &device) {
		std::cout << device.first << ": " << device.second << std::endl;
	});
}

std::string Playslave::DeviceID()
{
	std::string device = "";

	// TODO: Perhaps make this section more robust.
	if (this->arguments.size() < 2) {
		ListOutputDevices();
		throw ConfigError(MSG_DEV_NOID);
	} else {
		device = std::string(this->arguments[1]);
	}

	return device;
}

const Player::TP::UnitMap UNITS = {
                {"us", Player::TP::MkTime<std::chrono::microseconds>},
                {"usec", Player::TP::MkTime<std::chrono::microseconds>},
                {"usecs", Player::TP::MkTime<std::chrono::microseconds>},
                {"ms", Player::TP::MkTime<std::chrono::milliseconds>},
                {"msec", Player::TP::MkTime<std::chrono::milliseconds>},
                {"msecs", Player::TP::MkTime<std::chrono::milliseconds>},
                {"s", Player::TP::MkTime<std::chrono::seconds>},
                {"sec", Player::TP::MkTime<std::chrono::seconds>},
                {"secs", Player::TP::MkTime<std::chrono::seconds>},
                {"m", Player::TP::MkTime<std::chrono::minutes>},
                {"min", Player::TP::MkTime<std::chrono::minutes>},
                {"mins", Player::TP::MkTime<std::chrono::minutes>},
                {"h", Player::TP::MkTime<std::chrono::hours>},
                {"hour", Player::TP::MkTime<std::chrono::hours>},
                {"hours", Player::TP::MkTime<std::chrono::hours>},
                // Default when there is no unit
                {"", Player::TP::MkTime<std::chrono::microseconds>}};

Playslave::Playslave(int argc, char *argv[])
    : audio(), player(audio, time_parser), handler(), time_parser(UNITS)
{
	for (int i = 0; i < argc; i++) {
		this->arguments.push_back(std::string(argv[i]));
	}

	RegisterCommands(&this->player);

	auto size = this->arguments.size();

	using std::bind;
	using std::placeholders::_1;

	this->io = decltype(this->io)(new IoReactor(
	                this->player, this->handler,
	                2 < size ? this->arguments.at(2) : "0.0.0.0",
	                3 < size ? this->arguments.at(3) : "1350",
			bind(&Player::WelcomeClient, &this->player, _1)));
}

void Playslave::RegisterCommands(Player *p)
{
	using std::bind;
	using std::placeholders::_1;

	this->handler.AddNullary("play", bind(&Player::Play, p))
	                .AddNullary("stop", bind(&Player::Stop, p))
	                .AddNullary("ejct", bind(&Player::Eject, p))
	                .AddNullary("quit", bind(&Player::Quit, p))
	                .AddUnary("load", bind(&Player::Load, p, _1))
	                .AddUnary("seek", bind(&Player::Seek, p, _1));
}

int Playslave::Run()
{
	int exit_code = EXIT_SUCCESS;

	try
	{
		// Don't roll this into the constructor: it'll go out of scope!
		this->audio.SetDeviceID(DeviceID());
		this->player.SetPositionResponseCodePeriod(POSITION_PERIOD);
		this->player.SetResponder(*this->io);
		this->io->Run();
	}
	catch (Error &error)
	{
		io->RespondWithError(error);
		Debug("Unhandled exception caught, going away now.");
		exit_code = EXIT_FAILURE;
	}

	return exit_code;
}
