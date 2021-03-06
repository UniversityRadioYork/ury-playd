// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the Mp3AudioSource class.
 * @see audio/sources/mp3.cpp
 */

#ifndef PLAYD_AUDIO_SOURCES_MP3_H
#define PLAYD_AUDIO_SOURCES_MP3_H
#ifdef WITH_MP3

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

extern "C" {
#include <mpg123.h>
}

#include "../sample_format.h"
#include "../source.h"

namespace Playd::Audio
{
/// Audio source for use on MP3 files.
class MP3Source : public Source
{
public:
	/**
	 * Constructs an Mp3_audio_source.
	 * @param path The path to the file to load and decode using this
	 *   decoder.
	 */
	explicit MP3Source(std::string_view path);

	/// Destructs an Mp3AudioSource.
	~MP3Source();

	DecodeResult Decode() override;

	std::uint64_t Seek(std::uint64_t position) override;

	/// The length of the audio, in samples.
	std::uint64_t Length() const override;

	std::uint8_t ChannelCount() const override;

	std::uint32_t SampleRate() const override;

	SampleFormat OutputSampleFormat() const override;

	/**
	 * Constructs an Mp3_audio_source and returns a unique pointer to it.
	 * @param path The path to the file to load and decode using this
	 *   decoder.
	 * @returns A unique pointer to a Mp3_audio_source.
	 */
	static std::unique_ptr<MP3Source> MakeUnique(std::string_view path);

private:
	// This value is somewhat arbitrary, but corresponds to the minimum
	// buffer size used by ffmpeg, so it's probably sensible.
	static constexpr size_t buffer_size = 16384;

	std::array<std::byte, buffer_size> buffer; ///< The decoding buffer.

	/// Pointer to the mpg123 context associated with this source.
	mpg123_handle *context;

	/**
	 * @returns A span containing the available sample rates.
	 */
	static gsl::span<const long> AvailableRates();

	/**
	 * Adds a format for the given sample rate to mpg123.
	 * @param rate The sample rate to add.
	 */
	void AddFormat(long rate);
};

} // namespace Playd::Audio

#endif // WITH_MP3
#endif // PLAYD_AUDIO_SOURCES_MP3_H
