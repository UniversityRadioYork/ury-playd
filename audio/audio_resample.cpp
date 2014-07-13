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

Resampler::Resampler(AVCodecContext *input_context, AVSampleFormat output_format)
    : bytes_per_sample(av_get_bytes_per_sample(output_format) * input_context->channels),
      output_format(output_format)
{
}

AVSampleFormat Resampler::AVOutputFormat()
{
	// Output format should be set by derived classes.
	assert(this->output_format != AV_SAMPLE_FMT_NONE);

	return this->output_format;
}

std::vector<char> Resampler::MakeFrameVector(char *start, int sample_count)
{
	char *end = start + (sample_count * this->bytes_per_sample);
	return std::vector<char>(start, end);
}

PlanarResampler::PlanarResampler(AVCodecContext *codec)
	: Resampler(codec, av_get_packed_sample_fmt(codec->sample_fmt)),
	swr(codec->channel_layout, this->output_format,
	codec->sample_rate, codec->channel_layout,
	codec->sample_fmt, codec->sample_rate, 0, nullptr)
{
}

std::vector<char> PlanarResampler::Resample(AVFrame *frame)
{
	std::uint8_t *rbuf;

	std::int64_t in_samples = frame->nb_samples;
	std::int64_t rate = frame->sample_rate;
	std::int64_t out_samples = this->swr.GetDelay(rate) + in_samples;

	if (av_samples_alloc(&rbuf, nullptr, av_frame_get_channels(frame),
	                     out_samples, this->output_format, 0) < 0) {
		throw std::bad_alloc();
	}

	size_t n = static_cast<size_t>(this->swr.Convert(
	                &rbuf, out_samples,
	                const_cast<const uint8_t **>(frame->extended_data),
	                in_samples));

	std::vector<char> vec =
	                MakeFrameVector(reinterpret_cast<char *>(rbuf), n);
	av_freep(&rbuf);

	return vec;
}

PackedResampler::PackedResampler(AVCodecContext *codec)
    : Resampler(codec, codec->sample_fmt)
{}

std::vector<char> PackedResampler::Resample(AVFrame *frame)
{
	return MakeFrameVector(
	                reinterpret_cast<char *>(frame->extended_data[0]),
	                frame->nb_samples);
}
