/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#ifndef AUDIO_RESAMPLE_H
#define AUDIO_RESAMPLE_H

#include <functional>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

/* A class for performing resampling. */
class Resampler {
public:
	virtual size_t Resample(char **buf, AVFrame *frame) = 0;
	virtual AVSampleFormat AVOutputFormat();

protected:
	AVSampleFormat output_format;
};

/* A class for performing resampling on a planar sample format. */
class PlanarResampler : public Resampler {
public:
	PlanarResampler(AVCodecContext *codec);
	size_t Resample(char **buf, AVFrame *frame);

private:
	std::unique_ptr<SwrContext, std::function<void(SwrContext *)>> swr;
	std::unique_ptr<uint8_t, std::function<void(uint8_t *)>>
	                resample_buffer;
};

/* A class for performing resampling on a packed sample format. */
class PackedResampler : public Resampler {
public:
	PackedResampler(AVCodecContext *codec);
	size_t Resample(char **buf, AVFrame *frame);
};

#endif // AUDIO_RESAMPLE_H
