// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the DummyAudioSource class.
 * @see audio/audio_source.h
 * @see tests/dummy_audio_source.cpp
 */

#include <cstdint>

#include "../audio/sample_format.h"
#include "../audio/source.h"

namespace playd::tests
{
/// Dummy audio source, for testing audio pipelines.
class Dummy_audio_source : public audio::Source
{
public:
	/**
	 * Constructs a Dummy_audio_source.
	 * @param path The path of the file this Dummy_audio_source 'represents'.
	 */
	Dummy_audio_source(std::string_view path) : audio::Source(path){};

	audio::Source::Decode_result Decode() override;

	std::uint8_t ChannelCount() const override;

	std::uint32_t SampleRate() const override;

	audio::Sample_format OutputSampleFormat() const override;

	std::uint64_t Seek(std::uint64_t position) override;

	/// @return The path of the DummyAudioSource.
	std::string_view Path() const override;

	/// @return The length of the DummyAudioSource.
	std::uint64_t Length() const override;

	/// The position of the AudioSource, in samples.
	std::uint64_t position;

	/// If true, the audio source will claim it has run out.
	bool run_out = false;
};

} // namespace playd::tests
