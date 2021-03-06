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
#include <vector>

#undef max
#include <gsl/gsl>

namespace Playd::Audio
{
/**
 * A concurrent ring buffer.
 *
 * This is not particularly efficient, but does the job for playd.
 * It uses two release-acquire-atomic counters to store read and write.
 */
class RingBuffer
{
public:
	/**
	 * Constructs a Ring_buffer.
	 * @param capacity The capacity of the ring buffer, in bytes.
	 */
	explicit RingBuffer(size_t capacity);

	/// Destructs a Ring_buffer.
	~RingBuffer() = default;

	/// Deleted copy constructor.
	RingBuffer(const RingBuffer &) = delete;

	/// Deleted copy-assignment.
	RingBuffer &operator=(const RingBuffer &) = delete;

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
	size_t Write(gsl::span<const std::byte> src);

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
	size_t Read(gsl::span<std::byte> dest);

	/// Empties the ring buffer.
	void Flush();

private:
	/// Empties the ring buffer without acquiring locks.
	void FlushInner();

	std::vector<std::byte> buffer; ///< The array used by the ringbuffer.

	std::vector<std::byte>::const_iterator r_it; ///< The read iterator.
	std::vector<std::byte>::iterator w_it;       ///< The write iterator.

	std::atomic<size_t> count; ///< The current read capacity.
	// Write capacity is the total buffer capacity minus count.

	std::mutex r_lock; ///< The read lock.
	std::mutex w_lock; ///< The write lock.
};

} // namespace Playd::Audio

#endif // PLAYD_RINGBUFFER_HPP
