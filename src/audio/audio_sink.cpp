// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the Audio_sink class.
 * @see audio/audio_sink.h
 */

#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <string>

#include "SDL.h"

#include "../errors.h"
#include "../messages.h"
#include "audio_sink.h"
#include "audio_source.h"
#include "ringbuffer.h"
#include "sample_format.h"

//
// Audio_sink
//

Audio_sink::State Audio_sink::CurrentState()
{
	return Audio_sink::State::none;
}

//
// Sdl_audio_sink
//

/* static */ const std::array<SDL_AudioFormat, sample_format_count>
        Sdl_audio_sink::formats{{
                AUDIO_U8,  // uint8
                AUDIO_S8,  // sint8
                AUDIO_S16, // sint16
                AUDIO_S32, // sint32
                AUDIO_F32  // float32
        }};

/**
 * The callback used by SDL_Audio.
 * Trampolines back into vsink, which must point to an Sdl_audio_sink.
 */
static void SDLCallback(void *vsink, uint8_t *data, int len)
{
	Expects(vsink != nullptr);
	Expects(data != nullptr);

	auto sink = static_cast<Sdl_audio_sink *>(vsink);
	sink->Callback(gsl::span<uint8_t>(data, len));
}

Sdl_audio_sink::Sdl_audio_sink(const Audio_source &source, int device_id)
    : bytes_per_sample(source.BytesPerSample()),
      ring_buf((1 << ringbuf_power) * source.BytesPerSample()),
      position_sample_count(0),
      source_out(false),
      state(Audio_sink::State::stopped)
{
	auto name = SDL_GetAudioDeviceName(device_id, 0);
	if (name == nullptr) {
		throw Config_error(std::string("invalid device id: ") +
		                   std::to_string(device_id));
	}

	SDL_AudioSpec want;
	SDL_zero(want);
	want.freq = source.SampleRate();
	want.format = formats[static_cast<int>(source.OutputSampleFormat())];
	want.channels = source.ChannelCount();
	want.callback = &SDLCallback;
	want.userdata = static_cast<void *>(this);

	SDL_AudioSpec have;
	SDL_zero(have);

	this->device = SDL_OpenAudioDevice(name, 0, &want, &have, 0);
	if (this->device == 0) {
		throw Config_error(std::string("couldn't open device: ") +
		                   SDL_GetError());
	}
}

Sdl_audio_sink::~Sdl_audio_sink()
{
	if (this->device == 0) return;

	// Silence any currently playing audio.
	SDL_PauseAudioDevice(this->device, SDL_TRUE);
	SDL_CloseAudioDevice(this->device);
}

/* static */ void Sdl_audio_sink::InitLibrary()
{
	if (SDL_Init(SDL_INIT_AUDIO) != 0) {
		throw Config_error(std::string("could not initialise SDL: ") +
		                   SDL_GetError());
	}
}

/* static */ void Sdl_audio_sink::CleanupLibrary()
{
	SDL_Quit();
}

void Sdl_audio_sink::Start()
{
	if (this->state != Audio_sink::State::stopped) return;

	SDL_PauseAudioDevice(this->device, 0);
	this->state = Audio_sink::State::playing;
}

void Sdl_audio_sink::Stop()
{
	if (this->state == Audio_sink::State::stopped) return;

	SDL_PauseAudioDevice(this->device, 1);
	this->state = Audio_sink::State::stopped;
}

Audio_sink::State Sdl_audio_sink::CurrentState()
{
	return this->state;
}

void Sdl_audio_sink::SourceOut()
{
	// The sink should only be out if the source is.
	Expects(this->source_out || this->state != Audio_sink::State::at_end);

	this->source_out = true;
}

uint64_t Sdl_audio_sink::Position()
{
	return this->position_sample_count;
}

