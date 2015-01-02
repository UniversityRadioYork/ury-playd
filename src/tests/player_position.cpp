// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for the PlayerPosition class.
 */

#include "catch.hpp"

#include <sstream>
#include "../player/player_position.hpp"
#include "dummy_response_sink.hpp"


SCENARIO("PlayerPosition begins in position 0", "[player-position]") {
	GIVEN("a fresh PlayerPosition and DummyResponseSink") {
		std::ostringstream os;
		DummyResponseSink rs(os);

		PlayerPosition pp(nullptr, 0);

		WHEN("the current Position is emitted") {
			pp.Emit(rs);

			THEN("the emitted position is 0") {
				REQUIRE(os.str() == "TIME 0\n");
			}
		}
	}
}

SCENARIO("PlayerPosition pushes position changes to its registered response sink", "[player-position], [response-source]") {
	GIVEN("a PlayerPosition with a dummy ResponseSink") {
		std::ostringstream os;
		DummyResponseSink rs(os);

		PlayerPosition pp(&rs, 5000);

		WHEN("the Position is changed to a sufficiently different Position") {
			pp.Update(54321);

			THEN("the response sink receives a TIME response") {
				REQUIRE(os.str() == "TIME 54321\n");
			}

			THEN("any change to the same position does not emit a duplicate response") {
				pp.Update(54321);

				// The ostringstream should only contain the original response.
				REQUIRE(os.str() == "TIME 54321\n");
			}


			AND_WHEN("the Position is then changed to an insufficiently different Position") {
				pp.Update(55555);

				THEN("this change does not elicit a response") {
					REQUIRE(os.str() == "TIME 54321\n");
				}
			}
		}

		WHEN("the Position is changed from initial to a normally insufficiently different Position") {
			pp.Update(4321);

			THEN("the response sink still receives a TIME response") {
				// Because the first change in TIME is always emitted.

				REQUIRE(os.str() == "TIME 4321\n");
			}

			AND_WHEN("the Position is then changed to an insufficiently different Position") {
				pp.Update(4444);

				THEN("this change does not elicit a response") {
					REQUIRE(os.str() == "TIME 4321\n");
				}
			}
		}
	}

	GIVEN("a PlayerPosition with no ResponseSink") {
		PlayerPosition pp(nullptr, 5000);

		WHEN("the Position is changed") {
			// The Update has to be done in the THEN clause, as we're
			// checking the statement itself for exceptions.
			THEN("no error is thrown") {
				REQUIRE_NOTHROW(pp.Update(54321));
			}
		}
	}
}

SCENARIO("PlayerPosition::Reset() correctly resets the position counters", "[player-position]") {
	GIVEN("a PlayerPosition with a dummy ResponseSink") {
		std::ostringstream os1;
		DummyResponseSink rs1(os1);

		std::ostringstream os2;
		DummyResponseSink rs2(os2);

		PlayerPosition pp(&rs1, 5000);

		WHEN("the position has been set") {
			pp.Update(54321);

			AND_WHEN("the position is reset") {
				pp.Reset();

				THEN("the position is changed to zero") {
					pp.Emit(rs2);
					REQUIRE(os2.str() == "TIME 0\n");
				}

				THEN("no change to zero is pushed") {
					// Only the original Update should have been pushed.
					REQUIRE(os1.str() == "TIME 54321\n");
				}

				THEN("a change back to the original position is recorded") {
					pp.Update(54321);
					REQUIRE(os1.str() == "TIME 54321\nTIME 54321\n");

					AND_THEN("a significant change thereafter is also recorded") {
						pp.Update(65432);
						REQUIRE(os1.str() == "TIME 54321\nTIME 54321\nTIME 65432\n");
					}

					AND_THEN("an insignificant change thereafter is not recorded") {
						pp.Update(55555);
						REQUIRE(os1.str() == "TIME 54321\nTIME 54321\n");
					}
				}
			}
		}
	}
}
