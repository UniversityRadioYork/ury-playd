// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the FlacAudioSource class.
 * @see audio/audio_source.hpp
 */

#ifndef NO_FLAC

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include <FLAC++/decoder.hpp>

#include "../errors.hpp"
#include "../messages.h"
#include "../sample_formats.hpp"
#include "audio_source.hpp"

// This value is somewhat arbitrary, but corresponds to the minimum buffer size
// used by ffmpeg, so it's probably sensible.
const size_t FlacAudioSource::BUFFER_SIZE = 16384;

FlacAudioSource::FlacAudioSource(const std::string &path)
    : buffer(), context(), path(path)
{
	auto err = this->init(path);
	if (err != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		throw FileError("flac: can't open " + path ": " +
		                FlacAudioSource::StrError(err));

	// We need to decode one sample to get the sample rate etc.
	// Because libflac is callback-based, we have to spin on waiting for
	this->process_single();

	Debug() << "flac: sample rate:" << this->SampleRate() << std::endl;
	Debug() << "flac: bytes per sample:" << this->BytesPerSample()
	        << std::endl;
	Debug() << "flac: channels:" << (int)this->ChannelCount() << std::endl;
	Debug() << "flac: playd format:" << (int)this->OutputSampleFormat()
	        << std::endl;
}

FlacAudioSource::~FlacAudioSource()
{
	this->finish();
}

/* static */ std::string FlacAudioSource::InitStrError(int err)
{
	switch (err) {
		case FLAC__STREAM_DECODER_INIT_STATUS_OK:
			return "success";
		case FLAC__STREAM_DECODER_INIT_STATUS_UNSUPPORTED_CONTAINER:
			return "libflac missing support for container";
		case FLAC__STREAM_DECODER_INIT_STATUS_INVALID_CALLBACKS:
			return "internal error: bad callbacks";
		case FLAC__STREAM_DECODER_INIT_STATUS_MEMORY_ALLOCATION_ERROR:
			return "out of memory";
		case FLAC__STREAM_DECODER_INIT_STATUS_ERROR_OPENING_FILE:
			return "fopen() failed";
		case FLAC__STREAM_DECODER_INIT_STATUS_ALREADY_INITIALIZED:
			return "internal error: already initialised?";
		default:
			return "unknown error";
	}
}

std::string FlacAudioSource::Path() const
{
	return this->path;
}

std::uint8_t FlacAudioSource::ChannelCount() const
{
	return static_cast<std::uint8_t>(this->get_channels());
}

double FlacAudioSource::SampleRate() const
{
	return static_cast<double>(this->get_sample_rate());
}

size_t FlacAudioSource::BytesPerSample() const
{
	assert(this->context != nullptr);

	// Assuming, of course, there are 8 bits in the byte. :o)
	auto es = this->get_bits_per_sample() / 8;
	assert(es != 0);

	// This returns bytes per mono sample, so we need to
	// convert to bytes per all-channels sample.
	// All of FLAC's samples counts are like this, so you'll see this
	// pattern all over the class.
	return es * this->ChannelCount();
}

std::uint64_t FlacAudioSource::Seek(std::uint64_t position)
{
	assert(this->context != nullptr);

	auto samples = this->SamplesFromMicros(position);

	// See BytesPerSample() for an explanation of this ChannelCount().
	auto mono_samples = samples * this->ChannelCount();

	// Have we tried to seek past the end of the file?
	auto clen = static_cast<unsigned long>(this->get_total_samples());
	if (clen < mono_samples) {
		Debug() << "flac: seek at" << mono_samples << "past EOF at"
		        << clen << std::endl;
		Debug() << "flac: requested position micros:" << position
		        << std::endl;
		throw SeekError(MSG_SEEK_FAIL);
	}

	bool seeked = this->seek_absolute(mono_samples)
	if (!seeked || this->get_state() == FLAC__STREAM_DECODER_SEEK_ERROR) {
		Debug() << "flac: seek failed" << std::endl;
		this->flush();
		throw SeekError(MSG_SEEK_FAIL);
	}

	// Reset the decoder state, because otherwise the decoder will get very
	// confused.
	this->decode_state = DecodeState::DECODING;

	// The actual seek position may not be the same as the requested
	// position.
	// get_decode_position() tells us the new position, in bytes.
	uint64_t new_bytes = 0;
	if (!this->get_decode_position(&new_bytes)) {
		Debug() << "flac: seek failed" << std::endl;
		this->flush();
		throw SeekError(MSG_SEEK_FAIL);
	}
	return this->SamplesFromBytes(new_bytes);
}

FlacAudioSource::DecodeResult FlacAudioSource::Decode()
{
	if (!this->process_single()) {
		// Something went wrong... or we're at end of file.
		// Let's find out.
		int s = this->get_state();
		if (s == FLAC__STREAM_DECODER_END_OF_STREAM) {
			this->decode_state = DecodeState::END_OF_FILE;
			return std::make_pair(DecodeState::END_OF_FILE,
			  DecodeVector());
		} else {
			Debug() << "flac: decode error" << std::endl;
			// TODO: handle error correctly
			return std::make_pair(DecodeState::END_OF_FILE,
			  DecodeVector());
		}
	}

	// Else, we're good to go.
	// The callback will have set up the vector to have precisely the
	// correct number of bytes in it.
	return std::make_pair(DecodeState::DECODING,
		  DecodeVector(this->buffer.begin(), this->buffer.end()));
}

SampleFormat FlacAudioSource::OutputSampleFormat() const
{
	// All FLAC output formats are signed integer.
	// See https://xiph.org/flac/api/group__flac__stream__decoder.html.

	// See write_callback() for an explanation of why this is always 32 bits.
	return SampleFormat::PACKED_SIGNED_INT_32;
}

FLAC__StreamDecoderWriteStatus FlacAudioSource::write_callback(const ::FLAC__Frame *frame, const FLAC__int32 *const buffer[])
{
	// Each buffer index contains a full sample, padded up to 32 bits.
	// These are in planar (per-channel) format.

	size_t nsamples = frame->header.blocksize;
	size_t nchans = sizeof(buffer);

	// Need to prepare the buffer for writing however many bytes this works out as.
	int bps = this->BytesPerSample();
	this->buffer.clear();
	this->buffer.reserve(bps * nchans * nsamples);

	// FLAC returns its samples in planar format, and playd wants them to be packed.
	// We do a simple interleaving here.
	for (size_t s = 0; s < nsamples / ; b++) {
		for (size_t c = 0; c < nchans; c++) {
			std::int32_t samp = buffer[c][s];

			// Now, we've set up so that we always send 32-bit samples, but this 32-bit integer may actually contain an 8, 16, or 24 bit sample.
			// How do we convert to 32 bit?  Simple--left bitshift up the missing number of bytes.
			samp <<= (4 - bps);

			// Now to push it onto the buffer, which is byte-addressed.
			std::uint8_t *sbytes = std::reinterpret_cast<std::uint8_t *>&samp;
			for (int b = 0; b < 4; i++)
				this->buffer.push_back(sbytes[b]);
			}
		}
	}
}

#endif // NO_FLAC
