/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#ifndef PS_AUDIO_DECODER_HPP
#define PS_AUDIO_DECODER_HPP

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

#include "../errors.hpp"
#include "../sample_formats.hpp"

#include "audio_resample.hpp"

/**
 * An object responsible for decoding an audio file.
 *
 * The AudioDecoder is an interface to the ffmpeg library, which represents all
 * the ffmpeg state associated with one file.  It can be polled to decode
 * frames of audio data, which are returned as byte vectors.
 */
class AudioDecoder : public SampleByteConverter {
public:
	AudioDecoder(const std::string &path);
	~AudioDecoder();

	std::vector<char> Decode();

	std::uint8_t ChannelCount() const;
	double SampleRate() const;
	SampleFormat SampleFormat() const;
	size_t BufferSampleCapacity() const;

	/**
	 * Seeks to the given position, in microseconds.
	 * @param position  The new position in the file, in microseconds.
	 */
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
	std::vector<char> Resample();
	size_t BytesPerSample() const;

	bool UsingPlanarSampleFormat();
	std::int64_t AvPositionFromMicroseconds(
	                std::chrono::microseconds position);
};

#endif // PS_AUDIO_DECODER_HPP
