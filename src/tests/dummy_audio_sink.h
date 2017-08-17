// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the DummyAudioSink class.
 * @see audio/audio_sink.h
 * @see tests/dummy_audio_sink.cpp
 */

#include <cstdint>

#include "../audio/audio_source.h"
#include "../audio/audio_sink.h"

/// Dummy audio sink, for testing audio pipelines.
class Dummy_audio_sink : public Audio_sink
{
public:
	/**
	 * Constructs a Dummy_audio_sink.
	 * @param source Ignored.
	 * @param device_id Ignored.
	 */
	Dummy_audio_sink(const Audio_source &, int) {};

	void Start() override;
	void Stop() override;
	Audio_sink::State CurrentState() override;
	std::uint64_t Position() override;
	void SetPosition(uint64_t samples) override;
	void SourceOut() override;
	size_t Transfer(const gsl::span<const uint8_t> src) override;

	/// The current state of the DummyAudioSink.
	Audio_sink::State state = Audio_sink::State::stopped;

	/// The current position, in samples.
	uint64_t position = 0;
};
