// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the Swr class.
 * @see swr.hpp
 */

#include <cassert>
#include <new>

#include "swr.hpp"

extern "C" {
#include <libswresample/swresample.h>
}

Swr::Swr(std::int64_t out_ch_layout, AVSampleFormat out_sample_fmt,
         int out_sample_rate, std::int64_t in_ch_layout,
         AVSampleFormat in_sample_fmt, int in_sample_rate, int log_offset,
         void* log_ctx)
{
	this->context = swr_alloc_set_opts(nullptr, out_ch_layout,
	                                   out_sample_fmt, out_sample_rate,
	                                   in_ch_layout, in_sample_fmt,
	                                   in_sample_rate, log_offset, log_ctx);
	if (this->context == nullptr) {
		throw std::bad_alloc();
	}

	assert(this->context != nullptr);
	swr_init(this->context);
}

Swr::~Swr()
{
	if (this->context != nullptr) {
		swr_free(&this->context);
	}
	assert(this->context == nullptr);
}

std::int64_t Swr::GetDelay(std::int64_t base)
{
	assert(this->context != nullptr);
	return swr_get_delay(this->context, base);
}

int Swr::Convert(std::uint8_t* out_arg[SWR_CH_MAX], int out_count,
                 const std::uint8_t* in_arg[SWR_CH_MAX], int in_count)
{
	assert(this->context != nullptr);
	return swr_convert(this->context, out_arg, out_count, in_arg, in_count);
}
