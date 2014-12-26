// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the AudioSource class.
 * @see audio/audio_source.cpp
 */

#ifndef PLAYD_AUDIO_SOURCE_HPP
#define PLAYD_AUDIO_SOURCE_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "../errors.hpp"
#include "sample_formats.hpp"

/**
 * An object responsible for decoding an audio file.
 *
 * AudioSource is an abstract base class, implemented separately for each
 * supported audio file format.
 *
 * @see FlacAudioSource
 * @see Mp3AudioSource
 * @see OggAudioSource
 */
class AudioSource
{
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
	typedef std::vector<std::uint8_t> DecodeVector;

	/// Type of the result of Decode().
	typedef std::pair<DecodeState, DecodeVector> DecodeResult;

	/// Type for the count of bytes per sample.
	typedef int SampleByteCount;

	/**
	 * Constructs an AudioSource.
	 * @param path The path to the file from which this AudioSource is
	 *   decoding.
	 */
	AudioSource(const std::string &path);

	//
	// Methods that must be overridden
	//

	/**
	 * Performs a round of decoding.
	 * @return A pair of the decoder's state upon finishing the decoding
	 *   round and the vector of bytes decoded.  The vector may be empty,
	 *   if the decoding round did not finish off a frame.
	 */
	virtual DecodeResult Decode() = 0;

	/**
	 * Returns the channel count.
	 * @return The number of channels this AudioSource is decoding.
	 */
	virtual std::uint8_t ChannelCount() const = 0;

	/**
	 * Returns the sample rate.
	 * Should fail if, for some peculiar reason, the sample rate is above
	 * ((2^31) - 1)Hz; this probably implies something is wrong anyway.
	 * @return The output sample rate (Hz) as a 32-bit unsigned integer.
	 */
	virtual std::uint32_t SampleRate() const = 0;

	/**
	 * Returns the output sample format.
	 * @return The output sample format, as a SampleFormat.
	 */
	virtual SampleFormat OutputSampleFormat() const = 0;

	/**
	 * Seeks to the given position, in microseconds.
	 * For convenience, the new position (in terms of samples) is returned.
	 * @param position  The new position in the file, in microseconds.
	 * @return The new position in the file, in samples.
	 */
	virtual std::uint64_t Seek(std::uint64_t position) = 0;

	//
	// Methods provided 'for free'
	//

	/**
	 * Returns the number of bytes for each sample this decoder outputs.
	 * As the decoder returns packed samples, this includes the channel
	 * count as a factor.
	 * @return The number of bytes per sample.
	 */
	size_t BytesPerSample() const;

	/**
	 * Gets the file-path of this audio source's audio file.
	 * @return The audio file's path.
	 */
	const std::string &Path() const;

	/**
	 * Converts a position in microseconds to an elapsed sample count.
	 * @param position The song position, in microseconds.
	 * @return The corresponding number of elapsed samples.
	 */
	std::uint64_t SamplesFromMicros(std::uint64_t micros) const;

	/**
	 * Converts an elapsed sample count to a position in microseconds.
	 * @param samples The number of elapsed samples.
	 * @return The corresponding song position, in microseconds.
	 */
	std::uint64_t MicrosFromSamples(std::uint64_t samples) const;

protected:
	/// The file-path of this AudioSource's audio file.
	std::string path;
};

#endif // PLAYD_AUDIO_SOURCE_HPP
