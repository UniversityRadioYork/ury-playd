/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <map>
#include <memory>
#include <string>
#include <cstdint>

#include "audio_av.h"

#include "contrib/pa_ringbuffer.h" /* PaUtilRingBuffer */

#include "errors.hpp"

/* The audio structure contains all state pertaining to the currently
 * playing audio file.
 *
 * struct audio is an opaque structure; only audio.c knows its true
 * definition.
 */
class AudioOutput {
public:
	typedef std::map<std::string, const std::string> DeviceList;

	static void InitialiseLibraries();
	static void CleanupLibraries();

	static DeviceList ListDevices();

	/* Loads a file and constructs an audio structure to hold the playback
	* state.
	*/
	AudioOutput(const std::string &path, const std::string &device_id);
	~AudioOutput();

	void Start();
	void Stop();
	bool Update();

	ErrorCode LastError();
	bool IsHalted();

	/**
	 * Return the current position, as a std::chrono::duration.
	 * @return The current position in the audio.
	 */
	template <typename R>
	R CurrentPosition()
	{
		return std::chrono::duration_cast<R>(
		                CurrentPositionMicroseconds());
	}
	std::chrono::microseconds CurrentPositionMicroseconds();

	/**
	 * Seek to a position expressed as a std::chrono::duration.
	 * @param position The position to seek to in the audio.
	 */
	template <typename R>
	void SeekToPosition(R position)
	{
		SeekToPositionMicroseconds(std::chrono::duration_cast<
		                std::chrono::microseconds>(position));
	}
	void SeekToPositionMicroseconds(std::chrono::microseconds microseconds);

	void PreFillRingBuffer();

private:
	ErrorCode last_error;
	std::unique_ptr<au_in> av;
	/* shared state */
	char *frame_ptr;
	size_t frame_samples;
	/* PortAudio state */
	std::unique_ptr<PaUtilRingBuffer> ring_buf;
	std::unique_ptr<char[]> ring_data;
	PaStream *out_strm;
	int device_id;
	uint64_t position_sample_count;
	std::function<int(char *, unsigned long)> callback;

	void InitialisePortAudio(int device);
	void InitialiseRingBuffer(size_t bytes_per_sample);

	size_t ByteCountForSampleCount(size_t sample_count);

	int PlayCallback(char *out, unsigned long frames_per_buf);
	unsigned long ReadSamplesToOutput(char *&output,
	                                  unsigned long output_capacity,
	                                  unsigned long buffered_count);
	void AdvancePositionBySampleCount(uint64_t sample_count);

	bool DecodeIfFrameEmpty();

	void WriteAllAvailableToRingBuffer();
	void WriteToRingBuffer(unsigned long count);

	unsigned long RingBufferWriteCapacity();
	unsigned long RingBufferTransferCount();
	void IncrementFrameMarkers(unsigned long sample_count);

	PaDeviceIndex DeviceIdToPa(const std::string &id_string);
};

#endif /* not AUDIO_H */
