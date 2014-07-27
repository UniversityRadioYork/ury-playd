// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the AudioSink class.
 * @see audio/audio.cpp
 */

#ifndef PS_AUDIO_SINK_HPP
#define PS_AUDIO_SINK_HPP

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "portaudio.h"
#include "portaudiocpp/CallbackInterface.hxx"
#include "portaudiocpp/Stream.hxx"

#include "audio_source.hpp"
#include "audio_resample.hpp"
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
	using TransferIterator = Resampler::ResultVector::iterator;

	/**
	 * Constructs an AudioSink.
	 * @param c A function that can configure PortAudio streams.
	 * @param bytes_per_sample The number of bytes each audio sample
	 * occupies.
	 */
	AudioSink(const StreamConfigurator c,
	          Resampler::SampleByteCount bytes_per_sample);

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
	Resampler::SampleByteCount bytes_per_sample;

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
	 */
	int paCallbackFun(const void *inputBuffer, void *outputBuffer,
	                  unsigned long numFrames,
	                  const PaStreamCallbackTimeInfo *timeInfo,
	                  PaStreamCallbackFlags statusFlags) override;
	PlayCallbackStepResult PlayCallbackStep(char *out,
	                                        unsigned long frames_per_buf,
	                                        PlayCallbackStepResult in);
	PlayCallbackStepResult PlayCallbackSuccess(char *out,
	                                           unsigned long avail,
	                                           unsigned long frames_per_buf,
	                                           PlayCallbackStepResult in);
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
