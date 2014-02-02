/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <map>
#include <memory>
#include <string>
#include <cstdint>

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
	typedef std::map<std::string, const std::string> DeviceList;

	static DeviceList ListDevices();

	/* Loads a file and constructs an audio structure to hold the playback
	* state.
	*/
	audio(const std::string &path, const std::string &device_id);
	~audio();

	void start();
	void stop();
	bool decode();

	enum error last_error();
	bool halted();
	uint64_t usec();

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
	unsigned long audio::ReadSamplesToOutput(char *&output, unsigned long output_capacity, unsigned long buffered_count);

	bool DecodeIfFrameEmpty();

	void WriteAllAvailableToRingBuffer();
	void WriteToRingBuffer(unsigned long count);

	unsigned long RingBufferWriteCapacity();
	unsigned long RingBufferTransferCount();
	void IncrementFrameMarkers(unsigned long sample_count);

	PaDeviceIndex audio::DeviceIdToPa(const std::string &id_string);
};

#endif				/* not AUDIO_H */
