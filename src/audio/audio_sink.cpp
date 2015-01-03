// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the AudioSink class.
 * @see audio/audio_sink.hpp
 */

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstring>
#include <memory>
#include <string>

#include "SDL.h"

#include "../errors.hpp"
#include "../messages.h"
#include "audio_sink.hpp"
#include "audio_source.hpp"
#include "ringbuffer.hpp"
#include "sample_formats.hpp"

std::uint64_t SdlAudioSink::instances = 0;

const size_t SdlAudioSink::RINGBUF_POWER = 16;

/**
 * The callback used by SDL_Audio.
 * Trampolines back into vsink, which must point to an SdlAudioSink.
 */
static void SDLCallback(void *vsink, std::uint8_t *data, int len)
{
	assert(vsink != nullptr);
	auto sink = static_cast<SdlAudioSink *>(vsink);
	sink->Callback(data, len);
}

/* static */ std::unique_ptr<AudioSink> SdlAudioSink::Build(const AudioSource &source, int device_id)
{
	return std::unique_ptr<AudioSink>(new SdlAudioSink(source, device_id));
}

SdlAudioSink::SdlAudioSink(const AudioSource &source, int device_id)
    : bytes_per_sample(source.BytesPerSample()),
      ring_buf(RINGBUF_POWER, source.BytesPerSample()),
      position_sample_count(0),
      just_started(false),
      source_out(false),
      state(Audio::State::STOPPED)
{
	const char *name = SDL_GetAudioDeviceName(device_id, 0);
	if (name == nullptr) {
		throw ConfigError(std::string("invalid device id: ") + std::to_string(device_id));
	}

	SDL_AudioSpec want;
	SDL_zero(want);
	want.freq = source.SampleRate();
	want.format = SDLFormat(source.OutputSampleFormat());
	want.channels = source.ChannelCount();
	want.callback = &SDLCallback;
	want.userdata = (void *)this;

	SDL_AudioSpec have;
	SDL_zero(have);

	this->device = SDL_OpenAudioDevice(name, 0, &want, &have, 0);
	if (this->device == 0) {
		throw ConfigError(std::string("couldn't open device: ") + SDL_GetError());
	}
}

SdlAudioSink::~SdlAudioSink()
{
	if (this->device == 0) return;
	SDL_CloseAudioDevice(this->device);

	SdlAudioSink::CleanupLibrary();
}

/* static */ void SdlAudioSink::InitLibrary()
{
	if (SdlAudioSink::instances++ == 0) {
		Debug() << "initialising SDL\n";
		if (SDL_Init(SDL_INIT_AUDIO) != 0) {
			throw ConfigError(std::string("could not initialise SDL: ") + SDL_GetError());
		}
	}
}

/* static */ void SdlAudioSink::CleanupLibrary()
{
	if (--SdlAudioSink::instances == 0) {
		Debug() << "quitting SDL\n";
		SDL_Quit();
	}
}

void SdlAudioSink::Start()
{
	if (this->state != Audio::State::STOPPED) return;

	this->just_started = true;
	SDL_PauseAudioDevice(this->device, 0);
	this->state = Audio::State::PLAYING;
}

void SdlAudioSink::Stop()
{
	if (this->state == Audio::State::STOPPED) return;

	SDL_PauseAudioDevice(this->device, 1);
	this->state = Audio::State::STOPPED;
}

Audio::State SdlAudioSink::State()
{
	return this->state;
}

void SdlAudioSink::SourceOut()
{
	// The sink should only be out if the source is.
	assert(this->source_out || this->state != Audio::State::AT_END);

	this->source_out = true;
}

std::uint64_t SdlAudioSink::Position()
{
	return this->position_sample_count;
}

void SdlAudioSink::SetPosition(std::uint64_t samples)
{
	this->position_sample_count = samples;

	// We might have been at the end of the file previously.
	// If so, we might not be now, so clear the out flags.
	this->source_out = false;
	if (this->state == Audio::State::AT_END) {
		this->state = Audio::State::STOPPED;
		this->Stop();
	}

	// The ringbuf will have been full of samples from the old
	// position, so we need to get rid of them.
	this->ring_buf.Flush();
}

