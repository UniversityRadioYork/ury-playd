// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the DummyAudioSink class.
 * @see audio/audio_sink.hpp
 * @see tests/dummy_audio_sink.cpp
 */

#include <cstdint>

#include "../audio/audio.hpp"
#include "../audio/audio_sink.hpp"

/// Dummy AudioSink, for testing PipeAudio.
class DummyAudioSink : public AudioSink
{
public:
	/**
	 * Constructs a DummyAudioSink.
	 * @param source Ignored.
	 * @param device_id Ignored.
	 */
	DummyAudioSink(const AudioSource &, int) {};

	void Start() override;
	void Stop() override;
	Audio::State State() override;
	std::uint64_t Position() override;
	void SetPosition(std::uint64_t samples) override;
	void SourceOut() override;
	void Transfer(AudioSink::TransferIterator &start, const AudioSink::TransferIterator &end) override;

	/// The current state of the DummyAudioSink.
	Audio::State state = Audio::State::STOPPED;

	/// The current position, in samples.
	uint64_t position = 0;
};
