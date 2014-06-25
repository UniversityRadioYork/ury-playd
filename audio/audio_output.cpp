/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

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

#include <cassert>
#include <algorithm>
#include <string>

#include "portaudiocpp/PortAudioCpp.hxx"

#include "../constants.h"
#include "../sample_formats.hpp"
#include "../errors.hpp"

#include "audio_output.hpp"
#include "audio_decoder.hpp"

// Use the PortAudio ringbuffer by default.  This is because of unsettled bugs
// with the Boost ringbuffer—if the latter can be fixed, it should be used
// instead.
#ifdef USE_BOOST_RINGBUF

#define RINGBUF_CLASS BoostRingBuffer
#include "../ringbuffer/ringbuffer_boost.hpp"

#else

#define RINGBUF_CLASS PaRingBuffer
#include "../ringbuffer/ringbuffer_pa.hpp"

#endif

AudioOutput::AudioOutput(const std::string &path, const StreamConfigurator &c)
{
	this->av = std::unique_ptr<AudioDecoder>(new AudioDecoder(path));

	InitialisePortAudio(c);
	InitialiseRingBuffer(ByteCountForSampleCount(1L));

	this->position_sample_count = 0;

	ClearFrame();
}

AudioOutput::~AudioOutput()
{
	this->out_strm = nullptr;
	Debug("closed output stream");
}

void AudioOutput::Start()
{
	PreFillRingBuffer();

	this->out_strm->start();
	Debug("audio started");
}

void AudioOutput::Stop()
{
	this->out_strm->abort();
	Debug("audio stopped");

	// TODO: Possibly recover from dropping frames due to abort.
}

bool AudioOutput::IsHalted()
{
	return !this->out_strm->isActive();
}

std::chrono::microseconds AudioOutput::CurrentPositionMicroseconds()
{
	return this->av->PositionMicrosecondsForSampleCount(
	                this->position_sample_count);
}

std::uint64_t AudioOutput::ByteCountForSampleCount(std::uint64_t samples) const
{
	return this->av->ByteCountForSampleCount(samples);
}

std::uint64_t AudioOutput::SampleCountForByteCount(std::uint64_t bytes) const
{
	return this->av->SampleCountForByteCount(bytes);
}

void AudioOutput::PreFillRingBuffer()
{
	// Either fill the ringbuf or hit the maximum spin-up size, whichever
	// happens first.  (There's a maximum in order to prevent spin-up from
	// taking massive amounts of time and thus delaying playback.)
	bool more = true;
	std::uint64_t c = RingBufferWriteCapacity();
	while (more && c > 0 && RINGBUF_SIZE - c < SPINUP_SIZE) {
		more = Update();
		c = RingBufferWriteCapacity();
	}
}

void AudioOutput::SeekToPositionMicroseconds(
                std::chrono::microseconds microseconds)
{
	this->av->SeekToPositionMicroseconds(microseconds);
	this->position_sample_count =
	                this->av->SampleCountForPositionMicroseconds(
	                                microseconds);

	ClearFrame();
	this->ring_buf->Flush();
}

void AudioOutput::ClearFrame()
{
	this->frame.clear();
	this->frame_iterator = this->frame.end();
	this->file_ended = false;
}

bool AudioOutput::Update()
{
	bool more_frames_available = DecodeIfFrameEmpty();

	if (more_frames_available) {
		assert(!this->frame.empty());
		WriteAllAvailableToRingBuffer();
	}

	this->file_ended = !more_frames_available;
	return more_frames_available;
}

bool AudioOutput::DecodeIfFrameEmpty()
{
	// Either the current frame is in progress, or has been emptied.
	// AdvanceFrameIterator() establishes this assertion by emptying a
	// frame as soon as it finishes.
	assert(this->frame.empty() || !FrameFinished());

	if (FrameFinished()) {
		this->frame = this->av->Decode();
		this->frame_iterator = this->frame.begin();
	}

	// If the frame is empty, then the decoder has finished.
	// Otherwise, it has successfully decoded a frame.
	return !(this->frame.empty());
}

bool AudioOutput::FileEnded()
{
	return this->file_ended;
}

void AudioOutput::WriteAllAvailableToRingBuffer()
{
	std::uint64_t count = RingBufferTransferCount();
	if (0 < count) {
		WriteToRingBuffer(count);
	}
}

void AudioOutput::WriteToRingBuffer(std::uint64_t sample_count)
{
	// This should have been established by WriteAllAvailableToRingBuffer.
	assert(0 < sample_count);

	std::uint64_t written_count = this->ring_buf->Write(
	                &(*this->frame_iterator),
	                static_cast<ring_buffer_size_t>(sample_count));
	if (written_count != sample_count) {
		throw Error(ErrorCode::INTERNAL_ERROR, "ringbuf write error");
	}
	assert(0 < written_count);

	AdvanceFrameIterator(written_count);
}

bool AudioOutput::FrameFinished()
{
	return this->frame.end() <= this->frame_iterator;
}

void AudioOutput::AdvanceFrameIterator(std::uint64_t sample_count)
{
	auto byte_count = ByteCountForSampleCount(sample_count);
	assert(sample_count <= byte_count);
	assert(0 < byte_count);

	std::advance(this->frame_iterator, byte_count);

	// We empty the frame once we're done with it.  This
	// maintains FrameFinished(), as an empty frame is a finished one.
	if (FrameFinished()) {
		ClearFrame();
		assert(FrameFinished());
	}

	// The frame iterator should be somewhere between the beginning and
	// end of the frame, unless the frame was emptied.
	assert(this->frame.empty() ||
	       (this->frame.begin() < this->frame_iterator &&
	        this->frame_iterator < this->frame.end()));
}

void AudioOutput::InitialisePortAudio(const StreamConfigurator &c)
{
	this->out_strm = std::unique_ptr<portaudio::Stream>(
	                c.Configure(*this, *(this->av)));
}

void AudioOutput::InitialiseRingBuffer(std::uint64_t bytes_per_sample)
{
	this->ring_buf = decltype(this->ring_buf)(
	                new RINGBUF_CLASS<char, std::uint64_t, RINGBUF_POWER>(
	                                bytes_per_sample));
}

std::uint64_t AudioOutput::RingBufferWriteCapacity()
{
	return this->ring_buf->WriteCapacity();
}

std::uint64_t AudioOutput::RingBufferTransferCount()
{
	assert(!this->frame.empty());

	long bytes = std::distance(this->frame_iterator, this->frame.end());
	assert(0 <= bytes);

	std::uint64_t samples = SampleCountForByteCount(
	                static_cast<std::uint64_t>(bytes));
	return std::min(samples, RingBufferWriteCapacity());
}

int AudioOutput::paCallbackFun(const void *, void *out,
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

	if (FileEnded()) {
		result = std::make_pair(paComplete, in.second);
	} else {
		// Make up some silence to plug the gap.
		memset(out, 0, ByteCountForSampleCount(frames_per_buf));
		result = std::make_pair(paContinue, frames_per_buf);
	}

	return result;
}

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
