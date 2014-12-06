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

SCENARIO("PlayerFile successfully ejects Audio", "[player-file]") {
	GIVEN("a PlayerFile") {
		// We allocate this on the heap, because PlayerFile wille try
		// to free it when it goes out of scope.
		auto da = new DummyAudio;
		DummyAudioSystem ds(da);
		PlayerFile pf(nullptr, ds);

		WHEN("the PlayerFile has no audio loaded") {
			THEN("Eject is a harmless no-operation") {
				// We can't actually measure this directly using the
				// PlayerFile interface.  This is a best approximation.
				da->state = Audio::State::PLAYING;

				REQUIRE(pf.Update() == Audio::State::NONE);
				pf.Eject();
				REQUIRE(pf.Update() == Audio::State::NONE);
			}
		}

		WHEN("the PlayerFile has audio loaded") {
			pf.Load("/test/path");
			da->started = true;

			THEN("Eject stops the Audio") {
				pf.Eject();
				REQUIRE_FALSE(da->started);
			}

			THEN("Eject appears to unload the Audio") {
				// We can't actually measure this directly using the
				// PlayerFile interface.  This is a best approximation.
				da->state = Audio::State::PLAYING;

				REQUIRE(pf.Update() == Audio::State::PLAYING);
				pf.Eject();
				REQUIRE(pf.Update() == Audio::State::NONE);
			}
		}
	}
}

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

		WHEN("PlayerFile::Update is invoked") {
			THEN("the return value is the Audio's current state") {
				for (Audio::State s : {
					Audio::State::AT_END,
					Audio::State::PLAYING,
					Audio::State::STOPPED,
				}) {
					da->state = s;
					REQUIRE(pf.Update() == da->state);
				}					
			}
		}

		WHEN("PlayerFile::Position is invoked") {
			da->pos = 1337;
			auto pos = pf.Position();

			THEN("the return value is the Audio position") {
				REQUIRE(pos == 1337);
				REQUIRE(da->pos == 1337);
			}
		}

		WHEN("PlayerFile::Seek is invoked") {
			da->pos = 27;
			pf.Seek(53);

			THEN("the Audio's position should have changed") {
				REQUIRE(da->pos == 53);
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
