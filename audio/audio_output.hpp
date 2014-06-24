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
	bool Update();

	bool IsHalted();

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
	void SeekToPositionMicroseconds(std::chrono::microseconds microseconds);

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
	void InitialiseRingBuffer(std::uint64_t bytes_per_sample);

	std::uint64_t ByteCountForSampleCount(std::uint64_t sample_count) const override;
	std::uint64_t SampleCountForByteCount(std::uint64_t sample_count) const override;

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

	unsigned long ReadSamplesToOutput(char *&output,
	                                  unsigned long output_capacity,
	                                  unsigned long buffered_count);

	bool DecodeIfFrameEmpty();

	void WriteAllAvailableToRingBuffer();
	void WriteToRingBuffer(std::uint64_t count);

	std::uint64_t RingBufferWriteCapacity();
	std::uint64_t RingBufferTransferCount();
	void AdvanceFrameIterator(std::uint64_t sample_count);
};

#endif // PS_AUDIO_OUTPUT_HPP
