// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for PipeAudio.
 */

#include <chrono>
#include <sstream>

#include "catch.hpp"

#include "../audio/audio.hpp"
#include "dummy_response_sink.hpp"
#include "dummy_audio_source.hpp"
#include "dummy_audio_sink.hpp"

using namespace std::chrono;


SCENARIO("PipeAudio can be constructed with a DummyAudioSink and DummyAudioSource", "[pipe-audio]") {
	GIVEN("unique pointers to a sink and source") {
		auto src = std::make_unique<DummyAudioSource>("test");
		auto sink = std::make_unique<DummyAudioSink>(*src, 0);

		WHEN("a PipeAudio is created") {
			THEN("no exceptions are thrown and queries return the expected initial values") {
				PipeAudio pa(std::move(src), std::move(sink));
				REQUIRE(pa.Position().count() == 0ULL);
			}
		}
	}
}

SCENARIO("PipeAudio responds to getters with valid responses", "[pipe-audio]") {
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
				pa.SetPosition(microseconds {0});
				THEN("the position is reported as 0") {
					REQUIRE(pa.Position().count() == 0ULL);
				}
			}

			AND_WHEN("the position is not zero") {
				pa.SetPosition(microseconds {8675309});
				THEN("the position is reported as (rounded-off position)") {
					// The DummyAudioSource has 44100Hz
					// sample rate.  The following
					// calculation is what a round-trip to
					// and from samples should cause.
					// This *won't* be 8675309!
					auto expected = (((8675309ULL * 44100) / 1000000) * 1000000) / 44100;
					REQUIRE(pa.Position().count() == expected);
				}
			}
		}

	}
}

SCENARIO("PipeAudio propagates source emptiness correctly", "[pipe-audio]") {
	GIVEN("a valid set of dummy components") {
		auto src = std::make_unique<DummyAudioSource>("test");
		auto snk = std::make_unique<DummyAudioSink>(*src, 0);
		snk->state = Audio::State::STOPPED;

		// We build the PipeAudio later, because it moves src and snk
		// out of easy modification range.

		WHEN("the DummyResponseSource is reporting end of file") {
			src->run_out = true;

			PipeAudio pa(std::move(src), std::move(snk));

			THEN("Update() returns AT_END") {
				REQUIRE(pa.Update() == Audio::State::AT_END);
			}
		}
	}
}

SCENARIO("PipeAudio acquires state from the sink correctly", "[pipe-audio]") {
	GIVEN("a valid set of dummy components") {
		auto src = std::make_unique<DummyAudioSource>("test");
		auto snk = std::make_unique<DummyAudioSink>(*src, 0);

		// We build the PipeAudio later, because it moves src and snk
		// out of easy modification range.

		WHEN("the DummyResponseSink's state is STOPPED") {
			snk->state = Audio::State::STOPPED;

			PipeAudio pa(std::move(src), std::move(snk));

			THEN("Update() returns STOPPED") {
				REQUIRE(pa.Update() == Audio::State::STOPPED);
			}
		}

		WHEN("the DummyResponseSink's state is PLAYING") {
			snk->state = Audio::State::PLAYING;

			PipeAudio pa(std::move(src), std::move(snk));

			THEN("Update() returns PLAYING") {
				REQUIRE(pa.Update() == Audio::State::PLAYING);
			}
		}

		WHEN("the DummyResponseSink's state is AT_END") {
			snk->state = Audio::State::AT_END;

			PipeAudio pa(std::move(src), std::move(snk));

			THEN("Update() returns AT_END") {
				REQUIRE(pa.Update() == Audio::State::AT_END);
			}
		}
	}
}
