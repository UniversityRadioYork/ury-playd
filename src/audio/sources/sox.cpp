// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the SoXAudioSource class.
 * @see audio/sources/sox.hpp
 */

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>

// libsox
extern "C" {
#include <sox.h>
}

#include "../../errors.hpp"
#include "../../messages.h"
#include "../audio_source.hpp"
#include "../sample_formats.hpp"
#include "sox.hpp"

// This value is somewhat arbitrary, but corresponds to the minimum buffer size
// used by ffmpeg, so it's probably sensible.
const size_t SoXAudioSource::BUFFER_SIZE = 16384;

SoXAudioSource::SoXAudioSource(const std::string &path)
    : buffer(BUFFER_SIZE), context(nullptr)
{
	this->Open(path);
}

SoXAudioSource::~SoXAudioSource()
{
	this->Close();
}

std::string SoXAudioSource::Path() const
{
	assert(this->context != nullptr);
	assert(this->context->filename != nullptr);
	return std::string(this->context->filename);
}

std::uint8_t SoXAudioSource::ChannelCount() const
{
	assert(this->context != nullptr);
	return this->context->signal.channels;
}

size_t SoXAudioSource::BufferSampleCapacity() const
{
	return this->buffer.size() / this->BytesPerSample();
}

double SoXAudioSource::SampleRate() const
{
	assert(this->context != nullptr);
	return this->context->signal.rate;
}

std::uint64_t SoXAudioSource::Seek(std::uint64_t position)
{
	assert(this->context != nullptr);

	auto samples = this->SamplesFromMicros(position);

	// See BytesPerSample() for an explanation of this ChannelCount().
	auto sox_samples = samples * this->ChannelCount();

	// libsox doesn't seem to like seeking into an ended file, so close
	// and re-open it first.
	if (this->decode_state == DecodeState::END_OF_FILE) {
		std::string path = this->Path();
		this->Close();
		this->Open(path);
	}

	// Have we tried to seek past the end of the file?
	if (this->context->signal.length < sox_samples) {
		throw SeekError(MSG_SEEK_FAIL);
	}

	if (sox_seek(this->context, sox_samples, SOX_SEEK_SET) != SOX_SUCCESS) {
		throw SeekError(MSG_SEEK_FAIL);
	}

	// Reset the decoder state, because otherwise the decoder will get very
	// confused.
	this->decode_state = DecodeState::DECODING;

	return samples;
}

SoXAudioSource::DecodeResult SoXAudioSource::Decode()
{
	assert(this->context != nullptr);

	auto buf = reinterpret_cast<sox_sample_t *>(&this->buffer.front());

	// See BytesPerSample() for an explanation of this ChannelCount().
	auto sox_capacity = this->BufferSampleCapacity() * this->ChannelCount();
	size_t read = sox_read(this->context, buf, sox_capacity);

	DecodeVector decoded;

	if (read == 0) {
		this->decode_state = DecodeState::END_OF_FILE;
	} else {
		this->decode_state = DecodeState::DECODING;

		// Copy only the bit of the buffer occupied by decoded data
		// See BytesPerSample() for an explanation of the
		// ChannelCount() division.
		auto front = this->buffer.begin();
		auto read_bytes = (this->BytesPerSample() * read) /
		                  this->ChannelCount();
		decoded = DecodeVector(front, front + read_bytes);
	}

	return std::make_pair(this->decode_state, decoded);
}

/**
 * @return The sample format of the data returned by this decoder.
 */
SampleFormat SoXAudioSource::OutputSampleFormat() const
{
	// 'The function sox_read reads len samples in to buf using the format
	// handler specified by ft. All data read is converted to 32-bit signed
	// samples before being placed in to buf.'
	return SampleFormat::PACKED_SIGNED_INT_32;
}

void SoXAudioSource::Open(const std::string &path)
{
	this->Close();

	this->context = sox_open_read(path.c_str(), nullptr, nullptr, nullptr);
	if (this->context == nullptr) {
		throw FileError("couldn't open " + path);
	}
}

void SoXAudioSource::Close()
{
	if (this->context != nullptr) sox_close(this->context);
	this->context = nullptr;
}
