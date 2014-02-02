/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <memory> /* unique_ptr */

#include <stdint.h>		/* uint64_t */

#include "audio_av.h"

#include "contrib/pa_ringbuffer.h"	/* PaUtilRingBuffer */

#include "cuppa/errors.h"		/* enum error */

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
	bool decode();	/* Does some decoding work */

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

	void WriteToRingBuffer(unsigned long count);
	void IncrementFrameMarkers(unsigned long sample_count);
};

#endif				/* not AUDIO_H */
