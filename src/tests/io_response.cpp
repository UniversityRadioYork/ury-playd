// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Tests for response classes.
 */

#include <ostream>
#include <sstream>
#include <string>

#include "catch.hpp"
#include "../io/io_response.hpp"

// We need a dummy class in order to check the ResponseSink class.
#include "io_response.hpp"

DummyResponseSink::DummyResponseSink(std::ostream &os) : os(os)
{
}

void DummyResponseSink::RespondRaw(const std::string &response) const
{
	this->os << response;
}

SCENARIO("ResponseSinks correctly escape arguments with singLe quotes",
         "[response-sink]")
{
	GIVEN("A dummy ResponseSink")
	{
		std::ostringstream os;
		DummyResponseSink rs(os);

		WHEN("the ResponseSink is fed no arguments")
		{
			rs.RespondArgs(ResponseCode::OHAI, {});

			THEN("the emitted response has no quoting")
			{
				REQUIRE(os.str() == "OHAI");
			}
		}

		WHEN("the ResponseSink is fed a single argument with no quotes")
		{
			rs.Respond(ResponseCode::OHAI, "ulyoath");

			THEN("the emitted response's argument is not quoted")
			{
				REQUIRE(os.str() == "OHAI ulyoath");
			}
		}

		WHEN("the ResponseSink is fed a single argument with single "
		     "quotes")
		{
			rs.Respond(ResponseCode::OHAI, "chattur'gha");

			THEN("the emitted response's argument is single-quoted")
			{
				REQUIRE(os.str() == R"(OHAI 'chattur'\''gha')");
			}
		}

		WHEN("the ResponseSink is fed two arguments, both with single "
		     "quotes")
		{
			rs.RespondArgs(ResponseCode::OHAI,
			               { "chattur'gha", "xel'lotath" });

			THEN("the emitted response's arguments are both "
			     "single-quoted")
			{
				REQUIRE(os.str() == R"(OHAI 'chattur'\''gha' )"
				                    R"('xel'\''lotath')");
			}
		}

		WHEN("the ResponseSink is fed two arguments, one quoted, one "
		     "unquoted")
		{
			rs.RespondArgs(ResponseCode::OHAI,
			               { "chattur'gha", "ulyoath" });

			THEN("the emitted response's arguments are quoted "
			     "accordingly")
			{
				REQUIRE(os.str() ==
				        R"(OHAI 'chattur'\''gha' ulyoath)");
			}
		}

		WHEN("the ResponseSink is fed a single argument with double "
		     "quotes")
		{
			rs.Respond(ResponseCode::FILE, R"("scare"-quotes)");

			THEN("the emitted response's argument is single-quoted")
			{
				REQUIRE(os.str() == R"(FILE '"scare"-quotes')");
			}
		}

		WHEN("the ResponseSink is fed a single argument with "
		     "whitespace")
		{
			rs.Respond(ResponseCode::END, "pargon pargon pargon");

			THEN("the emitted response's argument is single-quoted")
			{
				REQUIRE(os.str() ==
				        R"(END 'pargon pargon pargon')");
			}
		}

		WHEN("the ResponseSink is fed several arguments with differing "
		     "whitespace")
		{
			rs.RespondArgs(ResponseCode::END,
			               { "a space", "new\nline", "tab\tstop" });

			THEN("the emitted response's argument is single-quoted")
			{
				REQUIRE(os.str() ==
				        "END 'a space' 'new\nline' "
				        "'tab\tstop'");
			}
		}

		WHEN("the ResponseSink is fed a representative example with "
		     "backslashes and spaces")
		{
			rs.Respond(ResponseCode::FILE,
			           R"(C:\Users\Test\Music\Bound 4 Da Reload )"
			           R"((Casualty).mp3)");

			THEN("the emitted response's argument is single-quoted")
			{
				REQUIRE(os.str() ==
				        R"(FILE 'C:\Users\Test\Music\Bound 4 )"
				        R"(Da Reload (Casualty).mp3')");
			}
		}
	}
}
