// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the PaAudioSystem class.
 * @see audio/audio_system.hpp
 */

#include <algorithm>
#include <cassert>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

#include "../errors.hpp"
#include "../messages.h"
#include "audio.hpp"
#include "audio_sink.hpp"
#include "audio_source.hpp"
#include "audio_system.hpp"
#include "sample_formats.hpp"

#include "sources/flac.hpp"
#include "sources/mp3.hpp"
#include "sources/sndfile.hpp"

PaAudioSystem::PaAudioSystem() : device_id(-1)
{
	AudioSink::InitLibrary();

#ifdef WITH_MP3
	mpg123_init();
#endif // WITH_MP3
}

PaAudioSystem::~PaAudioSystem()
{
	AudioSink::CleanupLibrary();

#ifdef WITH_MP3
	mpg123_exit();
#endif // WITH_MP3
}

std::vector<AudioSystem::Device> PaAudioSystem::GetDevicesInfo()
{
	return AudioSink::GetDevicesInfo();
}

bool PaAudioSystem::IsOutputDevice(int id)
{
	return AudioSink::IsOutputDevice(id);
}

void PaAudioSystem::SetDeviceID(int id)
{
	this->device_id = id;
}

std::unique_ptr<Audio> PaAudioSystem::Null() const
{
	return std::unique_ptr<Audio>(new NoAudio());
}

std::unique_ptr<Audio> PaAudioSystem::Load(const std::string &path) const
{
	AudioSource *source = this->LoadSource(path);
	assert(source != nullptr);

	auto sink = new AudioSink(*source, this->device_id);
	return std::unique_ptr<Audio>(new PipeAudio(source, sink));
}

AudioSource *PaAudioSystem::LoadSource(const std::string &path) const
{
	size_t extpoint = path.find_last_of('.');
	std::string ext = path.substr(extpoint + 1);

#ifdef WITH_FLAC
	if (ext == "flac") {
		Debug() << "Using FlacAudioSource" << std::endl;
		return new FlacAudioSource(path);
	}
#endif // WITH_FLAC

#ifdef WITH_MP3
	if (ext == "mp3") {
		Debug() << "Using Mp3AudioSource" << std::endl;
		return new Mp3AudioSource(path);
	}
#endif // WITH_MP3

#ifdef WITH_SNDFILE
	Debug() << "Using SndfileAudioSource" << std::endl;
	return new SndfileAudioSource(path);
#endif // WITH_SNDFILE

	throw FileError("Unknown file format: " + ext);
}
