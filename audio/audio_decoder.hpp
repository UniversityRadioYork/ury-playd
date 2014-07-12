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
	/// An enumeration of possible states the decoder can be in.
	enum class DecodeState : std::uint8_t {
		/// The decoder is currently trying to acquire a frame.
		WAITING_FOR_FRAME,
		/// The decoder is currently decoding a frame.
		DECODING,
		/// The decoder has run out of things to decode.
		END_OF_FILE
	};

	/// Type of decoded sample vectors.
	using DecodeVector = Resampler::ResultVector;

	/// Type of the result of Decode().
	using DecodeResult = std::pair<DecodeState, DecodeVector>;

	/**
	 * Constructs an AudioDecoder.
	 * @param path The path to the file to load and decode using this
	 *   decoder.
	 */
	AudioDecoder(const std::string &path);

	/**
	 * Destructs an AudioDecoder.
	 */
	~AudioDecoder();

	/**
	 * Performs a round of decoding.
	 * @return A pair of the decoder's state upon finishing the decoding
	 *   round and the vector of bytes decoded.  The vector may be empty,
	 *   if the decoding round did not finish off a frame.
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

	std::uint64_t SampleCountForByteCount(
	                std::uint64_t bytes) const override;
	std::uint64_t ByteCountForSampleCount(
	                std::uint64_t samples) const override;

private:
	/// The size of the internal decoding buffer, in bytes.
	static const size_t BUFFER_SIZE;

	DecodeState decode_state; ///< Current state of decoding.

	AVStream *stream; ///< The FFmpeg stream being decoded.
	int stream_id; ///< The ID of the input file's audio stream to decode.

	std::unique_ptr<AVFormatContext, std::function<void(AVFormatContext *)>>
	                context; ///< The input codec context.
	AVPacket packet;         ///< The last undecoded packet.
	std::unique_ptr<AVFrame, std::function<void(AVFrame *)>>
	                frame;                   ///< The last decoded frame.
	std::vector<uint8_t> buffer;             ///< The decoding buffer.
	std::unique_ptr<Resampler> resampler;    ///< The object providing
	/// resampling.

	void Open(const std::string &path);

	void InitialiseStream();
	void FindStreamInfo();
	void FindStreamAndInitialiseCodec();

	void InitialiseCodec(int stream, AVCodec *codec);
	void InitialiseFrame();
	void InitialisePacket();
	void InitialiseResampler();

	void DoFrame();
	DecodeVector DoDecode();

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
