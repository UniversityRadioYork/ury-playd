// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for the Null_audio class.
 */

#include <chrono>
#include <sstream>

#include "../audio/audio.h"
#include "../errors.h"
#include "../response.h"
#include "catch.hpp"
#include "dummy_response_sink.h"

namespace playd::tests
{
SCENARIO ("Null_audio reports the state as none", "[null-audio]") {
	GIVEN ("A Null_audio object") {
		audio::Null_audio n;

		WHEN ("Update() is called") {
			auto state = n.Update();

			THEN ("the resulting state is NONE") {
				REQUIRE(state == audio::Audio::State::none);
			}
		}
	}
}

SCENARIO ("Null_audio returns a state of NONE", "[null-audio]") {
	GIVEN ("A Null_audio object") {
		audio::Null_audio n;

		WHEN ("the Null_audio is asked for its state") {
			auto state = n.CurrentState();

			THEN ("the state is NONE") {
				REQUIRE(state == audio::Audio::State::none);
			}
		}
	}
}

SCENARIO ("Null_audio throws exceptions when asked to do audio-type things", "[null-audio]") {
	GIVEN ("A Null_audio object") {
		audio::Null_audio n;

		WHEN ("SetPlaying(true) is called") {
			THEN ("Null_audio_error is thrown") {
				REQUIRE_THROWS_AS(n.SetPlaying(true), Null_audio_error);
			}
		}

		WHEN ("SetPlaying(false) is called") {
			THEN ("Null_audio_error is thrown") {
				REQUIRE_THROWS_AS(n.SetPlaying(false), Null_audio_error);
			}
		}

		WHEN ("SetPosition() is called") {
			THEN ("Null_audio_error is thrown") {
				REQUIRE_THROWS_AS(n.SetPosition(std::chrono::microseconds{100}), Null_audio_error);
			}
		}

		WHEN ("Position() is called") {
			THEN ("Null_audio_error is thrown") {
				REQUIRE_THROWS_AS(n.Position(), Null_audio_error);
			}
		}

		WHEN ("File() is called") {
			THEN ("Null_audio_error is thrown") {
				REQUIRE_THROWS_AS(n.File(), Null_audio_error);
			}
		}
	}
}

} // namespace playd::tests
