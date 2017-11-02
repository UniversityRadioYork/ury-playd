// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the Mp3AudioSource class.
 * @see audio/sources/mp3.cpp
 */

#ifndef PLAYD_AUDIO_SOURCE_MP3_H
#define PLAYD_AUDIO_SOURCE_MP3_H
#ifdef WITH_MP3

#include <cstdint>
#include <string>
#include <vector>

extern "C" {
#include <mpg123.h>
}

#include "../audio_source.h"
#include "../sample_format.h"

/// AudioSource for use on MP3 files.
class Mp3_audio_source : public Audio_source
{
public:
	/**
	 * Constructs an Mp3_audio_source.
	 * @param path The path to the file to load and decode using this
	 *   decoder.
	 */
	Mp3_audio_source(const std::string &path);

	/// Destructs an Mp3AudioSource.
	~Mp3_audio_source();

	Decode_result Decode() override;
	std::uint64_t Seek(std::uint64_t position) override;

	/// The length of the audio, in samples.
	std::uint64_t Length() const override;

	std::uint8_t ChannelCount() const override;
	std::uint32_t SampleRate() const override;
	Sample_format OutputSampleFormat() const override;

	/**
	 * Constructs an Mp3_audio_source and returns a unique pointer to it.
	 * @param path The path to the file to load and decode using this
	 *   decoder.
	 * @returns A unique pointer to a Mp3_audio_source.
	 */
	static std::unique_ptr<Mp3_audio_source> MakeUnique(const std::string &path);

private:
	// This value is somewhat arbitrary, but corresponds to the minimum
	// buffer size used by ffmpeg, so it's probably sensible.
	static constexpr size_t buffer_size = 16384;

	std::array<std::uint8_t, buffer_size> buffer; ///< The decoding buffer.

	/// Pointer to the mpg123 context associated with this source.
	mpg123_handle *context;

	/**
	 * Adds a format for the given sample rate to mpg123.
	 * @param rate The sample rate to add.
	 */
	void AddFormat(long rate);
};

#endif // WITH_MP3
#endif // PLAYD_AUDIO_SOURCE_MP3_H
