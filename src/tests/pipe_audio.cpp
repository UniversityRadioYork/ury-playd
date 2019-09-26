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

namespace playd::tests
{
SCENARIO (
        "Basic_audio can be constructed with a DummyAudio_sink and "
        "DummyAudio_source",
        "[pipe-audio]") {
	GIVEN ("unique pointers to a sink and source") {
		auto src = std::make_unique<Dummy_audio_source>("test");
		auto sink = std::make_unique<Dummy_audio_sink>(*src, 0);

		WHEN ("a Basic_audio is created") {
			THEN ("no exceptions are thrown and queries return the "
			      "expected initial values") {
				audio::Basic_audio pa(std::move(src),
				                      std::move(sink));
				REQUIRE(pa.

				        Position()

				        == std::chrono::microseconds{0});
			}
		}
	}
}

SCENARIO ("Basic_audio responds to getters with valid responses",
          "[pipe-audio]") {
	GIVEN ("a valid Basic_audio and dummy components") {
		auto src = std::make_unique<Dummy_audio_source>("test");
		auto snk = std::make_unique<Dummy_audio_sink>(*src, 0);
		audio::Basic_audio pa(std::move(src), std::move(snk));

		WHEN ("the state is requested") {
			AND_WHEN("the state is 'playing'")
			{
				pa.SetPlaying(true);
				THEN ("the state is reported as 'playing'") {
					REQUIRE(pa.

					        CurrentState()

					        == audio::Audio::State::playing);
				}
			}

			AND_WHEN("the state is 'stopped'")
			{
				pa.SetPlaying(false);
				THEN ("the state is reported as 'stopped'") {
					REQUIRE(pa.

					        CurrentState()

					        == audio::Audio::State::stopped);
				}
			}
		}

		WHEN ("the position is requested") {
			AND_WHEN("the position is zero")
			{
				pa.SetPosition(std::chrono::microseconds{0});
				THEN ("the position is reported as 0") {
					REQUIRE(pa.

					        Position()

					        == std::chrono::microseconds{0});
				}
			}

			AND_WHEN("the position is not zero")
			{
				pa.SetPosition(std::chrono::microseconds{8675309});
				THEN ("the position is reported as "
				      "(rounded-off "
				      "position)") {
					// The Dummy_audio_source has 44100Hz
					// sample rate.  The following
					// calculation is what a round-trip to
					// and from samples should cause.
					// This *won't* be 8675309!
					std::chrono::microseconds expected{
					        (((8675309ULL * 44100) / 1000000) *
					         1000000) /
					        44100};
					REQUIRE(pa.

					        Position()

					        == expected);
				}
			}
		}
	}
}

SCENARIO ("Basic_audio propagates source emptiness correctly", "[pipe-audio]") {
	GIVEN ("a valid set of dummy components") {
		auto src = std::make_unique<Dummy_audio_source>("test");
		auto snk = std::make_unique<Dummy_audio_sink>(*src, 0);
		snk->state = audio::Audio::State::stopped;

		// We build the Basic_audio later, because it moves src and snk
		// out of easy modification range.

		WHEN ("the DummyResponseSource is reporting end of file") {
			src->run_out = true;

			audio::Basic_audio pa(std::move(src), std::move(snk));

			THEN ("Update() returns AT_END") {
				REQUIRE(pa.

				        Update()

				        == audio::Audio::State::at_end);
			}
		}
	}
}

SCENARIO ("Basic_audio acquires state from the sink correctly",
          "[pipe-audio]") {
	GIVEN ("a valid set of dummy components") {
		auto src = std::make_unique<Dummy_audio_source>("test");
		auto snk = std::make_unique<Dummy_audio_sink>(*src, 0);

		// We build the Basic_audio later, because it moves src and snk
		// out of easy modification range.

		WHEN ("the response sink's state is 'stopped'") {
			snk->state = audio::Audio::State::stopped;

			audio::Basic_audio pa(std::move(src), std::move(snk));

			THEN ("Update() returns 'stopped'") {
				REQUIRE(pa.

				        Update()

				        == audio::Audio::State::stopped);
			}
		}

		WHEN ("the response sink state is 'playing'") {
			snk->state = audio::Audio::State::playing;

			audio::Basic_audio pa(std::move(src), std::move(snk));

			THEN ("Update() returns 'playing'") {
				REQUIRE(pa.

				        Update()

				        == audio::Audio::State::playing);
			}
		}

		WHEN ("the DummyResponseSink's state is 'at_end'") {
			snk->state = audio::Audio::State::at_end;

			audio::Basic_audio pa(std::move(src), std::move(snk));

			THEN ("Update() returns 'at_end'") {
				REQUIRE(pa.

				        Update()

				        == audio::Audio::State::at_end);
			}
		}
	}
}

} // namespace playd::tests
