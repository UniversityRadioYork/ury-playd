// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for the Tokeniser class.
 */

#include "catch.hpp"
#include "../tokeniser.hpp"

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

// See http://universityradioyork.github.io/baps3-spec/comms/internal/protocol.html
SCENARIO("Tokeniser is compliant with the BAPS3 spec", "[tokeniser][spec]") {
	GIVEN("A fresh Tokeniser") {
		Tokeniser t;
		WHEN("the Tokeniser is fed E1") {
			auto lines = t.Feed("");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 0);
			}
		}
		WHEN("the Tokeniser is fed E2") {
			auto lines = t.Feed("\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 0);
			}
		}
		WHEN("the Tokeniser is fed E3") {
			auto lines = t.Feed("''\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 1);
				REQUIRE(lines[0][0] == "");
			}
		}
		WHEN("the Tokeniser is fed E4") {
			auto lines = t.Feed("\"\"\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 1);
				REQUIRE(lines[0][0] == "");
			}
		}
		WHEN("the Tokeniser is fed W1") {
			auto lines = t.Feed("foo bar baz\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 3);
				REQUIRE(lines[0][0] == "foo");
				REQUIRE(lines[0][1] == "bar");
				REQUIRE(lines[0][2] == "baz");
			}
		}
		WHEN("the Tokeniser is fed W2") {
			auto lines = t.Feed("foo\tbar\tbaz\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 3);
				REQUIRE(lines[0][0] == "foo");
				REQUIRE(lines[0][1] == "bar");
				REQUIRE(lines[0][2] == "baz");
			}
		}
		WHEN("the Tokeniser is fed W3") {
			auto lines = t.Feed("foo\rbar\rbaz\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 3);
				REQUIRE(lines[0][0] == "foo");
				REQUIRE(lines[0][1] == "bar");
				REQUIRE(lines[0][2] == "baz");
			}
		}
		WHEN("the Tokeniser is fed W4") {
			auto lines = t.Feed("silly windows\r\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 2);
				REQUIRE(lines[0][0] == "silly");
				REQUIRE(lines[0][1] == "windows");
			}
		}
		WHEN("the Tokeniser is fed W5") {
			auto lines = t.Feed("    abc def\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 2);
				REQUIRE(lines[0][0] == "abc");
				REQUIRE(lines[0][1] == "def");
			}
		}
		WHEN("the Tokeniser is fed W6") {
			auto lines = t.Feed("ghi jkl    \n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 2);
				REQUIRE(lines[0][0] == "ghi");
				REQUIRE(lines[0][1] == "jkl");
			}
		}
		WHEN("the Tokeniser is fed W7") {
			auto lines = t.Feed("    mno pqr    \n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 2);
				REQUIRE(lines[0][0] == "mno");
				REQUIRE(lines[0][1] == "pqr");
			}
		}
		WHEN("the Tokeniser is fed Q1") {
			auto lines = t.Feed("abc\\\ndef\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 1);
				REQUIRE(lines[0][0] == "abc\ndef");
			}
		}
		WHEN("the Tokeniser is fed Q2") {
			auto lines = t.Feed("\"abc\ndef\"\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 1);
				REQUIRE(lines[0][0] == "abc\ndef");
			}
		}
		WHEN("the Tokeniser is fed Q3") {
			auto lines = t.Feed("\"abc\\\ndef\"\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 1);
				REQUIRE(lines[0][0] == "abc\ndef");
			}
		}
		WHEN("the Tokeniser is fed Q4") {
			auto lines = t.Feed("'abc\ndef'\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 1);
				REQUIRE(lines[0][0] == "abc\ndef");
			}
		}
		WHEN("the Tokeniser is fed Q5") {
			auto lines = t.Feed("'abc\\\ndef'\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 1);
				REQUIRE(lines[0][0] == "abc\\\ndef");
			}
		}
		WHEN("the Tokeniser is fed Q6") {
			auto lines = t.Feed("Scare\\\" quotes\\\"\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 2);
				REQUIRE(lines[0][0] == "Scare\"");
				REQUIRE(lines[0][1] == "quotes\"");
			}
		}
		WHEN("the Tokeniser is fed Q7") {
			auto lines = t.Feed("I\\'m free\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 2);
				REQUIRE(lines[0][0] == "I'm");
				REQUIRE(lines[0][1] == "free");
			}
		}
		WHEN("the Tokeniser is fed Q8") {
			auto lines = t.Feed("'hello, I'\\''m an escaped single quote'\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 1);
				REQUIRE(lines[0][0] == "hello, I'm an escaped single quote");
			}
		}
		WHEN("the Tokeniser is fed Q9") {
			auto lines = t.Feed("\"hello, this is an \\\" escaped double quote\"\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 1);
				REQUIRE(lines[0][0] == "hello, this is an \" escaped double quote");
			}
		}
		WHEN("the Tokeniser is fed M1") {
			auto lines = t.Feed("first line\nsecond line\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 2);
				REQUIRE(lines[0].size() == 2);
				REQUIRE(lines[0][0] == "first");
				REQUIRE(lines[0][1] == "line");
				REQUIRE(lines[1].size() == 2);
				REQUIRE(lines[1][0] == "second");
				REQUIRE(lines[1][1] == "line");
			}
		}
		WHEN("the Tokeniser is fed U1") {
			auto lines = t.Feed("北野 武\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 1);
				REQUIRE(lines[0].size() == 2);
				REQUIRE(lines[0][0] == "北野");
				REQUIRE(lines[0][1] == "武");
			}
		}
		// We don't implement UTF-8 replacement.
		// Frankly, this should probably be removed from the spec tests.
		//WHEN("the Tokeniser is fed U2") {
		//	auto lines = t.Feed("f\xfcr\n");
		//	THEN ("the Tokeniser returns the specified result") {
		//		REQUIRE(lines.size() == 1);
		//		REQUIRE(lines[0].size() == 1);
		//		REQUIRE(lines[0][0] == "f\xef\xbf\xbdr");
		//	}
		//}
		WHEN("the Tokeniser is fed X1") {
			auto lines = t.Feed("enqueue file \"C:\\\\Users\\\\Test\\\\Artist - Title.mp3\" 1\"\n");
			THEN ("the Tokeniser returns the specified result") {
				REQUIRE(lines.size() == 0);
				REQUIRE(lines[0].size() == 4);
				REQUIRE(lines[0][0] == "enqueue");
				REQUIRE(lines[0][1] == "file");
				REQUIRE(lines[0][2] == "C:\\Users\\Test\\Artist - Title.mp3");
				REQUIRE(lines[0][3] == "1");
			}
		}
	}
}
