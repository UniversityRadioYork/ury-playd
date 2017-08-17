// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the Ring_buffer class.
 */

#include <atomic>
#include <cassert>
#include <gsl/gsl>
#include <mutex>
#include <vector>

#include "../errors.h"
#include "../messages.h"
#include "ringbuffer.h"

/* Assumptions:

   1) single producer, single consumer, enforced by locks.
      - when reading, count can only increase (read capacity can only increase)
      - when writing, count can only decrease (write capacity can only increase)
      - only the reader can move the read pointer, and it may do so
   non-atomically
      - only the writer can move the write pointer, and it may do so
   non-atomically 2) capacities always underestimate
      - when reading, ATOMICALLY decrease count (increase read capacity) AFTER
   read read capacity may be lower than actual
      - when writing, ATOMICALLY increase count (read capacity) AFTER write
                      write capacity may be lower than actual
      - always atomically read capacities */

Ring_buffer::Ring_buffer(size_t capacity)
{
	this->buffer = std::vector<uint8_t>(capacity);
	this->r_it = this->buffer.cbegin();
	this->w_it = this->buffer.begin();

	// Only one thread can hold this at the moment, so we're ok to
	// do this without locks.
	this->FlushInner();

	Ensures(ReadCapacity() == 0);
	Ensures(WriteCapacity() == capacity);
}

Ring_buffer::~Ring_buffer()
{
}

inline size_t Ring_buffer::ReadCapacity() const
{
	/* Acquire order here means two things:
	 *
	 * 1) no other loads in the thread checking WriteCapacity (ie the
	 *    producer) can be ordered before it;
	 * 2) this load sees all 'release' writes in other threads (usually
	 *    consumers increasing the write capacity).
	 */
	return this->count.load(std::memory_order_acquire);
}

inline size_t Ring_buffer::WriteCapacity() const
{
	return this->buffer.size() - ReadCapacity();
}

size_t Ring_buffer::Write(const gsl::span<const uint8_t> src)
{
	// This shouldn't be called with an empty (or backwards!) span.
	auto src_count = static_cast<size_t>(src.length());
	Expects(0 < src_count);

	/* Acquire write lock to make sure only one write can occur at a given
	 * time, and also that we can't be flushed in the middle of writing.
	 *
	 * This ensures that the write capacity can only go up from when we
	 * read it here.
	 */
	std::lock_guard<std::mutex> w_guard(this->w_lock);

	/* Remember, this is pessimistic:
	 * the write capacity can be increased after this point by a consumer.
	 */
	auto write_capacity_estimate = WriteCapacity();
	if (write_capacity_estimate < src_count)
		throw Internal_error("ringbuffer overflow");

	// Trim the span down to the amount we can write.
	auto write_count = std::min(write_capacity_estimate, src_count);
	auto src_to_write = src.first(write_count);

	/* At this stage, we're the only thread that can be accessing this part
	 * of the buffer, so we can proceed non-atomically.  The release we do
	 * at the end makes our changes available to the consumer.
	 */
	// Ringbuffers loop, so how many bytes can we store until we have to
	// loop?
	auto bytes_to_end = distance(this->w_it, this->buffer.end());
	// Make sure we got the iterators the right way round.
	assert(0 <= bytes_to_end);

	auto write_end_count =
	        std::min(write_count, static_cast<size_t>(bytes_to_end));

	auto src_end = src_to_write.first(write_end_count);
	this->w_it = copy(src_end.cbegin(), src_end.cend(), this->w_it);

	// Do we need to loop?  If so, do that.
	auto write_start_count = write_count - write_end_count;
	if (0 < write_start_count) {
		Expects(this->w_it == this->buffer.end());
		auto src_start = src_to_write.last(write_start_count);
		this->w_it = copy(src_start.cbegin(), src_start.cend(),
		                  this->buffer.begin());
		Ensures(this->w_it > this->buffer.begin());
	}

	/* Now tell the consumer it can read some more data (this HAS to be done
	 * here, to avoid the consumer over-reading).
	 *
	 * Because the other thread might have moved count since we last checked
	 * it, this needs to be acquire-release.
	 */
	this->count.fetch_add(write_count, std::memory_order_acq_rel);

	Ensures(write_start_count + write_end_count == write_count);
	return write_count;
}

size_t Ring_buffer::Read(gsl::span<uint8_t> dest)
{
	auto dest_count = static_cast<size_t>(dest.length());
	Expects(0 < dest_count);

	/* Acquire read lock to make sure only one read can occur at a given
	 * time, and also that we can't be flushed in the middle of reading.
	 *
	 * This ensures that the read capacity can only go up from when we
	 * read it here.
	 */
	std::lock_guard<std::mutex> r_guard(this->r_lock);

	/* Remember, this is pessimistic:
	 * the read capacity can be increased after this point by a producer.
	 */
	auto read_capacity_estimate = ReadCapacity();
	if (read_capacity_estimate < dest_count)
		throw Internal_error("ringbuffer underflow");

	/* See Write() for explanatory comments on what happens here:
	 * the two functions mirror each other almost perfectly.
	 */

	auto read_count = std::min(read_capacity_estimate, dest_count);
	auto dest_to_read = dest.first(read_count);

	auto bytes_to_end = distance(this->r_it, this->buffer.cend());
	assert(0 <= bytes_to_end);

	auto read_end_count =
	        std::min(read_count, static_cast<size_t>(bytes_to_end));
	auto rest = copy_n(this->r_it, read_end_count, dest_to_read.begin());
	this->r_it += read_end_count;

	auto read_start_count = read_count - read_end_count;
	if (0 < read_start_count) {
		Expects(this->r_it == this->buffer.cend());
		copy_n(this->buffer.cbegin(), read_start_count, rest);
		this->r_it = this->buffer.cbegin() + read_start_count;
		Ensures(this->r_it > this->buffer.cbegin());
	}

	if (this->count.fetch_sub(read_count, std::memory_order_acq_rel) <
	    read_count)
		throw Internal_error("capacity decreased unexpectedly");

	Ensures(read_start_count + read_end_count == read_count);
	return read_count;
}

void Ring_buffer::Flush()
{
	// Make sure nothing is reading or writing.
	std::lock_guard<std::mutex> r_guard(this->r_lock);
	std::lock_guard<std::mutex> w_guard(this->w_lock);

	FlushInner();

	Ensures(this->ReadCapacity() == 0);
}

inline void Ring_buffer::FlushInner()
{
	this->count.store(0, std::memory_order_release);
}
