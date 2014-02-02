/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
 */

#include <thread>
#include <chrono>
#include <iostream>

#include "cmd.h"

#include "io.hpp"

#include "constants.h"		/* LOOP_NSECS */
#include "messages.h"		/* MSG_xyz */
#include "player.h"

static std::string DeviceId(int argc, char *argv[]);
static void MainLoop(Player &player);
static void ListOutputDevices();
static void RegisterListeners(Player &p);

/* Names of the states in enum state. */
const static std::unordered_map<State, std::string> STATES = {
	{ State::EJECTED, "Ejected" },
	{ State::STOPPED, "Stopped" },
	{ State::PLAYING, "Playing" },
	{ State::QUITTING, "Quitting" }
};


/* The main entry point. */
int main(int argc, char *argv[])
{
	int	exit_code = EXIT_SUCCESS;

	try {
		AudioOutput::InitialiseLibraries();

		// Don't roll this into the constructor: it'll go out of scope!
		std::string device_id = DeviceId(argc, argv);

		Player p(device_id);
		RegisterListeners(p);
		MainLoop(p);
	} catch (Error &error) {
		error.ToResponse();
		Debug("Unhandled exception caught, going away now.");
		exit_code = EXIT_FAILURE;
	}

	AudioOutput::CleanupLibraries();

	return exit_code;
}

static void RegisterListeners(Player &p)
{
	p.RegisterPositionListener([](uint64_t position) {
		Respond(Response::TIME, position);
	}, TIME_USECS);
	p.RegisterStateListener([](State old_state, State new_state) {
		Respond(Response::STAT, STATES.at(old_state), STATES.at(new_state));
	});
}

static void MainLoop(Player &p)
{
	/* Set of commands that can be performed on the player. */
	command_set PLAYER_CMDS = {
		/* Nullary commands */
		{ "play", [&](const cmd_words &) { return p.Play(); } },
		{ "stop", [&](const cmd_words &) { return p.Stop(); } },
		{ "ejct", [&](const cmd_words &) { return p.Eject(); } },
		{ "quit", [&](const cmd_words &) { return p.Quit(); } },
		/* Unary commands */
		{ "load", [&](const cmd_words &words) { return p.Load(words[1]); } },
		{ "seek", [&](const cmd_words &words) { return p.Seek(words[1]); } }
	};

	Respond(Response::OHAI, MSG_OHAI); // Say hello

	while (p.CurrentState() != State::QUITTING) {
		/*
		* Possible Improvement: separate command checking and player
		* updating into two threads.  Player updating is quite
		* intensive and thus impairs the command checking latency.
		* Do this if it doesn't make the code too complex.
		*/
		check_commands(PLAYER_CMDS);
		/* TODO: Check to see if err was fatal */
		p.Update();

		std::this_thread::sleep_for(std::chrono::nanoseconds(LOOP_NSECS));
	}

	Respond(Response::TTFN, MSG_TTFN);	// Wave goodbye
}

/* Tries to parse the device ID. */
static std::string DeviceId(int argc, char *argv[])
{
	std::string device = "";

	/*
	 * Possible Improvement: This is rather dodgy code for getting the
	 * device ID out of the command line arguments, maybe make it a bit
	 * more robust.
	 */
	if (argc < 2) {
		ListOutputDevices();
		throw Error(ErrorCode::BAD_CONFIG, MSG_DEV_NOID);
	} else {
		device = std::string(argv[1]);
	}

	return device;
}

static void ListOutputDevices()
{
	for (auto device : AudioOutput::ListDevices()) {
		std::cout << device.first << ": " << device.second << std::endl;
	}
}
