// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for PipeAudio.
 */

#include "catch.hpp"

#include <sstream>

#include "../audio/audio.hpp"
#include "dummy_response_sink.hpp"
#include "dummy_audio_source.hpp"
#include "dummy_audio_sink.hpp"

SCENARIO("PipeAudio can be constructed with a DummyAudioSink and DummyAudioSource", "[pipe-audio]") {
	GIVEN("unique pointers to a sink and source") {
		auto src = std::make_unique<DummyAudioSource>("test");
		auto sink = std::make_unique<DummyAudioSink>(*src, 0);

		WHEN("a PipeAudio is created") {
			THEN("no exceptions are thrown and queries return the expected initial values") {
				PipeAudio pa(std::move(src), std::move(sink));
				REQUIRE(pa.Position() == 0);
			}
		}
	}
}

SCENARIO("PipeAudio responds to Emit calls with valid responses", "[pipe-audio]") {
	GIVEN("a valid PipeAudio and DummyResponseSink") {
		auto src = std::make_unique<DummyAudioSource>("test");
		auto snk = std::make_unique<DummyAudioSink>(*src, 0);
		PipeAudio pa(std::move(src), std::move(snk));

		WHEN("the state is requested") {
			AND_WHEN("the state is Playing") {
				pa.SetPlaying(true);
				THEN("the state is reported as Playing") {
					REQUIRE(pa.CurrentState() == Audio::State::PLAYING);
				}
			}

			AND_WHEN("the state is Stopped") {
				pa.SetPlaying(false);
				THEN("the state is reported as Stopped") {
					REQUIRE(pa.CurrentState() == Audio::State::STOPPED);
				}
			}
		}

		WHEN("the position is requested") {
			AND_WHEN("the position is zero") {
				pa.SetPosition(0);
				THEN("the position is reported as 0") {
					REQUIRE(pa.Position() == 0);
				}
			}

			AND_WHEN("the position is not zero") {
				pa.SetPosition(8675309);
				THEN("the position is reported as (rounded-off position)") {
					// The DummyAudioSource has 44100Hz
					// sample rate.  The following
					// calculation is what a round-trip to
					// and from samples should cause.
					// This *won't* be 8675309!
					auto expected = (((8675309L * 44100) / 1000000) * 1000000) / 44100;
					REQUIRE(pa.Position() == expected);
				}
			}
		}

	}
}
