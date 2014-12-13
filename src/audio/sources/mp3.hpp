// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the Mp3AudioSource class.
 * @see audio/sources/mp3.cpp
 */

#ifndef PLAYD_AUDIO_SOURCE_MP3_HPP
#define PLAYD_AUDIO_SOURCE_MP3_HPP

#include <cstdint>
#include <string>
#include <vector>

#include <mpg123.h>

#include "../audio_source.hpp"
#include "../../sample_formats.hpp"

/// AudioSource for use on MP3 files.
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

#endif // PLAYD_AUDIO_SOURCE_MP3_HPP
