// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the SndfileAudioSource class.
 * @see audio/sources/sndfile.cpp
 */

#ifndef PLAYD_AUDIO_SOURCES_SNDFILE_H
#define PLAYD_AUDIO_SOURCES_SNDFILE_H
#ifdef WITH_SNDFILE

#include <sndfile.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../sample_format.h"
#include "../source.h"

namespace Playd::Audio
{
/// Audio source for use on files supported by libsndfile.
class SndfileSource : public Source
{
public:
	/**
	 * Constructs a Sndfile_audio_source.
	 * @param path The path to the file to load and decode using this
	 *   decoder.
	 * @see http://www.mega-nerd.com/libsndfile/api.html#open
	 */
	explicit SndfileSource(std::string_view path);

	/// Destructs a Sndfile_audio_source.
	~SndfileSource();

	DecodeResult Decode() override;

	std::uint64_t Seek(std::uint64_t position) override;

	std::uint64_t Length() const override;

	std::uint8_t ChannelCount() const override;

	std::uint32_t SampleRate() const override;

	SampleFormat OutputSampleFormat() const override;

	/**
	 * Constructs an Sndfile_audio_source and returns a unique pointer to it.
	 * @param path The path to the file to load and decode using this
	 *   decoder.
	 * @returns A unique pointer to a Sndfile_audio_source.
	 */
	static std::unique_ptr<SndfileSource> MakeUnique(std::string_view path);

private:
	SF_INFO info;  ///< The libsndfile info structure.
	SNDFILE *file; ///< The libsndfile file structure.

	std::vector<int32_t> buffer; ///< The decoding buffer.
};

} // namespace Playd::Audio

#endif // WITH_SNDFILE
#endif // PLAYD_AUDIO_SOURCE_SNDFILE_H
