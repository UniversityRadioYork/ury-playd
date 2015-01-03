// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the Mp3AudioSource class.
 * @see audio/sources/mp3.hpp
 */

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>

extern "C" {
#include <mpg123.h>
}

#include "../../errors.hpp"
#include "../../messages.h"
#include "../audio_source.hpp"
#include "../sample_formats.hpp"
#include "mp3.hpp"

std::uint64_t Mp3AudioSource::instances = 0;

// This value is somewhat arbitrary, but corresponds to the minimum buffer size
// used by ffmpeg, so it's probably sensible.
const size_t Mp3AudioSource::BUFFER_SIZE = 16384;

/* static */ std::unique_ptr<AudioSource> Mp3AudioSource::Build(const std::string &path)
{
	return std::unique_ptr<AudioSource>(new Mp3AudioSource(path));
}

Mp3AudioSource::Mp3AudioSource(const std::string &path)
    : AudioSource(path), buffer(BUFFER_SIZE), context(nullptr)
{
	if (Mp3AudioSource::instances++ == 0) mpg123_init();

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

	if (--Mp3AudioSource::instances == 0) mpg123_exit();
}

void Mp3AudioSource::AddFormat(long rate)
{
	// The requested encodings correspond to the sample formats available in
	// the SampleFormat enum.
	if (mpg123_format(this->context, rate, MPG123_STEREO | MPG123_MONO,
	                  (MPG123_ENC_UNSIGNED_8 | MPG123_ENC_SIGNED_8 |
	                   MPG123_ENC_SIGNED_16 | MPG123_ENC_SIGNED_32 | MPG123_ENC_FLOAT_32)) ==
	    MPG123_ERR) {
		// Ignore the error for now -- another sample rate may be
		// available.
		// If no sample rates work, loading a file will fail anyway.
		Debug() << "can't support" << rate << std::endl;
	};
}

std::uint8_t Mp3AudioSource::ChannelCount() const
{
	assert(this->context != nullptr);

	int chans = 0;
	mpg123_getformat(this->context, nullptr, &chans, nullptr);
	assert(chans != 0);
	return static_cast<std::uint8_t>(chans);
}

std::uint32_t Mp3AudioSource::SampleRate() const
{
	assert(this->context != nullptr);

	long rate = 0;
	mpg123_getformat(this->context, &rate, nullptr, nullptr);

	assert(0 < rate);
	// INT32_MAX isn't a typo; if we compare against UINT32_MAX, we'll
	// set off sign-compare errors, and the sample rate shouldn't be above
	// INT32_MAX anyroad.
	assert(rate <= INT32_MAX);
	return static_cast<std::uint32_t>(rate);
}

std::uint64_t Mp3AudioSource::Seek(std::uint64_t in_samples)
{
	assert(this->context != nullptr);

	// See BytesPerSample() for an explanation of this ChannelCount().
	auto mono_samples = in_samples * this->ChannelCount();

	// Have we tried to seek past the end of the file?
	auto clen = static_cast<unsigned long>(mpg123_length(this->context));
	if (clen < mono_samples) {
		Debug() << "mp3: seek at" << mono_samples << "past EOF at"
		        << clen << std::endl;
		throw SeekError(MSG_SEEK_FAIL);
	}

	if (mpg123_seek(this->context, mono_samples, SEEK_SET) == MPG123_ERR) {
		Debug() << "mp3: seek failed:" << mpg123_strerror(this->context)
		        << std::endl;
		throw SeekError(MSG_SEEK_FAIL);
	}

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
	DecodeState decode_state;

	if (err == MPG123_DONE) {
		Debug() << "mp3: end of file" << std::endl;
		decode_state = DecodeState::END_OF_FILE;
	} else if (err != MPG123_OK && err != MPG123_NEW_FORMAT) {
		Debug() << "mp3: decode error:" << mpg123_strerror(this->context)
		        << std::endl;
		decode_state = DecodeState::END_OF_FILE;
	} else {
		decode_state = DecodeState::DECODING;

		// Copy only the bit of the buffer occupied by decoded data
		auto front = this->buffer.begin();
		decoded = DecodeVector(front, front + rbytes);
	}

	return std::make_pair(decode_state, decoded);
}

SampleFormat Mp3AudioSource::OutputSampleFormat() const
{
	assert(this->context != nullptr);

	int encoding = 0;
	mpg123_getformat(this->context, nullptr, nullptr, &encoding);

	switch (encoding) {
		case MPG123_ENC_UNSIGNED_8:
			return SampleFormat::PACKED_UNSIGNED_INT_8;
		case MPG123_ENC_SIGNED_8:
			return SampleFormat::PACKED_SIGNED_INT_8;
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
			return SampleFormat::PACKED_UNSIGNED_INT_8;
	}
}
