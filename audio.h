/*
 * =============================================================================
 *
 *       Filename:  audio.h
 *
 *    Description:  Interface to the mid-level audio structure and functions
 *
 *        Version:  1.0
 *        Created:  25/12/2012 05:39:46
 *       Revision:  none
 *       Compiler:  clang
 *
 *         Author:  Matt Windsor (CaptainHayashi), matt.windsor@ury.org.uk
 *        Company:  University Radio York Computing Team
 *
 * =============================================================================
 */
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

#ifndef AUDIO_H
#define AUDIO_H

/**  INCLUDES  ****************************************************************/

#include <memory> /* unique_ptr */

#include <stdint.h>		/* uint64_t */

#include "audio_av.h"

#include "contrib/pa_ringbuffer.h"	/* PaUtilRingBuffer */

#include "cuppa/errors.h"		/* enum error */

/**  DATA TYPES  **************************************************************/

/* The audio structure contains all state pertaining to the currently
 * playing audio file.
 *
 * struct audio is an opaque structure; only audio.c knows its true
 * definition.
 */
class audio {
public:
	/* Loads a file and constructs an audio structure to hold the playback
	* state.
	*/
	audio(
	  const std::string &path,	/* File to load into the audio struct */
	  int device);		/* ID of the device to play out on */
	  ~audio();	/* Frees an audio struct */

	void start();	/* Starts playback */
	void stop();	/* Stops playback */
	enum error decode();	/* Does some decoding work */

	enum error last_error();	/* Gets last playback error */
	bool halted();	/* Has stream halted itself? */
	uint64_t usec();	/* Current time in song */

	void seek_usec(uint64_t usec);
	void inc_used_samples(uint64_t samples);

	void spin_up();

	size_t samples2bytes(size_t samples);

private:
	enum error last_err;	/* Last result of decoding */
	std::unique_ptr<au_in> av;	/* ffmpeg state */
	/* shared state */
	char *frame_ptr;
	size_t frame_samples;
	/* PortAudio state */
	std::unique_ptr<PaUtilRingBuffer> ring_buf;
	std::unique_ptr<char[]> ring_data;
	PaStream *out_strm;	/* Output stream */
	int device_id;	/* PortAudio device ID */
	uint64_t used_samples;	/* Counter of samples played */
	std::function<int(char *, unsigned long)> callback;

	void init_sink(int device);
	void init_ring_buf(size_t bytes_per_sample);
	void free_ring_buf();

	int cb_play(char *out, unsigned long frames_per_buf);
};

#endif				/* not AUDIO_H */
