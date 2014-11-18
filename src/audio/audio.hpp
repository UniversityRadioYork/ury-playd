// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the Audio class.
 * @see audio/audio.cpp
 */

#ifndef PLAYD_AUDIO_HPP
#define PLAYD_AUDIO_HPP

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

#include "../time_parser.hpp"
#include "audio_source.hpp"

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
	bool FrameFinished() const;

	/**
	 * Returns whether the audio file has ended.
	 * This does NOT mean that playback has ended; the ring buffer may still
	 * have samples waiting to send to the audio library.
	 * @return True if there is no audio left to decode; false otherwise.
	 */
	bool FileEnded() const;

	/**
	 * Gets the current played position in the song, in microseconds.
	 * As this may be executing whilst the playing callback is running,
	 * do not expect it to be highly accurate.
	 * @return The current position, in microseconds.
	 */
	TimeParser::MicrosecondPosition CurrentPositionMicroseconds();

	/**
	 * Attempts to seek to the given position in microseconds.
	 * @param microseconds The position to seek to, in microseconds.
	 */
	void SeekToPositionMicroseconds(
	                TimeParser::MicrosecondPosition microseconds);

private:
	/// The source of audio data.
	std::unique_ptr<AudioSource> source;

	/// The sink to which audio data is sent.
	std::unique_ptr<AudioSink> sink;

	/// Whether the current file has stopped decoding.
	bool file_ended;

	/// The current decoded frame.
	AudioSource::DecodeVector frame;

	/// The current position in the current decoded frame.
	AudioSource::DecodeVector::iterator frame_iterator;

	/// Clears the current frame and its iterator.
	void ClearFrame();

	/**
	 * Decodes a new frame, if the current frame is empty.
	 * @return True if more frames are available to decode; false
	 *   otherwise.
	 */
	bool DecodeIfFrameEmpty();

	/// Transfers as much of the current frame as possible to the sink.
	void TransferFrame();
};

#endif // PLAYD_AUDIO_HPP
