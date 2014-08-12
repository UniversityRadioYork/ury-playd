// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the AudioSink class.
 * @see audio/audio_sink.hpp
 */

#include <cassert>
#include <climits>
#include <algorithm>
#include <string>

#include "portaudiocpp/PortAudioCpp.hxx"

#include "../errors.hpp"
#include "../sample_formats.hpp"
#include "../messages.h"

#include "audio_sink.hpp"

#include "ringbuffer.hpp"

const size_t AudioSink::RINGBUF_POWER = 16;

AudioSink::AudioSink(const StreamConfigurator c,
                     Resampler::SampleByteCount bytes_per_sample)
    : bytes_per_sample(bytes_per_sample),
      ring_buf(RINGBUF_POWER, bytes_per_sample),
      position_sample_count(0),
      just_started(false),
      input_ready(false)
{
	this->stream = decltype(this->stream)(c(*this));
}

void AudioSink::Start()
{
	this->just_started = true;
	this->stream->start();
}

void AudioSink::Stop()
{
	if (!this->stream->isStopped()) {
		this->stream->abort();
	}
}

bool AudioSink::IsStopped()
{
	return !this->stream->isActive();
}

bool AudioSink::InputReady()
{
	return this->input_ready;
}

void AudioSink::SetInputReady(bool ready)
{
	this->input_ready = ready;
}

AudioSink::SamplePosition AudioSink::Position()
{
	return this->position_sample_count;
}

void AudioSink::SetPosition(AudioSink::SamplePosition samples)
{
	this->position_sample_count = samples;
	this->ring_buf.Flush();
}

void AudioSink::Transfer(AudioSink::TransferIterator &start,
                         const AudioSink::TransferIterator &end)
{
	// No point transferring 0 bytes.
	if (start == end) {
		return;
	}
	assert(start < end);

	unsigned long bytes = std::distance(start, end);
	// There should be a whole number of samples being transferred.
	assert(bytes % bytes_per_sample == 0);
	assert(0 < bytes);

	auto samples = bytes / this->bytes_per_sample;

	// Only transfer as many samples as the ring buffer can take.
	// Don't bother trying to write 0 samples!
	auto count = std::min(samples, this->ring_buf.WriteCapacity());
	if (count == 0) {
		return;
	}
	assert(0 < count);

	unsigned long written_count = this->ring_buf.Write(&*start, count);
	// Since we never write more than the ring buffer can take, the written
	// count should equal the requested written count.
	assert(written_count == count);

	start += (written_count * this->bytes_per_sample);
	assert(start <= end);
}

int AudioSink::paCallbackFun(const void *, void *out,
                             unsigned long frames_per_buf,
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

PlayCallbackStepResult AudioSink::PlayCallbackStep(char *out,
                                                   unsigned long frames_per_buf,
                                                   PlayCallbackStepResult in)
{
	unsigned long avail = this->ring_buf.ReadCapacity();
	bool empty = avail == 0;

	/* If we've just started this stream, we don't want to hand PortAudio
	   an incomplete frame—we'd rather wait until we have enough in the
	   ring buffer before starting to play out. */
	bool wait = this->just_started && (avail < frames_per_buf);

	bool failed = wait || empty;
	auto fn = failed ? &AudioSink::PlayCallbackFailure
	                 : &AudioSink::PlayCallbackSuccess;
	return (this->*fn)(out, avail, frames_per_buf, in);
}

PlayCallbackStepResult AudioSink::PlayCallbackSuccess(
                char *out, unsigned long avail, unsigned long frames_per_buf,
                PlayCallbackStepResult in)
{
	this->just_started = false;

	auto samples_pa_wants = frames_per_buf - in.second;
	auto samples_read = ReadSamplesToOutput(out, avail, samples_pa_wants);

	return std::make_pair(paContinue, in.second + samples_read);
}

PlayCallbackStepResult AudioSink::PlayCallbackFailure(
                char *out, unsigned long, unsigned long frames_per_buf,
                PlayCallbackStepResult in)
{
	decltype(in) result;

	if (InputReady()) {
		// There's been some sort of genuine issue.
		// Make up some silence to plug the gap.
		memset(out, 0, this->bytes_per_sample * frames_per_buf);
		result = std::make_pair(paContinue, frames_per_buf);
	} else {
		// End of input is ok, it means the stream can finish.
		result = std::make_pair(paComplete, in.second);
	}

	return result;
}

unsigned long AudioSink::ReadSamplesToOutput(char *&output,
                                             unsigned long output_capacity,
                                             unsigned long buffered_count)
{
	// Transfer the maximum that we can offer to PortAudio without
	// overshooting
	// its sample request limit.
	long transfer_sample_count = static_cast<long>(
	                std::min({ output_capacity, buffered_count,
		                   static_cast<unsigned long>(LONG_MAX) }));
	output += this->ring_buf.Read(output, transfer_sample_count);

	// Update the position count so it reflects the last position that was
	// sent
	// for playback (*not* the last position decoded).
	this->position_sample_count += transfer_sample_count;
	return transfer_sample_count;
}
