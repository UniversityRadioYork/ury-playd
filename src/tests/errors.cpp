/**
 * @file
 * Tests for error classes.
 */

#include "../errors.h"

#include "catch.hpp"

namespace playd::tests
{
SCENARIO ("Errors contain a retrievable message", "[error]") {
	GIVEN ("An Error") {
		Error e = Net_error("need an RS-232 Interface Lead");

		WHEN ("Message() is called") {
			auto s = e.Message();

			THEN ("the result is the message given in the Error's "
			      "ctor") {
				REQUIRE(s == "need an RS-232 Interface Lead");
			}
		}
	}
}

} // namespace playd::tests
