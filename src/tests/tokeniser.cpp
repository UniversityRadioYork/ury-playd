// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

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
			auto lines = t.Feed("stop\n");

			THEN("one line is returned") {
				REQUIRE(lines.size() == 1);
			}

			THEN("the line contains one word") {
				REQUIRE(lines[0].size() == 1);
			}

			THEN("the word is the nullary command") {
				REQUIRE(lines[0][0] == "stop");
			}
		}

		WHEN("the Tokeniser is fed a unary command") {
			auto lines = t.Feed("seek 10s\n");

			THEN("one line is returned") {
				REQUIRE(lines.size() == 1);
			}

			THEN("the line contains two words") {
				REQUIRE(lines[0].size() == 2);
			}

			THEN("the first word is the command") {
				REQUIRE(lines[0][0] == "seek");
			}

			THEN("the second word is the argument") {
				REQUIRE(lines[0][1] == "10s");
			}
		}
	}
}

SCENARIO("Tokenisers can handle single-quoted strings", "[tokeniser]") {
	GIVEN("A fresh Tokeniser") {
		Tokeniser t;

		WHEN("the Tokeniser is fed a single-quoted string with no special characters") {
			auto lines = t.Feed("'normal_string'\n");

			THEN("one line is returned") {
				REQUIRE(lines.size() == 1);
			}

			THEN("the line contains only one word") {
				REQUIRE(lines[0].size() == 1);
			}

			THEN("the word contains the bytes enclosed in the single quotes") {
				REQUIRE(lines[0][0] == "normal_string");
			}
		}

		WHEN("the Tokeniser is fed a single-quoted string with spaces") {
			auto lines = t.Feed("'not three words'\n");

			THEN("one line is returned") {
				REQUIRE(lines.size() == 1);
			}

			THEN("the line contains only one word") {
				REQUIRE(lines[0].size() == 1);
			}

			THEN("the word contains the bytes enclosed in the single quotes") {
				REQUIRE(lines[0][0] == "not three words");
			}
		}

		// Backslashes are tested in their own scenario below.
	}
}

SCENARIO("Tokenisers can handle double-quoted strings", "[tokeniser]") {
	GIVEN("A fresh Tokeniser") {
		Tokeniser t;

		WHEN("the Tokeniser is fed a double-quoted string with no special characters") {
			auto lines = t.Feed("\"normal_string\"\n");

			THEN("one line is returned") {
				REQUIRE(lines.size() == 1);
			}

			THEN("the line contains only one word") {
				REQUIRE(lines[0].size() == 1);
			}

			THEN("the word contains the bytes enclosed in the double quotes") {
				REQUIRE(lines[0][0] == "normal_string");
			}
		}

		WHEN("the Tokeniser is fed a double-quoted string with spaces") {
			auto lines = t.Feed("\"not three words\"\n");

			THEN("one line is returned") {
				REQUIRE(lines.size() == 1);
			}

			THEN("the line contains only one word") {
				REQUIRE(lines[0].size() == 1);
			}

			THEN("the word contains the bytes enclosed in the double quotes") {
				REQUIRE(lines[0][0] == "not three words");
			}
		}

		// Backslashes are tested in their own scenario below.
	}
}

SCENARIO("Tokenisers can handle mixed-quoted strings", "[tokeniser]") {
	// This is a slightly strange concept, but is based on what happens in
	// POSIX shell.

	GIVEN("A fresh Tokeniser") {
		Tokeniser t;

		WHEN("the Tokeniser is fed a word with a mixture of different quote styles") {
			auto lines = t.Feed("This' is'\\ perfectly\"\\ valid \"syntax!\n");

			THEN("one line is returned") {
				REQUIRE(lines.size() == 1);
			}

			THEN("the line contains only one word") {
				REQUIRE(lines[0].size() == 1);
			}

			THEN("the word contains the concatenation of each") {
				REQUIRE(lines[0][0] == "This is perfectly valid syntax!");
			}
		}
	}
}

SCENARIO("Tokenisers can backslash-escape bytes", "[tokeniser]") {
	GIVEN("A fresh Tokeniser") {
		Tokeniser t;

		WHEN("the Tokeniser is fed a backslashed space in unquoted mode") {
			auto lines = t.Feed("backslashed\\ space\n");

			THEN("one line is returned") {
				REQUIRE(lines.size() == 1);
			}

			THEN("the line contains only one word") {
				REQUIRE(lines[0].size() == 1);
			}

			THEN("the line contains the space, but no backslash") {
				REQUIRE(lines[0][0] == "backslashed space");
			}
		}

		WHEN("the Tokeniser is fed a backslashed space in double-quoted mode") {
			auto lines = t.Feed("\"backslashed\\ space\"\n");

			THEN("one line is returned") {
				REQUIRE(lines.size() == 1);
			}

			THEN("the line contains only one word") {
				REQUIRE(lines[0].size() == 1);
			}

			THEN("the line contains the space, but no backslash or quotes") {
				REQUIRE(lines[0][0] == "backslashed space");
			}
		}

		WHEN("the Tokeniser is fed a backslashed space in single-quoted mode") {
			auto lines = t.Feed("'backslashed\\ space'\n");

			THEN("one line is returned") {
				REQUIRE(lines.size() == 1);
			}

			THEN("the line contains only one word") {
				REQUIRE(lines[0].size() == 1);
			}

			THEN("the line contains the space AND backslash, but no quotes") {
				REQUIRE(lines[0][0] == "backslashed\\ space");
			}
		}
	}
}
