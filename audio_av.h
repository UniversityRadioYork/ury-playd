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
#include <portaudio.h>

#include "audio_resample.h"
#include "errors.hpp"

/* The audio input structure (thusly named in case we ever generalise
 * away from ffmpeg), containing all state pertaining to the input
 * decoder for a file.
 *
 * struct au_in is an opaque structure; only audio_av.c knows its true
 * definition.
 */
class au_in : public SampleByteConverter {
public:
	au_in(const std::string &path);
	~au_in();
	size_t SetupPortAudio(int device, PaStreamParameters *params);

	std::vector<char> *Decode();
	double SampleRate();

	void SeekToPositionMicroseconds(std::chrono::microseconds position);

	// Unit conversion
	std::chrono::microseconds PositionMicrosecondsForSampleCount(
	                size_t samples);
	size_t SampleCountForPositionMicroseconds(
	                std::chrono::microseconds position);

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

	PaSampleFormat SetupPortAudioSampleFormat();
	void SetupPortAudioParameters(PaSampleFormat sf, int device, int chans,
	                              PaStreamParameters *pars);

	bool DecodePacket();
	std::vector<char> *Resample();

	PaSampleFormat SampleFormatAVToPA(AVSampleFormat av_format);
};

/* A structure containing a lump of decoded frame data.
 *
 * struct au_frame is an opaque structure; only audio_av.c knows its true
 * definition.
 */
struct au_frame;

#endif /* not AUDIO_AV_H */
