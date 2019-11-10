// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the Audio_sink class.
 * @see audio/audio_sink.h
 */

#include "sink.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <string>

#include "../errors.h"
#include "../messages.h"
#include "SDL.h"
#include "ringbuffer.h"
#include "sample_format.h"
#include "source.h"

namespace Playd::Audio
{
//
// Sink
//

Sink::State Sink::CurrentState()
{
	return Sink::State::NONE;
}

//
// SDLSink
//

/* static */ const std::array<SDL_AudioFormat, SAMPLE_FORMAT_COUNT> SDLSink::formats{{
        AUDIO_U8,  // UINT8
        AUDIO_S8,  // SINT8
        AUDIO_S16, // SINT16
        AUDIO_S32, // SINT32
        AUDIO_F32  // FLOAT32
}};

/**
 * The callback used by SDL_Audio.
 * Trampolines back into vsink, which must point to an Sdl_audio_sink.
 */
static void SDLCallback(void *vsink, unsigned char *data, int len)
{
	Expects(vsink != nullptr);
	Expects(data != nullptr);

	auto sink = static_cast<SDLSink *>(vsink);
	sink->Callback(gsl::span<std::byte>(reinterpret_cast<std::byte *>(data), len));
}

SDLSink::SDLSink(const Audio::Source &source, int device_id)
    : bytes_per_sample{source.BytesPerSample()},
      ring_buf{(1U << RINGBUF_POWER) * source.BytesPerSample()},
      position_sample_count{0},
      source_out{false},
      state{Sink::State::STOPPED}
{
	auto name = SDL_GetAudioDeviceName(device_id, 0);
	if (name == nullptr) {
		throw ConfigError(std::string("invalid device id: ") + std::to_string(device_id));
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
		throw ConfigError(std::string("couldn't open device: ") + SDL_GetError());
	}
}

SDLSink::~SDLSink()
{
	if (this->device == 0) return;

	// Silence any currently playing audio.
	SDL_PauseAudioDevice(this->device, SDL_TRUE);
	SDL_CloseAudioDevice(this->device);
}

/* static */ void SDLSink::InitLibrary()
{
	if (SDL_Init(SDL_INIT_AUDIO) != 0) {
		throw ConfigError(std::string("could not initialise SDL: ") + SDL_GetError());
	}
}

/* static */ void SDLSink::CleanupLibrary()
{
	SDL_Quit();
}

void SDLSink::Start()
{
	if (this->state != Sink::State::STOPPED) return;

	SDL_PauseAudioDevice(this->device, 0);
	this->state = Sink::State::PLAYING;
}

void SDLSink::Stop()
{
	if (this->state == Sink::State::STOPPED) return;

	SDL_PauseAudioDevice(this->device, 1);
	this->state = Sink::State::STOPPED;
}

Sink::State SDLSink::CurrentState()
{
	return this->state;
}

void SDLSink::SourceOut()
{
	// The sink should only be out if the source is.
	Expects(this->source_out || this->state != Sink::State::AT_END);

	this->source_out = true;
}

uint64_t SDLSink::Position()
{
	return this->position_sample_count;
}

void SDLSink::SetPosition(uint64_t samples)
{
	this->position_sample_count = samples;

	// We might have been at the end of the file previously.
	// If so, we might not be now, so clear the out flags.
	this->source_out = false;
	if (this->state == Sink::State::AT_END) {
		this->state = Sink::State::STOPPED;
		this->Stop();
	}

	// The ringbuf will have been full of samples from the old
	// position, so we need to get rid of them.
	this->ring_buf.Flush();
}

size_t SDLSink::Transfer(const gsl::span<const std::byte> src)
{
	// No point transferring 0 bytes.
	if (src.empty()) return 0;

	// There should be a whole number of samples being transferred.
	Expects(src.size() % bytes_per_sample == 0);
	Expects(!src.empty());
	const auto total = static_cast<size_t>(src.size());

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

void SDLSink::Callback(gsl::span<std::byte> dest)
{
	Expects(0 <= dest.size());

	// How many bytes do we want to pull out of the ring buffer?
	const auto req_bytes = static_cast<size_t>(dest.size());

	// Make sure anything not filled up with sound later is set to silence.
	// This is slightly inefficient (two writes to sound-filled regions
	// instead of one), but more elegant in failure cases.
	std::fill(dest.begin(), dest.end(), std::byte{0});

	// If we're not supposed to be playing, don't play anything.
	if (this->state != Sink::State::PLAYING) return;

	// Let's find out how many bytes are available in total to give SDL.
	//
	// Note: Since we run concurrently with the decoder, which is also
	// trying to modify the read capacity of the ringbuf (by adding
	// things), this technically causes a race condition when we try to
	// read `avail_samples` number of samples later.  Not to fear: the
	// actual read capacity can only be greater than or equal to
	// `avail_samples`, as this is the only place where we can *decrease*
	// it.
	const auto avail_bytes = this->ring_buf.ReadCapacity();

	// Have we run out of things to feed?
	if (avail_bytes == 0) {
		// Is this a temporary condition, or have we genuinely played
		// out all we can?  If the latter, we're now out too.
		if (this->source_out) this->state = Sink::State::AT_END;

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

/* static */ std::vector<std::pair<int, std::string>> SDLSink::GetDevicesInfo()
{
	std::vector<std::pair<int, std::string>> list;

	// The 0 in SDL_GetNumAudioDevices tells SDL we want playback devices.
	const auto is = SDL_GetNumAudioDevices(0);
	for (auto i = 0; i < is; i++) {
		auto n = SDL_GetAudioDeviceName(i, 0);
		if (n != nullptr) list.emplace_back(i, std::string(n));
	}

	return list;
}

/* static */ bool SDLSink::IsOutputDevice(int id)
{
	const auto ids = SDL_GetNumAudioDevices(0);

	// See comment in GetDevicesInfo for why this is sufficient.
	return 0 <= id && id < ids;
}

} // namespace Playd::Audio
