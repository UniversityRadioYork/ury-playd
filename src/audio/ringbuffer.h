// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * The Ring_buffer class.
 */

#ifndef PLAYD_RING_BUFFER_HPP
#define PLAYD_RING_BUFFER_HPP

#include <atomic>
#include <mutex>

#undef max
#include "../gsl/gsl"


/**
 * A concurrent ring buffer.
 * 
 * This is not particularly efficient, but does the job for playd.
 * It uses two release-acquire-atomic counters to store read and write.
 */
class Ring_buffer
{
public:
	/**
	 * Constructs a Ring_buffer.
	 * @param capacity The capacity of the ring buffer, in bytes.
	 */
	Ring_buffer(size_t capacity);

	/// Destructs a Ring_buffer.
	~Ring_buffer();

	/// Deleted copy constructor.
	Ring_buffer(const Ring_buffer &) = delete;

	/// Deleted copy-assignment.
	Ring_buffer &operator=(const Ring_buffer &) = delete;

	/**
	 * The current write capacity.
	 * @return The number of samples this ring buffer has space to store.
	 * @see Write
	 */
	size_t WriteCapacity() const;

	/**
	 * The current read capacity.
	 * @return The number of samples available in this ring buffer.
	 * @see Read
	 */
	size_t ReadCapacity() const;

	/**
	 * Writes samples from a span into the ring buffer.
	 *
	 * * Precondition: @a src is a valid span.
	 * * Postcondition: The ringbuffer has been written to with the contents
	 *     of the first WriteCapacity() bytes of @a src.
	 *
	 * @param src The span of bytes to write into the ring buffer.
	 * @return The number of bytes written.
	 * @see WriteCapacity
	 */
	size_t Write(const gsl::span<const uint8_t> src);

	/**
	 * Reads samples from the ring buffer into an array.
	 * To read one sample, pass a count of 1 and take a pointer to the
	 * sample variable.
	 *
	 * * Precondition: @a dest is a valid span.
	 * * Postcondition: The first ReadCapacity() bytes of @a dest have been
	 *     filled with the appropriate number of bytes from the front of the
	 *     ring buffer.
	 *
	 * @param dest The span of bytes to fill with bytes read from the ring
	 *  buffer.
	 * @return The number of bytes read.
	 * @see ReadCapacity
	 */
	size_t Read(const gsl::span<uint8_t> dest);

	/// Empties the ring buffer.
	void Flush();

private:
	/// Empties the ring buffer without acquiring locks.
	void FlushInner();

    std::vector<uint8_t> buffer;  ///< The array used by the ringbuffer.

    std::vector<uint8_t>::const_iterator r_it;  ///< The read iterator.
    std::vector<uint8_t>::iterator w_it;  ///< The write iterator.
    
    std::atomic<size_t> count;  ///< The current read capacity.
    // Write capacity is the total buffer capacity minus count.

	std::mutex r_lock; ///< The read lock.
	std::mutex w_lock; ///< The write lock.
};

#endif // PLAYD_RINGBUFFER_HPP
