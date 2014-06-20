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

/**
 * Lists on stdout all sound devices to which the audio output may connect.
 * This is mainly for the benefit of the end user.
 */
static void ListOutputDevices()
{
	for (auto device : AudioOutput::ListDevices()) {
		std::cout << device.first << ": " << device.second << std::endl;
	}
}

/**
 * Tries to get the output device ID from stdin.
 * If there is no stdin, the program prints up the available devices and dies.
 * @param argc The program argument count (from main()).
 * @param argv The program argument vector (from main()).
 * @return The device ID, as a string.
 */
static std::string DeviceId(int argc, char *argv[])
{
	std::string device = "";

	/* TODO: Perhaps make this section more robust. */
	if (argc < 2) {
		ListOutputDevices();
		throw Error(ErrorCode::BAD_CONFIG, MSG_DEV_NOID);
	} else {
		device = std::string(argv[1]);
	}

	return device;
}

/**
 * Registers various listeners with the Player.
 * This is so time and state changes can be sent out on stdout.
 * @param player The player to which the listeners will subscribe.
 */
static void RegisterListeners(Player &player)
{
	player.SetPositionListenerPeriod(POSITION_PERIOD);
	player.RegisterPositionListener([](std::chrono::microseconds position) {
		uint64_t p = position.count();
		Respond(Response::TIME, p);
	});
	player.RegisterStateListener([](Player::State old_state,
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
static void MainLoop(Player &player)
{
	CommandHandler handler(
	                {{"play",
	                  CommandHandler::NullCommand(player.PlayAction())},
	                 {"stop",
	                  CommandHandler::NullCommand(player.StopAction())},
	                 {"ejct",
	                  CommandHandler::NullCommand(player.EjectAction())},
	                 {"quit",
	                  CommandHandler::NullCommand(player.QuitAction())},
	                 {"load", CommandHandler::SingleRequiredWordCommand(
	                                          player.LoadAction())},
	                 {"seek", CommandHandler::SingleRequiredWordCommand(
	                                          player.SeekAction())}});

	Respond(Response::OHAI, MSG_OHAI);

	while (player.IsRunning()) {
		/* Possible Improvement: separate command checking and player
		 * updating into two threads.  Player updating is quite
		 * intensive and thus impairs the command checking latency.
		 * Do this if it doesn't make the code too complex.
		 */
		handler.Check();
		player.Update();

		std::this_thread::sleep_for(LOOP_PERIOD);
	}

	Respond(Response::TTFN, MSG_TTFN);
}

/**
 * The main entry point.
 * @param argc Program argument count.
 * @param argv Program argument vector.
 */
int main(int argc, char *argv[])
{
	int exit_code = EXIT_SUCCESS;

	try
	{
		AudioOutput::InitialiseLibraries();

		/* Don't roll this into the constructor: it'll go out of scope!
		 */
		std::string device_id = DeviceId(argc, argv);

		Player player(device_id);
		RegisterListeners(player);
		MainLoop(player);
	}
	catch (Error &error)
	{
		error.ToResponse();
		Debug("Unhandled exception caught, going away now.");
		exit_code = EXIT_FAILURE;
	}

	AudioOutput::CleanupLibraries();

	return exit_code;
}
