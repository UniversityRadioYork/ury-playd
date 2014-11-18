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

#include "portaudio.h"
#include "portaudiocpp/CallbackInterface.hxx"
#include "portaudiocpp/Stream.hxx"

#include "audio_source.hpp"
#include "ringbuffer.hpp"

/// Type of results emitted during the play callback step.
typedef std::pair<PaStreamCallbackResult, unsigned long> PlayCallbackStepResult;

/**
 * Interface for objects that can configure a PortAudio stream from an
 * AudioSource.
 */
class AudioSinkConfigurator {
public:
	/**
	 * Configures and returns a PortAudio stream.
	 * @param source The audio source to use to configure the stream.
	 * @param cb The object that PortAudio will call to receive audio.
	 * @return The configured PortAudio stream.
	 */
	virtual portaudio::Stream *Configure(
	                const AudioSource &source,
	                portaudio::CallbackInterface &cb) const = 0;
};

/**
 * An output stream for an Audio file.
 *
 * An AudioSink consists of a PortAudio output stream and a buffer that
 * stores decoded samples from the Audio object.  While active, the AudioSink
 * periodically transfers samples from its buffer to PortAudio in a separate
 * thread.
 */
class AudioSink : portaudio::CallbackInterface {
public:
	/// Type of positions measured in samples.
	typedef std::uint64_t SamplePosition;

	/// Type of iterators used in the Transfer() method.
	typedef AudioSource::DecodeVector::iterator TransferIterator;

	/**
	 * Constructs an AudioSink.
	 * @param source The source from which this sink will receive audio.
	 * @param conf The configurator to use to create streams for this sink.
	 */
	AudioSink(const AudioSource &source, const AudioSinkConfigurator &conf);

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
	 * Checks to see if audio playback has stopped.
	 * @return True if the audio stream is inactive; false otherwise.
	 * @see Start
	 * @see Stop
	 */
	bool IsStopped();

	/**
	 * Gets the current played position in the song, in samples.
	 * As this may be executing whilst the playing callback is running,
	 * do not expect it to be highly accurate.
	 * @return The current position, as a count of elapsed samples.
	 * @see SamplePosition
	 */
	SamplePosition Position();

	/**
	 * Sets the current played position, given a position in samples.
	 * This flushes out the AudioSink ready to receive sample data from the
	 * new position.
	 * @param samples The new position, as a count of elapsed samples.
	 * @see Position
	 */
	void SetPosition(SamplePosition samples);

	/**
	 * Gets whether this AudioSink is expecting input.
	 * @return Whether the input-ready flag has been set.
	 * @see SetInputReady
	 */
	bool InputReady();

	/**
	 * Set whether this AudioSink can expect input.
	 * @param ready True if there is input ready; false otherwise.
	 * @see InputReady
	 */
	void SetInputReady(bool ready);

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

private:
	/// n, where 2^n is the capacity of the Audio ring buffer.
	/// @see RINGBUF_SIZE
	static const size_t RINGBUF_POWER;

	/// Number of bytes in one sample.
	AudioSource::SampleByteCount bytes_per_sample;

	/// The ring buffer used to transfer samples to the playing callback.
	RingBuffer ring_buf;

	/// The PortAudio stream to which this AudioSink outputs.
	std::unique_ptr<portaudio::Stream> stream;

	/// The current position, in samples.
	std::uint64_t position_sample_count;

	/// Whether the current run of the audio playback stream has not yet
	/// successfully read its first set of samples from the buffer.
	bool just_started;

	/// Whether there is input ready for this sink.
	bool input_ready;

	/**
	 * The callback proper.
	 * This is executed in a separate thread by PortAudio once a stream is
	 * playing with the callback registered to it.
	 * @param inputBuffer The buffer containing input data; ignored.
	 * @param outputBuffer The output buffer to which our samples should
	 *   be written.
	 * @param numFrames The number of samples PortAudio wants to read from
	 *   @a outputBuffer.
	 * @param timeInfo The time information from PortAudio; ignored.
	 * @param statusFlags The PortAudio status flags; ignored.
	 * @return The number of samples written to the output buffer.
	 */
	int paCallbackFun(const void *inputBuffer, void *outputBuffer,
	                  unsigned long numFrames,
	                  const PaStreamCallbackTimeInfo *timeInfo,
	                  PaStreamCallbackFlags statusFlags) override;

	/**
	 * Performs one step in the callback.
	 * @param out The output buffer to which our samples should be written.
	 * @param frames_per_buf The number of samples PortAudio wants to read
	 *   @a out.
	 * @param in The result from the previous PlayCallbackStep.
	 * @return A PlayCallbackStepResult containing the status code and
	 *   number of samples written as of the end of this step.
	 */
	PlayCallbackStepResult PlayCallbackStep(char *out,
	                                        unsigned long frames_per_buf,
	                                        PlayCallbackStepResult in);

	/**
	 * Performs a successful play callback step.
	 * This ensures that the appropriate number of samples are read into
	 * PortAudio's buffer.
	 * @param out The output buffer to which our samples should be written.
	 * @param avail The number of samples available in the ring buffer.
	 * @param frames_per_buf The number of samples PortAudio wants to read
	 *   @a out.
	 * @param in The result from the previous PlayCallbackStep.
	 * @return A PlayCallbackStepResult containing the status code and
	 *   number of samples written as of the end of this step.
	 */
	PlayCallbackStepResult PlayCallbackSuccess(char *out,
	                                           unsigned long avail,
	                                           unsigned long frames_per_buf,
	                                           PlayCallbackStepResult in);

	/**
	 * Performs error cleanup for a failed play callback step.
	 * This ensures that the output buffer is filled with silence and the
	 * correct error code is returned.
	 * @param out The output buffer to which our samples should be written.
	 * @param avail The number of samples available in the ring buffer.
	 * @param frames_per_buf The number of samples PortAudio wants to read
	 *   @a out.
	 * @param in The result from the previous PlayCallbackStep.
	 * @return A PlayCallbackStepResult containing the status code and
	 *   number of samples written as of the end of this step.
	 */
	PlayCallbackStepResult PlayCallbackFailure(char *out,
	                                           unsigned long avail,
	                                           unsigned long frames_per_buf,
	                                           PlayCallbackStepResult in);

	/**
	 * Reads from the ringbuffer to output, updating the used samples count.
	 * @param output A reference to the output buffer's current pointer.
	 * @param output_capacity The capacity of the output buffer, in samples.
	 * @param buffered_count The number of samples available in the ring
	 *   buffer.
	 * @return The number of samples successfully written to the output
	 *   buffer.
	 */
	unsigned long ReadSamplesToOutput(char *&output,
	                                  unsigned long output_capacity,
	                                  unsigned long buffered_count);
};

#endif // PLAYD_AUDIO_SINK_HPP
