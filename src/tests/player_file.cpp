// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for the PlayerState class.
 */

#include "catch.hpp"

#include <sstream>
#include "../player/player_position.hpp"
#include "dummy_response_sink.hpp"
// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for the PlayerState class.
 */

#include "catch.hpp"

#include <sstream>
#include "../player/player_file.hpp"
#include "dummy_audio.hpp"
#include "dummy_response_sink.hpp"

SCENARIO("PlayerFile passes Audio commands through to loaded Audio", "[player-file]") {
	GIVEN("a PlayerFile with loaded dummy audio") {
		// We allocate this on the heap, because PlayerFile wille try
		// to free it when it goes out of scope.
		auto da = new DummyAudio;
		DummyAudioSystem ds(da);
		PlayerFile pf(nullptr, ds);

		pf.Load("/test/path");

		WHEN("PlayerFile::Start is invoked with stopped Audio") {
			da->started = false;
			pf.Start();

			THEN("the loaded Audio should be started") {
				REQUIRE(da->started);
			}
		}

		WHEN("PlayerFile::Stop is invoked with started Audio") {
			da->started = true;
			pf.Stop();

			THEN("the loaded Audio should be stopped") {
				REQUIRE_FALSE(da->started);
			}
		}

		WHEN("PlayerFile::Emit is invoked") {
			std::ostringstream os;
			DummyResponseSink rs(os);

			pf.Emit(rs);

			THEN("the response matches that from the Audio") {
				REQUIRE(os.str() == "FILE /test/path\n");
			}
		}
	}
}
