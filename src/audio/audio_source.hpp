// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

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

#include <sox.h>

#include "../errors.hpp"
#include "../sample_formats.hpp"

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
	using DecodeVector = std::vector<std::uint8_t>;

	/// Type of the result of Decode().
	using DecodeResult = std::pair<DecodeState, DecodeVector>;

	/// Type for the count of bytes per sample.
	using SampleByteCount = int;

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

	std::vector<uint8_t> buffer; ///< The decoding buffer.

	/// Pointer to the SoX context associated with this source.
	sox_format_t *context;

	/**
	 * Opens a new file for this AudioSource.
	 * @param path The absolute path to the audio file to load.
	 */
	void Open(const std::string &path);

	/// Closes the AudioSource's current file.
	void Close();
};

#endif // PS_AUDIO_SOURCE_HPP
