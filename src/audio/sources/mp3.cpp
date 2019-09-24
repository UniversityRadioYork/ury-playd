// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the Mp3_audio_source class.
 * @see audio/sources/mp3.h
 */

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <gsl/gsl>
#include <iostream>
#include <memory>
#include <string>

// We don't include mpg123.h directly here, because mp3.h does some polyfills
// before including it.

#include "../../errors.h"
#include "../../messages.h"
#include "../audio_source.h"
#include "../sample_format.h"
#include "mp3.h"

Mp3_audio_source::Mp3_audio_source(const std::string &path)
    : Audio_source{path}, buffer{}, context{nullptr}
{
	this->context = mpg123_new(nullptr, nullptr);
	mpg123_format_none(this->context);

	auto rates = AvailableRates();
	std::for_each(std::begin(rates), std::end(rates),
	              std::bind(&Mp3_audio_source::AddFormat, this,
	                        std::placeholders::_1));

	if (mpg123_open(this->context, path.c_str()) == MPG123_ERR) {
		throw File_error("mp3: can't open " + path + ": " +
		                 mpg123_strerror(this->context));
	}
}

Mp3_audio_source::~Mp3_audio_source()
{
	mpg123_delete(this->context);
	this->context = nullptr;
}

std::uint64_t Mp3_audio_source::Length() const
{
	assert(this->context != nullptr);

	return mpg123_length(this->context);
}

/* static */ gsl::span<const long> Mp3_audio_source::AvailableRates()
{
	const long *rawrates = nullptr;
	size_t nrates = 0;
	mpg123_rates(&rawrates, &nrates);
	return gsl::make_span(rawrates, nrates);
}

void Mp3_audio_source::AddFormat(long rate)
{
	Debug() << "trying to enable formats at " << rate << std::endl;

	// The requested encodings correspond to the sample formats available in
	// the SampleFormat enum.
	if (mpg123_format(this->context, rate, MPG123_STEREO | MPG123_MONO,
	                  (MPG123_ENC_UNSIGNED_8 | MPG123_ENC_SIGNED_8 |
	                   MPG123_ENC_SIGNED_16 | MPG123_ENC_SIGNED_32 |
	                   MPG123_ENC_FLOAT_32)) == MPG123_ERR) {
		// Ignore the error for now -- another sample rate may be
		// available.
		// If no sample rates work, loading a file will fail anyway.
		Debug() << "can't support" << rate << std::endl;
	};
}

std::uint8_t Mp3_audio_source::ChannelCount() const
{
	assert(this->context != nullptr);

	int chans = 0;
	mpg123_getformat(this->context, nullptr, &chans, nullptr);
	assert(chans != 0);
	return static_cast<std::uint8_t>(chans);
}

std::uint32_t Mp3_audio_source::SampleRate() const
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

std::uint64_t Mp3_audio_source::Seek(std::uint64_t in_samples)
{
	assert(this->context != nullptr);

	// Have we tried to seek past the end of the file?
	if (auto clen = static_cast<unsigned long>(mpg123_length(this->context));
	    clen < in_samples) {
		Debug() << "mp3: seek at" << in_samples << "past EOF at" << clen
		        << std::endl;
		throw Seek_error(MSG_SEEK_FAIL);
	}

	if (mpg123_seek(this->context, in_samples, SEEK_SET) == MPG123_ERR) {
		Debug() << "mp3: seek failed:" << mpg123_strerror(this->context)
		        << std::endl;
		throw Seek_error(MSG_SEEK_FAIL);
	}

	// The actual seek position may not be the same as the requested
	// position.
	// mpg123_tell gives us the exact mono-samples position.
	return mpg123_tell(this->context);
}

Mp3_audio_source::Decode_result Mp3_audio_source::Decode()
{
	assert(this->context != nullptr);

	auto buf = reinterpret_cast<unsigned char *>(&this->buffer.front());
	size_t rbytes = 0;
	const auto err =
	        mpg123_read(this->context, buf, this->buffer.size(), &rbytes);

	Decode_vector decoded;
	Decode_state decode_state = Decode_state::decoding;

	if (err == MPG123_DONE) {
		decode_state = Decode_state::eof;
	} else if (err != MPG123_OK && err != MPG123_NEW_FORMAT) {
		Debug() << "mp3: decode error:" << mpg123_strerror(this->context)
		        << std::endl;
		decode_state = Decode_state::eof;
	} else {
		// Copy only the bit of the buffer occupied by decoded data
		auto front = this->buffer.begin();
		decoded = Decode_vector(front, front + rbytes);
	}

	return std::make_pair(decode_state, decoded);
}

Sample_format Mp3_audio_source::OutputSampleFormat() const
{
	assert(this->context != nullptr);

	int encoding = 0;
	mpg123_getformat(this->context, nullptr, nullptr, &encoding);

	switch (encoding) {
		case MPG123_ENC_UNSIGNED_8:
			return Sample_format::uint8;
		case MPG123_ENC_SIGNED_8:
			return Sample_format::sint8;
		case MPG123_ENC_SIGNED_16:
			return Sample_format::sint16;
		case MPG123_ENC_SIGNED_32:
			return Sample_format::sint32;
		case MPG123_ENC_FLOAT_32:
			return Sample_format::float32;
		default:
			// We shouldn't get here, if the format was set up
			// correctly earlier.
			throw Internal_error(
			        "unsupported sample rate, should not "
			        "happen");
	}
}

std::unique_ptr<Mp3_audio_source> Mp3_audio_source::MakeUnique(const std::string &path)
{
	// This is in a separate function to let it be put into a jump table.
	return std::make_unique<Mp3_audio_source, const std::string &>(path);
}
