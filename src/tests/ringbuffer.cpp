// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for the RingBuffer class.
 */

#include "catch.hpp"

#include "../errors.h"
#include "../audio/ringbuffer.h"

SCENARIO("RingBuffer cannot be read from when empty", "[ringbuffer]") {
	GIVEN("an empty RingBuffer and properly sized buffer") {
		RingBuffer rb(1<<5);
		char buf[1 << 5];

		WHEN("a read is called for one item") {
			THEN("an InternalError is raised") {
				REQUIRE_THROWS_AS(rb.Read(buf, 1), InternalError);
			}
		}
		WHEN("a read is called for 2^power items") {
			THEN("an InternalError is raised") {
				REQUIRE_THROWS_AS(rb.Read(buf, 1<<5), InternalError);
			}
		}
		WHEN("a read is called for 2^power + 1 items") {
			THEN("an InternalError is raised") {
				REQUIRE_THROWS_AS(rb.Read(buf, (1<<5) + 1), InternalError);
			}
		}
	}
}

SCENARIO("RingBuffer cannot be written to when full", "[ringbuffer]") {
	GIVEN("an full RingBuffer") {
		RingBuffer rb(1<<5);
		const char *msg = "this message is 2^5 chars long!\0this bit isn't\0";
		rb.Write(msg, 1<<5);

		WHEN("a write is called for one item") {
			THEN("an InternalError is raised") {
				REQUIRE_THROWS_AS(rb.Write("x", 1), InternalError);
			}
		}
		WHEN("a write is called for 2^power items") {
			THEN("an InternalError is raised") {
				REQUIRE_THROWS_AS(rb.Write(msg, 1 << 5), InternalError);
			}
		}
		WHEN("a write is called for 2^power + 1 items") {
			THEN("an InternalError is raised") {
				REQUIRE_THROWS_AS(rb.Write(msg, (1 << 5) + 1), InternalError);
			}
		}
	}
}

SCENARIO("RingBuffer reports capacities correctly", "[ringbuffer]") {
	GIVEN("an empty RingBuffer and properly sized buffer") {
		RingBuffer rb(1<<5);
		char buf[1<<5];
		const char *msg = "this message is 2^5 chars long!\0this bit isn't\0";

		WHEN("nothing is written") {
			THEN("ReadCapacity() is 0") {
				REQUIRE(rb.ReadCapacity() == 0);
			}
			THEN("WriteCapacity() is 2^power") {
				REQUIRE(rb.WriteCapacity() == 1<<5);
			}
		}
		WHEN("the buffer is partially written to") {
			rb.Write(msg, 16);

			THEN("ReadCapacity() is the amount written") {
				REQUIRE(rb.ReadCapacity() == 16);
			}
			THEN("WriteCapacity() is 2^power - the amount written") {
				REQUIRE(rb.WriteCapacity() == (1<<5) - 16);
			}

			AND_WHEN("the buffer is fully read from") {
				rb.Read(buf, 16);

				THEN("ReadCapacity() is 0") {
					REQUIRE(rb.ReadCapacity() == 0);
				}
				THEN("WriteCapacity() is 2^power") {
					REQUIRE(rb.WriteCapacity() == 1<<5);
				}
			}

			AND_WHEN("the buffer is flushed") {
				rb.Flush();

				THEN("ReadCapacity() is 0") {
					REQUIRE(rb.ReadCapacity() == 0);
				}
				THEN("WriteCapacity() is 2^power") {
					REQUIRE(rb.WriteCapacity() == 1<<5);
				}
			}
		}
		WHEN("the buffer is filled") {
			rb.Write(msg, 1<<5);

			THEN("ReadCapacity() is 2^power") {
				REQUIRE(rb.ReadCapacity() == 1<<5);
			}
			THEN("WriteCapacity() is 0") {
				REQUIRE(rb.WriteCapacity() == 0);
			}

			AND_WHEN("the buffer is flushed") {
				rb.Flush();

				THEN("ReadCapacity() is 0") {
					REQUIRE(rb.ReadCapacity() == 0);
				}
				THEN("WriteCapacity() is 2^power") {
					REQUIRE(rb.WriteCapacity() == 1<<5);
				}
			}
		}
	}
}
