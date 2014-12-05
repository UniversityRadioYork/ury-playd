// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for the PlayerState class.
 */

#include "catch.hpp"

#include "../player/player_state.hpp"
#include "dummy_response_sink.hpp"

SCENARIO("PlayerState begins in a valid state", "[player-state]") {
	GIVEN("a fresh PlayerState") {
		PlayerState ps(nullptr);

		WHEN("the current state is compared with EJECTED") {
			bool is_ejected = ps.In({ PlayerState::State::EJECTED });

			THEN("the comparison is true") {
				REQUIRE(is_ejected);
			}
		}
	}
}

SCENARIO("PlayerState pushes state changes to its registered response sink", "[player-state], [response-source]") {
	GIVEN("a PlayerState with a dummy ResponseSink") {
		std::ostringstream os;
		DummyResponseSink rs(os);
		PlayerState ps(&rs);

		WHEN("the state is changed to a different state") {
			ps.Set(PlayerState::State::PLAYING);

			THEN("the response sink receives a STATE response") {
				REQUIRE(os.str() == "STATE Playing\n");
			}
		}
	}

	GIVEN("a PlayerState with no ResponseSink") {
		PlayerState ps(nullptr);

		WHEN("the state is changed to a different state") {
			// The Set has to be done in the THEN clause, as we're
			// checking the statement itself for exceptions.
			THEN("no error is thrown") {
				REQUIRE_NOTHROW(ps.Set(PlayerState::State::PLAYING));
			}
		}
	}
}

SCENARIO("PlayerState correctly returns whether or not it is running", "[player-state]") {
	GIVEN("a fresh PlayerState") {
		PlayerState ps(nullptr);

		WHEN("the current state is QUITTING") {
			ps.Set(PlayerState::State::QUITTING);
			THEN("the player is not running") {
				REQUIRE(!ps.IsRunning());
			}
		}

		// There are so few states in playd that we might as well
		// exhaustively test.
		WHEN("the current state is not QUITTING") {
			AND_WHEN("the current state is STARTING") {
				ps.Set(PlayerState::State::STARTING);

				THEN("the player is running") {
					REQUIRE(ps.IsRunning());
				}
			}
			AND_WHEN("the current state is EJECTED") {
				ps.Set(PlayerState::State::EJECTED);

				THEN("the player is running") {
					REQUIRE(ps.IsRunning());
				}
			}
			AND_WHEN("the current state is STOPPED") {
				ps.Set(PlayerState::State::STOPPED);

				THEN("the player is running") {
					REQUIRE(ps.IsRunning());
				}
			}
			AND_WHEN("the current state is PLAYING") {
				ps.Set(PlayerState::State::PLAYING);

				THEN("the player is running") {
					REQUIRE(ps.IsRunning());
				}
			}
		}
	}
}
