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

#endif // PS_RINGBUFFER_BOOST
