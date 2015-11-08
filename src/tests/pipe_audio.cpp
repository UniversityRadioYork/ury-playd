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
		auto src = std::unique_ptr<AudioSource>(new DummyAudioSource("test"));
		auto sink = std::unique_ptr<AudioSink>(new DummyAudioSink());

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
		PipeAudio pa(std::unique_ptr<AudioSource>(new DummyAudioSource("test")),
		             std::unique_ptr<AudioSink>(new DummyAudioSink()));

		WHEN("the STATE is requested") {
			AND_WHEN("the state is Playing") {
				pa.SetPlaying(true);
				THEN("the /control/state resource is set to Playing") {
					std::string type, value;
					std::tie(type, value) = pa.Emit("/player/state/current");
					REQUIRE(type == "string");
					REQUIRE(value == "playing");
				}
			}

			AND_WHEN("the state is Stopped") {
				pa.SetPlaying(false);
				THEN("the /control/state resource is set to Stopped") {
					std::string type, value;
					std::tie(type, value) = pa.Emit("/player/state/current");
					REQUIRE(type == "string");
					REQUIRE(value == "stopped");
				}
			}
		}

		WHEN("the TIME is requested") {
			AND_WHEN("the position is zero") {
				pa.Seek(0);
				THEN("the /player/time/elapsed resource is set to 0") {
					std::string type, value;
					std::tie(type, value) = pa.Emit("/player/time/elapsed");
					REQUIRE(type == "integer");
					REQUIRE(value == "0");
				}
			}

			AND_WHEN("the position is not zero") {
				pa.Seek(8675309);
				THEN("the /player/time/elapsed resource is set to (rounded-off position)") {
					// The DummyAudioSource has 44100Hz
					// sample rate.  The following
					// calculation is what a round-trip to
					// and from samples should cause.
					// This *won't* be 8675309!
					auto expected = (((8675309L * 44100) / 1000000) * 1000000) / 44100;
					std::string type, value;
					std::tie(type, value) = pa.Emit("/player/time/elapsed");
					REQUIRE(type == "integer");
					REQUIRE(value == std::to_string(expected));
				}
			}
		}

	}
}
