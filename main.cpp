/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#include <thread>
#include <chrono>
#include <iostream>

#include "cmd.hpp"
#include "constants.h"
#include "io.hpp"
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
	Playslave ps{argc, argv};
	return ps.Run();
}

/**
 * Lists on stdout all sound devices to which the audio output may connect.
 * This is mainly for the benefit of the end user.
 */
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
		throw Error(ErrorCode::BAD_CONFIG, MSG_DEV_NOID);
	} else {
		device = std::string(this->arguments[1]);
	}

	return device;
}

void Playslave::RegisterListeners()
{
	this->player->SetPositionListenerPeriod(POSITION_PERIOD);
	this->player->RegisterPositionListener([](
	                std::chrono::microseconds position) {
		std::uint64_t p = position.count();
		Respond(Response::TIME, p);
	});
	this->player->RegisterStateListener([](Player::State old_state,
	                                       Player::State new_state) {
		Respond(Response::STAT, Player::StateString(old_state),
		        Player::StateString(new_state));
	});
}

/**
 * Performs the playslave main loop.
 * This involves listening for commands and asking the player to do some work.
 * @todo Make the command check asynchronous/event based.
 * @todo Possibly separate command check and player updating into separate
 * threads?
 */
void Playslave::MainLoop()
{
	while (this->player->IsRunning()) {
		/* Possible Improvement: separate command checking and player
		 * updating into two threads.  Player updating is quite
		 * intensive and thus impairs the command checking latency.
		 * Do this if it doesn't make the code too complex.
		 */
		this->handler->Check();
		this->player->Update();

		std::this_thread::sleep_for(LOOP_PERIOD);
	}
}

Playslave::Playslave(int argc, char *argv[]) : audio{}
{
	for (int i = 0; i < argc; i++) {
		this->arguments.push_back(std::string(argv[i]));
	}

	this->time_parser = decltype(
	                this->time_parser) {new Player::TP{Player::TP::UnitMap{
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
	                {"", Player::TP::MkTime<std::chrono::microseconds>}}}};

	this->player = decltype(this->player) {
	                new Player{this->audio, *this->time_parser}};

	CommandHandler *h = new CommandHandler;

	using std::string;

	h->Add("play", [&]() { return this->player->Play(); });
	h->Add("stop", [&]() { return this->player->Stop(); });
	h->Add("ejct", [&]() { return this->player->Eject(); });
	h->Add("quit", [&]() { return this->player->Quit(); });

	h->Add("load", [&](const string &s) { return this->player->Load(s); });
	h->Add("seek", [&](const string &s) { return this->player->Seek(s); });

	this->handler = decltype(this->handler) {h};
}

int Playslave::Run()
{
	int exit_code = EXIT_SUCCESS;

	try
	{
		// Don't roll this into the constructor: it'll go out of scope!
		this->audio.SetDeviceID(DeviceID());

		RegisterListeners();

		Respond(Response::OHAI, MSG_OHAI);
		MainLoop();
		Respond(Response::TTFN, MSG_TTFN);
	}
	catch (Error &error)
	{
		error.ToResponse();
		Debug("Unhandled exception caught, going away now.");
		exit_code = EXIT_FAILURE;
	}

	return exit_code;
}
