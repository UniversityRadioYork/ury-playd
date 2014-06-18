#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <cassert>

#include <boost/circular_buffer.hpp>
#include "contrib/pa_ringbuffer.h"
#include "errors.hpp"

/**
 * Abstraction over a ring buffer.
 *
 * This generic abstract class represents a general concept of a ring buffer
 * for samples.  It can be implemented, for example, by adapters over the
 * PortAudio or Boost ring buffers.
 *
 * Note that all quantities are represented in terms of sample counts, not
 * the underlying representation RepT.  Implementations should ensure that they
 * implement the virtual methods of this class in terms of the former and not
 * the latter.
 */
template <typename RepT, typename SampleCountT>
class RingBuffer {
public:
	/**
	 * The current write capacity.
	 *
	 * @return The number of samples this ring buffer has space to store.
	 */
	virtual SampleCountT WriteCapacity() const = 0;

	/**
	 * The current read capacity.
	 *
	 * @return The number of samples available in this ring buffer.
	 */
	virtual SampleCountT ReadCapacity() const = 0;

	/**
	 * Writes samples from an array into the ring buffer.
	 *
	 * To write one sample, pass a count of 1 and take a pointer to the
	 * sample variable.
	 *
	 * Note that start pointer is not constant.  This is because the
	 * underlying implementation of the ring buffer might not guarantee
	 * that the array is left untouched.
	 *
	 * @param start The start of the array buffer from which we write
	 *              samples.  Must not be nullptr.
	 * @param count The number of samples to write.  This must not exceed
	 *              the minimum of WriteCapacity() and the length of the
	 *              array.
	 *
	 * @return The number of samples written, which should not exceed count.
	 */
	virtual SampleCountT Write(RepT *start, SampleCountT count) = 0;

	/**
	 * Reads samples from the ring buffer into an array.
	 *
	 * To read one sample, pass a count of 1 and take a pointer to the
	 * sample variable.
	 *
	 * @param start The start of the array buffer to which we write samples.
	 *              Must not be nullptr.
	 * @param count The number of samples to read.  This must not exceed the
	 *              minimum of ReadCapacity() and the length of the array.
	 *
	 * @return The number of samples read, which should not exceed count.
	 */
	virtual SampleCountT Read(RepT *start, SampleCountT count) = 0;

	/**
	 * Empties the ring buffer.
	 */
	virtual void Flush() = 0;
};

/**
 * Implementation of RingBuffer using the Boost circular buffer.
 *
 * This is currently experimental and has known audio glitches when used.
 * Here be undiscovered bugs.
 */
template <typename T1, typename T2, int P>
class BoostRingBuffer : public RingBuffer<T1, T2> {
public:
	BoostRingBuffer(int size = sizeof(T1))
	{
		this->rb = new boost::circular_buffer<char>((1 << P) * size);
		this->size = size;
	}

	~BoostRingBuffer()
	{
		assert(this->rb != nullptr);
		delete this->rb;
	}

	T2 WriteCapacity() const
	{
		return static_cast<T2>(this->rb->reserve() / this->size);
	}

	T2 ReadCapacity() const
	{
		return static_cast<T2>(this->rb->size() / this->size);
	}

	T2 Write(T1 *start, T2 count)
	{
		return OnBuffer(start, count,
		                [this](T1 *e) { this->rb->push_back(e); });
	}

	T2 Read(T1 *start, T2 count)
	{
		return OnBuffer(start, count, [this](T1 *e) {
			*e = this->rb->front();
			this->rb->pop_front();
		});
	}

	void Flush()
	{
		this->rb->clear();
	}

private:
	boost::circular_buffer<T1> *rb; ///< The internal Boost ring buffer.
	int size;                       ///< The size of one sample, in bytes.

	/**
	 * Transfers between this ring buffer and an external array buffer.
	 * @param start  The start of the array buffer.
	 * @param count  The number of samples in the array buffer.
	 * @param f      A function to perform on each position in the array
	 *               buffer.
	 * @return       The number of samples affected in the array buffer.
	 */
	T2 OnBuffer(T1 *start, T2 count, std::function<void(T1 *)> f)
	{
		T2 i;

		for (i = 0; i < count * this->size; i++) {
			f(start + i);
		}

		return (i + 1) / this->size;
	}
};

/**
 * Implementation of RingBuffer using the PortAudio C ring buffer.
 *
 * This is stable and performs well, but, as it is C code, necessitates some
 * hoop jumping to integrate and could do with being replaced with a native
 * solution.
 *
 * The PortAudio ring buffer is provided in the contrib/ directory.
 */
template <typename T1, typename T2, int P>
class PaRingBuffer : public RingBuffer<T1, T2> {
public:
	/**
	 * Constructs a PaRingBuffer.
	 * @param size  The size of one element in the ring buffer.
	 */
	PaRingBuffer(int size = sizeof(T1))
	{
		this->rb = new PaUtilRingBuffer;
		this->buffer = new char[(1 << P) * size];

		if (PaUtil_InitializeRingBuffer(
		                    this->rb, size,
		                    static_cast<ring_buffer_size_t>(1 << P),
		                    this->buffer) != 0) {
			throw Error(ErrorCode::INTERNAL_ERROR,
			            "ringbuf failed to init");
		}
	}

	~PaRingBuffer()
	{
		assert(this->rb != nullptr);
		delete this->rb;

		assert(this->buffer != nullptr);
		delete[] this->buffer;
	}

	T2 WriteCapacity() const
	{
		return CountCast(PaUtil_GetRingBufferWriteAvailable(this->rb));
	}

	T2 ReadCapacity() const
	{
		return CountCast(PaUtil_GetRingBufferReadAvailable(this->rb));
	}

	T2 Write(T1 *start, T2 count)
	{
		return CountCast(PaUtil_WriteRingBuffer(
		                this->rb, start,
		                static_cast<ring_buffer_size_t>(count)));
	}

	T2 Read(T1 *start, T2 count)
	{
		return CountCast(PaUtil_ReadRingBuffer(
		                this->rb, start,
		                static_cast<ring_buffer_size_t>(count)));
	}

	void Flush()
	{
		PaUtil_FlushRingBuffer(this->rb);
	}

private:
	char *buffer;         ///< The array used by the ringbuffer.
	PaUtilRingBuffer *rb; ///< The internal PortAudio ringbuffer.

	/**
	 * Converts a ring buffer size into an external size.
	 * @param count  The size/count in PortAudio form.
	 * @return       The size/count after casting to T2.
	 */
	T2 CountCast(ring_buffer_size_t count) const
	{
		return static_cast<T2>(count);
	}
};

#endif // RINGBUFFER_H