void SdlAudioSink::Transfer(AudioSink::TransferIterator &start,
                         const AudioSink::TransferIterator &end)
{
	assert(start <= end);

	// No point transferring 0 bytes.
	if (start == end) return;

	unsigned long bytes = std::distance(start, end);
	// There should be a whole number of samples being transferred.
	assert(bytes % bytes_per_sample == 0);
	assert(0 < bytes);

	auto samples = bytes / this->bytes_per_sample;

	// Only transfer as many samples as the ring buffer can take.
	// Don't bother trying to write 0 samples!
	auto count = std::min(samples, this->ring_buf.WriteCapacity());
	if (count == 0) return;

	auto start_ptr = reinterpret_cast<char *>(&*start);
	unsigned long written_count = this->ring_buf.Write(start_ptr, count);
	// Since we never write more than the ring buffer can take, the written
	// count should equal the requested written count.
	assert(written_count == count);

	start += (written_count * this->bytes_per_sample);
	assert(start <= end);
}

void SdlAudioSink::Callback(std::uint8_t *out, int nbytes)
{
	assert(out != nullptr);

	assert(0 <= nbytes);
	unsigned long lnbytes = static_cast<unsigned long>(nbytes);

	// First of all, let's find out how many samples are available in total
	// to give SDL.
	auto avail_samples = this->ring_buf.ReadCapacity();

	// Have we run out of things to feed?
	if (this->source_out && avail_samples == 0) {
		// Then we're out too.
		this->state = Audio::State::AT_END;

		memset(out, 0, lnbytes);
		return;
	}

	// How many samples do we want to pull out of the ring buffer?
	auto req_samples = lnbytes / this->bytes_per_sample;

	// How many can we pull out?  Send this amount to SDL.
	auto samples = std::min(req_samples, avail_samples);
	auto read_samples = this->ring_buf.Read(reinterpret_cast<char *>(out), samples);
	this->position_sample_count += read_samples;

	// Now, we need to fill any gaps with silence.
	auto read_bytes = read_samples * this->bytes_per_sample;
	assert(read_bytes <= lnbytes);

	// I have too little confidence in my own mathematics sometimes.
	auto silence_bytes = lnbytes - read_bytes;
	assert(read_bytes + silence_bytes == lnbytes);

	// SILENCE WILL FALL
	memset(out + read_bytes, 0, silence_bytes);
}

/// Mappings from SampleFormats to their equivalent SDL_AudioFormats.
static const std::map<SampleFormat, SDL_AudioFormat> sdl_from_sf = {
	{ SampleFormat::PACKED_UNSIGNED_INT_8, AUDIO_U8 },
	{ SampleFormat::PACKED_SIGNED_INT_8, AUDIO_S8 },
	{ SampleFormat::PACKED_SIGNED_INT_16, AUDIO_S16 },
	{ SampleFormat::PACKED_SIGNED_INT_32, AUDIO_S32 },
	{ SampleFormat::PACKED_FLOAT_32, AUDIO_F32 }
};

/* static */ SDL_AudioFormat SdlAudioSink::SDLFormat(SampleFormat fmt)
{
	try {
		return sdl_from_sf.at(fmt);
	} catch (std::out_of_range) {
		throw FileError(MSG_DECODE_BADRATE);
	}
}

/* static */ std::vector<std::pair<int, std::string>> SdlAudioSink::GetDevicesInfo()
{
	SdlAudioSink::InitLibrary();

	decltype(SdlAudioSink::GetDevicesInfo()) list;

	// The 0 in SDL_GetNumAudioDevices tells SDL we want playback devices.
	int is = SDL_GetNumAudioDevices(0);
	for (int i = 0; i < is; i++) {
		const char *n = SDL_GetAudioDeviceName(i, 0);
		if (n == nullptr) continue;

		list.emplace_back(i, std::string(n));
	}

	SdlAudioSink::CleanupLibrary();
	return list;
}

/* static */ bool SdlAudioSink::IsOutputDevice(int id)
{
	SdlAudioSink::InitLibrary();
	int ids = SDL_GetNumAudioDevices(0);
	SdlAudioSink::CleanupLibrary();

	// See above comment for why this is sufficient.
	return (0 <= id && id < ids);
}

