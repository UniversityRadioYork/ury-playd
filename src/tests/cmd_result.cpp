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


SCENARIO("CommandResult's convenience constructors create correct CommandResults", "[command-result]") {
	GIVEN("A CommandResult created via CommandResult::Success()") {
		CommandResult c = CommandResult::Success();

		WHEN("IsSuccess() is called") {
			bool success = c.IsSuccess();

			THEN("the result is true (the result is a success)") {
				REQUIRE(success);
			}
		}

		GIVEN("A dummy ResponseSink and command") {
			std::ostringstream os;
			DummyResponseSink d(os);
			std::vector<std::string> cmd({ "OHAI", "testy test" });

			WHEN("Emit(cmd) is called") {
				c.Emit(d, cmd);

				THEN("the response is OK followed by the command words") {
					REQUIRE(os.str() == "OK OHAI 'testy test'\n");
				}
			}
		}
	}

	GIVEN("A CommandResult created via CommandResult::Invalid()") {
		CommandResult c = CommandResult::Invalid("PEBCAK error");

		WHEN("IsSuccess() is called") {
			bool success = c.IsSuccess();

			THEN("the result is false (the result is not a success)") {
				REQUIRE(!success);
			}
		}

		GIVEN("A dummy ResponseSink and command") {
			std::ostringstream os;
			DummyResponseSink d(os);
			std::vector<std::string> cmd({ "OHAI", "testy test" });

			WHEN("Emit(cmd) is called") {
				c.Emit(d, cmd);

				THEN("the response is WHAT followed by the failure message and command") {
					REQUIRE(os.str() == "WHAT 'PEBCAK error' OHAI 'testy test'\n");
				}
			}
		}
	}


	GIVEN("A CommandResult created via CommandResult::Failure()") {
		CommandResult c = CommandResult::Failure("lp0 on fire");

		WHEN("IsSuccess() is called") {
			bool success = c.IsSuccess();

			THEN("the result is false (the result is not a success)") {
				REQUIRE(!success);
			}
		}

		GIVEN("A dummy ResponseSink and command") {
			std::ostringstream os;
			DummyResponseSink d(os);
			std::vector<std::string> cmd({ "OHAI", "testy test" });

			WHEN("Emit(cmd) is called") {
				c.Emit(d, cmd);

				THEN("the response is FAIL followed by the failure message") {
					REQUIRE(os.str() == "FAIL 'lp0 on fire' OHAI 'testy test'\n");
				}
			}
		}
	}
}
