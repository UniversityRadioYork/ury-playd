// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the Audio_sink class.
 * @see audio/audio.cpp
 */

#ifndef PLAYD_AUDIO_SINK_H
#define PLAYD_AUDIO_SINK_H

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "SDL.h"

#include "audio_source.h"
#include "ringbuffer.h"
#include "sample_format.h"

/// Abstract class for audio output sinks.
class Audio_sink
{
public:
	/**
	 * Enumeration of possible states for audio.
	 * @see Update
	 */
	enum class State : uint8_t {
		none,    ///< There is no audio.
		stopped, ///< The audio has been stopped, or not yet played.
		playing, ///< The audio is currently playing.
		at_end,  ///< The audio has ended and can't play without a seek.
	};

	/// Virtual, empty destructor for Audio_sink.
	virtual ~Audio_sink() = default;

	/**
	 * Starts the audio stream.
	 * @see Stop
	 * @see IsHalted
	 */
	virtual void Start() = 0;

	/**
	 * Stops the audio stream.
	 * @see Start
	 * @see IsHalted
	 */
	virtual void Stop() = 0;

	/**
	 * Gets this sink's current state (playing/stopped/at end).
	 * @return The Audio_sink::State representing this sink's state.
	 * @see Audio_sink::State
	 */
	virtual State CurrentState();

	/**
	 * Gets the current played position in the song, in samples.
	 * As this may be executing whilst the playing callback is running,
	 * do not expect it to be highly accurate.
	 * @return The current position, as a count of elapsed samples.
	 */
	virtual Samples Position() = 0;

	/**
	 * Sets the current played position, given a position in samples.
	 * This flushes out the audio sink ready to receive sample data from the
	 * new position.
	 * @param samples The new position, as a count of elapsed samples.
	 * @see Position
	 */
	virtual void SetPosition(Samples samples) = 0;

	/**
	 * Tells this AudioSink that the source has run out.
	 *
	 * When this occurs, the next time the ringbuf goes empty, the sink has
	 * also run out and should stop.
	 */
	virtual void SourceOut() = 0;

	/**
	 * Transfers a span of sample bytes into the audio sink.
	 * The span may be empty, but must be valid.
	 *
	 * * Precondition: @a src must contain a whole number of samples.
	 * * Postcondition: The return value is no greater than the size
	 *     of @a src.
	 *
	 * @param src A span representing the sample bytes to transfer.
	 * @return The number of bytes transferred.
	 */
	virtual size_t Transfer(const gsl::span<const std::uint8_t> src) = 0;
};

/**
 * An output stream for audio, using SDL.
 *
 * An Sdl_audio_sink consists of an SDL output device and a buffer that stores
 * decoded samples from the Audio object.  While active, the Sdl_audio_sink
 * periodically transfers samples from its buffer to SDL2 in a separate thread.
 */
class Sdl_audio_sink : public Audio_sink
{
public:
	/**
	 * Constructs an Sdl_audio_sink.
	 * @param source The source from which this sink will receive audio.
	 * @param device_id The device ID to which this sink will output.
	 */
	Sdl_audio_sink(const Audio_source &source, int device_id);

	/// Destructs an Sdl_audio_sink.
	~Sdl_audio_sink() override;

	void Start() override;
	void Stop() override;
	Audio_sink::State CurrentState() override;
	Samples Position() override;
	void SetPosition(Samples samples) override;
	void SourceOut() override;
	size_t Transfer(const gsl::span<const std::uint8_t> src) override;

	/**
	 * The audio callback.
	 * This is executed in a separate thread by SDL once a stream is
	 * playing with the callback registered to it.
	 * @param dest The output span to which our samples should be written.
	 */
	void Callback(gsl::span<std::uint8_t> dest);

	/**
	 * Gets the number and name of each output device entry in the
	 * AudioSystem.
	 * @return List of output devices, as strings.
	 */
	static std::vector<std::pair<int, std::string>> GetDevicesInfo();

	/**
	 * Can a sound device output sound?
	 * @param id Device ID.
	 * @return If the device can handle outputting sound.
	 */
	static bool IsOutputDevice(int id);

	/// Initialises the AudioSink's libraries, if not initialised already.
	static void InitLibrary();

	/// Cleans up the AudioSink's libraries, if not cleaned up already.
	static void CleanupLibrary();

private:
	/// The SDL device to which we are outputting sound.
	SDL_AudioDeviceID device;

	/// n, where 2^n is the capacity of the Audio ring buffer.
	static constexpr size_t ringbuf_power = 16;

	/// Mapping from SampleFormats to their equivalent SDL_AudioFormats.
	static const std::array<SDL_AudioFormat, sample_format_count> formats;

	/// Number of bytes in one sample.
	size_t bytes_per_sample;

	/// The ring buffer used to transfer samples to the playing callback.
	Ring_buffer ring_buf;

	/// The current position, in samples.
	Samples position_sample_count;

	/// Whether the source has run out of things to feed the sink.
	bool source_out;

	/// The decoder's current state.
	Audio_sink::State state;
};

#endif // PLAYD_AUDIO_SINK_H
