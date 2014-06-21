/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#include <algorithm>
#include <cstring>

#include "portaudiocpp/PortAudioCpp.hxx"

#include "../errors.hpp"

#include "audio_output.hpp"

/* The callback proper, which is executed in a separate thread by PortAudio once
 * a stream is playing with the callback registered to it.
 */
int AudioOutput::paCallbackFun(const void *, void *out, unsigned long frames_per_buf,
                  const PaStreamCallbackTimeInfo *,
                  PaStreamCallbackFlags)
{
        char *cout = static_cast<char *>(out);

	std::pair<PaStreamCallbackResult, unsigned long> result =
	                std::make_pair(paContinue, 0);

	while (result.first == paContinue && result.second < frames_per_buf) {
		result = PlayCallbackStep(cout, frames_per_buf, result);
	}
	return static_cast<int>(result.first);
}

PlayCallbackStepResult AudioOutput::PlayCallbackStep(
                char *out, unsigned long frames_per_buf,
                PlayCallbackStepResult in)
{
	decltype(in) result;

	unsigned long avail = this->ring_buf->ReadCapacity();
	if (avail == 0) {
		result = PlayCallbackFailure(out, frames_per_buf, in);
	} else {
		result = std::make_pair(
		                paContinue,
		                in.second + ReadSamplesToOutput(
		                                            out, avail,
		                                            frames_per_buf -
		                                                            in.second));
	}

	return result;
}

PlayCallbackStepResult AudioOutput::PlayCallbackFailure(
                char *out, unsigned long frames_per_buf,
                PlayCallbackStepResult in)
{
	decltype(in) result;

	switch (LastError()) {
		case ErrorCode::END_OF_FILE:
			// We've just hit the end of the file.  This is ok.
			result = std::make_pair(paComplete, in.second);
			break;
		case ErrorCode::OK: // Fallthrough intentional
		case ErrorCode::INCOMPLETE:
			/*
			 * Looks like we're just waiting for the
			 * decoding to go through. In other
			 * words,
			 * this is a buffer underflow.
			 */
			Debug("buffer underflow");
			// Break out of the loop inelegantly
			memset(out, 0, ByteCountForSampleCount(frames_per_buf));
			result.second = frames_per_buf;
			break;
		default:
			// Something genuinely went tits-up.  Abort!
			result = std::make_pair(paAbort, in.second);
			break;
	}

	return result;
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
	output += this->ring_buf->Read(
	                output, static_cast<long>(transfer_sample_count));

	this->position_sample_count += transfer_sample_count;
	return transfer_sample_count;
}
