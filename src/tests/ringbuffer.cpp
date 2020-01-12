// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for the Ring_buffer class.
 */

#include "../audio/ringbuffer.h"

#include <gsl/gsl>

#include "../errors.h"
#include "catch.hpp"

namespace Playd::Tests
{
SCENARIO ("Ring buffer cannot be read from when empty", "[ringbuffer]") {
	GIVEN ("an empty ring buffer and properly sized buffer") {
		constexpr int cap{32};
		Audio::RingBuffer rb{cap};
		std::byte buf[cap];

		WHEN ("a read is called for one item") {
			THEN ("an InternalError is raised") {
				REQUIRE_THROWS_AS(rb.Read(gsl::span<std::byte>{buf, 1}), InternalError);
			}
		}
		WHEN ("a read is called for 'cap' items") {
			THEN ("an InternalError is raised") {
				REQUIRE_THROWS_AS(rb.Read(gsl::span<std::byte>{buf, cap}), InternalError);
			}
		}
		WHEN ("a read is called for 'cap' + 1 items") {
			THEN ("an InternalError is raised") {
				REQUIRE_THROWS_AS(rb.Read(gsl::span<std::byte>{buf, cap + 1}), InternalError);
			}
		}
	}
}

SCENARIO ("Ring buffer cannot be written to when full", "[ringbuffer]") {
	constexpr int cap{32};

	GIVEN ("a full ring buffer") {
		Audio::RingBuffer rb{cap};
		gsl::czstring<> msg{"this message is 2^5 chars long!\0this bit isn't\0"};
		// TODO(MattWindsor91): this is technically not portable in the slightest.
		auto m8 = reinterpret_cast<const std::byte *>(msg);

		rb.Write(gsl::span<const std::byte>{m8, cap});

		WHEN ("a write is called for one item") {
			THEN ("an internal error is raised") {
				REQUIRE_THROWS_AS(rb.Write(gsl::span<const std::byte>(m8, 1)), InternalError);
			}
		}
		WHEN ("a write is called for 'cap' items") {
			THEN ("an internal error is raised") {
				REQUIRE_THROWS_AS(rb.Write(gsl::span<const std::byte>{m8, cap}), InternalError);
			}
		}
		WHEN ("a write is called for 'cap' + 1 items") {
			THEN ("an internal error is raised") {
				REQUIRE_THROWS_AS(rb.Write(gsl::span<const std::byte>{m8, cap + 1}), InternalError);
			}
		}
	}
}

SCENARIO ("Ring buffer reports capacities correctly", "[ringbuffer]") {
	GIVEN ("an empty ring buffer and properly sized buffer") {
		constexpr int cap{32};
		Audio::RingBuffer rb{cap};
		std::byte buf[cap];
		gsl::czstring<> msg{"this message is 2^5 chars long!\0this bit isn't\0"};
		// TODO(MattWindsor91): this is technically not portable in the slightest.
		auto m8 = reinterpret_cast<const std::byte *>(msg);

		WHEN ("nothing is written") {
			THEN ("ReadCapacity() is 0") {
				REQUIRE(rb.ReadCapacity() == 0);
			}
			THEN ("WriteCapacity() is 'cap'") {
				REQUIRE(rb.WriteCapacity() == cap);
			}
		}
		WHEN ("the buffer is partially written to") {
			constexpr int amt{16};
			static_assert(amt < cap, "should be a partial write");

			rb.Write(gsl::span<const std::byte>{m8, amt});

			THEN ("ReadCapacity() is the amount written") {
				REQUIRE(rb.ReadCapacity() == amt);
			}
			THEN ("WriteCapacity() is 'cap' - the amount written") {
				REQUIRE(rb.WriteCapacity() == cap - amt);
			}

			AND_WHEN("the buffer is fully read from")
			{
				rb.Read(gsl::span<std::byte>{buf, cap - amt});

				THEN ("ReadCapacity() is 0") {
					REQUIRE(rb.ReadCapacity() == 0);
				}
				THEN ("WriteCapacity() is 'cap'") {
					REQUIRE(rb.WriteCapacity() == cap);
				}
			}

			AND_WHEN("the buffer is flushed")
			{
				rb.Flush();

				THEN ("ReadCapacity() is 0") {
					REQUIRE(rb.ReadCapacity() == 0);
				}
				THEN ("WriteCapacity() is 'cap'") {
					REQUIRE(rb.WriteCapacity() == cap);
				}
			}
		}
		WHEN ("the buffer is filled") {
			rb.Write(gsl::span<const std::byte>{m8, cap});

			THEN ("ReadCapacity() is 'cap'") {
				REQUIRE(rb.ReadCapacity() == cap);
			}
			THEN ("WriteCapacity() is 0") {
				REQUIRE(rb.WriteCapacity() == 0);
			}

			AND_WHEN("the buffer is flushed")
			{
				rb.Flush();

				THEN ("ReadCapacity() is 0") {
					REQUIRE(rb.ReadCapacity() == 0);
				}
				THEN ("WriteCapacity() is 'cap'") {
					REQUIRE(rb.WriteCapacity() == cap);
				}
			}
		}
	}
}

} // namespace playd::tests
