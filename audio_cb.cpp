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


/**  INCLUDES  ****************************************************************/

#include <string.h>

#include <portaudio.h>

#include "cuppa/errors.h"	/* dbug */
#include "contrib/pa_ringbuffer.h"	/* Ringbuffer */

#include "audio.h"		/* Manipulating the audio structure */

/**  PUBLIC FUNCTIONS  ********************************************************/

/* The callback proper, which is executed in a separate thread by PortAudio once
 * a stream is playing with the callback registered to it.
 */
int
audio_cb_play(const void *in,
void *out,
unsigned long frames_per_buf,
const PaStreamCallbackTimeInfo *timeInfo,
PaStreamCallbackFlags statusFlags,
void *v_au)
{
	char *cout = static_cast<char *>(out);
	auto f = static_cast<std::function<int(char *, unsigned long)> *>(v_au);

	return (*f)(cout, frames_per_buf);
}

int
audio::cb_play(char *out, unsigned long frames_per_buf)
{
	unsigned long avail;
	PaStreamCallbackResult result = paContinue;

	unsigned long frames_written = 0;

	while (result == paContinue && frames_written < frames_per_buf) {
		avail = PaUtil_GetRingBufferReadAvailable(this->ring_buf.get());
		if (avail == 0) {
			/*
			 * We've run out of sound, ruh-roh. Let's see if
			 * something went awry during the last decode
			 * cycle...
			 */
			switch (last_error()) {
			case E_EOF:
				/*
				 * We've just hit the end of the file.
				 * Nothing to worry about!
				 */
				result = paComplete;
				break;
			case E_OK:
			case E_INCOMPLETE:
				/*
				 * Looks like we're just waiting for the
				 * decoding to go through. In other words,
				 * this is a buffer underflow.
				 */
				dbug("buffer underflow");
				/* Break out of the loop inelegantly */
				memset(out,
				       0,
				       samples2bytes(frames_per_buf)
					);
				frames_written = frames_per_buf;
				break;
			default:
				/* Something genuinely went tits-up. */
				result = paAbort;
				break;
			}
		} else {
			unsigned long samples;

			/* How many samples do we have? */
			if (avail > frames_per_buf - frames_written)
				samples = frames_per_buf - frames_written;
			else
				samples = avail;

			/*
			 * TODO: handle the ulong->long cast more gracefully,
			 * perhaps.
			 */
			out += PaUtil_ReadRingBuffer(this->ring_buf.get(),
						      out,
					       (ring_buffer_size_t)samples);
			frames_written += samples;
			inc_used_samples(samples);
		}
	}
	return (int)result;
}
