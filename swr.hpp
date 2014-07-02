// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the Swr class.
 * @see swr.hpp
 */

#ifndef PS_SWR_HPP
#define PS_SWR_HPP

#include <cstdint>

extern "C" {
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

/**
 * A thin layer over the ffmpeg software resampler (libswresample).
 *
 * This is primarily used to convert sample formats from planar to packed, as
 * PortAudio can deal with the latter much easier.
 *
 * @see http://www.ffmpeg.org/libswresample.html
 * @see http://www.ffmpeg.org/doxygen/trunk/group__lswr.html
 */
class Swr {
public:
	/**
	 * Constructs a Swr.
	 * @param out_ch_layout    The desired ffmpeg output channel layout.
	 * @param out_sample_fmt   The desired ffmpeg output sample format.
	 * @param out_sample_rate  The desired ffmpeg output sample rate.
	 * @param in_ch_layout     The ffmpeg channel layout of the input.
	 * @param in_sample_fmt    The ffmpeg sample format of the input.
	 * @param in_sample_rate   The ffmpeg sample rate of the input.
	 * @param log_offset       The ffmpeg log offset.
	 * @param log_ctx          The ffmpeg log context.
	 */
	Swr(std::int64_t out_ch_layout, AVSampleFormat out_sample_fmt,
	    int out_sample_rate, std::int64_t in_ch_layout,
	    AVSampleFormat in_sample_fmt, int in_sample_rate, int log_offset,
	    void* log_ctx);

	/**
	 * Destructs a Swr.
	 */
	~Swr();

	/**
	 * Gets the delay incurred by the resampling process.
	 * @param base  The time-base to use for the return value.
	 * @return      The delay, in units of 1/@a base seconds.
	 */
	std::int64_t GetDelay(std::int64_t base);

	/**
	 * Resamples a buffer of samples.
	 *
	 * The number of buffers is 1 for packed audio, and the number of
	 * channels for planar audio.
	 * @param out_arg    The output buffers.
	 * @param out_count  The output capacity, in samples per channel.
	 * @param in_arg     The input buffers.
	 * @param in_count   The input reserve, in samples per channel.
	 * @return           The number of samples output per channel, or -1 if
	 *                   an error occurred.
	 */
	int Convert(std::uint8_t* out_arg[SWR_CH_MAX], int out_count,
	            const std::uint8_t* in_arg[SWR_CH_MAX], int in_count);

private:
	SwrContext* context; //< The internal libswresample context.
};

#endif // PS_SWR_HPP
