/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
 */

#define _POSIX_C_SOURCE 200809

#include <stdio.h>
#include <string.h>
#include <time.h>

extern "C" {
#ifdef WIN32
#define inline __inline
#endif
#include <libavformat/avformat.h>
#ifdef WIN32
#undef inline
#endif
}

#include <thread>
#include <chrono>

#include <portaudio.h>

#include "cmd.h"

#include "cuppa/io.h"

#include "constants.h"		/* LOOP_NSECS */
#include "messages.h"		/* MSG_xyz */
#include "player.h"

static PaDeviceIndex device_id(int argc, char *argv[]);
static void MainLoop(Player &player);

/* Names of the states in enum state. */
const static std::unordered_map<State, std::string> STATES = {
	{ State::EJECTED, "Ejected" },
	{ State::STOPPED, "Stopped" },
	{ State::PLAYING, "Playing" },
	{ State::QUITTING, "Quitting" }
};


/* The main entry point. */
int
main(int argc, char *argv[])
{
	int		exit_code = EXIT_SUCCESS;

	try {
		if (Pa_Initialize() != (int)paNoError) {
			throw error(E_AUDIO_INIT_FAIL, "couldn't init portaudio");
		}

		PaDeviceIndex device = device_id(argc, argv);

		av_register_all();

		Player p(device);
		p.RegisterPositionListener([](uint64_t position) { response(R_TIME, "%u", position); }, TIME_USECS);
		p.RegisterStateListener([](State old_state, State new_state) {
			response(R_STAT, "%s %s", STATES.at(old_state).c_str(), STATES.at(new_state).c_str());
		});
		MainLoop(p);
	}
	catch (enum error) {
		exit_code = EXIT_FAILURE;
	}

	Pa_Terminate();

	return exit_code;
}

static void
MainLoop(Player &p)
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

	response(R_OHAI, "%s", MSG_OHAI);	/* Say hello */

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

	response(R_TTFN, "%s", MSG_TTFN);	/* Wave goodbye */
}

/* Tries to parse the device ID. */
static PaDeviceIndex
device_id(int argc, char *argv[])
{
	int	num_devices = Pa_GetDeviceCount();
	PaDeviceIndex device = 0;

	/*
	 * Possible Improvement: This is rather dodgy code for getting the
	 * device ID out of the command line arguments, maybe make it a bit
	 * more robust.
	 */
	if (argc < 2) {
		int		i;
		const PaDeviceInfo *dev;

		/* Print out the available devices */
		for (i = 0; i < num_devices; i++) {
			dev = Pa_GetDeviceInfo(i);
			dbug("%u: %s", i, dev->name);
		}

		throw error(E_BAD_CONFIG, MSG_DEV_NOID);
	} else {
		device = (int)strtoul(argv[1], NULL, 10);
		if (device >= num_devices) {
			throw error(E_BAD_CONFIG, MSG_DEV_BADID);
		}
	}

	return device;
}
