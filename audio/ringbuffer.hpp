// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * The RingBuffer class template.
 */

#ifndef PS_RINGBUFFER_HPP
#define PS_RINGBUFFER_HPP

extern "C" {
#include "../contrib/pa_ringbuffer.h"
}

#include "../errors.hpp"
#include "../messages.h"

/**
 * A ring buffer.
 * This ring buffer is based on the PortAudio ring buffer, provided in the
 * contrib/ directory.
 * This is stable and performs well, but, as it is C code, necessitates some
 * hoop jumping to integrate and could do with being replaced with a native
 * solution.
 */
template <typename RepT, typename SampleCountT>
class RingBuffer {
public:
	/**
	 * Constructs a RingBuffer.
	 * @param power n, where 2^n is the number of elements in the ring
	 *   buffer.
	 * @param size The size of one element in the ring buffer.
	 */
	RingBuffer(int power, int size)
	{
		this->rb = new PaUtilRingBuffer;
		this->buffer = new char[(1 << power) * size];

		int init_result = PaUtil_InitializeRingBuffer(
		                this->rb, size,
		                static_cast<ring_buffer_size_t>(1 << power),
		                this->buffer);
		if (init_result != 0) {
			throw new InternalError(MSG_OUTPUT_RINGINIT);
		}
	}

	/// Destructs a PaRingBuffer.
	~RingBuffer()
	{
		assert(this->rb != nullptr);
		delete this->rb;

		assert(this->buffer != nullptr);
		delete[] this->buffer;
	}

	/// Deleted copy constructor.
	RingBuffer(const RingBuffer &) = delete;

	/// Deleted copy-assignment.
	RingBuffer &operator=(const RingBuffer &) = delete;

	/**
	 * The current write capacity.
	 * @return The number of samples this ring buffer has space to store.
	 */
	SampleCountT WriteCapacity() const
	{
		return CountCast(PaUtil_GetRingBufferWriteAvailable(this->rb));
	}

	/**
	 * The current read capacity.
	 * @return The number of samples available in this ring buffer.
	 */
	SampleCountT ReadCapacity() const
	{
		return CountCast(PaUtil_GetRingBufferReadAvailable(this->rb));
	}

	/**
	 * Writes samples from an array into the ring buffer.
	 * To write one sample, pass a count of 1 and take a pointer to the
	 * sample variable.
	 * Note that start pointer is not constant.  This is because the
	 * underlying implementation of the ring buffer might not guarantee
	 * that the array is left untouched.
	 * @param start The start of the array buffer from which we read
	 *   samples.  Must not be nullptr.
	 * @param count The number of samples to write.  This must not exceed
	 *   the minimum of WriteCapacity() and the length of the array.
	 * @return The number of samples written, which should not exceed count.
	 */
	SampleCountT Write(RepT *start, SampleCountT count)
	{
		return CountCast(PaUtil_WriteRingBuffer(
		                this->rb, start,
		                static_cast<ring_buffer_size_t>(count)));
	}

	/**
	 * Reads samples from the ring buffer into an array.
	 * To read one sample, pass a count of 1 and take a pointer to the
	 * sample variable.
	 * @param start The start of the array buffer to which we write samples.
	 *   Must not be nullptr.
	 * @param count The number of samples to read.  This must not exceed the
	 *   minimum of ReadCapacity() and the length of the array.
	 * @return The number of samples read, which should not exceed count.
	 */
	SampleCountT Read(RepT *start, SampleCountT count)
	{
		return CountCast(PaUtil_ReadRingBuffer(
		                this->rb, start,
		                static_cast<ring_buffer_size_t>(count)));
	}

	/// Empties the ring buffer.
	void Flush()
	{
		PaUtil_FlushRingBuffer(this->rb);
	}

private:
	char *buffer;         ///< The array used by the ringbuffer.
	PaUtilRingBuffer *rb; ///< The internal PortAudio ringbuffer.

	/**
	 * Converts a ring buffer size into an external size.
	 * @param count The size/count in PortAudio form.
	 * @return The size/count after casting to SampleCountT.
	 */
	SampleCountT CountCast(ring_buffer_size_t count) const
	{
		return static_cast<SampleCountT>(count);
	}
};

#endif // PS_RINGBUFFER_HPP
