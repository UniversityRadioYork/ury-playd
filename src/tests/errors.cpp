// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for command results.
 */

#include <sstream>

#include "catch.hpp"
#include "../cmd_result.hpp"
#include "dummy_response_sink.hpp"
// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for error classes.
 */

#include "catch.hpp"

#include "../errors.hpp"


SCENARIO("Errors contain a retrievable message", "[error]") {
	GIVEN("An Error") {
		Error e = NetError("need an RS-232 Interface Lead");

		WHEN("Message() is called") {
			auto s = e.Message();

			THEN("the result is the message given in the Error's ctor") {
				REQUIRE(s == "need an RS-232 Interface Lead");
			}
		}
	}
}

