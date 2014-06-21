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

#include "audio_output.hpp"
#include "audio_decoder.hpp"
#include "audio_cb.h" /* audio_cb_play */

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
	this->last_error = ErrorCode::INCOMPLETE;
	this->av = std::unique_ptr<AudioDecoder>(new AudioDecoder(path));

	InitialisePortAudio(c);
	InitialiseRingBuffer(ByteCountForSampleCount(1L));

	this->position_sample_count = 0;

	this->frame = std::vector<char>();
	this->frame_iterator = this->frame.end();
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

/* Returns the last decoding error, or E_OK if the last decode succeeded. */
ErrorCode AudioOutput::LastError()
{
	return this->last_error;
}

/* Checks to see if audio playback has halted of its own accord.
 *
 * If audio is still playing, E_OK will be returned; otherwise the decoding
 * error that caused playback to halt will be returned.  E_UNKNOWN is returned
 * if playback has halted but the last error report was E_OK.
 */
bool AudioOutput::IsHalted()
{
	return !this->out_strm->isActive();
}

/* Gets the current played position in the song, in microseconds.
 *
 * As this may be executing whilst the playing callback is running,
 * do not expect it to be highly accurate.
 */
std::chrono::microseconds AudioOutput::CurrentPositionMicroseconds()
{
	return this->av->PositionMicrosecondsForSampleCount(
	                this->position_sample_count);
}

size_t AudioOutput::ByteCountForSampleCount(size_t samples) const
{
	return this->av->ByteCountForSampleCount(samples);
}

size_t AudioOutput::SampleCountForByteCount(size_t bytes) const
{
	return this->av->SampleCountForByteCount(bytes);
}

/* Tries to place enough audio into the audio buffer to prevent a
 * buffer underrun during a player start.
 *
 * If end of file is reached, it is ignored and converted to E_OK so that it can
 * later be caught by the player callback once it runs out of sound.
 */
void AudioOutput::PreFillRingBuffer()
{
	/* Either fill the ringbuf or hit the maximum spin-up size,
	 * whichever happens first.  (There's a maximum in order to
	 * prevent spin-up from taking massive amounts of time and
	 * thus delaying playback.)
	 */
	bool more = true;
	unsigned long c = RingBufferWriteCapacity();
	while (more && c > 0 && RINGBUF_SIZE - c < SPINUP_SIZE) {
		more = Update();
		c = RingBufferWriteCapacity();
	}
}

/* Attempts to seek to the given position in microseconds. */
void AudioOutput::SeekToPositionMicroseconds(
                std::chrono::microseconds microseconds)
{
	this->av->SeekToPositionMicroseconds(microseconds);

	size_t new_position_sample_count =
	                this->av->SampleCountForPositionMicroseconds(
	                                microseconds);
	this->frame.clear();
	this->frame_iterator = this->frame.end();

	this->last_error = ErrorCode::INCOMPLETE;
	this->position_sample_count = new_position_sample_count;

	// while (!Pa_IsStreamStopped(this->out_strm)) {
	//	decode();
	//}; // Spin until stream finishes
	this->ring_buf->Flush();
}

/**
 * Performs an update cycle on this AudioOutput.
 * This ensures the ring buffer has output to offer to the sound driver.
 * It does this by by asking the AudioDecoder to decode if necessary.
 *
 * @return True if there is more output to send to the sound card; false
 *otherwise.
 */
bool AudioOutput::Update()
{
	bool more_frames_available = DecodeIfFrameEmpty();

	if (more_frames_available) {
		assert(!this->frame.empty());
		WriteAllAvailableToRingBuffer();
	}

	return more_frames_available;
}

/**
 * Decode the next frame if the current frame has been fully used.
 * @return True if there were some samples left to decode; false otherwise.
 */
bool AudioOutput::DecodeIfFrameEmpty()
{
	assert(this->frame.empty() || this->frame_iterator < this->frame.end());

	if (this->frame.end() <= this->frame_iterator) {
		this->frame = this->av->Decode();

		this->frame_iterator = this->frame.begin();
	}

	return !(this->frame.empty());
}

/**
 * Writes all samples currently waiting to be transferred to the ring buffer.
 */
void AudioOutput::WriteAllAvailableToRingBuffer()
{
	unsigned long count = RingBufferTransferCount();
	if (0 < count) {
		WriteToRingBuffer(count);
	}
}

/**
 * Write a given number of samples from the current frame to the ring buffer.
 * @param sample_count The number of samples to write to the ring buffer.
 */
void AudioOutput::WriteToRingBuffer(unsigned long sample_count)
{
	assert(0 < sample_count);

	unsigned long written_count = this->ring_buf->Write(
	                &(*this->frame_iterator),
	                static_cast<ring_buffer_size_t>(sample_count));
	if (written_count != sample_count) {
		throw Error(ErrorCode::INTERNAL_ERROR, "ringbuf write error");
	}
	assert(0 < written_count);

	AdvanceFrameIterator(written_count);
}

/**
 * Moves the decoded data iterator forwards by a number of samples.
 *
 * If the iterator runs off the end of the decoded data vector, the data
 * vector is freed and set to nullptr, so that a new decoding run can occur.
 *
 * @param sample_count The number of samples to move the markers.
 */
void AudioOutput::AdvanceFrameIterator(unsigned long sample_count)
{
	auto byte_count = this->av->ByteCountForSampleCount(sample_count);
	assert(sample_count <= byte_count);
	assert(0 < byte_count);

	std::advance(this->frame_iterator, byte_count);
	if (this->frame_iterator >= this->frame.end()) {
		this->frame.clear();
		this->frame_iterator = this->frame.end();
	}

	assert(this->frame.empty() ||
	       (this->frame.begin() < this->frame_iterator &&
	        this->frame_iterator < this->frame.end()));
}

void AudioOutput::InitialisePortAudio(const StreamConfigurator &c)
{
	this->out_strm = std::unique_ptr<portaudio::Stream>(
	                c.Configure(*this, *(this->av)));
}

/* Initialises an audio structure's ring buffer so that decoded
 * samples can be placed into it.
 *
 * Any existing ring buffer will be freed.
 *
 * The number of bytes for each sample must be provided; see
 * audio_av_samples2bytes for one way of getting this.
 */
void AudioOutput::InitialiseRingBuffer(size_t bytes_per_sample)
{
	this->ring_buf = std::unique_ptr<RingBuffer<char, long>>(
	                new RINGBUF_CLASS<char, long, RINGBUF_POWER>(
	                                bytes_per_sample));
}

/**
 * Gets the current write capacity of the ring buffer.
 * @return The write capacity, in samples.
 */
unsigned long AudioOutput::RingBufferWriteCapacity()
{
	return this->ring_buf->WriteCapacity();
}

/**
 * Gets the current number of samples that may be transferred from the frame to
 * the ring buffer.
 * @return The transfer count, in samples.
 */
unsigned long AudioOutput::RingBufferTransferCount()
{
	assert(!this->frame.empty());

	long bytes = std::distance(this->frame_iterator, this->frame.end());
	assert(0 <= bytes);

	size_t samples = SampleCountForByteCount(static_cast<size_t>(bytes));
	return std::min(samples, RingBufferWriteCapacity());
}
