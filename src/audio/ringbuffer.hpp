// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * The RingBuffer class template.
 */

#ifndef PLAYD_RINGBUFFER_HPP
#define PLAYD_RINGBUFFER_HPP

extern "C" {
#include "../contrib/pa_ringbuffer/pa_ringbuffer.h"
}

/**
 * A ring buffer.
 * This ring buffer is based on the PortAudio ring buffer, provided in the
 * contrib/ directory.
 * This is stable and performs well, but, as it is C code, necessitates some
 * hoop jumping to integrate and could do with being replaced with a native
 * solution.
 */
class RingBuffer
{
public:
	/**
	 * Constructs a RingBuffer.
	 *
	 * * Precondition: 0 < power, 0 < size.
	 * * Postcondition: The RingBuffer is fully initialised.
	 *
	 * @param power n, where 2^n is the number of elements in the ring
	 *   buffer.
	 * @param size The size of one element in the ring buffer.
	 */
	RingBuffer(int power, int size);

	/// Destructs a PaRingBuffer.
	~RingBuffer();

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
	 * Writes samples from an array into the ring buffer.
	 * To write one sample, pass a count of 1 and take a pointer to the
	 * sample variable.
	 * Note that start pointer is not constant.  This is because the
	 * underlying implementation of the ring buffer might not guarantee
	 * that the array is left untouched.
	 *
	 * * Precondition: start points to a valid array, 0 < count <= the size
	 *     of the block of memory pointed to by start, count <=
	 *     WriteCapacity().
	 * * Postcondition: The ringbuffer has been written to with the contents
	 *     of the memory pointed to by start and bounded by count and
	 *     WriteCapacity().
	 *
	 * @param start The start of the array buffer from which we read
	 *   samples.  Must not be nullptr.
	 * @param count The number of samples to write.  This must not exceed
	 *   the minimum of WriteCapacity() and the length of the array.
	 * @return The number of samples written, which should not exceed count.
	 * @see WriteCapacity
	 */
	unsigned long Write(const char *start, unsigned long count);

	/**
	 * Reads samples from the ring buffer into an array.
	 * To read one sample, pass a count of 1 and take a pointer to the
	 * sample variable.
	 *
	 * * Precondition: start points to a valid array, 0 < count <= the size
	 *     of the block of memory pointed to by start, count <=
	 *     ReadCapacity().
	 * * Postcondition: The block of memory pointed to by start and bounded
	 *     by count and ReadCapacity() has been filled with the appropriate
	 *     number of samples from the front of the ring buffer.
	 *
	 * @param start The start of the array buffer to which we write samples.
	 *   Must not be nullptr.
	 * @param count The number of samples to read.  This must not exceed the
	 *   minimum of ReadCapacity() and the length of the array.
	 * @return The number of samples read, which should not exceed count.
	 * @see ReadCapacity
	 */
	unsigned long Read(char *start, unsigned long count);

	/// Empties the ring buffer.
	void Flush();

private:
	char *buffer;         ///< The array used by the ringbuffer.
	PaUtilRingBuffer *rb; ///< The internal PortAudio ringbuffer.

	/**
	 * Converts a ring buffer size into an external size.
	 * @param count The size/count in PortAudio form.
	 * @return The size/count after casting to unsigned long.
	 */
	static unsigned long CountCast(ring_buffer_size_t count);
};

#endif // PLAYD_RINGBUFFER_HPP
