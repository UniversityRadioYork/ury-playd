// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the Basic_audio, Null_audio and Audio classes.
 * @see audio/audio.cpp
 */

#ifndef PLAYD_AUDIO_H
#define PLAYD_AUDIO_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#undef max
#include <gsl/gsl>

#include "../response.h"
#include "audio_sink.h"
#include "audio_source.h"


/**
 * An audio item.
 *
 * Audio abstractly represents an audio item that can be played, stopped,
 * and queried for its position and path (or equivalent).
 *
 * Audio is a virtual interface implemented concretely by PipeAudio, and
 * also by mock implementations for testing purposes.
 *
 * @see PipeAudio
 */
class Audio
{
public:
    /// Enumeration of possible states for Audio.
    using State = Audio_sink::State;
    
	/// Virtual, empty destructor for Audio.
	virtual ~Audio() = default;

	//
	// Control interface
	//

	/**
	 * Performs an update cycle on this Audio.
	 *
	 * Depending on the Audio implementation, this may do actions such as
	 * performing a decoding round, checking for end-of-file, transferring
	 * frames, and so on.
	 *
	 * @return The state of the Audio after updating.
	 * @see State
	 */
	virtual State Update() = 0;

	/**
	 * Sets whether this Audio should be playing or not.
	 * @param playing True for playing; false for stopped.
	 * @exception NoAudioError if the current state is NONE.
	 */
	virtual void SetPlaying(bool playing) = 0;

	/**
	 * Attempts to seek to the given position.
	 * @param position The position to seek to, in microseconds.
	 * @exception NoAudioError if the current state is NONE.
	 * @see Position
	 */
	virtual void SetPosition(microseconds position) = 0;

	//
	// Property access
	//

	/**
	 * This Audio's current file.
	 * @return The filename of this current file.
	 * @exception NoAudioError if the current state is NONE.
	 */
	virtual const std::string &File() const = 0;

	/**
	 * The state of this Audio.
	 * @return this Audio's current state.
	 */
	virtual State CurrentState() const = 0;

	/**
	 * This Audio's current position.
	 *
	 * As this may be executing whilst the playing callback is running,
	 * do not expect it to be highly accurate.
	 *
	 * @return The current position, in microseconds.
	 * @exception NoAudioError if the current state is NONE.
	 * @see Seek
	 */
    virtual microseconds Position() const = 0;

    /**
     * This Audio's length.
     *
     * @return The length, in microseconds.
     * @exception NoAudioError if the current state is NONE.
     * @see Seek
     */
    virtual microseconds Length() const = 0;
};

/**
 * A dummy Audio implementation representing a lack of file.
 *
 * NoAudio throws exceptions if any attempt is made to change, start, or stop
 * the audio, and returns Audio::State::NONE during any attempt to Update.
 * If asked to emit the audio file, NoAudio does nothing.
 *
 * @see Audio
 */
class Null_audio : public Audio
{
public:
	Audio::State Update() override;
	Audio::State CurrentState() const override;

	// The following all raise an exception:

	void SetPlaying(bool playing) override;
	void SetPosition(microseconds position) override;
	microseconds Position() const override;
    microseconds Length() const override;
	const std::string &File() const override;
};

/**
 * A concrete implementation of Audio as a 'pipe'.
 *
 * PipeAudio is comprised of a 'source', which decodes frames from a
 * file, and a 'sink', which plays out the decoded frames.  Updating
 * consists of shifting frames from the source to the sink.
 *
 * @see Audio
 * @see AudioSink
 * @see AudioSource
 */
class Basic_audio : public Audio
{
public:
	/**
	 * Constructs audio from a source and a sink.
	 * @param src The source of decoded audio frames.
	 * @param sink The target of decoded audio frames.
	 * @see AudioSystem::Load
	 */
	Basic_audio(std::unique_ptr<Audio_source> src, std::unique_ptr<Audio_sink> sink);

	Audio::State Update() override;
	const std::string &File() const override;

	void SetPlaying(bool playing) override;
	Audio::State CurrentState() const override;

	void SetPosition(microseconds position) override;
	microseconds Position() const override;
    microseconds Length() const override;

private:
	/// The source of audio data.
	std::unique_ptr<Audio_source> src;

	/// The sink to which audio data is sent.
	std::unique_ptr<Audio_sink> sink;

	/// The current decoded frame.
	Audio_source::Decode_vector frame;

	/// A span representing the unclaimed part of the decoded frame.
	gsl::span<const uint8_t> frame_span;

	/// Clears the current frame and its iterator.
	void ClearFrame();

	/**
	 * Decodes a new frame, if the current frame is empty.
	 * @return True if more frames are available to decode; false
	 *   otherwise.
	 */
	bool DecodeIfFrameEmpty();

	/**
	 * Returns whether the current frame has been finished.
	 * If this is true, then either the frame is empty, or all of the
	 * samples in the frame have been fed to the ringbuffer.
	 * @return True if the frame is finished; false otherwise.
	 */
	bool FrameFinished() const;

	/// Transfers as much of the current frame as possible to the sink.
	void TransferFrame();
};

#endif // PLAYD_AUDIO_H
