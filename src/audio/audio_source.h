// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the Audio_source class.
 * @see audio/audio_source.cpp
 */

#ifndef PLAYD_AUDIO_SOURCE_H
#define PLAYD_AUDIO_SOURCE_H

#include <cstdint>
#include <chrono>
#include <string>
#include <vector>

#include "../errors.h"
#include "sample_format.h"

using namespace std::chrono;

/**
 * An object responsible for decoding an audio file.
 *
 * AudioSource is an abstract base class, implemented separately for each
 * supported audio file format.
 *
 * @see Mp3_audio_source
 * @see Sndfile_audio_source
 *
 * @note When we refer to 'samples' in this class, this usually refers to
 *   the smallest unit of data for *all* channels.  Some audio decoders
 *   call the smallest unit of data for one channel a 'sample'--so that
 *   there are exactly ChannelCount() of their samples to one of ours.
 *   We usually call this a 'mono sample'.
 */
class Audio_source
{
public:
	/// An enumeration of possible states the decoder can be in.
	enum class Decode_state : std::uint8_t {
		/// The decoder is currently trying to acquire a frame.
		need_frame,
		/// The decoder is currently decoding a frame.
		decoding,
		/// The decoder has run out of things to decode.
		eof
	};

	/// Type of decoded sample vectors.
	using Decode_vector = std::vector<std::uint8_t>;

	/// Type of the result of Decode().
	using Decode_result = std::pair<Decode_state, Decode_vector>;

	/**
	 * Constructs an Audio_source.
	 * @param path The path to the file from which this AudioSource is
	 *   decoding.
	 */
	Audio_source(const std::string &path);

	/// Virtual, empty destructor for AudioSource.
	virtual ~Audio_source() = default;

	//
	// Methods that must be overridden
	//

	/**
	 * Performs a round of decoding.
	 * @return A pair of the decoder's state upon finishing the decoding
	 *   round and the vector of bytes decoded.  The vector may be empty,
	 *   if the decoding round did not finish off a frame.
	 */
	virtual Decode_result Decode() = 0;

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
	virtual Sample_format OutputSampleFormat() const = 0;

	/**
	 * Seeks to the given position, in samples.
	 * For convenience, the new position (in terms of samples) is returned.
	 * @param position The requested new position in the file, in samples.
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
	virtual size_t BytesPerSample() const;

	/**
	 * Gets the file-path of this audio source's audio file.
	 * @return The audio file's path.
	 */
	virtual const std::string &Path() const;

	/**
	 * Converts a position in microseconds to an elapsed sample count.
	 * @param micros The song position, in microseconds.
	 * @return The corresponding number of elapsed samples.
	 */
	virtual Samples SamplesFromMicros(microseconds micros) const;

	virtual std::uint64_t Length() const = 0;

	/**
	 * Converts an elapsed sample count to a position in microseconds.
	 * @param samples The number of elapsed samples.
	 * @return The corresponding song position, in microseconds.
	 */
	microseconds MicrosFromSamples(Samples samples) const;

protected:
	/// The file-path of this AudioSource's audio file.
	std::string path;
};

#endif // PLAYD_AUDIO_SOURCE_H
