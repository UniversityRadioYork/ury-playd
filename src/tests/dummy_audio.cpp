// This file is part of playd.
// playd is licensed under the MIT licence: see LICENCE.txt.

/**
 * @file
 * Implementation of DummyAudio and related classes.
 */

#include <cstdint>
#include <string>
#include <vector>

#include "../audio/audio.hpp"
#include "../audio/audio_system.hpp"
#include "../response.hpp"
#include "dummy_audio.hpp"

//
// DummyAudio
//

DummyAudio::DummyAudio(DummyAudioSystem &sys) : sys(sys)
{
}

void DummyAudio::Emit(Response::Code code, const ResponseSink *sink, size_t id)
{
	if (sink == nullptr) return;

	auto r = Response(code);

	if (code == Response::Code::STATE) {
		auto playing = this->sys.started;
		r.AddArg(playing ? "Playing" : "Stopped");
	} else if (code == Response::Code::FILE) {
		r.AddArg(this->sys.path);
	} else {
		return;
	}

	sink->Respond(id, r);
}

void DummyAudio::SetPlaying(bool playing)
{
	this->sys.started = playing;
}

Audio::State DummyAudio::Update()
{
	return this->sys.state;
}

std::uint64_t DummyAudio::Position() const
{
	return this->sys.pos;
}

void DummyAudio::Seek(std::uint64_t position)
{
	this->sys.pos = position;
}

//
// DummyAudioSystem
//

DummyAudioSystem::DummyAudioSystem() : path(""), pos(0), state(Audio::State::STOPPED)
{
}

std::unique_ptr<Audio> DummyAudioSystem::Null() const
{
	return std::unique_ptr<Audio>(new NoAudio());
}

std::unique_ptr<Audio> DummyAudioSystem::Load(const std::string &path) const
{
	// Kids, don't try this at home.
	// Were this not a test mock, I'd shoot myself for this!  ~ Matt
	DummyAudioSystem &notconst = const_cast<DummyAudioSystem &>(*this);
	notconst.path = path;
	return std::unique_ptr<Audio>(new DummyAudio(notconst));
}

/* static */ std::unique_ptr<AudioSource> DummyAudioSource::Build(const std::string &path)
{
	return std::unique_ptr<AudioSource>(new DummyAudioSource(path));
}

std::uint8_t DummyAudioSource::ChannelCount() const
{
	return 2;
}

std::uint32_t DummyAudioSource::SampleRate() const
{
	return 44100;
}

std::uint64_t DummyAudioSource::Seek(std::uint64_t in_samples)
{
	return in_samples;
}

DummyAudioSource::DecodeResult DummyAudioSource::Decode()
{
	return std::make_pair(DecodeState::DECODING, DecodeVector());
}

SampleFormat DummyAudioSource::OutputSampleFormat() const
{
	return SampleFormat::PACKED_UNSIGNED_INT_8;
}
