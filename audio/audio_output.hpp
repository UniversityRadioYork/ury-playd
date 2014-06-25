/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#ifndef PS_AUDIO_OUTPUT_HPP
#define PS_AUDIO_OUTPUT_HPP

#include <map>
#include <memory>
#include <string>
#include <cstdint>

#include "portaudiocpp/PortAudioCpp.hxx"

#include "../constants.h"
#include "../errors.hpp"
#include "../ringbuffer/ringbuffer.hpp"

#include "audio_decoder.hpp"
#include "audio_resample.hpp"

using PlayCallbackStepResult = std::pair<PaStreamCallbackResult, unsigned long>;

/**
 * Abstract class for objects that can configure PortAudio streams for audio
 * files.
 */
class StreamConfigurator {
public:
	/**
	 * Configures and returns a PortAudio stream for the given decoder.
	 * @param decoder  The decoder whose output will be fed into the stream.
	 * @return         The configured PortAudio stream.
	 */
	virtual portaudio::Stream *Configure(portaudio::CallbackInterface &cb,
	                                     const AudioDecoder &av) const = 0;
};

/* The audio structure contains all state pertaining to the currently
 * playing audio file.
 *
 * struct audio is an opaque structure; only audio.c knows its true
 * definition.
 */
class AudioOutput : portaudio::CallbackInterface, SampleByteConverter {
public:
	/* Loads a file and constructs an audio structure to hold the playback
	* state.
	*/
	AudioOutput(const std::string &path, const StreamConfigurator &c);
	~AudioOutput();

	void Start();
	void Stop();

	/**
	 * Performs an update cycle on this AudioOutput.
	 * This ensures the ring buffer has output to offer to the sound driver.
	 * It does this by by asking the AudioDecoder to decode if necessary.
	 * @return True if there is more output to send to the sound card; false
	 *   otherwise.
	 */
	bool Update();

	/* Checks to see if audio playback has halted of its own accord.
	 *
	 * If audio is still playing, E_OK will be returned; otherwise the
	 *decoding
	 * error that caused playback to halt will be returned.  E_UNKNOWN is
	 *returned
	 * if playback has halted but the last error report was E_OK.
	 */
	bool IsHalted();

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

	/* Gets the current played position in the song, in microseconds.
	 *
	 * As this may be executing whilst the playing callback is running,
	 * do not expect it to be highly accurate.
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

	/* Attempts to seek to the given position in microseconds. */
	void SeekToPositionMicroseconds(std::chrono::microseconds microseconds);

	/* Tries to place enough audio into the audio buffer to prevent a
	 * buffer underrun during a player start.
	 *
	 * If end of file is reached, it is ignored and converted to E_OK so
	 *that it
	 * can later be caught by the player callback once it runs out of sound.
	 */
	void PreFillRingBuffer();

private:
	bool file_ended; ///< Whether the current file has stopped decoding.

	std::unique_ptr<AudioDecoder> av;

	std::vector<char> frame;
	std::vector<char>::iterator frame_iterator;
	std::unique_ptr<RingBuffer<char, std::uint64_t>> ring_buf;

	std::unique_ptr<portaudio::Stream> out_strm;

	uint64_t position_sample_count;

	void ClearFrame();

	void InitialisePortAudio(const StreamConfigurator &c);

	std::uint64_t ByteCountForSampleCount(std::uint64_t sample_count) const
	                override;
	std::uint64_t SampleCountForByteCount(std::uint64_t sample_count) const
	                override;

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
	PlayCallbackStepResult PlayCallbackFailure(char *out,
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

	/* Initialises an audio structure's ring buffer so that decoded
	 * samples can be placed into it.
	 *
	 * Any existing ring buffer will be freed.
	 *
	 * The number of bytes for each sample must be provided; see
	 * audio_av_samples2bytes for one way of getting this.
	 */
	void InitialiseRingBuffer(std::uint64_t bytes_per_sample);

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
