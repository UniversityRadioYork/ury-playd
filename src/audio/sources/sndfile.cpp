// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the SndfileAudio_source class.
 * @see audio/audio_source.h
 */

#include "sndfile.h"

#include <sndfile.h>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

#include "../../errors.h"
#include "../../messages.h"
#include "../sample_format.h"
#include "../source.h"

namespace Playd::Audio
{
SndfileSource::SndfileSource(std::string_view path) : Source{path}, file{nullptr}, buffer{}
{
	this->info.format = 0;

	this->file = sf_open(this->path.c_str(), SFM_READ, &this->info);
	if (this->file == nullptr) {
		throw FileError("sndfile: can't open " + this->path + ": " + sf_strerror(nullptr));
	}

	// Reserve enough space for a given number of frames.
	// (Frames being what libsndfile calls multi-channel samples, for some
	// reason; incidentally, it calls mono-samples items.)
	assert(0 < this->info.channels);
	this->buffer.clear();
	this->buffer.insert(this->buffer.begin(), 4096 * this->info.channels, 0);
}

SndfileSource::~SndfileSource()
{
	if (this->file != nullptr) sf_close(this->file);
}

std::uint8_t SndfileSource::ChannelCount() const
{
	assert(0 < this->info.channels);
	return static_cast<std::uint8_t>(this->info.channels);
}

std::uint32_t SndfileSource::SampleRate() const
{
	assert(0 < this->info.samplerate);
	// INT32_MAX isn't a typo; if we compare against UINT32_MAX, we'll
	// set off sign-compare errors, and the sample rate shouldn't be above
	// INT32_MAX anyroad.
	assert(this->info.samplerate <= INT32_MAX);
	return static_cast<std::uint32_t>(this->info.samplerate);
}

std::uint64_t SndfileSource::Seek(std::uint64_t in_samples)
{
	// Have we tried to seek past the end of the file?
	if (auto clen = static_cast<unsigned long>(this->info.frames); clen < in_samples) {
		Debug() << "sndfile: seek at" << in_samples << "past EOF at" << clen << std::endl;
		throw SeekError(MSG_SEEK_FAIL);
	}

	auto out_samples = sf_seek(this->file, in_samples, SEEK_SET);
	if (out_samples == -1) {
		Debug() << "sndfile: seek failed" << std::endl;
		throw SeekError(MSG_SEEK_FAIL);
	}

	return out_samples;
}

std::uint64_t SndfileSource::Length() const
{
	return (this->info.frames);
}

SndfileSource::DecodeResult SndfileSource::Decode()
{
	auto read = sf_read_int(this->file, &*this->buffer.begin(), this->buffer.size());

	// Have we hit the end of the file?
	if (read == 0) {
		return std::make_pair(DecodeState::END_OF_FILE, DecodeVector());
	}

	// Else, we're good to go (hopefully).
	//
	// The buffer on our side is addressed as ints (32-bit), because this is
	// easier for sndfile.  However, the DecodeVector is addressed as bytes
	// (8-bit) as the sample length could vary between files and decoders
	// (from 8-bit up to 32-bit, and maybe even 32-bit float)!
	//
	// So, we reinterpret the decoded bits as a vector of bytes, which is
	// relatively safe--they'll be interpreted by the Sink in the
	// exact same way once we tell it how long the samples really are.
	auto *begin = reinterpret_cast<std::byte *>(&*this->buffer.begin());

	// The end is 'read' 32-bit items--read*4 bytes--after.
	auto *end = begin + (read * 4);

	return std::make_pair(DecodeState::DECODING, DecodeVector{begin, end});
}

SampleFormat SndfileSource::OutputSampleFormat() const
{
	// Because we use int-sized reads, assume this corresponds to 32-bit
	// signed int.
	// Really, we shouldn't assume int is 32-bit!
	static_assert(sizeof(int) == 4, "sndfile outputs int, which we need to be 4 bytes");
	return SampleFormat::SINT32;
}

std::unique_ptr<SndfileSource> SndfileSource::MakeUnique(std::string_view path)
{
	return std::make_unique<SndfileSource, std::string_view>(std::move(path));
}

} // namespace Playd::Audio
