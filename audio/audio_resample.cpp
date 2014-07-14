// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementations of the audio resampler classes.
 * @see audio/audio_resample.hpp
 */

#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "../errors.hpp"
#include "../swr.hpp"

#include "audio_resample.hpp"

Resampler::Resampler(AVSampleFormat output_format)
    : output_format(output_format)
{
}

AVSampleFormat Resampler::AVOutputFormat()
{
	// Output format should be set by derived classes.
	assert(this->output_format != AV_SAMPLE_FMT_NONE);

	return this->output_format;
}

std::vector<char> Resampler::MakeFrameVector(char *start, int channels,
                                             int samples)
{
	// We've been given the number of samples in the buffer, but, in order
	// to find the end of the buffer, we need to convert the sample count
	// to a byte count.
	int buf_size = av_samples_get_buffer_size(nullptr, channels, samples,
	                                          AVOutputFormat(), 1);
	char *end = start + buf_size;
	return std::vector<char>(start, end);
}

PlanarResampler::PlanarResampler(AVCodecContext *codec)
    : Resampler(av_get_packed_sample_fmt(codec->sample_fmt)),
      swr(codec->channel_layout, this->output_format, codec->sample_rate,
          codec->channel_layout, codec->sample_fmt, codec->sample_rate, 0,
          nullptr)
{
}

std::vector<char> PlanarResampler::Resample(AVFrame *frame)
{
	// The goal here is to convert a set of planar samples into a packed
	// format, using the libswresample resampler.  Nothing else will be
	// changed.

	// There should be some samples for us to resample.
	std::int64_t in_samples = frame->nb_samples;
	assert(0 < in_samples);

	// The resampling may incur a delay, so we need to account for that.
	// The delay compensation shouldn't reduce the number of samples.
	std::int64_t rate = frame->sample_rate;
	std::int64_t out_samples = this->swr.GetDelay(rate) + in_samples;
	assert(in_samples <= out_samples);

	// Allocate a sample buffer to hold the resampler's output.
	std::uint8_t *rbuf;
	if (av_samples_alloc(&rbuf, nullptr, av_frame_get_channels(frame),
	                     out_samples, this->output_format, 1) < 0) {
		throw std::bad_alloc();
	}

	// Actually perform the resampling here.
	// The number of samples produced should not exceed the capacity
	// allocated.
	int n = static_cast<int>(this->swr.Convert(
	                &rbuf, out_samples,
	                const_cast<const uint8_t **>(frame->extended_data),
	                in_samples));
	assert(n <= out_samples);

	// Finally, copy the samples into a byte vector.
	// Since we're copying, we can free the sample buffer we made earlier.
	std::vector<char> vec =
	                MakeFrameVector(reinterpret_cast<char *>(rbuf),
	                                av_frame_get_channels(frame), n);
	av_freep(&rbuf);
	return vec;
}

PackedResampler::PackedResampler(AVCodecContext *codec)
    : Resampler(codec->sample_fmt)
{
}

std::vector<char> PackedResampler::Resample(AVFrame *frame)
{
	// The samples are already in packed format, so we don't need to do
	// anything other than copying the frame data to a vector.
	return MakeFrameVector(
	                reinterpret_cast<char *>(frame->extended_data[0]),
	                av_frame_get_channels(frame), frame->nb_samples);
}
