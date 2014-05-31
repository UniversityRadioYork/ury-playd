/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#define _POSIX_C_SOURCE 200809

#include <functional>
#include <string>
#include <memory>
#include <cstdlib>
#include <iostream>
#include <sstream>

/* ffmpeg */
extern "C" {
#ifdef WIN32
#define inline __inline
#endif
#include <libavcodec/avcodec.h>
#include <libavcodec/version.h> /* For old version patchups */
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

#include <portaudio.h>

#include "errors.hpp"

#include "audio_av.h"
#include "audio_resample.h"
#include "constants.h"

au_in::au_in(const std::string &path)
{
	this->buffer = std::unique_ptr<unsigned char[]>(
	                new unsigned char[BUFFER_SIZE]);

	Open(path);
	InitialiseStream();
	InitialisePacket();
	InitialiseFrame();

	Debug("stream id:", this->stream_id);
	Debug("codec:", this->stream->codec->codec->long_name);
}

au_in::~au_in()
{
}

size_t au_in::SetupPortAudio(int device, PaStreamParameters *params)
{
	PaSampleFormat sf = SetupPortAudioSampleFormat();
	SetupPortAudioParameters(sf, device, this->stream->codec->channels,
	                         params);

	return SampleCountForByteCount(BUFFER_SIZE);
}

/* Returns the sample rate, providing av points to a properly initialised
 * au_in.
 */
double au_in::SampleRate()
{
	return (double)this->stream->codec->sample_rate;
}

/* Converts stream position (in microseconds) to estimated sample count. */
size_t au_in::SampleCountForPositionMicroseconds(std::chrono::microseconds usec)
{
	auto sample_micros = usec * SampleRate();
	return std::chrono::duration_cast<std::chrono::seconds>(sample_micros)
	                .count();
}

/* Converts sample count to estimated stream position (in microseconds). */
std::chrono::microseconds au_in::PositionMicrosecondsForSampleCount(
                size_t samples)
{
	auto position_secs = std::chrono::seconds(samples) / SampleRate();
	return std::chrono::duration_cast<std::chrono::microseconds>(
	                position_secs);
}

/* Converts buffer size (in bytes) to sample count (in samples). */
size_t au_in::SampleCountForByteCount(size_t bytes) const
{
	return ((bytes / this->stream->codec->channels) /
	        av_get_bytes_per_sample(this->stream->codec->sample_fmt));
}

/* Converts sample count (in samples) to buffer size (in bytes). */
size_t au_in::ByteCountForSampleCount(size_t samples) const
{
	return (samples * this->stream->codec->channels *
	        av_get_bytes_per_sample(this->stream->codec->sample_fmt));
}

/* Attempts to seek to the position 'usec' milliseconds into the file. */
void au_in::SeekToPositionMicroseconds(std::chrono::microseconds position)
{
	auto position_timebase_microseconds =
	                (position * this->stream->time_base.den) /
	                this->stream->time_base.num;
	auto position_timebase_seconds =
	                std::chrono::duration_cast<std::chrono::seconds>(
	                                position_timebase_microseconds);
	auto c = position_timebase_seconds.count();
	Debug("Seeking to:", c);

	if (av_seek_frame(this->context.get(), this->stream_id,
	                  static_cast<int64_t>(
	                                  position_timebase_seconds.count()),
	                  AVSEEK_FLAG_ANY) != 0) {
		throw Error(ErrorCode::INTERNAL_ERROR, "seek failed");
	}
}

/* Tries to decode an entire frame and returns a vector of its contents.
 *
 * If successful, returns a pointer to the resulting vector of decoded data,
 * which is owned by the caller.  If the return value is nullptr, we have run
 * out of frames to decode.
 */
std::vector<char> *au_in::Decode()
{
	bool complete = false;
	bool more = true;
	std::vector<char> *vector = nullptr;

	while (!(complete) && more) {
		if (av_read_frame(this->context.get(), this->packet.get()) <
		    0) {
			more = false;
		} else if (this->packet->stream_index == this->stream_id) {
			complete = DecodePacket();
			if (complete) {
				vector = Resample();
			}
		}
	}

	return vector;
}

std::vector<char> *au_in::Resample()
{
	return this->resampler->Resample(this->frame.get());
}

/* Converts from ffmpeg sample format to PortAudio sample format.
 *
 * The conversion is currently done straight with no attempts to
 * convert disallowed sample formats, and as such may fail with more
 * esoteric ffmpeg sample formats.
 */
PaSampleFormat au_in::SetupPortAudioSampleFormat()
{
	/* We need to convert planar samples into packed samples. */
	std::function<Resampler *(const SampleByteConverter &, AVCodecContext *)> rs;
	if (av_sample_fmt_is_planar(this->stream->codec->sample_fmt)) {
		rs = [](const SampleByteConverter &s, AVCodecContext *c) {
			return new PlanarResampler(s, c);
		};
	} else {
		rs = [](const SampleByteConverter &s, AVCodecContext *c) {
			return new PackedResampler(s, c);
		};
	}
	this->resampler = std::unique_ptr<Resampler>(
	                rs(*this, this->stream->codec));
	return SampleFormatAVToPA(this->resampler->AVOutputFormat());
}

/**
 * Converts a sample format enumeration from FFmpeg to PortAudio.
 * @param av_format The FFmpeg input format (must be packed).
 * @return The corresponding PortAudio format.
 * @note This only works for a small range of sample formats.
 */
PaSampleFormat au_in::SampleFormatAVToPA(AVSampleFormat av_format)
{
	PaSampleFormat pa_format = paUInt8;

	switch (av_format) {
		case AV_SAMPLE_FMT_U8:
			pa_format = paUInt8;
			break;
		case AV_SAMPLE_FMT_S16:
			pa_format = paInt16;
			break;
		case AV_SAMPLE_FMT_S32:
			pa_format = paInt32;
			break;
		case AV_SAMPLE_FMT_FLT:
			pa_format = paFloat32;
			break;
		default:
			throw Error(ErrorCode::BAD_FILE,
			            "unusable sample rate");
	}

	return pa_format;
}

/* Sets up a PortAudio parameter set ready for ffmpeg frames to be thrown at it.
 *
 * The parameter set pointed to by *params MUST already be allocated, and its
 * contents should only be used if this function returns E_OK.
 */
void au_in::SetupPortAudioParameters(PaSampleFormat sf, int device, int chans,
                                     PaStreamParameters *pars)
{
	memset(pars, 0, sizeof(*pars));
	pars->channelCount = chans;
	pars->device = device;
	pars->hostApiSpecificStreamInfo = NULL;
	pars->sampleFormat = sf;
	pars->suggestedLatency =
	                (Pa_GetDeviceInfo(device)->defaultLowOutputLatency);
}

void au_in::Open(const std::string &path)
{
	AVFormatContext *ctx = nullptr;

	if (avformat_open_input(&ctx, path.c_str(), NULL, NULL) < 0) {
		std::ostringstream os;
		os << "couldn't open " << path;
		throw Error(ErrorCode::NO_FILE, os.str());
	}

	auto free_context = [](AVFormatContext *ctx) {
		avformat_close_input(&ctx);
	};
	this->context = std::unique_ptr<AVFormatContext,
	                                decltype(free_context)>(ctx,
	                                                        free_context);
}

void au_in::InitialiseStream()
{
	FindStreamInfo();
	FindStreamAndInitialiseCodec();
}

void au_in::FindStreamInfo()
{
	if (avformat_find_stream_info(this->context.get(), NULL) < 0) {
		throw Error(ErrorCode::BAD_FILE, "no audio stream in file");
	}
}

void au_in::FindStreamAndInitialiseCodec()
{
	AVCodec *codec;
	int stream = av_find_best_stream(this->context.get(),
	                                 AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);

	if (stream < 0) {
		throw Error(ErrorCode::BAD_FILE, "can't open codec for file");
	}

	InitialiseCodec(stream, codec);
}

void au_in::InitialiseCodec(int stream, AVCodec *codec)
{
	AVCodecContext *codec_context = this->context->streams[stream]->codec;
	if (avcodec_open2(codec_context, codec, NULL) < 0) {
		throw Error(ErrorCode::BAD_FILE, "can't open codec for file");
	}

	this->stream = this->context->streams[stream];
	this->stream_id = stream;
}

void au_in::InitialiseFrame()
{
	auto frame_deleter = [](AVFrame *frame) { av_frame_free(&frame); };
	this->frame = std::unique_ptr<AVFrame, decltype(frame_deleter)>(
	                av_frame_alloc(), frame_deleter);
	if (this->frame == nullptr) {
		throw Error(ErrorCode::NO_MEM, "can't alloc frame");
	}
}

void au_in::InitialisePacket()
{
	auto packet_deleter = [](AVPacket *packet) {
		av_free_packet(packet);
		delete packet;
	};
	this->packet = std::unique_ptr<AVPacket, decltype(packet_deleter)>(
	                new AVPacket, packet_deleter);

	AVPacket *pkt = this->packet.get();
	av_init_packet(pkt);
	pkt->data = this->buffer.get();
	pkt->size = BUFFER_SIZE;
}

/*  Also see the non-static functions for the frontend for frame decoding */

bool au_in::DecodePacket()
{
	int frame_finished = 0;

	if (avcodec_decode_audio4(this->stream->codec, this->frame.get(),
	                          &frame_finished, this->packet.get()) < 0) {
		throw Error(ErrorCode::BAD_FILE, "decoding error");
	}
	return frame_finished;
}
