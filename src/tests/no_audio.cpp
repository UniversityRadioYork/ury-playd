// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for the NoAudio class.
 */

#include <sstream>

#include "catch.hpp"

#include "../audio/audio.hpp"
#include "../errors.hpp"
#include "../response.hpp"
#include "dummy_response_sink.hpp"

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

SCENARIO("NoAudio emits state, but no file or position", "[no-audio]") {
	GIVEN("A NoAudio object") {
		NoAudio n;

		WHEN("the NoAudio is asked to emit a state") {
			std::string type, value;
			std::tie(type, value) = n.Emit("/player/state/current");

			THEN("'Ejected' is emitted") {
				REQUIRE(type == "Entry");
				REQUIRE(value == "ejected");
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

		WHEN("Seek() is called") {
			THEN("NoAudioError is thrown") {
				REQUIRE_THROWS_AS(n.Seek(100), NoAudioError);
			}
		}

		WHEN("Position() is called") {
			THEN("NoAudioError is thrown") {
				REQUIRE_THROWS_AS(n.Position(), NoAudioError);
			}
		}

		WHEN("asked to emit a file") {
			THEN("nothing is emitted") {
				REQUIRE_THROWS_AS(n.Emit("/player/file"), FileError);
			}
		}

		WHEN("asked to emit a time") {
			THEN("nothing is emitted") {
				REQUIRE_THROWS_AS(n.Emit("/player/time/elapsed"), FileError);
			}
		}
	}
}
