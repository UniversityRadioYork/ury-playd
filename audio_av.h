/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
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
	void Resample(char **buf, size_t *n);

	PaSampleFormat SampleFormatAVToPA(AVSampleFormat av_format);
};

/* A structure containing a lump of decoded frame data.
 *
 * struct au_frame is an opaque structure; only audio_av.c knows its true
 * definition.
 */
struct au_frame;

#endif				/* not AUDIO_AV_H */
