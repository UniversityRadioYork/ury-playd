/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
 */

#define _POSIX_C_SOURCE 200809

extern "C" {
#ifdef WIN32
#define inline __inline
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#ifdef WIN32
#undef inline
#endif
}

#include <algorithm>
#include <map>
#include <string>
#include <sstream>

#include <portaudio.h>

#include "contrib/pa_ringbuffer.h"

#include "audio.h"
#include "audio_av.h"
#include "audio_cb.h"		/* audio_cb_play */
#include "constants.h"

/**
 * Initialises the libraries required for the audio subsystem.
 */
void AudioOutput::InitialiseLibraries()
{
	if (Pa_Initialize() != (int)paNoError) {
		throw Error(ErrorCode::AUDIO_INIT_FAIL, "couldn't init portaudio");
	}
	av_register_all();
}

/**
 * Cleans up the libraries initialised by InitialiseLibraries.
 */
void AudioOutput::CleanupLibraries()
{
	Pa_Terminate();
}

AudioOutput::DeviceList AudioOutput::ListDevices()
{
	const PaDeviceInfo *dev;
	DeviceList devices = {};

	PaDeviceIndex num_devices = Pa_GetDeviceCount();
	for (PaDeviceIndex i = 0; i < num_devices; i++) {
		dev = Pa_GetDeviceInfo(i);
		devices.emplace(std::to_string(i), std::string(dev->name));
	}

	return devices;
}

AudioOutput::AudioOutput(const std::string &path, const std::string &device_id)
{
	this->last_error = ErrorCode::INCOMPLETE;
	this->av = std::unique_ptr<au_in>(new au_in(path));

	InitialisePortAudio(DeviceIdToPa(device_id));
	InitialiseRingBuffer(ByteCountForSampleCount(1L));

	this->frame_ptr = nullptr;
	this->frame_samples = 0;

	this->position_sample_count = 0;

	this->callback = [this](char *out, unsigned long frames_per_buf) {
		return this->PlayCallback(out, frames_per_buf);
	};
}

AudioOutput::~AudioOutput()
{
	if (this->out_strm != nullptr) {
		Pa_CloseStream(this->out_strm);
		this->out_strm = nullptr;
		Debug("closed output stream");
	}
}

void AudioOutput::Start()
{
	PreFillRingBuffer();

	PaError pa_err = Pa_StartStream(this->out_strm);
	if (pa_err != paNoError) {
		throw Error(ErrorCode::INTERNAL_ERROR, "couldn't start stream");
	}

	Debug("audio started");
}

void AudioOutput::Stop()
{
	PaError		pa_err = Pa_AbortStream(this->out_strm);
	if (pa_err != paNoError) {
		throw Error(ErrorCode::INTERNAL_ERROR, "couldn't stop stream");
	}

	Debug("audio stopped");

	/* TODO: Possibly recover from dropping frames due to abort. */
}

/* Returns the last decoding error, or E_OK if the last decode succeeded. */
ErrorCode AudioOutput::LastError()
{
	return this->last_error;
}

/* Checks to see if audio playback has halted of its own accord.
 *
 * If audio is still playing, E_OK will be returned; otherwise the decoding
 * error that caused playback to halt will be returned.  E_UNKNOWN is returned
 * if playback has halted but the last error report was E_OK.
 */
bool AudioOutput::IsHalted()
{
	return !Pa_IsStreamActive(this->out_strm);
}

/* Gets the current played position in the song, in microseconds.
 *
 * As this may be executing whilst the playing callback is running,
 * do not expect it to be highly accurate.
 */
std::chrono::microseconds AudioOutput::CurrentPositionMicroseconds()
{
	return this->av->PositionMicrosecondsForSampleCount(this->position_sample_count);
}

size_t AudioOutput::ByteCountForSampleCount(size_t samples)
{
	return this->av->ByteCountForSampleCount(samples);
}

/* Tries to place enough audio into the audio buffer to prevent a
 * buffer underrun during a player start.
 *
 * If end of file is reached, it is ignored and converted to E_OK so that it can
 * later be caught by the player callback once it runs out of sound.
 */
void AudioOutput::PreFillRingBuffer()
{
    /* Either fill the ringbuf or hit the maximum spin-up size,
     * whichever happens first.  (There's a maximum in order to
     * prevent spin-up from taking massive amounts of time and
     * thus delaying playback.)
     */
	bool more = true;
	unsigned long c = RingBufferWriteCapacity();
	while (more && c > 0 && RINGBUF_SIZE - c < SPINUP_SIZE) {
		more = Update();
		c = RingBufferWriteCapacity();
	}
}

/* Attempts to seek to the given position in microseconds. */
void AudioOutput::SeekToPositionMicroseconds(std::chrono::microseconds microseconds)
{
	size_t new_position_sample_count = this->av->SampleCountForPositionMicroseconds(microseconds);
	this->av->SeekToPositionMicroseconds(microseconds);

	this->frame_samples = 0;
	this->last_error = ErrorCode::INCOMPLETE;
	this->position_sample_count = new_position_sample_count;

	//while (!Pa_IsStreamStopped(this->out_strm)) {
	//	decode();
	//}; // Spin until stream finishes
	PaUtil_FlushRingBuffer(this->ring_buf.get());


}

