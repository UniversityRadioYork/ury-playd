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
#include "../audio_source.h"
#include "../sample_format.h"

Sndfile_audio_source::Sndfile_audio_source(std::string_view path)
    : Audio_source{path}, file{nullptr}, buffer{}
{
	this->info.format = 0;

	this->file = sf_open(this->path.c_str(), SFM_READ, &this->info);
	if (this->file == nullptr) {
		throw File_error("sndfile: can't open " + this->path + ": " +
		                 sf_strerror(nullptr));
	}

	// Reserve enough space for a given number of frames.
	// (Frames being what libsndfile calls multi-channel samples, for some
	// reason; incidentally, it calls mono-samples items.)
	assert(0 < this->info.channels);
	this->buffer.clear();
	this->buffer.insert(this->buffer.begin(), 4096 * this->info.channels, 0);
}

Sndfile_audio_source::~Sndfile_audio_source()
{
	if (this->file != nullptr) sf_close(this->file);
}

std::uint8_t Sndfile_audio_source::ChannelCount() const
{
	assert(0 < this->info.channels);
	return static_cast<std::uint8_t>(this->info.channels);
}

std::uint32_t Sndfile_audio_source::SampleRate() const
{
	assert(0 < this->info.samplerate);
	// INT32_MAX isn't a typo; if we compare against UINT32_MAX, we'll
	// set off sign-compare errors, and the sample rate shouldn't be above
	// INT32_MAX anyroad.
	assert(this->info.samplerate <= INT32_MAX);
	return static_cast<std::uint32_t>(this->info.samplerate);
}

std::uint64_t Sndfile_audio_source::Seek(std::uint64_t in_samples)
{
	// Have we tried to seek past the end of the file?
	if (auto clen = static_cast<unsigned long>(this->info.frames);
	    clen < in_samples) {
		Debug() << "sndfile: seek at" << in_samples << "past EOF at"
		        << clen << std::endl;
		throw Seek_error(MSG_SEEK_FAIL);
	}

	auto out_samples = sf_seek(this->file, in_samples, SEEK_SET);
	if (out_samples == -1) {
		Debug() << "sndfile: seek failed" << std::endl;
		throw Seek_error(MSG_SEEK_FAIL);
	}

	return out_samples;
}

std::uint64_t Sndfile_audio_source::Length() const
{
	return (this->info.frames);
}

Sndfile_audio_source::Decode_result Sndfile_audio_source::Decode()
{
	auto read = sf_read_int(this->file, &*this->buffer.begin(),
	                        this->buffer.size());

	// Have we hit the end of the file?
	if (read == 0) {
		return std::make_pair(Decode_state::eof, Decode_vector());
	}

	// Else, we're good to go (hopefully).
	//
	// The buffer on our side is addressed as ints (32-bit), because this is
	// easier for sndfile.  However, the DecodeVector is addressed as bytes
	// (8-bit) as the sample length could vary between files and decoders
	// (from 8-bit up to 32-bit, and maybe even 32-bit float)!
	//
	// So, we reinterpret the decoded bits as a vector of bytes, which is
	// relatively safe--they'll be interpreted by the Audio_sink in the
	// exact same way once we tell it how long the samples really are.
	uint8_t *begin = reinterpret_cast<uint8_t *>(&*this->buffer.begin());

	// The end is 'read' 32-bit items--read*4 bytes--after.
	uint8_t *end = begin + (read * 4);

	return std::make_pair(Decode_state::decoding, Decode_vector(begin, end));
}

Sample_format Sndfile_audio_source::OutputSampleFormat() const
{
	// Because we use int-sized reads, assume this corresponds to 32-bit
	// signed int.
	// Really, we shouldn't assume int is 32-bit!
	static_assert(sizeof(int) == 4,
	              "sndfile outputs int, which we need to be 4 bytes");
	return Sample_format::sint32;
}

std::unique_ptr<Sndfile_audio_source> Sndfile_audio_source::MakeUnique(
        std::string_view path)
{
	return std::make_unique<Sndfile_audio_source, std::string_view>(
	        std::move(path));
}
