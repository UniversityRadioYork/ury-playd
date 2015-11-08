// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the DummyAudioSource class.
 * @see audio/audio_source.hpp
 * @see tests/dummy_audio_source.cpp
 */

#include <cstdint>

#include "../audio/audio.hpp"
#include "../audio/audio_source.hpp"

/// Dummy AudioSource, for testing PipeAudio.
class DummyAudioSource : public AudioSource
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
	 * Constructs a DummyAudioSource.
	 * @param path The path of the file this DummyAudioSource 'represents'.
	 */
	DummyAudioSource(const std::string &path) : AudioSource(path) {};
	AudioSource::DecodeResult Decode() override;
	std::uint8_t ChannelCount() const override;
	std::uint32_t SampleRate() const override;
	SampleFormat OutputSampleFormat() const override;
	std::uint64_t Seek(std::uint64_t position) override;
	std::uint64_t Length() const override;

	/// @return The path of the DummyAudioSource.
	const std::string &Path() const;

	/// The position of the AudioSource, in samples.
	std::uint64_t position;
};
