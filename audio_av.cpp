/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
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
#include <libavcodec/version.h>		/* For old version patchups */
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

#include <portaudio.h>

#include "errors.hpp"

#include "audio_av.h"
#include "constants.h"

au_in::au_in(const std::string &path)
{
	this->resample_buffer = nullptr;
	this->buffer = std::unique_ptr<unsigned char[]>(new unsigned char[BUFFER_SIZE]);
	
	Open(path);
	InitialiseStream();
	InitialisePacket();
	InitialiseFrame();
	InitialiseResampler();

	Debug("stream id:", this->stream_id);
	Debug("codec:", this->stream->codec->codec->long_name);
}

au_in::~au_in()
{
}

void au_in::InitialiseResampler()
{
	this->use_resampler = false;
}

size_t au_in::SetupPortAudio(int device, PaStreamParameters *params)
{
	PaSampleFormat sf = SetupPortAudioSampleFormat();
	SetupPortAudioParameters(sf, device, this->stream->codec->channels, params);

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
	return std::chrono::duration_cast<std::chrono::seconds>(sample_micros).count();
}

/* Converts sample count to estimated stream position (in microseconds). */
std::chrono::microseconds au_in::PositionMicrosecondsForSampleCount(size_t samples)
{
	auto position_secs = std::chrono::seconds(samples) / SampleRate();
	return std::chrono::duration_cast<std::chrono::microseconds>(position_secs);
}

/* Converts buffer size (in bytes) to sample count (in samples). */
size_t au_in::SampleCountForByteCount(size_t bytes)
{
	return (bytes /
		this->stream->codec->channels /
		av_get_bytes_per_sample(this->stream->codec->sample_fmt));
}

/* Converts sample count (in samples) to buffer size (in bytes). */
size_t au_in::ByteCountForSampleCount(size_t samples)
{
	return (samples *
		this->stream->codec->channels *
		av_get_bytes_per_sample(this->stream->codec->sample_fmt));
}

/* Attempts to seek to the position 'usec' milliseconds into the file. */
void au_in::SeekToPositionMicroseconds(std::chrono::microseconds position)
{
	auto position_timebase_microseconds = (position * this->stream->time_base.den)
			/ this->stream->time_base.num;
	auto position_timebase_seconds = std::chrono::duration_cast<std::chrono::seconds>(position_timebase_microseconds);
	auto c = position_timebase_seconds.count();
	Debug("Seeking to:", c);

	if (av_seek_frame(this->context.get(),
		this->stream_id,
		static_cast<int64_t>(position_timebase_seconds.count()),
		AVSEEK_FLAG_ANY) != 0) {
		throw Error(ErrorCode::INTERNAL_ERROR, "seek failed");
	}
}

/* Tries to decode an entire frame and points to its contents.
 *
 * The current state in *av is used to try run ffmpeg's decoder.
 *
 * If successful, returns E_OK and sets 'buf' and 'n' to a pointer to the buffer
 * and number of bytes decoded into it respectively.
 *
 * If the return value is false, we have run out of frames to decode; any other
 * return value signifies a decode error.  Do NOT rely on 'buf' and 'n' having
 * sensible values if E_OK is not returned.
 */
bool au_in::Decode(char **buf, size_t *n)
{
	bool complete = false;
	bool more = true;

	while (!(complete) && more) {
		if (av_read_frame(this->context.get(), this->packet.get()) < 0) {
			more = false;
		}
		else if (this->packet->stream_index == this->stream_id) {
			complete = DecodePacket();

			if (complete) {
				if (this->use_resampler) {
					Resample(buf, n);
				}
				else {
					// Only use first channel, as we have packed data.
					*buf = (char *)this->frame->extended_data[0];
					*n = this->frame->nb_samples;
				}
			}
		}
	}

	return more;
}

void au_in::Resample(char **buf, size_t *n)
{
	auto resample_buffer_deleter = [](uint8_t *buffer){ av_freep(&buffer); };
	uint8_t *rbuf;

	int in_samples = this->frame->nb_samples;
	int rate = this->frame->sample_rate;
	int out_samples = swr_get_delay(this->resampler.get(), rate) + in_samples;

	if (av_samples_alloc(
		&rbuf,
		nullptr,
		av_frame_get_channels(this->frame.get()),
		out_samples,
		this->sample_format,
		0) < 0) {
		throw Error(ErrorCode::INTERNAL_ERROR, "Couldn't allocate samples for reallocation!");
	}

	this->resample_buffer = std::unique_ptr<uint8_t, decltype(resample_buffer_deleter)>(rbuf, resample_buffer_deleter);

	*n = (size_t)swr_convert(
		this->resampler.get(),
		&rbuf,
		out_samples,
		const_cast<const uint8_t**>(this->frame->extended_data),
		this->frame->nb_samples);

	*buf = (char *)this->resample_buffer.get();
	*n = out_samples;
}

