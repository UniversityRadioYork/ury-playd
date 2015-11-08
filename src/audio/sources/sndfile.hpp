// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the SndfileAudioSource class.
 * @see audio/sources/sndfile.cpp
 */

#ifndef PLAYD_AUDIO_SOURCE_SNDFILE_HPP
#define PLAYD_AUDIO_SOURCE_SNDFILE_HPP
#ifdef WITH_SNDFILE

#include <cstdint>
#include <string>
#include <vector>

#include <sndfile.h>

#include "../audio_source.hpp"
#include "../sample_formats.hpp"

/// AudioSource for use on files supported by libsndfile.
class SndfileAudioSource : public AudioSource
{
public:
	/**
	 * Helper function for creating uniquely pointed-to
	 * SndfileAudioSources.
	 * @param path The path to the file to load and decode using this
	 *   decoder.
	 * @return A unique pointer to an AudioSource for the given path.
	 */
	static std::unique_ptr<AudioSource> Build(const std::string &path);

	/**
	 * Constructs a SndfileAudioSource.
	 * @param path The path to the file to load and decode using this
	 *   decoder.
	 * @see http://www.mega-nerd.com/libsndfile/api.html#open
	 */
	SndfileAudioSource(const std::string &path);

	/// Destructs an Mp3AudioSource.
	~SndfileAudioSource();

	DecodeResult Decode() override;
	std::uint64_t Seek(std::uint64_t position) override;
	std::uint64_t Length() const override;

	std::uint8_t ChannelCount() const override;
	std::uint32_t SampleRate() const override;
	SampleFormat OutputSampleFormat() const override;

private:
	SF_INFO info;  ///< The libsndfile info structure.
	SNDFILE *file; ///< The libsndfile file structure.

	std::vector<int32_t> buffer; ///< The decoding buffer.
};

#endif // WITH_SNDFILE
#endif // PLAYD_AUDIO_SOURCE_SNDFILE_HPP
