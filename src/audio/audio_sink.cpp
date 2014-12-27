// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the AudioSink class.
 * @see audio/audio_sink.hpp
 */

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstring>
#include <memory>
#include <string>

#include "portaudiocpp/PortAudioCpp.hxx"

#include "../errors.hpp"
#include "../messages.h"
#include "audio_sink.hpp"
#include "audio_source.hpp"
#include "ringbuffer.hpp"
#include "sample_formats.hpp"

const size_t AudioSink::RINGBUF_POWER = 16;

AudioSink::AudioSink(const AudioSource &source, int device_id)
    : bytes_per_sample(source.BytesPerSample()),
      ring_buf(RINGBUF_POWER, source.BytesPerSample()),
      position_sample_count(0),
      just_started(false),
      source_out(false),
      sink_out(false)
{
	std::uint8_t channel_count = source.ChannelCount();
	SampleFormat sample_format = source.OutputSampleFormat();
	double sample_rate = source.SampleRate();
	const portaudio::Device &device = AudioSink::PaDevice(device_id);

	portaudio::DirectionSpecificStreamParameters out_pars(
	                device, channel_count,
	                AudioSink::PaFormat(sample_format), true,
	                device.defaultLowOutputLatency(), nullptr);

	portaudio::StreamParameters pars(
	                portaudio::DirectionSpecificStreamParameters::null(),
	                out_pars, sample_rate, paFramesPerBufferUnspecified,
	                paClipOff);

	this->stream = std::make_unique<portaudio::InterfaceCallbackStream>(pars, *this);
}

void AudioSink::Start()
{
	this->just_started = true;
	this->stream->start();
}

void AudioSink::Stop()
{
	if (!this->stream->isStopped()) this->stream->abort();
}

Audio::State AudioSink::State()
{
	if (this->sink_out) return Audio::State::AT_END;
	if (this->stream->isActive()) return Audio::State::PLAYING;
	return Audio::State::STOPPED;
}

void AudioSink::SourceOut()
{
	// The sink should only be out if the source is.
	assert(this->source_out || !this->sink_out);

	this->source_out = true;
}

std::uint64_t AudioSink::Position()
{
	return this->position_sample_count;
}

void AudioSink::SetPosition(std::uint64_t samples)
{
	this->position_sample_count = samples;

	// We might have been at the end of the file previously.
	// If so, we might not be now, so clear the out flags.
	this->source_out = this->sink_out = false;

	// The ringbuf will have been full of samples from the old
	// position, so we need to get rid of them.
	this->ring_buf.Flush();
}

void AudioSink::Transfer(AudioSink::TransferIterator &start,
                         const AudioSink::TransferIterator &end)
{
	assert(start <= end);

	// No point transferring 0 bytes.
	if (start == end) return;

	unsigned long bytes = std::distance(start, end);
	// There should be a whole number of samples being transferred.
	assert(bytes % bytes_per_sample == 0);
	assert(0 < bytes);

	auto samples = bytes / this->bytes_per_sample;

	// Only transfer as many samples as the ring buffer can take.
	// Don't bother trying to write 0 samples!
	auto count = std::min(samples, this->ring_buf.WriteCapacity());
	if (count == 0) return;

	auto start_ptr = reinterpret_cast<char *>(&*start);
	unsigned long written_count = this->ring_buf.Write(start_ptr, count);
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
                char *out, unsigned long avail, unsigned long frames_per_buf,
                PlayCallbackStepResult in)
{
	decltype(in) result;

	// We've hit end of file if:
	// 1) The decoder has said it's run out;
	// 2) The number of available samples is exactly zero.
	// End of input is ok: it means the stream can finish.
	if (this->source_out && avail == 0) {
		// Make sure future state queries return AT_END.
		this->sink_out = true;

		return std::make_pair(paComplete, in.second);
	}

	// There's been some sort of genuine issue.
	// Make up some silence to plug the gap.
	Debug() << "Buffer underflow" << std::endl;
	memset(out, 0, this->bytes_per_sample * frames_per_buf);
	return std::make_pair(paContinue, frames_per_buf);
}

unsigned long AudioSink::ReadSamplesToOutput(char *&output,
                                             unsigned long output_capacity,
                                             unsigned long buffered_count)
{
	// Transfer the maximum that we can offer to PortAudio without
	// overshooting its sample request limit.
	long transfer_sample_count = static_cast<long>(
	                std::min({ output_capacity, buffered_count,
		                   static_cast<unsigned long>(LONG_MAX) }));
	output += this->ring_buf.Read(output, transfer_sample_count);

	// Update the position count so it reflects the last position that was
	// sent for playback (*not* the last position decoded).
	this->position_sample_count += transfer_sample_count;
	return transfer_sample_count;
}

/* static */ const portaudio::Device &AudioSink::PaDevice(int id)
{
	auto &pa = portaudio::System::instance();
	if (pa.deviceCount() <= id) throw ConfigError(MSG_DEV_BADID);
	return pa.deviceByIndex(id);
}

/// Mappings from SampleFormats to their equivalent PaSampleFormats.
static const std::map<SampleFormat, portaudio::SampleDataFormat> pa_from_sf = {
	{ SampleFormat::PACKED_UNSIGNED_INT_8, portaudio::UINT8 },
	{ SampleFormat::PACKED_SIGNED_INT_8, portaudio::INT8 },
	{ SampleFormat::PACKED_SIGNED_INT_16, portaudio::INT16 },
	{ SampleFormat::PACKED_SIGNED_INT_24, portaudio::INT24 },
	{ SampleFormat::PACKED_SIGNED_INT_32, portaudio::INT32 },
	{ SampleFormat::PACKED_FLOAT_32, portaudio::FLOAT32 }
};

/* static */ portaudio::SampleDataFormat AudioSink::PaFormat(SampleFormat fmt)
{
	try {
		return pa_from_sf.at(fmt);
	} catch (std::out_of_range) {
		throw FileError(MSG_DECODE_BADRATE);
	}
}
