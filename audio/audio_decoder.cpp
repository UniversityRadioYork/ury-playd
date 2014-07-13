// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the AudioDecoder class.
 * @see audio/audio_decoder.hpp
 */

#include <functional>
#include <string>
#include <memory>
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <map>

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

#include "../errors.hpp"
#include "../messages.h"
#include "../sample_formats.hpp"

#include "audio_decoder.hpp"
#include "audio_resample.hpp"

const size_t AudioDecoder::BUFFER_SIZE = (size_t)FF_MIN_BUFFER_SIZE;

AudioDecoder::AudioDecoder(const std::string &path)
    : decode_state(DecodeState::WAITING_FOR_FRAME), buffer(BUFFER_SIZE)
{
	Open(path);
	InitialiseStream();
	InitialisePacket();
	InitialiseFrame();
	InitialiseResampler();
}

AudioDecoder::~AudioDecoder() {}

std::uint8_t AudioDecoder::ChannelCount() const
{
	return this->stream->codec->channels;
}

size_t AudioDecoder::BufferSampleCapacity() const
{
	return SampleCountForByteCount(BUFFER_SIZE);
}

double AudioDecoder::SampleRate() const
{
	return (double)this->stream->codec->sample_rate;
}

std::uint64_t AudioDecoder::SampleCountForPositionMicroseconds(
                std::chrono::microseconds usec) const
{
	auto sample_micros = usec * SampleRate();
	return std::chrono::duration_cast<std::chrono::seconds>(sample_micros)
	                .count();
}

std::chrono::microseconds AudioDecoder::PositionMicrosecondsForSampleCount(
                std::uint64_t samples) const
{
	auto position_secs = std::chrono::seconds(samples) / SampleRate();
	return std::chrono::duration_cast<std::chrono::microseconds>(
	                position_secs);
}

std::uint64_t AudioDecoder::SampleCountForByteCount(std::uint64_t bytes) const
{
	return (bytes / ChannelCount()) / BytesPerSample();
}

std::uint64_t AudioDecoder::ByteCountForSampleCount(std::uint64_t samples) const
{
	return (samples * ChannelCount()) * BytesPerSample();
}

size_t AudioDecoder::BytesPerSample() const
{
	return av_get_bytes_per_sample(this->resampler->AVOutputFormat());
}

void AudioDecoder::SeekToPositionMicroseconds(
                std::chrono::microseconds position)
{
	std::int64_t ffmpeg_position = AvPositionFromMicroseconds(position);

	Debug("Seeking to:", ffmpeg_position);

	if (av_seek_frame(this->context.get(), this->stream_id, ffmpeg_position,
	                  AVSEEK_FLAG_ANY) != 0) {
		throw InternalError(MSG_SEEK_FAIL);
	}

	this->decode_state = DecodeState::WAITING_FOR_FRAME;
	InitialisePacket();
}

std::int64_t AudioDecoder::AvPositionFromMicroseconds(
                std::chrono::microseconds position)
{
	auto position_timebase_microseconds =
	                (position * this->stream->time_base.den) /
	                this->stream->time_base.num;
	auto position_timebase_seconds =
	                std::chrono::duration_cast<std::chrono::seconds>(
	                                position_timebase_microseconds);
	return position_timebase_seconds.count();
}

AudioDecoder::DecodeResult AudioDecoder::Decode()
{
	Resampler::ResultVector decoded;

	switch (this->decode_state) {
		case DecodeState::WAITING_FOR_FRAME:
			DoFrame();
			break;
		case DecodeState::DECODING:
			decoded = DoDecode();
			break;
		case DecodeState::END_OF_FILE:
			// Intentionally ignore
			break;
	}

	return std::make_pair(this->decode_state, decoded);
}

void AudioDecoder::DoFrame()
{
	bool read_frame = ReadFrame();

	if (!read_frame) {
		// We've run out of frames to decode.
		// (TODO: Start flushing the buffer here?)
		this->decode_state = DecodeState::END_OF_FILE;
	} else if (this->packet.stream_index == this->stream_id) {
		// Only switch to decoding if the frame belongs to the audio
		// stream.  Else, we ignore it.
		this->decode_state = DecodeState::DECODING;
	}
}

AudioDecoder::DecodeVector AudioDecoder::DoDecode()
{
	DecodeVector result;

	bool finished_decoding = DecodePacket();
	if (finished_decoding) {
		result = Resample();

		// Get ready to process the next frame.
		InitialisePacket();
		this->decode_state = DecodeState::WAITING_FOR_FRAME;
	} else {
		// Send through an empty vector, so that the audio output will
		// safely run its course and make way for the next decode round.
		result = std::vector<char>();
	}

	return result;
}

