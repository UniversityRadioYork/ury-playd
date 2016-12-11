// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for response classes.
 */

#include <ostream>
#include <sstream>
#include <string>

#include "catch.hpp"
#include "../response.h"

SCENARIO("Response's convenience constructors create correct Responses", "[command-result]") {
	GIVEN("A Response created via Response::Success()") {
		Response c = Response::Success("tag");

		WHEN("the response is packed") {
			std::string s = c.Pack();

			THEN("the result is ACK OK success") {
				REQUIRE(s == "tag ACK OK success");
			}
		}
	}

	GIVEN("A Response created via Response::Invalid()") {
		Response c = Response::Invalid("gat", "PEBCAK error");


		WHEN("the response is packed") {
			std::string s = c.Pack();

			THEN("the result is ACK WHAT followed by the failure message") {
				REQUIRE(s == "gat ACK WHAT 'PEBCAK error'");
			}
		}
	}


	GIVEN("A Response created via Response::Failure()") {
		Response c = Response::Failure("cat people", "lp0 on fire");

		WHEN("the response is packed") {
			std::string s = c.Pack();

			THEN("the response is ACK FAIL followed by the failure message") {
				REQUIRE(s == "'cat people' ACK FAIL 'lp0 on fire'");
			}
		}
	}
}

SCENARIO("Responses correctly escape arguments with single quotes", "[response]") {
	WHEN("the Response is fed no arguments") {
		auto r = Response("tag", Response::Code::OHAI);

		THEN("the emitted response has no quoting") {
			REQUIRE(r.Pack() == "tag OHAI");
		}
	}

	WHEN("the Response is fed a single argument with no quotes") {
		auto r = Response("tag", Response::Code::OHAI).AddArg("ulyoath");

		THEN("the emitted response's argument is not quoted") {
			REQUIRE(r.Pack() == "tag OHAI ulyoath");
		}
	}

	WHEN("the Response is fed a single argument with single quotes") {
		auto r = Response("tag", Response::Code::OHAI).AddArg("chattur'gha");

		THEN("the emitted response's argument is single-quoted") {
			REQUIRE(r.Pack() == R"(tag OHAI 'chattur'\''gha')");
		}
	}

	WHEN("the Response is fed two arguments, both with single quotes") {
		auto r = Response("tag", Response::Code::OHAI).AddArg("chattur'gha").AddArg("xel'lotath");

		THEN("the emitted response's arguments are both single-quoted") {
			REQUIRE(r.Pack() == R"(tag OHAI 'chattur'\''gha' 'xel'\''lotath')");
		}
	}

	WHEN("the Response is fed two arguments, one quoted, one unquoted") {
		auto r = Response("tag", Response::Code::OHAI).AddArg("chattur'gha").AddArg("ulyoath");

		THEN("the emitted response's arguments are quoted accordingly") {
			REQUIRE(r.Pack() == R"(tag OHAI 'chattur'\''gha' ulyoath)");
		}
	}

	WHEN("the Response is fed a single argument with double quotes") {
		auto r = Response("tag", Response::Code::FLOAD).AddArg(R"("scare"-quotes)");

		THEN("the emitted response's argument is single-quoted") {
			REQUIRE(r.Pack() == R"(tag FLOAD '"scare"-quotes')");
		}
	}

	WHEN("the Response is fed a single argument with whitespace") {
		auto r = Response("tag", Response::Code::END).AddArg("pargon pargon pargon");

		THEN("the emitted response's argument is single-quoted") {
			REQUIRE(r.Pack() == R"(tag END 'pargon pargon pargon')");
		}
	}

	WHEN("the Response is fed several arguments with differing whitespace") {
		auto r = Response("tag", Response::Code::END).AddArg("a space").AddArg("new\nline").AddArg("tab\tstop");

		THEN("the emitted response's argument is single-quoted") {
			REQUIRE(r.Pack() == "tag END 'a space' 'new\nline' 'tab\tstop'");
		}
	}

	WHEN("the Response is fed a representative example with backslashes and spaces") {
		auto r = Response("tag", Response::Code::FLOAD).AddArg(R"(C:\Users\Test\Music\Bound 4 Da Reload (Casualty).mp3)");

		THEN("the emitted response's argument is single-quoted") {
			REQUIRE(r.Pack() == R"(tag FLOAD 'C:\Users\Test\Music\Bound 4 Da Reload (Casualty).mp3')");
		}
	}
}
