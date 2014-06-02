/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#ifndef AUDIO_AV_H
#define AUDIO_AV_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

#include "audio_resample.h"
#include "errors.hpp"
#include "sample_formats.hpp"

/* The audio input structure (thusly named in case we ever generalise
 * away from ffmpeg), containing all state pertaining to the input
 * decoder for a file.
 *
 * struct AudioDecoder is an opaque structure; only audio_av.c knows its true
 * definition.
 */
class AudioDecoder : public SampleByteConverter {
public:
	AudioDecoder(const std::string &path);
	~AudioDecoder();

	std::vector<char> *Decode();

	std::uint8_t ChannelCount() const;
	double SampleRate() const;
	SampleFormat SampleFormat() const;
	size_t BufferSampleCapacity() const;

	void SeekToPositionMicroseconds(std::chrono::microseconds position);

	// Unit conversion
	std::chrono::microseconds PositionMicrosecondsForSampleCount(
	                size_t samples) const;
	size_t SampleCountForPositionMicroseconds(
	                std::chrono::microseconds position) const;

	size_t SampleCountForByteCount(size_t bytes) const;
	size_t ByteCountForSampleCount(size_t samples) const;

private:
	AVStream *stream;
	int stream_id;

	std::unique_ptr<AVFormatContext, std::function<void(AVFormatContext *)>>
	                context;
	std::unique_ptr<AVPacket, std::function<void(AVPacket *)>>
	                packet; /* Last undecoded packet */
	std::unique_ptr<AVFrame, std::function<void(AVFrame *)>>
	                frame; /* Last decoded frame */
	std::unique_ptr<unsigned char[]> buffer;
	std::unique_ptr<Resampler> resampler;

	void Open(const std::string &path);

	void InitialiseStream();
	void FindStreamInfo();
	void FindStreamAndInitialiseCodec();

	void InitialiseCodec(int stream, AVCodec *codec);
	void InitialiseFrame();
	void InitialisePacket();
	void InitialiseResampler();

	bool DecodePacket();
	std::vector<char> *Resample();
	size_t BytesPerSample() const;
};

#endif /* not AUDIO_AV_H */
