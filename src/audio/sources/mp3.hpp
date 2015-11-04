// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the Mp3AudioSource class.
 * @see audio/sources/mp3.cpp
 */

#ifndef PLAYD_AUDIO_SOURCE_MP3_HPP
#define PLAYD_AUDIO_SOURCE_MP3_HPP
#ifdef WITH_MP3

#include <cstdint>
#include <string>
#include <vector>

extern "C" {
// MPG123 seems to assume the whole world has ssize_t defined.
// Naturally, MSVC doesn't.
#ifdef _MSC_VER
typedef ptrdiff_t ssize_t;
#endif
#include <mpg123.h>
}

#include "../audio_source.hpp"
#include "../sample_formats.hpp"

/// AudioSource for use on MP3 files.
class Mp3AudioSource : public AudioSource
{
public:
	/**
	 * Helper function for creating uniquely pointed-to Mp3AudioSources.
	 * @param path The path to the file to load and decode using this
	 *   decoder.
	 * @return A unique pointer to a Mp3AudioSource for the given path.
	 */
	static std::unique_ptr<AudioSource> Build(const std::string &path);

	/**
	 * Constructs an Mp3AudioSource.
	 * @param path The path to the file to load and decode using this
	 *   decoder.
	 */
	Mp3AudioSource(const std::string &path);

	/// Destructs an Mp3AudioSource.
	~Mp3AudioSource();

	DecodeResult Decode() override;
	std::uint64_t Seek(std::uint64_t position) override;
	std::uint64_t Length() override;

	std::uint8_t ChannelCount() const override;
	std::uint32_t SampleRate() const override;
	SampleFormat OutputSampleFormat() const override;

private:
	/// The size of the internal decoding buffer, in bytes.
	static const size_t BUFFER_SIZE;

	std::vector<std::uint8_t> buffer; ///< The decoding buffer.

	/// Pointer to the mpg123 context associated with this source.
	mpg123_handle *context;

	/**
	 * Adds a format for the given sample rate to mpg123.
	 * @param rate The sample rate to add.
	 */
	void AddFormat(long rate);
};

#endif // WITH_MP3
#endif // PLAYD_AUDIO_SOURCE_MP3_HPP
