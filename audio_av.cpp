/*
 * =============================================================================
 *
 *       Filename:  audio_av.c
 *
 *    Description:  ffmpeg-specific code
 *
 *        Version:  1.0
 *        Created:  23/12/2012 01:21:49
 *       Revision:  none
 *       Compiler:  clang
 *
 *         Author:  Matt Windsor (CaptainHayashi), matt.windsor@ury.york.ac.uk
 *        Company:  University Radio York Computing Team
 *
 * =============================================================================
 */

/*-
 * Copyright (C) 2012  University Radio York Computing Team
 *
 * This file is a part of playslave.
 *
 * playslave is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * playslave is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * playslave; if not, write to the Free Software Foundation, Inc., 51 Franklin
 * Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#define _POSIX_C_SOURCE 200809

#include <functional>
#include <string>
#include <memory>
#include <cstdlib>

/* ffmpeg */
extern "C" {
#ifdef WIN32
#define inline __inline
#endif
#include <libavcodec/avcodec.h>
#include <libavcodec/version.h>		/* For old version patchups */
#include <libavformat/avformat.h>
#include <libavresample/avresample.h>
#include <libavutil/opt.h>
}

#include <portaudio.h>

#include "cuppa/errors.h"               /* dbug, error */
#include "cuppa/constants.h"            /* USECS_IN_SEC */

#include "audio_av.h"
#include "constants.h"

static enum error
setup_pa(PaSampleFormat sf, int device,
	 int chans, PaStreamParameters *pars);

au_in::au_in(const std::string &path)
{
	this->buffer = std::unique_ptr<unsigned char[]>(new unsigned char[BUFFER_SIZE]);
	
	load_file(path);
	init_stream();
	init_packet();
	init_frame();
	init_resampler();

	dbug("stream id: %u", this->stream_id);
	dbug("codec: %s", this->stream->codec->codec->long_name);
}

au_in::~au_in()
{
}

void
au_in::init_resampler() {
	auto resampler_deleter = [](AVAudioResampleContext *avr) { avresample_free(&avr); };
	this->avr = std::unique_ptr<AVAudioResampleContext, decltype(resampler_deleter)>(avresample_alloc_context(), resampler_deleter);
	if (this->avr == nullptr) {
		throw E_NO_MEM;
	}
}

size_t
au_in::pa_config(int device, PaStreamParameters *params)
{
	PaSampleFormat	sf;

	sf = conv_sample_fmt(this->stream->codec->sample_fmt);
	setup_pa(sf, device, this->stream->codec->channels, params);

	return bytes2samples(BUFFER_SIZE);
}

/* Returns the sample rate, providing av points to a properly initialised
 * au_in.
 */
double
au_in::sample_rate()
{
	return (double)this->stream->codec->sample_rate;
}

/* Converts stream position (in microseconds) to estimated sample count. */
size_t
au_in::usec2samples(uint64_t usec)
{
	return (usec * sample_rate()) / USECS_IN_SEC;
}

/* Converts sample count to estimated stream position (in microseconds). */
uint64_t
au_in::samples2usec(size_t samples)
{
	return (samples * USECS_IN_SEC) / sample_rate();
}

/* Converts buffer size (in bytes) to sample count (in samples). */
size_t
au_in::bytes2samples(size_t bytes)
{
	return (bytes /
		this->stream->codec->channels /
		av_get_bytes_per_sample(this->stream->codec->sample_fmt));
}

/* Converts sample count (in samples) to buffer size (in bytes). */
size_t
au_in::samples2bytes(size_t samples)
{
	return (samples *
		this->stream->codec->channels *
		av_get_bytes_per_sample(this->stream->codec->sample_fmt));
}

