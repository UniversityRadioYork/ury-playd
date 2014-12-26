// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the SndfileAudioSource class.
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

#include <sndfile.h>

#include "../../errors.hpp"
#include "../../messages.h"
#include "../sample_formats.hpp"
#include "../audio_source.hpp"
#include "sndfile.hpp"

SndfileAudioSource::SndfileAudioSource(const std::string &path)
    : AudioSource(path), file(nullptr), buffer()
{
	this->info.format = 0;

	// See http://www.mega-nerd.com/libsndfile/api.html#open
	// for more details.
	this->file = sf_open(path.c_str(), SFM_READ, &this->info);
	if (this->file == nullptr) {
		throw FileError("sndfile: can't open " + path + ": " +
		                sf_strerror(nullptr));
	}

	// Reserve enough space for a given number of frames.
	// (Frames being what libsndfile calls multi-channel samples, for some
	// reason; incidentally, it calls mono-samples items.)
	assert(0 < this->info.channels);
	this->buffer.clear();
	this->buffer.insert(this->buffer.begin(), 4096 * this->info.channels, 0);
}

SndfileAudioSource::~SndfileAudioSource()
{
	if (this->file != nullptr) sf_close(this->file);
}

std::uint8_t SndfileAudioSource::ChannelCount() const
{
	assert(0 < this->info.channels);
	return static_cast<std::uint8_t>(this->info.channels);
}

double SndfileAudioSource::SampleRate() const
{
	assert(0 < this->info.samplerate);
	return static_cast<double>(this->info.samplerate);
}

std::uint64_t SndfileAudioSource::Seek(std::uint64_t position)
{
	auto samples = this->SamplesFromMicros(position);

	// Have we tried to seek past the end of the file?
	auto clen = static_cast<unsigned long>(this->info.frames);
	if (clen < samples) {
		Debug() << "sndfile: seek at" << samples << "past EOF at"
		        << clen << std::endl;
		Debug() << "sndfile: requested position micros:" << position
		        << std::endl;
		throw SeekError(MSG_SEEK_FAIL);
	}

	auto new_samples = sf_seek(this->file, samples, SEEK_SET);
	if (new_samples == -1) {
		Debug() << "sndfile: seek failed" << std::endl;
		throw SeekError(MSG_SEEK_FAIL);
	}

	return new_samples;
}

SndfileAudioSource::DecodeResult SndfileAudioSource::Decode()
{
	auto read = sf_read_int(this->file, &*this->buffer.begin(),
	                        this->buffer.size());

	// Have we hit the end of the file?
	if (read == 0) {
		return std::make_pair(DecodeState::END_OF_FILE, DecodeVector());
	}

	// Else, we're good to go (hopefully).
	uint8_t *begin = reinterpret_cast<uint8_t *>(&*this->buffer.begin());

	// The end is 'read' 32-bit items--read*4 bytes--after.
	uint8_t *end = begin + (read * 4);

	return std::make_pair(DecodeState::DECODING, DecodeVector(begin, end));
}

SampleFormat SndfileAudioSource::OutputSampleFormat() const
{
	// Because we use int-sized reads, assume this corresponds to 32-bit
	// signed int.
	// Really, we shouldn't assume int is 32-bit!
	static_assert(sizeof(int) == 4,
	              "sndfile outputs int, which we need to be 4 bytes");
	return SampleFormat::PACKED_SIGNED_INT_32;
}