/* Increments the used samples counter, which is used to determine the current
 * position in the song, by 'samples' samples.
 */
void AudioOutput::AdvancePositionBySampleCount(uint64_t sample_count)
{
	this->position_sample_count += sample_count;
}

/**
 * Performs an update cycle on this AudioOutput.
 * This ensures the ring buffer has output to offer to the sound driver.
 * It does this by by asking the AudioDecoder to decode if necessary.
 *
 * @return True if there is more output to send to the sound card; false otherwise.
 */
bool AudioOutput::Update()
{
	bool more_frames_available = DecodeIfFrameEmpty();
	WriteAllAvailableToRingBuffer();

	return more_frames_available;
}

/**
 * Asks the decoder to decode the next frame if the current frame has been fully used.
 * @return true if the decoder has more frames available; false if it has run out.
 */
bool AudioOutput::DecodeIfFrameEmpty()
{
	bool more = true;
	if (this->frame_samples == 0) {
		/* We need to decode some new frames! */
		more = this->av->Decode(&(this->frame_ptr), &(this->frame_samples));
	}
	return true;
}

/**
 * Writes all samples currently waiting to be transferred to the ring buffer.
 */
void AudioOutput::WriteAllAvailableToRingBuffer()
{
	unsigned long count = RingBufferTransferCount();
	if (count > 0) {
		WriteToRingBuffer(count);
	}
}

/** 
 * Write a given number of samples from the current frame to the ring buffer.
 * @param sample_count The number of samples to write to the ring buffer.
 */
void AudioOutput::WriteToRingBuffer(unsigned long sample_count)
{
	unsigned long written_count = PaUtil_WriteRingBuffer(this->ring_buf.get(),
		this->frame_ptr,
		static_cast<ring_buffer_size_t>(sample_count));
	if (written_count != sample_count) {
		throw Error(ErrorCode::INTERNAL_ERROR, "ringbuf write error");
	}

	IncrementFrameMarkers(written_count);
}

/**
 * Moves the frame samples and data markers forward.
 * @param sample_count The number of samples to move the markers.
 */
void AudioOutput::IncrementFrameMarkers(unsigned long sample_count)
{
	this->frame_samples -= sample_count;
	this->frame_ptr += this->av->ByteCountForSampleCount(sample_count);
}

void AudioOutput::InitialisePortAudio(int device)
{
	double sample_rate = this->av->SampleRate();
	PaStreamParameters pars;
	size_t samples_per_buf = this->av->SetupPortAudio(device, &pars);
	PaError pa_err = Pa_OpenStream(&this->out_strm,
			       NULL,
			       &pars,
			       sample_rate,
			       samples_per_buf,
			       paClipOff,
			       audio_cb_play,
				   static_cast<void *>(&this->callback));
	if (pa_err != paNoError) {
		throw Error(ErrorCode::AUDIO_INIT_FAIL, "couldn't open stream");
	}
}

/* Initialises an audio structure's ring buffer so that decoded
 * samples can be placed into it.
 *
 * Any existing ring buffer will be freed.
 *
 * The number of bytes for each sample must be provided; see
 * audio_av_samples2bytes for one way of getting this.
 */
void AudioOutput::InitialiseRingBuffer(size_t bytes_per_sample)
{
	this->ring_data = std::unique_ptr<char[]>(new char[RINGBUF_SIZE * bytes_per_sample]);
	this->ring_buf = std::unique_ptr<PaUtilRingBuffer>(new PaUtilRingBuffer);
	if (PaUtil_InitializeRingBuffer(this->ring_buf.get(),
		static_cast<ring_buffer_size_t>(bytes_per_sample),
		static_cast<ring_buffer_size_t>(RINGBUF_SIZE),
		this->ring_data.get()) != 0) {
		throw Error(ErrorCode::INTERNAL_ERROR, "ringbuf failed to init");
	}
}

/**
 * Gets the current write capacity of the ring buffer.
 * @return The write capacity, in samples.
 */
unsigned long AudioOutput::RingBufferWriteCapacity()
{
	return PaUtil_GetRingBufferWriteAvailable(this->ring_buf.get());
}

/**
 * Gets the current number of samples that may be transferred from the frame to the ring buffer.
 * @return The transfer count, in samples.
 */
unsigned long AudioOutput::RingBufferTransferCount()
{
	return std::min(static_cast<unsigned long>(this->frame_samples), RingBufferWriteCapacity());
}

/**
 * Converts a string device ID to a PaDeviceID.
 * @param id_string The device ID, as a string.
 * @return The device ID, as a PaDeviceID.
 */
PaDeviceIndex AudioOutput::DeviceIdToPa(const std::string &id_string)
{
	PaDeviceIndex id_pa = 0;

	std::istringstream is(id_string);
	is >> id_pa;

	if (id_pa >= Pa_GetDeviceCount()) {
		throw Error(ErrorCode::BAD_CONFIG, "Bad PortAudio ID.");
	}

	return id_pa;
}
