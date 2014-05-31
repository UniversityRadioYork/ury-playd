/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#include <memory>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

#include "audio_resample.h"
#include "errors.hpp"

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

std::vector<char> *Resampler::MakeFrameVector(char *start, int sample_count)
{
	char *end = start + ByteCountForSampleCount(sample_count);
	return new std::vector<char>(start, end);
}

PlanarResampler::PlanarResampler(const SampleByteConverter &out,
                                 AVCodecContext *codec)
    : Resampler(out)
{
	this->resample_buffer = nullptr;
	this->output_format = av_get_packed_sample_fmt(codec->sample_fmt);

	auto resampler_deleter = [](SwrContext *avr) { swr_free(&avr); };
	this->swr = std::unique_ptr<SwrContext, decltype(resampler_deleter)>(
	                swr_alloc_set_opts(nullptr, codec->channel_layout,
	                                   this->output_format,
	                                   codec->sample_rate,
	                                   codec->channel_layout,
	                                   codec->sample_fmt,
	                                   codec->sample_rate, 0, nullptr),
	                resampler_deleter);
	if (this->swr == nullptr) {
		throw Error(ErrorCode::NO_MEM, "Out of memory for resampler!");
	}

	swr_init(this->swr.get());
}

std::vector<char> *PlanarResampler::Resample(AVFrame *frame)
{
	uint8_t *rbuf;

	int in_samples = frame->nb_samples;
	int rate = frame->sample_rate;
	int out_samples = swr_get_delay(this->swr.get(), rate) + in_samples;

	if (av_samples_alloc(&rbuf, nullptr, av_frame_get_channels(frame),
	                     out_samples, this->output_format, 0) < 0) {
		throw Error(ErrorCode::INTERNAL_ERROR,
		            "Couldn't allocate samples for reallocation!");
	}

	size_t n = (size_t)swr_convert(
	                this->swr.get(), &rbuf, out_samples,
	                const_cast<const uint8_t **>(frame->extended_data),
	                in_samples);

	auto vector = MakeFrameVector(reinterpret_cast<char *>(rbuf), n);
	av_freep(&rbuf);

	return vector;
}

PackedResampler::PackedResampler(const SampleByteConverter &out,
                                 AVCodecContext *codec)
    : Resampler(out)
{
	this->output_format = codec->sample_fmt;
}

std::vector<char> *PackedResampler::Resample(AVFrame *frame)
{
	return MakeFrameVector(
	                reinterpret_cast<char *>(frame->extended_data[0]),
	                frame->nb_samples);
}
