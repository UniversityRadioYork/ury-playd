// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * The BoostRingBuffer class template.
 * @see ringbuffer/ringbuffer.hpp
 * @see ringbuffer/ringbuffer_pa.hpp
 */

#ifndef PS_RINGBUFFER_BOOST_HPP
#define PS_RINGBUFFER_BOOST_HPP

#include <boost/circular_buffer.hpp>
#include "ringbuffer.hpp"

/**
 * Implementation of RingBuffer using the Boost circular buffer.
 *
 * This is currently experimental and has known audio glitches when used.
 * Here be undiscovered bugs.
 */
template <typename T1, typename T2, int P>
class BoostRingBuffer : public RingBuffer<T1, T2> {
public:
	using InternalBuffer = boost::circular_buffer<T1>;
	using InternalBufferPtr = std::unique_ptr<InternalBuffer>;

	/**
	 * Constructs a BoostRingBuffer.
	 * @param size  The size of one element in the ring buffer.
	 */
	BoostRingBuffer(int size)
	{
		this->rb = InternalBufferPtr(new InternalBuffer((1 << P) * size));
		this->size = size;
	}

	T2 WriteCapacity() const override
	{
		return static_cast<T2>(this->rb->reserve() / this->size);
	}

	T2 ReadCapacity() const override
	{
		return static_cast<T2>(this->rb->size() / this->size);
	}

	T2 Write(T1 *start, T2 count) override
	{
		return OnBuffer(start, count,
		                [this](T1 *e) { this->rb->push_back(*e); });
	}

	T2 Read(T1 *start, T2 count) override
	{
		return OnBuffer(start, count, [this](T1 *e) {
			*e = this->rb->front();
			this->rb->pop_front();
		});
	}

	void Flush() override
	{
		this->rb->clear();
	}

private:
	InternalBufferPtr rb; ///< The internal Boost ring buffer.
	int size;             ///< The size of one sample, in bytes.

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
		T1 *end = start + (count * this->size);

		for (T1 *ptr = start; ptr < end; ptr++) {
			f(ptr);
		}

		return count;
	}
};

#endif // PS_RINGBUFFER_BOOST
