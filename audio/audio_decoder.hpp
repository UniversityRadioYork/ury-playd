// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the AudioDecoder class.
 * @see audio/audio_decoder.cpp
 */

#ifndef PS_AUDIO_DECODER_HPP
#define PS_AUDIO_DECODER_HPP

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

#include "../constants.h"
#include "../errors.hpp"
#include "../sample_formats.hpp"

#include "audio_resample.hpp"

#include <boost/optional.hpp>

/**
 * An object responsible for decoding an audio file.
 *
 * The AudioDecoder is an interface to the ffmpeg library, which represents all
 * the ffmpeg state associated with one file.  It can be polled to decode
 * frames of audio data, which are returned as byte vectors.
 */
class AudioDecoder : public SampleByteConverter {
public:
	/// Type of the result of Decode().
	using DecodeResult = boost::optional<Resampler::ResultVector>;

	/**
	 * Constructs an AudioDecoder.
	 * @param path The path to the file to load and decode using this
	 * decoder.
	 */
	AudioDecoder(const std::string &path);

	/**
	 * Destructs an AudioDecoder.
	 */
	~AudioDecoder();

	/**
	 * Performs a round of decoding.
	 * The decoder may return one of three types of result:
	 *
	 * - An empty optional value, which means the decoder has run out of data
	 *   to decode (for example, the file has ended);
	 * - A full optional value containing an empty buffer, which means the
	 *   decoder is still processing the current packet;
	 * - A full optional value containing a non-empty buffer, which means the
	 *   decoder successfully finished a packet.
	 *
	 * @return An optional result which may contain a vector of sample data
	 *   decoded in this round.
	 */
	DecodeResult Decode();

	/**
	 * Returns the channel count.
	 * @return The number of channels this AudioDecoder is decoding.
	 */
	std::uint8_t ChannelCount() const;

	/**
	 * Returns the sample rate.
	 * @return The output sample rate (Hz) as a double-precision floating
	 * point.
	 */
	double SampleRate() const;

	/**
	 * Returns the output sample format.
	 * @return The output sample format, as a SampleFormat.
	 */
	SampleFormat OutputSampleFormat() const;

	/**
	 * Returns the number of samples this decoder's buffer can store.
	 * @return The buffer sample capacity, in samples.
	 */
	size_t BufferSampleCapacity() const;

	/**
	 * Seeks to the given position, in microseconds.
	 * @param position  The new position in the file, in microseconds.
	 */
	void SeekToPositionMicroseconds(std::chrono::microseconds position);

	//
	// Unit conversion
	//

	/**
	 * Converts an elapsed sample count to a position in microseconds.
	 * @param samples The number of elapsed samples.
	 * @return The corresponding song position, in microseconds.
	 */
	std::chrono::microseconds PositionMicrosecondsForSampleCount(
	                std::uint64_t samples) const;

	/**
	 * Converts a position in microseconds to an elapsed sample count.
	 * @param position The song position, in microseconds.
	 * @return The corresponding number of elapsed samples.
	 */
	std::uint64_t SampleCountForPositionMicroseconds(
	                std::chrono::microseconds position) const;

	std::uint64_t SampleCountForByteCount(std::uint64_t bytes) const
	                override;
	std::uint64_t ByteCountForSampleCount(std::uint64_t samples) const
	                override;

private:
	enum class DecodeState : std::uint8_t {
		WAITING_FOR_FRAME,
		DECODING,
		END_OF_FILE
	};

	DecodeState decode_state; ///< Current state of decoding.

	AVStream *stream; ///< The FFmpeg stream being decoded.
	int stream_id; ///< The ID of the input file's audio stream to decode.

	std::unique_ptr<AVFormatContext, std::function<void(AVFormatContext *)>>
	                context; ///< The input codec context.
	AVPacket packet; ///< The last undecoded packet.
	std::unique_ptr<AVFrame, std::function<void(AVFrame *)>>
	                frame;                   ///< The last decoded frame.
	std::array<uint8_t, BUFFER_SIZE> buffer; ///< The decoding buffer.
	std::unique_ptr<Resampler> resampler;    ///< The object providing
	                                         ///resampling.

	void Open(const std::string &path);

	void InitialiseStream();
	void FindStreamInfo();
	void FindStreamAndInitialiseCodec();

	void InitialiseCodec(int stream, AVCodec *codec);
	void InitialiseFrame();
	void InitialisePacket();
	void InitialiseResampler();

	DecodeResult DoFrame();
	DecodeResult DoDecode();

	bool ReadFrame();
	bool DecodePacket();
	Resampler::ResultVector Resample();
	size_t BytesPerSample() const;
	std::pair<int, bool> AvCodecDecode();

	bool UsingPlanarSampleFormat();
	std::int64_t AvPositionFromMicroseconds(
	                std::chrono::microseconds position);
};

#endif // PS_AUDIO_DECODER_HPP
