/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#include <algorithm>

#include <string.h>

#include <portaudio.h>

#include "errors.hpp"
#include "contrib/pa_ringbuffer.h" /* Ringbuffer */

#include "audio.h" /* Manipulating the audio structure */

/* The callback proper, which is executed in a separate thread by PortAudio once
 * a stream is playing with the callback registered to it.
 */
int audio_cb_play(const void * /*in*/, void *out, unsigned long frames_per_buf,
                  const PaStreamCallbackTimeInfo * /*timeInfo*/,
                  PaStreamCallbackFlags /*statusFlags*/, void *v_au)
{
	char *cout = static_cast<char *>(out);
	auto f = static_cast<std::function<int(char *, unsigned long)> *>(v_au);

	return (*f)(cout, frames_per_buf);
}

int AudioOutput::PlayCallback(char *out, unsigned long frames_per_buf)
{
	PaStreamCallbackResult result = paContinue;
	unsigned long frames_written = 0;

	while (result == paContinue && frames_written < frames_per_buf) {
		unsigned long avail = PaUtil_GetRingBufferReadAvailable(
		                this->ring_buf.get());
		if (avail == 0) {
			/*
			 * We've run out of sound, ruh-roh. Let's see if
			 * something went awry during the last decode
			 * cycle...
			 */
			switch (LastError()) {
				case ErrorCode::END_OF_FILE:
					/*
					 * We've just hit the end of the file.
					 * Nothing to worry about!
					 */
					result = paComplete;
					break;
				case ErrorCode::OK:
				case ErrorCode::INCOMPLETE:
					/*
					 * Looks like we're just waiting for the
					 * decoding to go through. In other
					 * words,
					 * this is a buffer underflow.
					 */
					Debug("buffer underflow");
					/* Break out of the loop inelegantly */
					memset(out, 0,
					       ByteCountForSampleCount(
					                       frames_per_buf));
					frames_written = frames_per_buf;
					break;
				default:
					/* Something genuinely went tits-up. */
					result = paAbort;
					break;
			}
		} else {
			frames_written += ReadSamplesToOutput(
			                out, avail,
			                frames_per_buf - frames_written);
		}
	}
	return (int)result;
}

/**
 * Reads samples from the ring buffer to an output, and updates the used samples
 * count.
 * @param output A reference to the output buffer's current pointer.
 * @param output_capacity The capacity of the output buffer, in samples.
 * @param buffered_count The number of samples available in the ring buffer.
 * @return The number of samples successfully written to the output buffer.
 */
unsigned long AudioOutput::ReadSamplesToOutput(char *&output,
                                               unsigned long output_capacity,
                                               unsigned long buffered_count)
{
	unsigned long transfer_sample_count =
	                std::min(output_capacity, buffered_count);

	// TODO: handle the ulong->long cast more gracefully, perhaps.
	output += PaUtil_ReadRingBuffer(
	                this->ring_buf.get(), output,
	                static_cast<ring_buffer_size_t>(transfer_sample_count));

	AdvancePositionBySampleCount(transfer_sample_count);
	return transfer_sample_count;
}
