// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the AudioOutput class.
 * @see audio/audio_output.cpp
 */

#ifndef PS_AUDIO_OUTPUT_HPP
#define PS_AUDIO_OUTPUT_HPP

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

template <typename RepT, typename SampleCountT>
class RingBuffer;

#include "audio_decoder.hpp"
#include "audio_resample.hpp"

/// Type of results emitted during the play callback step.
using PlayCallbackStepResult = std::pair<PaStreamCallbackResult, unsigned long>;

/**
 * Abstract class for objects that can configure PortAudio streams for audio
 * files.
 */
class StreamConfigurator {
public:
	/**
	 * Configures and returns a PortAudio stream for the given decoder.
	 * @param cb The object that PortAudio will call to receive audio.
	 * @param decoder The decoder whose output will be fed into the stream.
	 * @return The configured PortAudio stream.
	 */
	virtual portaudio::Stream *Configure(
	                portaudio::CallbackInterface &cb,
	                const AudioDecoder &decoder) const = 0;
};

/**
 * An audio file and its associated decoder, ringbuffer and output stream.
 *
 * AudioOutput contains all state pertaining to the output of one file to one
 * stream.  It contains an AudioDecoder, a RingBuffer, and implements the
 * portaudio::CallbackInterface (allowing it to send PortAudio decoded audio)
 * and SampleByteConverter (allowing it to be queried for conversions from
 * sample counts to byte counts).
 */
class AudioOutput : portaudio::CallbackInterface, SampleByteConverter {
public:
	/**
	 * Loads a file and constructs an AudioOutput for it.
	 * @param path The absolute path to the file to open.
	 * @param c An object that can configure PortAudio streams.  This will
	 *   usually be the AudioSystem.
	 * @see AudioSystem::Load
	 */
	AudioOutput(const std::string &path, const StreamConfigurator &c);
	~AudioOutput();

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
	 * Performs an update cycle on this AudioOutput.
	 * This ensures the ring buffer has output to offer to the sound driver.
	 * It does this by by asking the AudioDecoder to decode if necessary.
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
	 * @return  True if the frame is finished; false otherwise.
	 */
	bool FrameFinished();

	/**
	 * Returns whether the audio file has ended.
	 * This does NOT mean that playback has ended; the ring buffer may still
	 * have samples waiting to send to the audio library.
	 * @return  True if there is no audio left to decode; false otherwise.
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

	/**
	 * Tries to place enough audio into the audio buffer to prevent a
	 * buffer underrun during a player start.
	 */
	void PreFillRingBuffer();

private:
	// Type for the ring buffer.
	using RingBuf = RingBuffer<char, std::uint64_t>;

	/// n, where 2^n is the capacity of the AudioOutput ring buffer.
	/// @see RINGBUF_SIZE
	static const size_t RINGBUF_POWER;

	/// The number of bytes to pre-load into the buffer before playing.
	static const size_t SPINUP_SIZE;

	bool file_ended; ///< Whether the current file has stopped decoding.

	/// The audio decoder providing the actual audio data.
	std::unique_ptr<AudioDecoder> av;

	/// The current decoded frame.
	std::vector<char> frame;

	/// The current position in the current decoded frame.
	std::vector<char>::iterator frame_iterator;

	/// The ring buffer used to transfer samples to the playing callback.
	std::unique_ptr<RingBuf> ring_buf;

	/// The PortAudio stream to which this AudioOutput outputs.
	std::unique_ptr<portaudio::Stream> out_strm;

	/// The current position, in samples.
	uint64_t position_sample_count;

	/**
	 * Clears the current frame and its iterator.
	 */
	void ClearFrame();

	std::uint64_t ByteCountForSampleCount(
	                std::uint64_t sample_count) const override;
	std::uint64_t SampleCountForByteCount(
	                std::uint64_t sample_count) const override;

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

	/**
	 * Decode the next frame if the current frame has been fully used.
	 * @return True if there were some samples left to decode; false
	 *   otherwise.
	 */
	bool DecodeIfFrameEmpty();

	//
	// Ring buffer
	//

	/**
	 * Writes all samples currently waiting to be transferred to the ring
	 * buffer.
	 */
	void WriteAllAvailableToRingBuffer();

	/**
	 * Write a given number of samples from the current frame to the ring
	 * buffer.
	 * @param sample_count  The number of samples to write to the ring
	 *                      buffer.
	 */
	void WriteToRingBuffer(std::uint64_t sample_count);

	/**
	 * The current write capacity of the ring buffer.
	 * @return The write capacity, in samples.
	 */
	std::uint64_t RingBufferWriteCapacity();

	/**
	 * The current read capacity of the ring buffer.
	 * @return The read capacity, in samples.
	 */
	std::uint64_t RingBufferReadCapacity();

	/**
	 * The number of samples that may currently be placed in the ringbuffer.
	 * the ring buffer.
	 * @return The transfer count, in samples.
	 */
	std::uint64_t RingBufferTransferCount();

	/**
	 * Moves the decoded data iterator forwards by a number of samples.
	 *
	 * If the iterator runs off the end of the decoded data vector, the data
	 * vector is freed and set to nullptr, so that a new decoding run can
	 * occur.
	 *
	 * @param sample_count The number of samples to move the markers.
	 */
	void AdvanceFrameIterator(std::uint64_t sample_count);
};

#endif // PS_AUDIO_OUTPUT_HPP