void Sdl_audio_sink::SetPosition(uint64_t samples)
{
	this->position_sample_count = samples;

	// We might have been at the end of the file previously.
	// If so, we might not be now, so clear the out flags.
	this->source_out = false;
	if (this->state == Audio_sink::State::at_end) {
		this->state = Audio_sink::State::stopped;
		this->Stop();
	}

	// The ringbuf will have been full of samples from the old
	// position, so we need to get rid of them.
	this->ring_buf.Flush();
}

size_t Sdl_audio_sink::Transfer(const gsl::span<const uint8_t> src)
{
	// No point transferring 0 bytes.
	if (src.empty()) return 0;

	// There should be a whole number of samples being transferred.
	Expects(src.length() % bytes_per_sample == 0);
	Expects(0 < src.length());
	auto total = static_cast<size_t>(src.length());

	// Only transfer as many bytes as the ring buffer can take,
	// truncated to the nearest sample.
	// Don't bother trying to write 0 samples!
	auto count = std::min(total, this->ring_buf.WriteCapacity());
	count -= (count % bytes_per_sample);
	if (count == 0) return 0;

	auto written_count = this->ring_buf.Write(src.first(count));
	// Since we never write more than the ring buffer can take, and we're
	// the only thread writing, the written count should equal the requested
	// written count.
	Ensures(written_count == count);
	Ensures(written_count % bytes_per_sample == 0);
	return written_count;
}

void Sdl_audio_sink::Callback(gsl::span<uint8_t> dest)
{
	Expects(0 <= dest.length());

	// How many bytes do we want to pull out of the ring buffer?
	auto req_bytes = static_cast<size_t>(dest.length());

	// Make sure anything not filled up with sound later is set to silence.
	// This is slightly inefficient (two writes to sound-filled regions
	// instead of one), but more elegant in failure cases.
	std::fill(dest.begin(), dest.end(), 0);

	// If we're not supposed to be playing, don't play anything.
	if (this->state != Audio_sink::State::playing) return;

	// Let's find out how many bytes are available in total to give SDL.
	//
	// Note: Since we run concurrently with the decoder, which is also
	// trying to modify the read capacity of the ringbuf (by adding
	// things), this technically causes a race condition when we try to
	// read `avail_samples` number of samples later.  Not to fear: the
	// actual read capacity can only be greater than or equal to
	// `avail_samples`, as this is the only place where we can *decrease*
	// it.
	auto avail_bytes = this->ring_buf.ReadCapacity();

	// Have we run out of things to feed?
	if (avail_bytes == 0) {
		// Is this a temporary condition, or have we genuinely played
		// out all we can?  If the latter, we're now out too.
		if (this->source_out) this->state = Audio_sink::State::at_end;

		// Don't even bother reading from the ring buffer.
		return;
	}

	// Of the bytes available, how many do we need?  Send this amount to
	// SDL.
	auto bytes = std::min(req_bytes, avail_bytes);
	// We should be asking for a whole number of samples.
	assert(bytes % bytes_per_sample == 0);

	auto read_bytes = this->ring_buf.Read(dest.first(bytes));

	// We should have received a whole number of samples.
	assert(read_bytes % this->bytes_per_sample == 0);
	auto read_samples = read_bytes / this->bytes_per_sample;

	this->position_sample_count += read_samples;
}

/* static */ std::vector<std::pair<int, std::string>> Sdl_audio_sink::GetDevicesInfo()
{
	std::vector<std::pair<int, std::string>> list;

	// The 0 in SDL_GetNumAudioDevices tells SDL we want playback devices.
	auto is = SDL_GetNumAudioDevices(0);
	for (auto i = 0; i < is; i++) {
		auto n = SDL_GetAudioDeviceName(i, 0);
		if (n != nullptr) list.emplace_back(i, std::string(n));
	}

	return list;
}

/* static */ bool Sdl_audio_sink::IsOutputDevice(int id)
{
	auto ids = SDL_GetNumAudioDevices(0);

	// See comment in GetDevicesInfo for why this is sufficient.
	return 0 <= id && id < ids;
}
