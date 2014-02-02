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

#include <portaudio.h>

#include "contrib/pa_ringbuffer.h"

#include "audio.h"
#include "audio_av.h"
#include "audio_cb.h"		/* audio_cb_play */
#include "constants.h"

audio::DeviceList audio::ListDevices()
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

audio::audio(const std::string &path, int device)
{
	this->last_err = E_INCOMPLETE;
	this->av = std::unique_ptr<au_in>(new au_in(path));

	init_sink(device);
	init_ring_buf(this->av->samples2bytes(1L));

	this->frame_ptr = nullptr;
	this->frame_samples = 0;

	this->used_samples = 0;

	this->callback = [this](char *out, unsigned long frames_per_buf) {
		return this->cb_play(out, frames_per_buf);
	};
}

audio::~audio()
{
	free_ring_buf();

	if (this->out_strm != nullptr) {
		Pa_CloseStream(this->out_strm);
		this->out_strm = nullptr;
		dbug("closed output stream");
	}
}

void
audio::start()
{
	PaError		pa_err;
	enum error	err = E_OK;

	spin_up();
	pa_err = Pa_StartStream(this->out_strm);
	if (pa_err) {
		throw error(E_INTERNAL_ERROR, "couldn't start stream");
	}

	dbug("audio started");
}

void
audio::stop()
{
	PaError		pa_err;
	enum error	err = E_OK;

	pa_err = Pa_AbortStream(this->out_strm);
	if (pa_err) {
		throw error(E_INTERNAL_ERROR, "couldn't stop stream");
	}

	dbug("audio stopped");

	/* TODO: Possibly recover from dropping frames due to abort. */
}

/* Returns the last decoding error, or E_OK if the last decode succeeded. */
enum error
audio::last_error()
{
	return this->last_err;
}

/* Checks to see if audio playback has halted of its own accord.
 *
 * If audio is still playing, E_OK will be returned; otherwise the decoding
 * error that caused playback to halt will be returned.  E_UNKNOWN is returned
 * if playback has halted but the last error report was E_OK.
 */
bool
audio::halted()
{
	return !Pa_IsStreamActive(this->out_strm);
}

/* Gets the current played position in the song, in microseconds.
 *
 * As this may be executing whilst the playing callback is running,
 * do not expect it to be highly accurate.
 */
uint64_t
audio::usec()
{
	return this->av->samples2usec(this->used_samples);
}

size_t
audio::samples2bytes(size_t samples)
{
	return this->av->samples2bytes(samples);
}

/* Tries to place enough audio into the audio buffer to prevent a
 * buffer underrun during a player start.
 *
 * If end of file is reached, it is ignored and converted to E_OK so that it can
 * later be caught by the player callback once it runs out of sound.
 */
void
audio::spin_up()
{
    /* Either fill the ringbuf or hit the maximum spin-up size,
     * whichever happens first.  (There's a maximum in order to
     * prevent spin-up from taking massive amounts of time and
     * thus delaying playback.)
     */
	bool more = true;
	unsigned long c = RingBufferWriteCapacity();
	while (more && c > 0 && RINGBUF_SIZE - c < SPINUP_SIZE) {
		more = decode();
		c = RingBufferWriteCapacity();
	}
}

/* Attempts to seek to the given position in microseconds. */
void
audio::seek_usec(uint64_t usec)
{
	size_t samples = this->av->usec2samples(usec);
	this->av->seek(usec);

	this->frame_samples = 0;
	this->last_err = E_INCOMPLETE;
	this->used_samples = samples;	/* Update position marker */

	//while (!Pa_IsStreamStopped(this->out_strm)) {
	//	decode();
	//}; // Spin until stream finishes
	PaUtil_FlushRingBuffer(this->ring_buf.get());


}

/* Increments the used samples counter, which is used to determine the current
 * position in the song, by 'samples' samples.
 */
void
audio::inc_used_samples(uint64_t samples)
{
	this->used_samples += samples;
}

bool audio::decode()
{
	bool more_frames_available = DecodeIfFrameEmpty();
	WriteAllAvailableToRingBuffer();

	return more_frames_available;
}

/**
 * Asks the decoder to decode the next frame if the current frame has been fully used.
 * @return true if the decoder has more frames available; false if it has run out.
 */
bool audio::DecodeIfFrameEmpty()
{
	bool more = true;
	if (this->frame_samples == 0) {
		/* We need to decode some new frames! */
		more = this->av->decode(&(this->frame_ptr), &(this->frame_samples));
	}
	return true;
}

/**
 * Writes all samples currently waiting to be transferred to the ring buffer.
 */
void audio::WriteAllAvailableToRingBuffer()
{
	unsigned long count = RingBufferTransferCount();
	if (count > 0) {
		audio::WriteToRingBuffer(count);
	}
}

/** 
 * Write a given number of samples from the current frame to the ring buffer.
 * @param sample_count The number of samples to write to the ring buffer.
 */
void audio::WriteToRingBuffer(unsigned long sample_count)
{
	unsigned long written_count = PaUtil_WriteRingBuffer(this->ring_buf.get(),
		this->frame_ptr,
		static_cast<ring_buffer_size_t>(sample_count));
	if (written_count != sample_count)
		throw error(E_INTERNAL_ERROR, "ringbuf write error");

	audio::IncrementFrameMarkers(written_count);
}

/**
 * Moves the frame samples and data markers forward.
 * @param sample_count The number of samples to move the markers.
 */
void audio::IncrementFrameMarkers(unsigned long sample_count)
{
	this->frame_samples -= sample_count;
	this->frame_ptr += this->av->samples2bytes(sample_count);
}

void
audio::init_sink(int device)
{
	PaError		pa_err;
	double		sample_rate;
	PaStreamParameters pars;

	sample_rate = this->av->sample_rate();

	size_t samples_per_buf = this->av->pa_config(device, &pars);

	pa_err = Pa_OpenStream(&this->out_strm,
			       NULL,
			       &pars,
			       sample_rate,
			       samples_per_buf,
			       paClipOff,
			       audio_cb_play,
				   static_cast<void *>(&this->callback));
	if (pa_err) {
		throw error(E_AUDIO_INIT_FAIL, "couldn't open stream: %i samplerate %d", pa_err, sample_rate);
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
void
audio::init_ring_buf(size_t bytes_per_sample)
{
	/* Get rid of any existing ring buffer stuff */
	free_ring_buf();

	this->ring_data = std::unique_ptr<char[]>(new char[RINGBUF_SIZE * bytes_per_sample]);
	this->ring_buf = std::unique_ptr<PaUtilRingBuffer>(new PaUtilRingBuffer);
	if (PaUtil_InitializeRingBuffer(this->ring_buf.get(),
		static_cast<ring_buffer_size_t>(bytes_per_sample),
		static_cast<ring_buffer_size_t>(RINGBUF_SIZE),
		this->ring_data.get()) != 0) {
		throw error(E_INTERNAL_ERROR, "ringbuf failed to init");
	}
}

/* Frees an audio structure's ring buffer. */
void
audio::free_ring_buf()
{
}

/**
 * Gets the current write capacity of the ring buffer.
 * @return The write capacity, in samples.
 */
unsigned long audio::RingBufferWriteCapacity()
{
	return PaUtil_GetRingBufferWriteAvailable(this->ring_buf.get());
}

/**
 * Gets the current number of samples that may be transferred from the frame to the ring buffer.
 * @return The transfer count, in samples.
 */
unsigned long audio::RingBufferTransferCount()
{
	return std::min(static_cast<unsigned long>(this->frame_samples), RingBufferWriteCapacity());
}
