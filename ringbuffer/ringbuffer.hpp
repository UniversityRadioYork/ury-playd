#ifndef PS_RINGBUFFER_HPP
#define PS_RINGBUFFER_HPP

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
	 * Virtual destructor for RingBuffer.
	 */
	virtual ~RingBuffer() {};

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

#endif // PS_RINGBUFFER_HPP
