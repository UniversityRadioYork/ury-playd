// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Tests for response classes.
 */

#include <string>

#include "catch.hpp"
#include "../io/io_response.hpp"

// We need a dummy class in order to check the ResponseSink class.
#include "io_response.hpp"

std::string DummyResponseSink::LastResponse() const
{
	return this->last_response;
}

void DummyResponseSink::RespondRaw(const std::string &response)
{
	this->last_response = std::string(response);
}

SCENARIO("ResponseSinks correctly escape arguments with singLe quotes", "[rs-quote]") {
	GIVEN("A dummy ResponseSink") {
		DummyResponseSink rs;

		WHEN("the ResponseSink is fed no arguments") {
			rs.RespondArgs(ResponseCode::OHAI, {});

			THEN("the emitted response has no quoting") {
				REQUIRE(rs.LastResponse() == "OHAI");
			}
		}

		WHEN("the ResponseSink is fed a single argument with no quotes") {
			rs.Respond(ResponseCode::OHAI, "ulyoath");

			THEN("the emitted response's argument is not quoted") {
				REQUIRE(rs.LastResponse() == "OHAI ulyoath");
			}
		}

		WHEN("the ResponseSink is fed a single argument with single quotes") {
			rs.Respond(ResponseCode::OHAI, "chattur'gha");

			THEN("the emitted response's argument is single-quoted") {
				REQUIRE(rs.LastResponse() == R"(OHAI 'chattur'\''gha')");
			}
		}

		WHEN("the ResponseSink is fed two arguments, both with single quotes") {
			rs.RespondArgs(ResponseCode::OHAI, { "chattur'gha", "xel'lotath" });

			THEN("the emitted response's arguments are both single-quoted") {
				REQUIRE(rs.LastResponse() == R"(OHAI 'chattur'\''gha' 'xel'\''lotath')");
			}
		}

		WHEN("the ResponseSink is fed two arguments, one quoted, one unquoted") {
			rs.RespondArgs(ResponseCode::OHAI, { "chattur'gha", "ulyoath" });

			THEN("the emitted response's arguments are quoted accordingly") {
				REQUIRE(rs.LastResponse() == R"(OHAI 'chattur'\''gha' ulyoath)");
			}
		}

		WHEN("the ResponseSink is fed a single argument with double quotes") {
			rs.Respond(ResponseCode::FILE, R"("scare"-quotes)");

			THEN("the emitted response's argument is single-quoted") {
				REQUIRE(rs.LastResponse() == R"(FILE '"scare"-quotes')");
			}
		}

		WHEN("the ResponseSink is fed a single argument with whitespace") {
			rs.Respond(ResponseCode::END, "pargon pargon pargon");

			THEN("the emitted response's argument is single-quoted") {
				REQUIRE(rs.LastResponse() == R"(END 'pargon pargon pargon')");
			}
		}

		WHEN("the ResponseSink is fed several arguments with differing whitespace") {
			rs.RespondArgs(ResponseCode::END, { "a space", "new\nline", "tab\tstop" });

			THEN("the emitted response's argument is single-quoted") {
				REQUIRE(rs.LastResponse() == "END 'a space' 'new\nline' 'tab\tstop'");
			}
		}

		WHEN("the ResponseSink is fed a representative example with backslashes and spaces") {
			rs.Respond(ResponseCode::FILE, R"(C:\Users\Test\Music\Bound 4 Da Reload (Casualty).mp3)");

			THEN("the emitted response's argument is single-quoted") {
				REQUIRE(rs.LastResponse() == R"(FILE 'C:\Users\Test\Music\Bound 4 Da Reload (Casualty).mp3')");
			}
		}
	}
}
