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

#ifndef AUDIO_AV_H
#define AUDIO_AV_H

#include <functional>
#include <string>
#include <cstdint>		/* uint64_t */

#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <portaudio.h>		/* PaStreamParameters */

#include "cuppa/errors.h"	/* enum error */

/* The audio input structure (thusly named in case we ever generalise
 * away from ffmpeg), containing all state pertaining to the input
 * decoder for a file.
 *
 * struct au_in is an opaque structure; only audio_av.c knows its true
 * definition.
 */
class au_in {
public:
	au_in(const std::string &path);
	~au_in();
	size_t pa_config(
		int device,	/* PortAudio device */
		PaStreamParameters *params);

	bool	decode(char **buf, size_t *n);
	double		sample_rate();

	enum error	seek(uint64_t usec);

	/* Unit conversion */
	uint64_t	samples2usec(size_t samples);
	size_t		usec2samples(uint64_t usec);
	size_t		bytes2samples(size_t bytes);
	size_t		samples2bytes(size_t samples);

private:
	AVStream *stream;
	int stream_id;

	std::unique_ptr<AVFormatContext, std::function<void(AVFormatContext *)>> context;
	std::unique_ptr<AVPacket, std::function<void(AVPacket *)>> packet;	/* Last undecoded packet */
	std::unique_ptr<AVFrame, std::function<void(AVFrame *)>> frame;	/* Last decoded frame */
	std::unique_ptr<unsigned char []> buffer;


	std::unique_ptr<SwrContext, std::function<void(SwrContext *)>> resampler;
	enum AVSampleFormat sample_format;
	bool use_resampler;
	std::unique_ptr<uint8_t, std::function<void(uint8_t *)>> resample_buffer;


	void load_file(const std::string &path);
	void init_stream();
	void init_codec(int stream, AVCodec *codec);
	void init_frame();
	void init_packet();
	void init_resampler();

	PaSampleFormat conv_sample_fmt(enum AVSampleFormat in);

	void au_in::setup_pa(PaSampleFormat sf, int device, int chans, PaStreamParameters *pars);
		
	bool decode_packet();
};

/* A structure containing a lump of decoded frame data.
 *
 * struct au_frame is an opaque structure; only audio_av.c knows its true
 * definition.
 */
struct au_frame;

#endif				/* not AUDIO_AV_H */
