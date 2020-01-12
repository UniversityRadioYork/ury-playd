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

namespace Playd::Tests
{
SCENARIO ("NullAudio reports the state as none", "[null-audio]") {
	GIVEN ("A NullAudio object") {
		Audio::NullAudio n;

		WHEN ("Update() is called") {
			auto state = n.Update();

			THEN ("the resulting state is NONE") {
				REQUIRE(state == Audio::Audio::State::NONE);
			}
		}
	}
}

SCENARIO ("NullAudio returns a state of NONE", "[null-audio]") {
	GIVEN ("A NullAudio object") {
		Audio::NullAudio n;

		WHEN ("the NullAudio is asked for its state") {
			auto state = n.CurrentState();

			THEN ("the state is NONE") {
				REQUIRE(state == Audio::Audio::State::NONE);
			}
		}
	}
}

SCENARIO ("NullAudio throws exceptions when asked to do audio-type things", "[null-audio]") {
	GIVEN ("A NullAudio object") {
		Audio::NullAudio n;

		WHEN ("SetPlaying(true) is called") {
			THEN ("NullAudioError is thrown") {
				REQUIRE_THROWS_AS(n.SetPlaying(true), NullAudioError);
			}
		}

		WHEN ("SetPlaying(false) is called") {
			THEN ("NullAudioError is thrown") {
				REQUIRE_THROWS_AS(n.SetPlaying(false), NullAudioError);
			}
		}

		WHEN ("SetPosition() is called") {
			THEN ("NullAudioError is thrown") {
				REQUIRE_THROWS_AS(n.SetPosition(std::chrono::microseconds{100}), NullAudioError);
			}
		}

		WHEN ("Position() is called") {
			THEN ("NullAudioError is thrown") {
				REQUIRE_THROWS_AS(n.Position(), NullAudioError);
			}
		}

		WHEN ("File() is called") {
			THEN ("NullAudioError is thrown") {
				REQUIRE_THROWS_AS(n.File(), NullAudioError);
			}
		}
	}
}

} // namespace Playd::Tests