/* Attempts to seek to the position 'usec' milliseconds into the file. */
enum error
au_in::seek(uint64_t usec)
{
	int64_t		seek_pos;
	enum error	err = E_OK;

	seek_pos = ((usec * this->stream->time_base.den) /
			this->stream->time_base.num) / USECS_IN_SEC;
	if (av_seek_frame(this->context.get(),
			  this->stream_id,
			  (int64_t)seek_pos,
			  AVSEEK_FLAG_ANY) != 0)
		err = error(E_INTERNAL_ERROR, "seek failed");

	return err;
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
bool
au_in::decode(char **buf, size_t *n)
{
	bool complete = false;
	bool more = true;

	while (!(complete) && more) {
		if (av_read_frame(this->context.get(), this->packet.get()) < 0) {
			more = false;
		}
		else if (this->packet->stream_index == this->stream_id) {
			complete = decode_packet(buf, n);
		}
	}

	return more;
}

/* Converts from ffmpeg sample format to PortAudio sample format.
 *
 * The conversion is currently done straight with no attempts to
 * convert disallowed sample formats, and as such may fail with more
 * esoteric ffmpeg sample formats.
 */
PaSampleFormat
au_in::conv_sample_fmt(enum AVSampleFormat in)
{
	PaSampleFormat out = 0;

	/* We need to convert planar samples into packed samples. */
	if (av_sample_fmt_is_planar(in)) {
		av_opt_set_int(this->avr.get(), "in_sample_fmt", in, 0);
		in = av_get_packed_sample_fmt(in);
		av_opt_set_int(this->avr.get(), "out_sample_fmt", in, 0);
	}

	switch (in) {
	case AV_SAMPLE_FMT_U8:
		out = paUInt8;
		break;
	case AV_SAMPLE_FMT_S16:
		out = paInt16;
		break;
	case AV_SAMPLE_FMT_S32:
		out = paInt32;
		break;
	case AV_SAMPLE_FMT_FLT:
		out = paFloat32;
		break;
	default:
		throw E_BAD_FILE;
	}

	return out;
}

/* Sets up a PortAudio parameter set ready for ffmpeg frames to be thrown at it.
 *
 * The parameter set pointed to by *params MUST already be allocated, and its
 * contents should only be used if this function returns E_OK.
 */
void
au_in::setup_pa(PaSampleFormat sf, int device, int chans, PaStreamParameters *pars)
{
	memset(pars, 0, sizeof(*pars));
	pars->channelCount = chans;
	pars->device = device;
	pars->hostApiSpecificStreamInfo = NULL;
	pars->sampleFormat = sf;
	pars->suggestedLatency = (Pa_GetDeviceInfo(device)->
				  defaultLowOutputLatency);
}

void
au_in::load_file(const std::string &path)
{
	enum error	err = E_OK;

	AVFormatContext *ctx = nullptr;

	if (avformat_open_input(&ctx,
		path.c_str(),
		NULL,
		NULL) < 0) {
		throw E_NO_FILE;
	}

	auto free_context = [](AVFormatContext *ctx) { avformat_close_input(&ctx); };
	this->context = std::unique_ptr<AVFormatContext, decltype(free_context)>(ctx, free_context);
}

void
au_in::init_stream()
{
	AVCodec        *codec;
	int		stream;
	enum error	err = E_OK;

	if (avformat_find_stream_info(this->context.get(), NULL) < 0) {
		throw E_BAD_FILE;
	}

	stream = av_find_best_stream(this->context.get(),
	  AVMEDIA_TYPE_AUDIO,
	  -1,
	  -1,
	  &codec,
	  0);

	if (stream < 0) {
		throw E_BAD_FILE;
	}

	init_codec(stream, codec);
}

void
au_in::init_codec(int stream, AVCodec *codec)
{
	enum error	err = E_OK;

	AVCodecContext *codec_context = this->context->streams[stream]->codec;
	if (avcodec_open2(codec_context, codec, NULL) < 0)
		throw E_BAD_FILE;

	this->stream = this->context->streams[stream];
	this->stream_id = stream;
}

void
au_in::init_frame()
{
	auto frame_deleter = [](AVFrame *frame) { avcodec_free_frame(&frame); };
	this->frame = std::unique_ptr<AVFrame, decltype(frame_deleter)>(avcodec_alloc_frame(), frame_deleter);
	if (this->frame == nullptr)
		throw E_NO_MEM;
}

void
au_in::init_packet()
{
	auto packet_deleter = [](AVPacket *packet) { av_free_packet(packet); delete packet; }; 
	this->packet = std::unique_ptr<AVPacket, decltype(packet_deleter)>(new AVPacket, packet_deleter);

	AVPacket *pkt = this->packet.get();
	av_init_packet(pkt);
	pkt->data = this->buffer.get();
	pkt->size = BUFFER_SIZE;
}

/*  Also see the non-static functions for the frontend for frame decoding */

bool
au_in::decode_packet(char **buf, size_t *n)
{
	int		frame_finished = 0;

	if (avcodec_decode_audio4(this->stream->codec,
				  this->frame.get(),
				  &frame_finished,
				  this->packet.get()) < 0) {
		/* Decode error */
		throw error(E_BAD_FILE, "decoding error");
	}
	if (frame_finished) {
		/* Record data that we'll use in the play loop */
		*buf = (char *)this->frame->extended_data[0];
		*n = this->frame->nb_samples;
	}
	return frame_finished;
}
