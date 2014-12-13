// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the SoxAudioSource class.
 * @see audio/audio_source.cpp
 */

#ifndef PLAYD_AUDIO_SOURCE_SOX_HPP
#define PLAYD_AUDIO_SOURCE_SOX_HPP

#include <cstdint>
#include <string>
#include <vector>

#include <sox.h>

#include "../../errors.hpp"
#include "../../sample_formats.hpp"
#include "../audio_source.hpp"

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
