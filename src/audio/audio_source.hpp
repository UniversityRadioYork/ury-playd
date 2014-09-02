// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the AudioSource class.
 * @see audio/audio_source.cpp
 */

#ifndef PS_AUDIO_SOURCE_HPP
#define PS_AUDIO_SOURCE_HPP

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libswresample/swresample.h>
}

#include "../errors.hpp"
#include "../sample_formats.hpp"
#include "audio_resample.hpp"

/**
 * An object responsible for decoding an audio file.
 *
 * The AudioSource is an interface to the ffmpeg library, which represents all
 * the ffmpeg state associated with one file.  It can be polled to decode
 * frames of audio data, which are returned as byte vectors.
 */
class AudioSource {
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
	 * Constructs an AudioSource.
	 * @param path The path to the file to load and decode using this
	 *   decoder.
	 */
	AudioSource(const std::string &path);

	/// Destructs an AudioSource.
	~AudioSource();

	/**
	 * Gets the file-path of this audio source's audio file.
	 * @return The audio file's path.
	 */
	std::string Path() const;

	/**
	 * Performs a round of decoding.
	 * @return A pair of the decoder's state upon finishing the decoding
	 *   round and the vector of bytes decoded.  The vector may be empty,
	 *   if the decoding round did not finish off a frame.
	 */
	DecodeResult Decode();

	/**
	 * Returns the channel count.
	 * @return The number of channels this AudioSource is decoding.
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
	 * Returns the number of bytes for each sample this decoder outputs.
	 * As the decoder returns packed samples, this includes the channel
	 *   count as a factor.
	 * @return The number of bytes per sample.
	 */
	size_t BytesPerSample() const;

	/**
	 * Seeks to the given position, in microseconds.
	 * For convenience, the new position (in terms of samples) is returned.
	 * @param position  The new position in the file, in microseconds.
	 * @return The new position in the file, in samples.
	 */
	std::uint64_t Seek(std::chrono::microseconds position);

	/**
	 * Converts a position in microseconds to an elapsed sample count.
	 * @param position The song position, in microseconds.
	 * @return The corresponding number of elapsed samples.
	 */
	std::uint64_t SamplePositionFromMicroseconds(
	                std::chrono::microseconds position) const;

	/**
	 * Converts an elapsed sample count to a position in microseconds.
	 * @param samples The number of elapsed samples.
	 * @return The corresponding song position, in microseconds.
	 */
	std::chrono::microseconds MicrosecondPositionFromSamples(
	                std::uint64_t samples) const;

private:
	/// The size of the internal decoding buffer, in bytes.
	static const size_t BUFFER_SIZE;

	/// The current state of decoding.
	/// @see DecodeState
	DecodeState decode_state;

	/// The ID of the audio stream to decode.
	int stream_id;

	std::vector<uint8_t> buffer; ///< The decoding buffer.

	AVStream *stream;         ///< The FFmpeg stream being decoded.
	AVFormatContext *context; ///< The input codec context.
	AVPacket packet;          ///< The last undecoded packet.
	AVFrame *frame;           ///< The last decoded frame.

	/// The resampler used to resample frames for output.
	/// @see Resample
	std::unique_ptr<Resampler> resampler;

	std::string path; ///< The path of the file loaded in this source.

	/**
	 * Opens a new file for this AudioSource.
	 * @param path The absolute path to the audio file to load.
	 */
	void Open(const std::string &path);

	/// Finds the appropriate stream and initialises the stream structures.
	void InitialiseStream();

	/// Finds the stream information for the audio file.
	void FindStreamInfo();

	/// Finds the best audio stream and initialises structures for it.
	void FindStreamAndInitialiseCodec();

	/**
	 * Opens the FFmpeg codec and sets up the appropriate streams.
	 *
	 * @param stream The index of the stream for which this AudioSource
	 *   should initialise stream structures.
	 * @param codec A pointer to an AvCodec structure containing the codec
	 *   found by FindStreamAndInitialiseCodec.
	 *
	 * @exception FileError if the codec cannot be opened.
	 */
	void InitialiseCodec(int stream, AVCodec *codec);

	/// Initialises the FFmpeg frame structure.
	void InitialiseFrame();

	/// Initialises the FFmpeg packet structure.
	void InitialisePacket();

	/// Initialises the appropriate Resampler for the loaded file.
	void InitialiseResampler();

	/// Attempts to grab a frame to decode.
	void DoFrame();

	/**
	 * Pumps the decoder.
	 * When the decoder finishes, this AudioSource will invoke the
	 * resampler and reset the state to DecodeState::WAITING_FOR_FRAME.
	 * @return A DecodeVector containing the results of decoding and
	 *   resampling if the decoder has finished, or nothing otherwise.
	 *   Note that, even if the decoder succeeds, the vector may be empty.
	 */
	DecodeVector DoDecode();

	/**
	 * Attempts to read a frame.
	 * @return True if a frame was read; false otherwise.  If this returns
	 * false, then the decoder has run out of new frames to read.
	 */
	bool ReadFrame();

	/**
	 * Attempts to decode a single packet.
	 * @return True if the decoding run has finished; false otherwise.
	 *   If true, then DecodePacket need not be called again for this
	 *   frame.
	 */
	bool DecodePacket();

	/**
	 * Performs the resampling step of the decoder.
	 * This pushes the samples from a successful decode through the
	 * resampler object, which converts them into a format suitable for
	 * outputting.
	 * @return A DecodeVector containing resampled, raw bytes suitable for
	 *   emitting from the AudioSource.
	 */
	DecodeVector Resample();

	/**
	 * Calls the AvCodec decode function and makes sense of the results.
	 * @return A pair containing the number of bytes decoded, and whether
	 *   the got-frame flag was set high.
	 */
	std::pair<int, bool> AvCodecDecode();

	/**
	 * Determines whether this AudioSource's own sample format is planar.
	 * The output format is _always_ packed (not planar), so this refers
	 * entirely to the internal, pre-resampling sample format.
	 * @return True if the internal sample format is planar; false
	 *   otherwise.
	 */
	bool UsingPlanarSampleFormat();

	/**
	 * Converts a track position, in microseconds, into AV internal units.
	 * @param position The input position, in microseconds.
	 * @return The corresponding position, in the appropriate units to be
	 *   used in FFmpeg functions for this audio source.
	 */
	std::int64_t AvPositionFromMicroseconds(
	                std::chrono::microseconds position);
};

#endif // PS_AUDIO_SOURCE_HPP
