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
#include "io/io_responder.hpp"
#include "io/io_reactor.hpp"
#include "io/io_reactor_tcp.hpp"
#include "messages.h"
#include "player/player.hpp"
#include "audio/audio_system.hpp"
#include "main.hpp"

#include <boost/asio.hpp>
#if defined(_WIN32)

#define HAVE_STD_IO_REACTOR
#include "io/io_reactor_win.hpp"
using StdIoReactor = WinIoReactor;

#elif defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

#include "io/io_reactor_posix.hpp"
#define HAVE_STD_IO_REACTOR
using StdIoReactor = PosixIoReactor;

#endif

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

void Playslave::RegisterListeners()
{
	this->player.SetPositionListenerPeriod(POSITION_PERIOD);
	this->player.RegisterPositionListener([this](
	                std::chrono::microseconds position) {
		std::uint64_t p = position.count();
		io->Respond(Response::TIME, p);
	});
	this->player.RegisterStateListener([this](Player::State old_state,
	                                          Player::State new_state) {
		io->Respond(Response::STAT, Player::StateString(old_state),
		            Player::StateString(new_state));
		if (new_state == Player::State::QUITTING) {
			io->End();
		}
	});
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

	IoReactor *io = nullptr;
	if (this->arguments.size() == 4) {
		io = new TcpIoReactor(this->player, this->handler,
		                      this->arguments.at(2),
		                      this->arguments.at(3));
	} else {
#ifdef HAVE_STD_IO_REACTOR
		io = new StdIoReactor(this->player, this->handler);
#else
		throw Error("Cannot use standard IO, not supported on this "
		            "platform.");
#endif // HAVE_STD_IO_REACTOR
	}
	this->io = decltype(this->io)(io);
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
		RegisterListeners();
		io->Run();
	}
	catch (Error &error)
	{
		io->RespondWithError(error);
		Debug("Unhandled exception caught, going away now.");
		exit_code = EXIT_FAILURE;
	}

	return exit_code;
}
