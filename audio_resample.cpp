/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

#include "audio_resample.h"
#include "errors.hpp"

AVSampleFormat Resampler::AVOutputFormat()
{
	return this->output_format;
}

PlanarResampler::PlanarResampler(AVCodecContext *codec)
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

size_t PlanarResampler::Resample(char **buf, AVFrame *frame)
{
	auto resample_buffer_deleter = [](uint8_t *buffer) {
		av_freep(&buffer);
	};
	uint8_t *rbuf;

	int in_samples = frame->nb_samples;
	int rate = frame->sample_rate;
	int out_samples = swr_get_delay(this->swr.get(), rate) + in_samples;

	if (av_samples_alloc(&rbuf, nullptr, av_frame_get_channels(frame),
	                     out_samples, this->output_format, 0) < 0) {
		throw Error(ErrorCode::INTERNAL_ERROR,
		            "Couldn't allocate samples for reallocation!");
	}

	this->resample_buffer = std::unique_ptr<
	                uint8_t, decltype(resample_buffer_deleter)>(
	                rbuf, resample_buffer_deleter);

	size_t n = (size_t)swr_convert(
	                this->swr.get(), &rbuf, out_samples,
	                const_cast<const uint8_t **>(frame->extended_data),
	                in_samples);

	*buf = (char *)this->resample_buffer.get();
	return n;
}

PackedResampler::PackedResampler(AVCodecContext *codec)
{
	this->output_format = codec->sample_fmt;
}

size_t PackedResampler::Resample(char **buf, AVFrame *frame)
{
	// Only use first channel, as we have packed data.
	*buf = (char *)frame->extended_data[0];
	return frame->nb_samples;
}
