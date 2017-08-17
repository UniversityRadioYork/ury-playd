// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the DummyAudioSource class.
 * @see audio/audio_source.h
 * @see tests/dummy_audio_source.cpp
 */

#include <cstdint>

#include "../audio/audio_source.h"
#include "../audio/sample_format.h"

/// Dummy audio source, for testing audio pipelines.
class Dummy_audio_source : public Audio_source
{
public:
	/**
	 * Constructs a Dummy_audio_source.
	 * @param path The path of the file this Dummy_audio_source 'represents'.
	 */
	Dummy_audio_source(const std::string &path) : Audio_source(path) {};
	Audio_source::Decode_result Decode() override;
	std::uint8_t ChannelCount() const override;
	std::uint32_t SampleRate() const override;
	Sample_format OutputSampleFormat() const override;
	std::uint64_t Seek(std::uint64_t position) override;

	/// @return The path of the DummyAudioSource.
	const std::string &Path() const override;

	/// @return The length of the DummyAudioSource.
	std::uint64_t Length() const override;

	/// The position of the AudioSource, in samples.
	std::uint64_t position;

	/// If true, the audio source will claim it has run out.
	bool run_out = false;
};