bool AudioDecoder::ReadFrame()
{
	int read_result = av_read_frame(this->context.get(), &this->packet);
	return 0 == read_result;
}

Resampler::ResultVector AudioDecoder::Resample()
{
	return this->resampler->Resample(this->frame.get());
}

static const std::map<AVSampleFormat, SampleFormat> sf_from_av = {
                {AV_SAMPLE_FMT_U8, SampleFormat::PACKED_UNSIGNED_INT_8},
                {AV_SAMPLE_FMT_S16, SampleFormat::PACKED_SIGNED_INT_16},
                {AV_SAMPLE_FMT_S32, SampleFormat::PACKED_SIGNED_INT_32},
                {AV_SAMPLE_FMT_FLT, SampleFormat::PACKED_FLOAT_32}};

/**
 * @return The sample format of the data returned by this decoder.
 */
SampleFormat AudioDecoder::OutputSampleFormat() const
{
	try { return sf_from_av.at(this->resampler->AVOutputFormat()); }
	catch (std::out_of_range) { throw FileError(MSG_DECODE_BADRATE); }
}

void AudioDecoder::Open(const std::string &path)
{
	AVFormatContext *ctx = nullptr;

	if (avformat_open_input(&ctx, path.c_str(), NULL, NULL) < 0) {
		std::ostringstream os;
		os << "couldn't open " << path;
		throw FileError(os.str());
	}

	auto free_context = [](AVFormatContext *ctx) {
		avformat_close_input(&ctx);
	};
	this->context = std::unique_ptr<AVFormatContext,
	                                decltype(free_context)>(ctx,
	                                                        free_context);
}

void AudioDecoder::InitialiseStream()
{
	FindStreamInfo();
	FindStreamAndInitialiseCodec();
}

void AudioDecoder::FindStreamInfo()
{
	if (avformat_find_stream_info(this->context.get(), NULL) < 0) {
		throw FileError(MSG_DECODE_NOAUDIO);
	}
}

void AudioDecoder::FindStreamAndInitialiseCodec()
{
	AVCodec *codec;
	int stream = av_find_best_stream(this->context.get(),
	                                 AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);

	if (stream < 0) {
		throw FileError(MSG_DECODE_NOSTREAM);
	}

	InitialiseCodec(stream, codec);
}

void AudioDecoder::InitialiseCodec(int stream, AVCodec *codec)
{
	AVCodecContext *codec_context = this->context->streams[stream]->codec;
	if (avcodec_open2(codec_context, codec, NULL) < 0) {
		throw FileError(MSG_DECODE_NOCODEC);
	}

	this->stream = this->context->streams[stream];
	this->stream_id = stream;
}

void AudioDecoder::InitialiseFrame()
{
	auto frame_deleter = [](AVFrame *frame) { av_frame_free(&frame); };
	this->frame = std::unique_ptr<AVFrame, decltype(frame_deleter)>(
	                av_frame_alloc(), frame_deleter);
	if (this->frame == nullptr) {
		throw std::bad_alloc();
	}
}

void AudioDecoder::InitialisePacket()
{
	av_init_packet(&this->packet);
	this->packet.data = &*this->buffer.begin();
	this->packet.size = this->buffer.size();
}

void AudioDecoder::InitialiseResampler()
{
	Resampler *rs;
	AVCodecContext *codec = this->stream->codec;

	if (UsingPlanarSampleFormat()) {
		rs = new PlanarResampler(codec);
	} else {
		rs = new PackedResampler(codec);
	}
	this->resampler = std::unique_ptr<Resampler>(rs);
}

bool AudioDecoder::UsingPlanarSampleFormat()
{
	return av_sample_fmt_is_planar(this->stream->codec->sample_fmt);
}

bool AudioDecoder::DecodePacket()
{
	assert(this->packet.data != nullptr);
	assert(0 < this->packet.size);

	auto decode_result = AvCodecDecode();

	int bytes_decoded = decode_result.first;
	if (bytes_decoded < 0) {
		throw FileError(MSG_DECODE_FAIL);
	}
	this->packet.data += bytes_decoded;
	this->packet.size -= bytes_decoded;
	assert(0 <= this->packet.size);

	bool frame_finished = decode_result.second;
	return frame_finished || (0 < this->packet.size);
}

std::pair<int, bool> AudioDecoder::AvCodecDecode()
{
	int frame_finished = 0;
	int bytes_decoded = avcodec_decode_audio4(
	                this->stream->codec, this->frame.get(), &frame_finished,
	                &this->packet);

	return std::make_pair(bytes_decoded, frame_finished != 0);
}
