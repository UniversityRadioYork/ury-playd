/*-
 * Copyright (C) 2012  University Radio York Computing Team
 *
 * This file is a part of playslave.
 *
 * playslave is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * playslave is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * playslave; if not, write to the Free Software Foundation, Inc., 51 Franklin
 * Street, Fifth Floor, Boston, MA 02110-1301, USA.
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
#include <portaudio.h>

#include "cuppa/io.h"

#include "constants.h"		/* LOOP_NSECS */
#include "messages.h"		/* MSG_xyz */
#include "player.h"

static PaDeviceIndex device_id(int argc, char *argv[]);

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

		player p(device);
		p.main_loop();

		Pa_Terminate();

	}
	catch (enum error) {
		exit_code = EXIT_FAILURE;
	}

	return exit_code;
}

/**  STATIC FUNCTIONS  ********************************************************/

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
