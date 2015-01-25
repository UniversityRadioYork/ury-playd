// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the FlacAudioSource class.
 * @see audio/sources/flac.cpp
 */

#ifndef PLAYD_AUDIO_SOURCE_FLAC_HPP
#define PLAYD_AUDIO_SOURCE_FLAC_HPP
#ifdef WITH_FLAC

#include <cstdint>
#include <string>
#include <vector>

#include <FLAC++/decoder.h>

#include "../audio_source.hpp"
#include "../sample_formats.hpp"

/// AudioSource for use on FLAC files.
class FlacAudioSource : public AudioSource, protected FLAC::Decoder::File
{
public:
	/**
	 * Helper function for creating uniquely pointed-to FlacAudioSources.
	 * @param path The path to the file to load and decode using this
	 *   decoder.
	 * @return A unique pointer to an AudioSource for the given path.
	 */
	static std::unique_ptr<AudioSource> Build(const std::string &path);

	/**
	 * Constructs a FlacAudioSource.
	 * @param path The path to the file to load and decode using this
	 *   decoder.
	 */
	FlacAudioSource(const std::string &path);

	/// Destructs an Mp3AudioSource.
	~FlacAudioSource();

	DecodeResult Decode() override;
	std::uint64_t Seek(std::uint64_t position) override;

	std::uint8_t ChannelCount() const override;
	std::uint32_t SampleRate() const override;
	SampleFormat OutputSampleFormat() const override;

	/**
	 * Converts a FLAC init error to an error message.
	 * @param err The error number.
	 * @return The corresponding error message.
	 */
	/* static */ std::string InitStrError(int err);

protected:
	FLAC__StreamDecoderWriteStatus write_callback(
	        const FLAC__Frame *frame,
	        const FLAC__int32 *const buffer[]) override;
	void error_callback(FLAC__StreamDecoderErrorStatus status) override;

private:
	std::vector<int32_t> buffer; ///< The decoding buffer.
};

#endif // WITH_FLAC
#endif // PLAYD_AUDIO_SOURCE_FLAC_HPP
