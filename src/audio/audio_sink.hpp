// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the AudioSink class.
 * @see audio/audio.cpp
 */

#ifndef PS_AUDIO_SINK_HPP
#define PS_AUDIO_SINK_HPP

#include <cstdint>
#include <functional>
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
using PlayCallbackStepResult = std::pair<PaStreamCallbackResult, unsigned long>;

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
	/// A function that configures and returns a stream, given this
	/// AudioSink.
	using StreamConfigurator = std::function<
	                portaudio::Stream *(portaudio::CallbackInterface &)>;

	/// Type of results emitted during the play callback step.
	using PlayCallbackStepResult =
	                std::pair<PaStreamCallbackResult, unsigned long>;

	/// Type of positions measured in samples.
	using SamplePosition = std::uint64_t;

	/// Type of iterators used in the Transfer() method.
	using TransferIterator = AudioSource::DecodeVector::iterator;

	/**
	 * Constructs an AudioSink.
	 * @param c A function that can configure PortAudio streams.
	 * @param bytes_per_sample The number of bytes each audio sample
	 * occupies.
	 */
	AudioSink(const StreamConfigurator c,
	          AudioSource::SampleByteCount bytes_per_sample);

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
	RingBuffer<char, unsigned long> ring_buf;

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

#endif // PS_AUDIO_SINK_HPP
