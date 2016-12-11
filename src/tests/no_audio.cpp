// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for the NoAudio class.
 */

#include <chrono>
#include <sstream>

#include "catch.hpp"

#include "../audio/audio.hpp"
#include "../errors.hpp"
#include "../response.hpp"
#include "dummy_response_sink.hpp"

using namespace std::chrono;


SCENARIO("NoAudio reports the state as NONE", "[no-audio]") {
	GIVEN("A NoAudio object") {
		NoAudio n;

		WHEN("Update() is called") {
			auto state = n.Update();

			THEN("the resulting state is NONE") {
				REQUIRE(state == Audio::State::NONE);
			}
		}
	}
}

SCENARIO("NoAudio returns a state of NONE", "[no-audio]") {
	GIVEN("A NoAudio object") {
		NoAudio n;

		WHEN("the NoAudio is asked for its state") {
			auto state = n.CurrentState();

			THEN("the state is NONE") {
				REQUIRE(state == Audio::State::NONE);
			}
		}
	}
}

SCENARIO("NoAudio throws exceptions when asked to do audio-type things", "[no-audio]") {
	GIVEN("A NoAudio object") {
		NoAudio n;

		WHEN("SetPlaying(true) is called") {
			THEN("NoAudioError is thrown") {
				REQUIRE_THROWS_AS(n.SetPlaying(true), NoAudioError);
			}
		}

		WHEN("SetPlaying(false) is called") {
			THEN("NoAudioError is thrown") {
				REQUIRE_THROWS_AS(n.SetPlaying(false), NoAudioError);
			}
		}

		WHEN("SetPosition() is called") {
			THEN("NoAudioError is thrown") {
				REQUIRE_THROWS_AS(n.SetPosition(microseconds {100}), NoAudioError);
			}
		}

		WHEN("Position() is called") {
			THEN("NoAudioError is thrown") {
				REQUIRE_THROWS_AS(n.Position(), NoAudioError);
			}
		}

		WHEN("File() is called") {
			THEN("NoAudioError is thrown") {
				REQUIRE_THROWS_AS(n.File(), NoAudioError);
			}
		}

	}
}
