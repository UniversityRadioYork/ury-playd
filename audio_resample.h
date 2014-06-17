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
}

#include "swr.h"

class SampleByteConverter {
public:
	virtual size_t SampleCountForByteCount(size_t bytes) const = 0;
	virtual size_t ByteCountForSampleCount(size_t samples) const = 0;
};

/* A class for performing resampling. */
class Resampler : protected SampleByteConverter {
public:
	Resampler(const SampleByteConverter &out);

	virtual std::vector<char> Resample(AVFrame *frame) = 0;
	virtual AVSampleFormat AVOutputFormat();

protected:
	size_t SampleCountForByteCount(size_t bytes) const;
	size_t ByteCountForSampleCount(size_t samples) const;
	std::vector<char> MakeFrameVector(char *start, int sample_count);

	AVSampleFormat output_format;
	const SampleByteConverter &out;
};

/* A class for performing resampling on a planar sample format. */
class PlanarResampler : public Resampler {
public:
	PlanarResampler(const SampleByteConverter &out, AVCodecContext *codec);
	std::vector<char> Resample(AVFrame *frame);

private:
	std::unique_ptr<Swr> swr;
	std::unique_ptr<uint8_t, std::function<void(uint8_t *)>>
	                resample_buffer;
};

/* A class for performing resampling on a packed sample format. */
class PackedResampler : public Resampler {
public:
	PackedResampler(const SampleByteConverter &out, AVCodecContext *codec);
	std::vector<char> Resample(AVFrame *frame);
};

#endif // AUDIO_RESAMPLE_H
