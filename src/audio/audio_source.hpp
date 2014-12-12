// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the AudioSource class.
 * @see audio/audio_source.cpp
 */

#ifndef PLAYD_AUDIO_SOURCE_HPP
#define PLAYD_AUDIO_SOURCE_HPP

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <mpg123.h>
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
	 * Gets the file-path of this audio source's audio file.
	 * @return The audio file's path.
	 */
	virtual std::string Path() const = 0;

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
	 * @return The output sample rate (Hz) as a double-precision floating
	 * point.
	 */
	virtual double SampleRate() const = 0;

	/**
	 * Returns the output sample format.
	 * @return The output sample format, as a SampleFormat.
	 */
	virtual SampleFormat OutputSampleFormat() const = 0;

	/**
	 * Returns the number of bytes for each sample this decoder outputs.
	 * As the decoder returns packed samples, this includes the channel
	 *   count as a factor.
	 * @return The number of bytes per sample.
	 */
	virtual size_t BytesPerSample() const = 0;

	/**
	 * Seeks to the given position, in microseconds.
	 * For convenience, the new position (in terms of samples) is returned.
	 * @param position  The new position in the file, in microseconds.
	 * @return The new position in the file, in samples.
	 */
	virtual std::uint64_t Seek(std::uint64_t position) = 0;

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
};

class Mp3AudioSource : public AudioSource
{
public:
	/**
	 * Constructs an Mp3AudioSource.
	 * @param path The path to the file to load and decode using this
	 *   decoder.
	 */
	Mp3AudioSource(const std::string &path);

	/// Destructs an Mp3AudioSource.
	~Mp3AudioSource();

	DecodeResult Decode() override;
	std::uint64_t Seek(std::uint64_t position) override;

	std::string Path() const override;
	std::uint8_t ChannelCount() const override;
	double SampleRate() const override;
	SampleFormat OutputSampleFormat() const override;
	size_t BytesPerSample() const override;

private:
	/// The size of the internal decoding buffer, in bytes.
	static const size_t BUFFER_SIZE;

	/// The current state of decoding.
	/// @see DecodeState
	DecodeState decode_state;

	std::vector<uint8_t> buffer; ///< The decoding buffer.

	/// Pointer to the mpg123 context associated with this source.
	mpg123_handle *context;

	/// The current path.
	/// @see Path
	std::string path;

	/**
	 * Adds a format for the given sample rate to mpg123.
	 * @param rate The sample rate to add.
	 */
	void AddFormat(long rate);
};

class SoXAudioSource : public AudioSource
{
public:
	/**
	 * Constructs an SoXAudioSource.
	 * @param path The path to the file to load and decode using this
	 *   decoder.
	 */
	SoXAudioSource(const std::string &path);

	/// Destructs a SoXAudioSource.
	~SoXAudioSource();

	DecodeResult Decode() override;
	std::uint64_t Seek(std::uint64_t position) override;

	std::string Path() const override;
	std::uint8_t ChannelCount() const override;
	double SampleRate() const override;
	SampleFormat OutputSampleFormat() const override;
	size_t BytesPerSample() const override;

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

	/**
	 * Returns the number of samples this decoder's buffer can store.
	 * @return The buffer sample capacity, in samples.
	 */
	size_t BufferSampleCapacity() const;

};

#endif // PLAYD_AUDIO_SOURCE_HPP
