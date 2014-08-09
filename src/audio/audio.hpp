// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the Audio class.
 * @see audio/audio.cpp
 */

#ifndef PS_AUDIO_HPP
#define PS_AUDIO_HPP

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "portaudio.h"
#include "portaudiocpp/CallbackInterface.hxx"
namespace portaudio {
class Stream;
}

class AudioSource;
class AudioSink;

/**
 * An audio file.
 *
 * Audio contains all state pertaining to the output of one file to one
 * stream.  It contains a source of audio data and a sink to which it should
 * be sent.
 */
class Audio {
public:
	/**
	 * Constructs an Audio from a source and a sink.
	 * @param source The source of decoded audio frames.
	 * @param sink The target of decoded audio frames.
	 * @see AudioSystem::Load
	 */
	Audio(AudioSource *source, AudioSink *sink);

	/**
	 * Gets the file-path of this audio file.
	 * @return The audio file's path.
	 */
	std::string Path() const;

	/**
	 * Starts playback of this audio file.
	 * @see Stop
	 * @see IsStopped
	 */
	void Start();

	/**
	 * Stops playback of this audio file.
	 * @see Start
	 * @see IsHalted
	 */
	void Stop();

	/**
	 * Performs an update cycle on this Audio.
	 * This ensures the ring buffer has output to offer to the sound driver.
	 * It does this by by asking the AudioSource to decode if necessary.
	 * @return True if there is more output to send to the sound card; false
	 *   otherwise.
	 */
	bool Update();

	/**
	 * Checks to see if audio playback has stopped.
	 * @return True if the audio stream is inactive; false otherwise.
	 * @see Start
	 * @see Stop
	 */
	bool IsStopped();

	/**
	 * Returns whether the current frame has been finished.
	 * If this is true, then either the frame is empty, or all of the
	 * samples in the frame have been fed to the ringbuffer.
	 * @return True if the frame is finished; false otherwise.
	 */
	bool FrameFinished();

	/**
	 * Returns whether the audio file has ended.
	 * This does NOT mean that playback has ended; the ring buffer may still
	 * have samples waiting to send to the audio library.
	 * @return True if there is no audio left to decode; false otherwise.
	 */
	bool FileEnded();

	/**
	 * Return the current position, as a std::chrono::duration.
	 * @return The current position in the audio.
	 */
	template <typename R>
	R CurrentPosition()
	{
		return std::chrono::duration_cast<R>(
		                CurrentPositionMicroseconds());
	}

	/**
	 * Gets the current played position in the song, in microseconds.
	 * As this may be executing whilst the playing callback is running,
	 * do not expect it to be highly accurate.
	 * @return The current position, in microseconds.
	 */
	std::chrono::microseconds CurrentPositionMicroseconds();

	/**
	 * Seek to a position expressed as a std::chrono::duration.
	 * @param position The position to seek to in the audio.
	 */
	template <typename R>
	void SeekToPosition(R position)
	{
		SeekToPositionMicroseconds(std::chrono::duration_cast<
		                std::chrono::microseconds>(position));
	}

	/**
	 * Attempts to seek to the given position in microseconds.
	 * @param microseconds The position to seek to, in microseconds.
	 */
	void SeekToPositionMicroseconds(std::chrono::microseconds microseconds);

private:
	/// The source of audio data.
	std::unique_ptr<AudioSource> source;

	/// The sink to which audio data is sent.
	std::unique_ptr<AudioSink> sink;

	/// Whether the current file has stopped decoding.
	bool file_ended;

	/// The current decoded frame.
	std::vector<char> frame;

	/// The current position in the current decoded frame.
	std::vector<char>::iterator frame_iterator;

	/// Clears the current frame and its iterator.
	void ClearFrame();

	bool DecodeIfFrameEmpty();

	/// Transfers as much of the current frame as possible to the sink.
	void TransferFrame();
};

#endif // PS_AUDIO_HPP
