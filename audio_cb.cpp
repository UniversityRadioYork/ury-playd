/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
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
