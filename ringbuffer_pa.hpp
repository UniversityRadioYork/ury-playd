#ifndef PS_RINGBUFFER_PA_HPP
#define PS_RINGBUFFER_PA_HPP

#include <cassert>

extern "C" {
#include "contrib/pa_ringbuffer.h"
}

#include "errors.hpp"


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

#endif // PS_RINGBUFFER_PA_HPP