/* Converts from ffmpeg sample format to PortAudio sample format.
 *
 * The conversion is currently done straight with no attempts to
 * convert disallowed sample formats, and as such may fail with more
 * esoteric ffmpeg sample formats.
 */
PaSampleFormat au_in::SetupPortAudioSampleFormat()
{
	AVSampleFormat in = this->stream->codec->sample_fmt;
	PaSampleFormat out = 0;

	/* We need to convert planar samples into packed samples. */
	if (av_sample_fmt_is_planar(in)) {
		this->sample_format = av_get_packed_sample_fmt(in);

		auto resampler_deleter = [](SwrContext *avr) { swr_free(&avr); };
		this->resampler = std::unique_ptr<SwrContext, decltype(resampler_deleter)>(
			swr_alloc_set_opts(
			nullptr,
			this->stream->codec->channel_layout,
			this->sample_format,
			this->stream->codec->sample_rate,
			this->stream->codec->channel_layout,
			in,
			this->stream->codec->sample_rate,
			0,
			nullptr
			), resampler_deleter);
		if (this->resampler == nullptr) {
			throw Error(ErrorCode::NO_MEM, "Out of memory for resampler!");
		}

		swr_init(this->resampler.get());
		this->use_resampler = true;
	}
	else {
		this->sample_format = in;
		this->resampler = nullptr;
		this->use_resampler = false;
	}

	return SampleFormatAVToPA(this->sample_format);
}

/**
 * Converts a sample format enumeration from FFmpeg to PortAudio.
 * @param av_format The FFmpeg input format (must be packed).
 * @return The corresponding PortAudio format.
 * @note This only works for a small range of sample formats.
 */
PaSampleFormat au_in::SampleFormatAVToPA(AVSampleFormat av_format) {
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
		throw Error(ErrorCode::BAD_FILE, "unusable sample rate");
	}

	return pa_format;
}

/* Sets up a PortAudio parameter set ready for ffmpeg frames to be thrown at it.
 *
 * The parameter set pointed to by *params MUST already be allocated, and its
 * contents should only be used if this function returns E_OK.
 */
void au_in::SetupPortAudioParameters(PaSampleFormat sf, int device, int chans, PaStreamParameters *pars)
{
	memset(pars, 0, sizeof(*pars));
	pars->channelCount = chans;
	pars->device = device;
	pars->hostApiSpecificStreamInfo = NULL;
	pars->sampleFormat = sf;
	pars->suggestedLatency = (Pa_GetDeviceInfo(device)->
				  defaultLowOutputLatency);
}

void au_in::Open(const std::string &path)
{
	AVFormatContext *ctx = nullptr;

	if (avformat_open_input(&ctx,
		path.c_str(),
		NULL,
		NULL) < 0) {
		std::ostringstream os;
		os << "couldn't open " << path;
		throw Error(ErrorCode::NO_FILE, os.str());
	}

	auto free_context = [](AVFormatContext *ctx) { avformat_close_input(&ctx); };
	this->context = std::unique_ptr<AVFormatContext, decltype(free_context)>(ctx, free_context);
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
	  AVMEDIA_TYPE_AUDIO,
	  -1,
	  -1,
	  &codec,
	  0);

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
	auto frame_deleter = [](AVFrame *frame) { avcodec_free_frame(&frame); };
	this->frame = std::unique_ptr<AVFrame, decltype(frame_deleter)>(avcodec_alloc_frame(), frame_deleter);
	if (this->frame == nullptr) {
		throw Error(ErrorCode::NO_MEM, "can't alloc frame");
	}
}

void au_in::InitialisePacket()
{
	auto packet_deleter = [](AVPacket *packet) { av_free_packet(packet); delete packet; }; 
	this->packet = std::unique_ptr<AVPacket, decltype(packet_deleter)>(new AVPacket, packet_deleter);

	AVPacket *pkt = this->packet.get();
	av_init_packet(pkt);
	pkt->data = this->buffer.get();
	pkt->size = BUFFER_SIZE;
}

/*  Also see the non-static functions for the frontend for frame decoding */

bool au_in::DecodePacket()
{
	int		frame_finished = 0;

	if (avcodec_decode_audio4(this->stream->codec,
				  this->frame.get(),
				  &frame_finished,
				  this->packet.get()) < 0) {
		throw Error(ErrorCode::BAD_FILE, "decoding error");
	}
	return frame_finished;
}
