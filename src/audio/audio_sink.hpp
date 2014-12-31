// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the AudioSink class.
 * @see audio/audio.cpp
 */

#ifndef PLAYD_AUDIO_SINK_HPP
#define PLAYD_AUDIO_SINK_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "SDL.h"

#include "audio.hpp"
#include "audio_source.hpp"
#include "ringbuffer.hpp"
#include "sample_formats.hpp"

/**
 * An output stream for an Audio file.
 *
 * An AudioSink consists of a PortAudio output stream and a buffer that
 * stores decoded samples from the Audio object.  While active, the AudioSink
 * periodically transfers samples from its buffer to PortAudio in a separate
 * thread.
 */
class AudioSink
{
public:
	/// Type of iterators used in the Transfer() method.
	typedef AudioSource::DecodeVector::iterator TransferIterator;

	/**
	 * Constructs an AudioSink.
	 * @param source The source from which this sink will receive audio.
	 * @param device_id The device ID to which this sink will output.
	 */
	AudioSink(const AudioSource &source, int device_id);

	/// Destructs an AudioSink.
	~AudioSink();

	/**
	 * Starts the audio stream.
	 * @see Stop
	 * @see IsHalted
	 */
	void Start();

	/**
	 * Stops the audio stream.
	 * @see Start
	 * @see IsHalted
	 */
	void Stop();

	/**
	 * Gets this AudioSink's current state (playing/stopped/at end).
	 * @return The Audio::State representing this AudioSink's state.
	 * @see Audio::State
	 */
	Audio::State State();

	/**
	 * Gets the current played position in the song, in samples.
	 * As this may be executing whilst the playing callback is running,
	 * do not expect it to be highly accurate.
	 * @return The current position, as a count of elapsed samples.
	 */
	std::uint64_t Position();

	/**
	 * Sets the current played position, given a position in samples.
	 * This flushes out the AudioSink ready to receive sample data from the
	 * new position.
	 * @param samples The new position, as a count of elapsed samples.
	 * @see Position
	 */
	void SetPosition(std::uint64_t samples);

	/**
	 * Tells this AudioSink that the source has run out.
	 *
	 * When this occurs, the next time the ringbuf goes empty, the sink has
	 * also run out and should stop.
	 */
	void SourceOut();

	/**
	 * Transfers a range of sample bytes into the AudioSink.
	 * The range may be empty, but must be valid.
	 *
	 * * Precondition: @a start <= @a end, @a start and @a end point to a
	 *     valid contiguous block of sample bytes.
	 * * Postcondition: @a start <= @a end, @a old(start) <= @a start,
	 *     @a *start and @a end point to a valid contiguous block of sample
	 *     bytes.
	 *
	 * @param start An iterator denoting the start of the range.  This
	 *   iterator will be advanced by the number of bytes accepted.
	 * @param end An iterator denoting the end of the range.
	 */
	void Transfer(TransferIterator &start, const TransferIterator &end);

	/**
	 * Converts a sample format identifier from playd to SDL.
	 * @param fmt The playd sample format identifier.
	 * @return The SDL equivalent of the given SampleFormat.
	 */
	static SDL_AudioFormat SDLFormat(SampleFormat fmt);

	/**
	 * Gets the number and name of each output device entry in the
	 * AudioSystem.
	 * @return List of output devices, as strings.
	 */
	static std::vector<std::pair<int, std::string>> GetDevicesInfo();

	/**
	 * The callback proper.
	 * This is executed in a separate thread by PortAudio once a stream is
	 * playing with the callback registered to it.
	 * @param outputBuffer The output buffer to which our samples should
	 *   be written.
	 * @param numFrames The number of samples PortAudio wants to read from
	 *   @a outputBuffer.
	 */
	void Callback(std::uint8_t *inputBuffer, int numFrames);

	/**
	 * Can a sound device output sound?
	 * @param id Device ID.
	 * @return If the device can handle outputting sound.
	 */
	static bool IsOutputDevice(int id);

	/// Initialises the AudioSink's library.
	static void InitLibrary();

	/// Cleans up the AudioSink's library.
	static void CleanupLibrary();

private:
	/// The SDL device to which we are outputting sound.
	SDL_AudioDeviceID device;

	/// n, where 2^n is the capacity of the Audio ring buffer.
	/// @see RINGBUF_SIZE
	static const size_t RINGBUF_POWER;

	/// Number of bytes in one sample.
	size_t bytes_per_sample;

	/// The ring buffer used to transfer samples to the playing callback.
	RingBuffer ring_buf;

	/// The current position, in samples.
	std::uint64_t position_sample_count;

	/// Whether the current run of the audio playback stream has not yet
	/// successfully read its first set of samples from the buffer.
	bool just_started;

	/// Whether the source has run out of things to feed the sink.
	bool source_out;

	/// The decoder's current state.
	Audio::State state;
};

#endif // PLAYD_AUDIO_SINK_HPP
