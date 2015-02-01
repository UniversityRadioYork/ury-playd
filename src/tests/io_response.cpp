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
#include "../response.hpp"

SCENARIO("Responses correctly escape arguments with single quotes", "[response]") {
	WHEN("the Response is fed no arguments") {
		auto r = Response(Response::Code::OHAI);

		THEN("the emitted response has no quoting") {
			REQUIRE(r.Pack() == "OHAI");
		}
	}

	WHEN("the Response is fed a single argument with no quotes") {
		auto r = Response(Response::Code::OHAI).AddArg("ulyoath");

		THEN("the emitted response's argument is not quoted") {
			REQUIRE(r.Pack() == "OHAI ulyoath");
		}
	}

	WHEN("the Response is fed a single argument with single quotes") {
		auto r = Response(Response::Code::OHAI).AddArg("chattur'gha");

		THEN("the emitted response's argument is single-quoted") {
			REQUIRE(r.Pack() == R"(OHAI 'chattur'\''gha')");
		}
	}

	WHEN("the Response is fed two arguments, both with single quotes") {
		auto r = Response(Response::Code::OHAI).AddArg("chattur'gha").AddArg("xel'lotath");

		THEN("the emitted response's arguments are both single-quoted") {
			REQUIRE(r.Pack() == R"(OHAI 'chattur'\''gha' 'xel'\''lotath')");
		}
	}

	WHEN("the Response is fed two arguments, one quoted, one unquoted") {
		auto r = Response(Response::Code::OHAI).AddArg("chattur'gha").AddArg("ulyoath");

		THEN("the emitted response's arguments are quoted accordingly") {
			REQUIRE(r.Pack() == R"(OHAI 'chattur'\''gha' ulyoath)");
		}
	}

	WHEN("the Response is fed a single argument with double quotes") {
		auto r = Response(Response::Code::FILE).AddArg(R"("scare"-quotes)");

		THEN("the emitted response's argument is single-quoted") {
			REQUIRE(r.Pack() == R"(FILE '"scare"-quotes')");
		}
	}

	WHEN("the Response is fed a single argument with whitespace") {
		auto r = Response(Response::Code::END).AddArg("pargon pargon pargon");

		THEN("the emitted response's argument is single-quoted") {
			REQUIRE(r.Pack() == R"(END 'pargon pargon pargon')");
		}
	}

	WHEN("the Response is fed several arguments with differing whitespace") {
		auto r = Response(Response::Code::END).AddArg("a space").AddArg("new\nline").AddArg("tab\tstop");

		THEN("the emitted response's argument is single-quoted") {
			REQUIRE(r.Pack() == "END 'a space' 'new\nline' 'tab\tstop'");
		}
	}

	WHEN("the Response is fed a representative example with backslashes and spaces") {
		auto r = Response(Response::Code::FILE).AddArg(R"(C:\Users\Test\Music\Bound 4 Da Reload (Casualty).mp3)");

		THEN("the emitted response's argument is single-quoted") {
			REQUIRE(r.Pack() == R"(FILE 'C:\Users\Test\Music\Bound 4 Da Reload (Casualty).mp3')");
		}
	}
}
