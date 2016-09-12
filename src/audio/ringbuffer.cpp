// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the RingBuffer class.
 */

#include <cassert>

extern "C" {
#include "../contrib/pa_ringbuffer/pa_ringbuffer.h"
}

#include "../errors.hpp"
#include "../messages.h"
#include "ringbuffer.hpp"

RingBuffer::RingBuffer(int power, int size)
{
	if (power <= 0) throw InternalError("ringbuffer power must be positive");
	if (size <= 0) throw InternalError("ringbuffer element size must be positive");

	this->rb = new PaUtilRingBuffer;
	this->buffer = new char[(1 << power) * size];

	if (PaUtil_InitializeRingBuffer(
	            this->rb, size, static_cast<ring_buffer_size_t>(1 << power),
	            this->buffer) != 0) {
		throw new InternalError(MSG_OUTPUT_RINGINIT);
	}

	assert(this->rb != nullptr);
	assert(this->buffer != nullptr);
}

RingBuffer::~RingBuffer()
{
	assert(this->rb != nullptr);
	delete this->rb;

	assert(this->buffer != nullptr);
	delete[] this->buffer;
}

size_t RingBuffer::WriteCapacity() const
{
	return CountCast(PaUtil_GetRingBufferWriteAvailable(this->rb));
}

size_t RingBuffer::ReadCapacity() const
{
	return CountCast(PaUtil_GetRingBufferReadAvailable(this->rb));
}

unsigned long RingBuffer::Write(const char *start, unsigned long count)
{
	if (count == 0) throw InternalError("tried to store 0 items in ringbuffer");
	if (WriteCapacity() < count) throw InternalError("ringbuffer overflow");

	return CountCast(PaUtil_WriteRingBuffer(
	        this->rb, start, static_cast<ring_buffer_size_t>(count)));
}

unsigned long RingBuffer::Read(char *start, unsigned long count)
{
	if (count == 0) throw InternalError("tried to read 0 items from ringbuffer");
	if (ReadCapacity() < count) throw InternalError("ringbuffer underflow");

	return CountCast(PaUtil_ReadRingBuffer(
	        this->rb, start, static_cast<ring_buffer_size_t>(count)));
}

void RingBuffer::Flush()
{
	PaUtil_FlushRingBuffer(this->rb);
	assert(this->ReadCapacity() == 0);
}

/* static */ unsigned long RingBuffer::CountCast(ring_buffer_size_t count)
{
	return static_cast<unsigned long>(count);
}
