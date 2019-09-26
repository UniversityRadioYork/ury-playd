// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the DummyAudioSink class.
 * @see audio/audio_sink.h
 * @see tests/dummy_audio_sink.cpp
 */

#include <cstdint>

#include "../audio/sink.h"
#include "../audio/source.h"

namespace playd::tests
{
/// Dummy audio sink, for testing audio pipelines.
class Dummy_audio_sink : public audio::Sink
{
public:
	/**
	 * Constructs a Dummy_audio_sink.
	 * @param source Ignored.
	 * @param device_id Ignored.
	 */
	Dummy_audio_sink(const audio::Source &, int){};

	void Start() override;

	void Stop() override;

	audio::Sink::State CurrentState() override;

	std::uint64_t Position() override;

	void SetPosition(uint64_t samples) override;

	void SourceOut() override;

	size_t Transfer(gsl::span<const std::byte> src) override;

	/// The current state of the sink.
	audio::Sink::State state = audio::Sink::State::stopped;

	/// The current position, in samples.
	uint64_t position = 0;
};

} // namespace playd::tests
