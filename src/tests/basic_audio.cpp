// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for Basic_audio.
 */

#include <chrono>
#include <sstream>

#include "../audio/audio.h"
#include "catch.hpp"
#include "dummy_audio_sink.h"
#include "dummy_audio_source.h"
#include "dummy_response_sink.h"

namespace Playd::Tests
{
SCENARIO (
        "BasicAudio can be constructed with a DummyAudioSink and "
        "DummyAudioSource",
        "[basic-audio]") {
	GIVEN ("unique pointers to a sink and source") {
		auto src = std::make_unique<DummyAudioSource>("test");
		auto sink = std::make_unique<DummyAudioSink>(*src, 0);

		WHEN ("a BasicAudio is created") {
			THEN ("no exceptions are thrown and queries return the "
			      "expected initial values") {
				Audio::BasicAudio pa(std::move(src), std::move(sink));
				REQUIRE(pa.Position() == std::chrono::microseconds{0});
			}
		}
	}
}

SCENARIO ("BasicAudio responds to getters with valid responses", "[basic-audio]") {
	GIVEN ("a valid BasicAudio and dummy components") {
		auto src = std::make_unique<DummyAudioSource>("test");
		auto snk = std::make_unique<DummyAudioSink>(*src, 0);
		Audio::BasicAudio pa(std::move(src), std::move(snk));

		WHEN ("the state is requested") {
			AND_WHEN("the state is 'playing'")
			{
				pa.SetPlaying(true);
				THEN ("the state is reported as 'playing'") {
					REQUIRE(pa.CurrentState() == Audio::Audio::State::PLAYING);
				}
			}

			AND_WHEN("the state is 'stopped'")
			{
				pa.SetPlaying(false);
				THEN ("the state is reported as 'stopped'") {
					REQUIRE(pa.CurrentState() == Audio::Audio::State::STOPPED);
				}
			}
		}

		WHEN ("the position is requested") {
			AND_WHEN("the position is zero")
			{
				pa.SetPosition(std::chrono::microseconds{0});
				THEN ("the position is reported as 0") {
					REQUIRE(pa.Position() == std::chrono::microseconds{0});
				}
			}

			AND_WHEN("the position is not zero")
			{
				pa.SetPosition(std::chrono::microseconds{8675309});
				THEN ("the position is reported as (rounded-off position)") {
					// The DummyAudioSource has 44100Hz
					// sample rate.  The following
					// calculation is what a round-trip to
					// and from samples should cause.
					// This *won't* be 8675309!
					std::chrono::microseconds expected{
					        (((8675309ULL * 44100) / 1000000) * 1000000) / 44100};
					REQUIRE(pa.Position() == expected);
				}
			}
		}
	}
}

SCENARIO ("BasicAudio propagates source emptiness correctly", "[basic-audio]") {
	GIVEN ("a valid set of dummy components") {
		auto src = std::make_unique<DummyAudioSource>("test");
		auto snk = std::make_unique<DummyAudioSink>(*src, 0);
		snk->state = Audio::Audio::State::STOPPED;

		// We build the BasicAudio later, because it moves src and snk
		// out of easy modification range.

		WHEN ("the DummyResponseSource is reporting end of file") {
			src->run_out = true;

			Audio::BasicAudio pa(std::move(src), std::move(snk));

			THEN ("Update() returns AT_END") {
				REQUIRE(pa.Update() == Audio::Audio::State::AT_END);
			}
		}
	}
}

SCENARIO ("BasicAudio acquires state from the sink correctly", "[basic-audio]") {
	GIVEN ("a valid set of dummy components") {
		auto src = std::make_unique<DummyAudioSource>("test");
		auto snk = std::make_unique<DummyAudioSink>(*src, 0);

		// We build the BasicAudio later, because it moves src and snk
		// out of easy modification range.

		WHEN ("the response sink's state is 'stopped'") {
			snk->state = Audio::Audio::State::STOPPED;

			Audio::BasicAudio pa(std::move(src), std::move(snk));

			THEN ("Update() returns 'stopped'") {
				REQUIRE(pa.Update() == Audio::Audio::State::STOPPED);
			}
		}

		WHEN ("the response sink state is 'playing'") {
			snk->state = Audio::Audio::State::PLAYING;

			Audio::BasicAudio pa(std::move(src), std::move(snk));

			THEN ("Update() returns 'playing'") {
				REQUIRE(pa.Update() == Audio::Audio::State::PLAYING);
			}
		}

		WHEN ("the DummyResponseSink's state is 'at_end'") {
			snk->state = Audio::Audio::State::AT_END;

			Audio::BasicAudio pa(std::move(src), std::move(snk));

			THEN ("Update() returns 'at_end'") {
				REQUIRE(pa.Update() == Audio::Audio::State::AT_END);
			}
		}
	}
}

} // namespace Playd::Tests
