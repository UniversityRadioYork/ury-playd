// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the Mp3AudioSource class.
 * @see audio/audio_source.hpp
 */

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>

extern "C" {
#include <mpg123.h>
}

#include "../errors.hpp"
#include "../messages.h"
#include "../sample_formats.hpp"
#include "audio_source.hpp"

// This value is somewhat arbitrary, but corresponds to the minimum buffer size
// used by ffmpeg, so it's probably sensible.
const size_t Mp3AudioSource::BUFFER_SIZE = 16384;

Mp3AudioSource::Mp3AudioSource(const std::string &path)
    : buffer(BUFFER_SIZE), context(nullptr), path(path)
{
	this->context = mpg123_new(nullptr, nullptr);
	mpg123_format_none(this->context);

	const long *rates = nullptr;
	size_t nrates = 0;
	mpg123_rates(&rates, &nrates);
	for (size_t r = 0; r < nrates; r++) {
		Debug() << "trying to enable formats at " << rates[r]
		        << std::endl;
		AddFormat(rates[r]);
	}

	if (mpg123_open(this->context, path.c_str()) == MPG123_ERR) {
		throw FileError("mp3: can't open " + path + ": " +
		                mpg123_strerror(this->context));
	}

	Debug() << "mp3: sample rate:" << this->SampleRate() << std::endl;
	Debug() << "mp3: bytes per sample:" << this->BytesPerSample()
	        << std::endl;
	Debug() << "mp3: channels:" << (int)this->ChannelCount() << std::endl;
	Debug() << "mp3: playd format:" << (int)this->OutputSampleFormat()
	        << std::endl;
}

Mp3AudioSource::~Mp3AudioSource()
{
	mpg123_delete(this->context);
	this->context = nullptr;
}

void Mp3AudioSource::AddFormat(long rate)
{
	// The requested encodings correspond to the sample formats available in
	// the SampleFormat enum.
	if (mpg123_format(this->context, rate, MPG123_STEREO | MPG123_MONO,
	                  (MPG123_ENC_UNSIGNED_8 | MPG123_ENC_SIGNED_16 |
	                   MPG123_ENC_SIGNED_32 | MPG123_ENC_FLOAT_32)) ==
	    MPG123_ERR) {
		// Ignore the error for now -- another sample rate may be available.
		// If no sample rates work, loading a file will fail anyway.
		Debug() << "can't support " << rate << std::endl;
	};
}

std::string Mp3AudioSource::Path() const
{
	return this->path;
}

std::uint8_t Mp3AudioSource::ChannelCount() const
{
	assert(this->context != nullptr);

	long rate = 0;
	int chans = 0;
	int encoding = 0;

	mpg123_getformat(this->context, &rate, &chans, &encoding);
	assert(chans != 0);
	return static_cast<std::uint8_t>(chans);
}

double Mp3AudioSource::SampleRate() const
{
	assert(this->context != nullptr);

	long rate = 0;
	int chans = 0;
	int encoding = 0;

	mpg123_getformat(this->context, &rate, &chans, &encoding);
	return static_cast<double>(rate);
}

size_t Mp3AudioSource::BytesPerSample() const
{
	assert(this->context != nullptr);

	long rate = 0;
	int chans = 0;
	int encoding = 0;

	mpg123_getformat(this->context, &rate, &chans, &encoding);
	auto es = mpg123_encsize(encoding);
	assert(es != 0);

	// mpg123_encsize returns bytes per mono sample, so we need to
	// convert to bytes per all-channels sample.
	// All of mpg123's samples counts are like this, so you'll see this
	// pattern all over the class.
	return es * this->ChannelCount();
}

std::uint64_t Mp3AudioSource::Seek(std::uint64_t position)
{
	assert(this->context != nullptr);

	auto samples = this->SamplesFromMicros(position);

	// See BytesPerSample() for an explanation of this ChannelCount().
	auto mono_samples = samples * this->ChannelCount();

	// Have we tried to seek past the end of the file?
	auto clen = static_cast<unsigned long>(mpg123_length(this->context));
	if (clen < mono_samples) {
		Debug() << "mp3: seek at" << mono_samples << "past EOF at"
		        << clen << std::endl;
		Debug() << "mp3: requested position micros:" << position
		        << std::endl;
		throw SeekError(MSG_SEEK_FAIL);
	}

	if (mpg123_seek(this->context, mono_samples, SEEK_SET) == MPG123_ERR) {
		Debug() << "mp3: seek failed:" << mpg123_strerror(this->context)
		        << std::endl;
		throw SeekError(MSG_SEEK_FAIL);
	}

	// Reset the decoder state, because otherwise the decoder will get very
	// confused.
	this->decode_state = DecodeState::DECODING;

	// The actual seek position may not be the same as the requested
	// position.
	// mpg123_tell gives us the exact mono-samples position.
	return mpg123_tell(this->context) / this->ChannelCount();
}

Mp3AudioSource::DecodeResult Mp3AudioSource::Decode()
{
	assert(this->context != nullptr);

	auto buf = reinterpret_cast<unsigned char *>(&this->buffer.front());
	size_t rbytes = 0;
	int err = mpg123_read(this->context, buf, this->buffer.size(), &rbytes);

	DecodeVector decoded;

	if (err == MPG123_DONE) {
		Debug() << "mp3: end of file" << std::endl;
		this->decode_state = DecodeState::END_OF_FILE;
	} else if (err != MPG123_OK && err != MPG123_NEW_FORMAT) {
		Debug() << "mp3: decode error:" << mpg123_strerror(this->context)
		        << std::endl;
		this->decode_state = DecodeState::END_OF_FILE;
	} else {
		this->decode_state = DecodeState::DECODING;

		// Copy only the bit of the buffer occupied by decoded data
		auto front = this->buffer.begin();
		decoded = DecodeVector(front, front + rbytes);
	}

	return std::make_pair(this->decode_state, decoded);
}

SampleFormat Mp3AudioSource::OutputSampleFormat() const
{
	assert(this->context != nullptr);

	long rate = 0;
	int chans = 0;
	int encoding = 0;

	mpg123_getformat(this->context, &rate, &chans, &encoding);

	switch (encoding) {
		case MPG123_ENC_UNSIGNED_8:
			return SampleFormat::PACKED_UNSIGNED_INT_8;
		case MPG123_ENC_SIGNED_16:
			return SampleFormat::PACKED_SIGNED_INT_16;
		case MPG123_ENC_SIGNED_32:
			return SampleFormat::PACKED_SIGNED_INT_32;
		case MPG123_ENC_FLOAT_32:
			return SampleFormat::PACKED_FLOAT_32;
		default:
			// We shouldn't get here, if the format was set up
			// correctly earlier.
			assert(false);
	}
}