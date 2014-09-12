// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Tests for the Tokeniser class.
 */

#include "catch.hpp"
#include "../io/tokeniser.hpp"

SCENARIO("Tokenisers can handle complete, unquoted commands", "[tokeniser]") {
	GIVEN("A fresh Tokeniser") {
		Tokeniser t;

		WHEN("the Tokeniser is fed a nullary command") {
			auto words = t.Feed("stop\n");

			THEN("one line is returned") {
				REQUIRE(words.size() == 1);
			}

			THEN("the line contains one word") {
				REQUIRE(words[0].size() == 1);
			}

			THEN("the word is the nullary command") {
				REQUIRE(words[0][0] == "stop");
			}
		}

		WHEN("the Tokeniser is fed a unary command") {
			auto words = t.Feed("seek 10s\n");

			THEN("one line is returned") {
				REQUIRE(words.size() == 1);
			}

			THEN("the line contains two words") {
				REQUIRE(words[0].size() == 2);
			}

			THEN("the first word is the command") {
				REQUIRE(words[0][0] == "seek");
			}

			THEN("the second word is the argument") {
				REQUIRE(words[0][1] == "10s");
			}
		}
	}
}
