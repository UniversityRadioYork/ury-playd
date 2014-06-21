/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#include <cstdint>
#include <memory>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "../errors.hpp"
#include "../swr.hpp"

#include "audio_resample.hpp"

Resampler::Resampler(const SampleByteConverter &conv) : out(conv)
{
}

AVSampleFormat Resampler::AVOutputFormat()
{
	return this->output_format;
}

size_t Resampler::SampleCountForByteCount(size_t bytes) const
{
	return this->out.SampleCountForByteCount(bytes);
}

size_t Resampler::ByteCountForSampleCount(size_t samples) const
{
	return this->out.ByteCountForSampleCount(samples);
}

std::vector<char> Resampler::MakeFrameVector(char *start, int sample_count)
{
	char *end = start + ByteCountForSampleCount(sample_count);
	return std::vector<char>(start, end);
}

PlanarResampler::PlanarResampler(const SampleByteConverter &out,
                                 AVCodecContext *codec)
    : Resampler(out)
{
	this->resample_buffer = nullptr;
	this->output_format = av_get_packed_sample_fmt(codec->sample_fmt);

	this->swr = std::unique_ptr<Swr>(new Swr(
	                codec->channel_layout, this->output_format,
	                codec->sample_rate, codec->channel_layout,
	                codec->sample_fmt, codec->sample_rate, 0, nullptr));
}

std::vector<char> PlanarResampler::Resample(AVFrame *frame)
{
	std::uint8_t *rbuf;

	std::int64_t in_samples = frame->nb_samples;
	std::int64_t rate = frame->sample_rate;
	std::int64_t out_samples = this->swr->GetDelay(rate) + in_samples;

	if (av_samples_alloc(&rbuf, nullptr, av_frame_get_channels(frame),
	                     out_samples, this->output_format, 0) < 0) {
		throw Error(ErrorCode::INTERNAL_ERROR,
		            "Couldn't allocate samples for reallocation!");
	}

	size_t n = static_cast<size_t>(this->swr->Convert(
	                &rbuf, out_samples,
	                const_cast<const uint8_t **>(frame->extended_data),
	                in_samples));

	std::vector<char> vec =
	                MakeFrameVector(reinterpret_cast<char *>(rbuf), n);
	av_freep(&rbuf);

	return vec;
}

PackedResampler::PackedResampler(const SampleByteConverter &out,
                                 AVCodecContext *codec)
    : Resampler(out)
{
	this->output_format = codec->sample_fmt;
}

std::vector<char> PackedResampler::Resample(AVFrame *frame)
{
	return MakeFrameVector(
	                reinterpret_cast<char *>(frame->extended_data[0]),
	                frame->nb_samples);
}
