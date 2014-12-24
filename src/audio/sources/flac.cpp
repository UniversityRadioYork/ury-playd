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

#include <FLAC++/decoder.h>

#include "../../errors.hpp"
#include "../../messages.h"
#include "../sample_formats.hpp"
#include "../audio_source.hpp"
#include "flac.hpp"

FlacAudioSource::FlacAudioSource(const std::string &path)
    : buffer(), path(path)
{
	auto err = this->init(path);
	if (err != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		throw FileError("flac: can't open " + path + ": " +
		                FlacAudioSource::InitStrError(err));
	}

	// We need to decode some to get the sample rate etc.
	// Because libflac is callback-based, we have to spin on waiting for
	// the sample rate to materialise.
	do {
		this->process_single();
	} while (this->SampleRate() == 0);

	Debug() << "flac: sample rate:" << this->SampleRate() << std::endl;
	Debug() << "flac: bytes per sample:" << this->BytesPerSample()
	        << std::endl;
	Debug() << "flac: mono bytes per sample:"
	        << (this->get_bits_per_sample() / 8)
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
	// We convert the samples from FLAC to 32-bit signed, for convenience.
	// Thus, bytes per mono sample is 4; bytes per sample is 4*channels.
	//
	// NOTE: Because the external bps is different from the libflac bps,
	// do NOT use this in the decoder logic!  Use the frame parameters.
	return 4 * this->ChannelCount();
}

std::uint64_t FlacAudioSource::Seek(std::uint64_t position)
{
	auto samples = this->SamplesFromMicros(position);

	// Have we tried to seek past the end of the file?
	auto clen = static_cast<unsigned long>(this->get_total_samples());
	if (clen < samples) {
		Debug() << "flac: seek at" << samples << "past EOF at"
		        << clen << std::endl;
		Debug() << "flac: requested position micros:" << position
		        << std::endl;
		throw SeekError(MSG_SEEK_FAIL);
	}

	bool seeked = this->seek_absolute(samples);
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
	
	// Don't use this->BytesPerSample() here--it returns bytes per *output* sample.
	return new_bytes / (this->get_bits_per_sample() / 8);
}

FlacAudioSource::DecodeResult FlacAudioSource::Decode()
{
	// Have we hit the end of the file?
	// (Note: in order to know this, we need a definite count of total
	// samples in file, bits per sample, and decode position.  If any of these
	// are missing, we assume we're not at EOF just yet.)
	auto total = this->get_total_samples();
	if (0 < total) {
		std::uint64_t current;
		if (this->get_decode_position(&current)) {
			auto bps = this->get_bits_per_sample() / 8;
			if ((0 < bps) && (total <= (current / bps))) {
				this->decode_state = DecodeState::END_OF_FILE;
				return std::make_pair(DecodeState::END_OF_FILE,
			  		DecodeVector());
			}
		}
	}

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
	uint8_t *begin = reinterpret_cast<uint8_t *>(&*this->buffer.begin());
	uint8_t *end = reinterpret_cast<uint8_t *>(&*this->buffer.end());
	return std::make_pair(DecodeState::DECODING,
		  DecodeVector(begin, end));
}

SampleFormat FlacAudioSource::OutputSampleFormat() const
{
	// All FLAC output formats are signed integer.
	// See https://xiph.org/flac/api/group__flac__stream__decoder.html.

	// See write_callback() for an explanation of why this is always 32 bits.
	// (This is also why BytesPerSample() is slightly weird.)
	return SampleFormat::PACKED_SIGNED_INT_32;
}

FLAC__StreamDecoderWriteStatus FlacAudioSource::write_callback(const FLAC__Frame *frame, const FLAC__int32 *const buf[])
{	
	// Each buffer index contains a full sample, padded up to 32 bits.
	// These are in planar (per-channel) format.

	this->buffer.clear();
	assert(this->buffer.empty());
	
	size_t nsamples = frame->header.blocksize;
	if (nsamples == 0) {
		// No point trying to decode 0 samples.
		return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
	}
	
	size_t nchans = frame->header.channels;

	// Need to prepare the buffer for writing however many bytes this works out as.
	// We need to get the bytes-per-sample from the *frame*, because it may well be that the global bytes-per-sample count hasn't populated yet.
	int bits = frame->header.bits_per_sample;
	this->buffer = std::vector<std::int32_t>(nsamples * nchans, 0);

	// FLAC returns its samples in planar format, and playd wants them to be packed.
	// We do a simple interleaving here.
	size_t sc = 0;
	for (size_t s = 0; s < nsamples; s++) {
		for (size_t c = 0; c < nchans; c++) {
			std::int32_t samp = buf[c][s];

			// Now, we've set up so that we always send 32-bit samples, but this 32-bit integer may actually contain an 8, 16, or 24 bit sample.
			// How do we convert to 32 bit?  Simple--left bitshift up the missing number of bits.
			this->buffer[sc] = samp << (32 - bits);
			sc++;
		}
	}

	assert(this->buffer.size() == nchans * nsamples);
	assert(sc == nchans * nsamples);
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FlacAudioSource::error_callback(FLAC__StreamDecoderErrorStatus)
{
	// Currently ignored.
	// TODO(CaptainHayashi): not ignore these?
}

#endif // NO_FLAC
