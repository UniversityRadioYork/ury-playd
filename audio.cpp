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

/**  INCLUDES  ****************************************************************/

extern "C" {
#ifdef WIN32
#define inline __inline
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#ifdef WIN32
#undef inline
#endif
}

#include <portaudio.h>

#include "contrib/pa_ringbuffer.h"

#include "audio.h"
#include "audio_av.h"
#include "audio_cb.h"		/* audio_cb_play */
#include "constants.h"

audio::audio(const std::string &path, int device)
{
	this->last_err = E_INCOMPLETE;
	this->av = std::unique_ptr<au_in>(new au_in(path));

	init_sink(device);
	init_ring_buf(this->av->samples2bytes(1L));

	this->frame_ptr = nullptr;
	this->frame_samples = 0;
}

audio::~audio()
{
	free_ring_buf();

	if (this->out_strm != nullptr) {
		Pa_CloseStream(this->out_strm);
		this->out_strm = nullptr;
		dbug("closed output stream");
	}
}

void
audio::start()
{
	PaError		pa_err;
	enum error	err = E_OK;

	spin_up();
	pa_err = Pa_StartStream(this->out_strm);
	if (pa_err) {
		throw error(E_INTERNAL_ERROR, "couldn't start stream");
	}

	dbug("audio started");
}

void
audio::stop()
{
	PaError		pa_err;
	enum error	err = E_OK;

	pa_err = Pa_AbortStream(this->out_strm);
	if (pa_err) {
		throw error(E_INTERNAL_ERROR, "couldn't stop stream");
	}

	dbug("audio stopped");

	/* TODO: Possibly recover from dropping frames due to abort. */
}

/* Returns the last decoding error, or E_OK if the last decode succeeded. */
enum error
audio::last_error()
{
	return this->last_err;
}

/* Checks to see if audio playback has halted of its own accord.
 *
 * If audio is still playing, E_OK will be returned; otherwise the decoding
 * error that caused playback to halt will be returned.  E_UNKNOWN is returned
 * if playback has halted but the last error report was E_OK.
 */
bool
audio::halted()
{
	bool halted = !Pa_IsStreamActive(this->out_strm);

	if (halted && this->last_err == E_OK) {
		/* Abnormal stream halts with error being OK are weird... */
		throw E_UNKNOWN;
	}
	return halted;
}

/* Gets the current played position in the song, in microseconds.
 *
 * As this may be executing whilst the playing callback is running,
 * do not expect it to be highly accurate.
 */
uint64_t
audio::usec()
{
	return this->av->samples2usec(this->used_samples);
}

size_t
audio::samples2bytes(size_t samples)
{
	return this->av->samples2bytes(samples);
}

/* Tries to place enough audio into the audio buffer to prevent a
 * buffer underrun during a player start.
 *
 * If end of file is reached, it is ignored and converted to E_OK so that it can
 * later be caught by the player callback once it runs out of sound.
 */
void
audio::spin_up()
{
	ring_buffer_size_t  c;
	enum error	err;
	PaUtilRingBuffer *r = this->ring_buf.get();

    /* Either fill the ringbuf or hit the maximum spin-up size,
     * whichever happens first.  (There's a maximum in order to
     * prevent spin-up from taking massive amounts of time and
     * thus delaying playback.)
     */
	for (err = E_OK, c = PaUtil_GetRingBufferWriteAvailable(r);
	     err == E_OK && (c > 0 && RINGBUF_SIZE - c < SPINUP_SIZE);
	     err = decode(), c = PaUtil_GetRingBufferWriteAvailable(r));

	/* Allow EOF, this'll be caught by the player callback once it hits the
	 * end of file itself
	 */
	if (err != E_EOF) {
		throw err;
	}
}

/* Attempts to seek to the given position in microseconds. */
void
audio::seek_usec(uint64_t usec)
{
	size_t samples = this->av->usec2samples(usec);

	while (!Pa_IsStreamStopped(this->out_strm)); // Spin until stream finishes
	PaUtil_FlushRingBuffer(this->ring_buf.get());

	this->av->seek(usec);

	this->frame_samples = 0;
	this->last_err = E_INCOMPLETE;
	this->used_samples = samples;	/* Update position marker */
}

/* Increments the used samples counter, which is used to determine the current
 * position in the song, by 'samples' samples.
 */
void
audio::inc_used_samples(uint64_t samples)
{
	this->used_samples += samples;
}

enum error
audio::decode()
{
	unsigned long	cap;
	unsigned long	count;
	enum error	err = E_OK;

	if (this->frame_samples == 0) {
		/* We need to decode some new frames! */
		this->av->decode(&(this->frame_ptr), &(this->frame_samples));
	}
	cap = (unsigned long)PaUtil_GetRingBufferWriteAvailable(this->ring_buf.get());
	count = (cap < this->frame_samples ? cap : this->frame_samples);
	if (count > 0 && err == E_OK) {
		/*
		 * We can copy some already decoded samples into the ring
		 * buffer
		 */
		unsigned long	num_written;

		num_written = PaUtil_WriteRingBuffer(this->ring_buf.get(),
						     this->frame_ptr,
						 (ring_buffer_size_t)count);
		if (num_written != count)
			err = error(E_INTERNAL_ERROR, "ringbuf write error");

		this->frame_samples -= num_written;
		this->frame_ptr += this->av->samples2bytes(num_written);
	}
	this->last_err = err;
	return err;
}

void
audio::init_sink(int device)
{
	PaError		pa_err;
	double		sample_rate;
	PaStreamParameters pars;
	enum error	err = E_OK;

	sample_rate = this->av->sample_rate();

	size_t samples_per_buf = this->av->pa_config(device, &pars);
	
	auto callback = [this](char *out, unsigned long frames_per_buf) {
		return this->cb_play(out, frames_per_buf);
	};

	pa_err = Pa_OpenStream(&this->out_strm,
			       NULL,
			       &pars,
			       sample_rate,
			       samples_per_buf,
			       paClipOff,
			       audio_cb_play,
				   static_cast<void *>(&callback));
	if (pa_err) {
		throw error(E_AUDIO_INIT_FAIL, "couldn't open stream: %i samplerate %d", pa_err, sample_rate);
	}
}

/* Initialises an audio structure's ring buffer so that decoded
 * samples can be placed into it.
 *
 * Any existing ring buffer will be freed.
 *
 * The number of bytes for each sample must be provided; see
 * audio_av_samples2bytes for one way of getting this.
 */
void
audio::init_ring_buf(size_t bytes_per_sample)
{
	enum error	err = E_OK;

	/* Get rid of any existing ring buffer stuff */
	free_ring_buf();

	this->ring_data = std::unique_ptr<char[]>(new char[RINGBUF_SIZE * bytes_per_sample]);
	this->ring_buf = std::unique_ptr<PaUtilRingBuffer>(new PaUtilRingBuffer);
	if (PaUtil_InitializeRingBuffer(this->ring_buf.get(),
		static_cast<ring_buffer_size_t>(bytes_per_sample),
		static_cast<ring_buffer_size_t>(RINGBUF_SIZE),
		this->ring_data.get()) != 0) {
		throw error(E_INTERNAL_ERROR, "ringbuf failed to init");
	}
}

/* Frees an audio structure's ring buffer. */
void
audio::free_ring_buf()
{
}
