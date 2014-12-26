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

extern "C" {
#include "portaudio.h"
}

#include "portaudiocpp/Device.hxx"
#include "portaudiocpp/DirectionSpecificStreamParameters.hxx"
#include "portaudiocpp/InterfaceCallbackStream.hxx"
#include "portaudiocpp/SampleDataFormat.hxx"
#include "portaudiocpp/Stream.hxx"
#include "portaudiocpp/StreamParameters.hxx"
#include "portaudiocpp/System.hxx"
#include "portaudiocpp/SystemDeviceIterator.hxx"
namespace portaudio
{
class CallbackInterface;
}

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

PaAudioSystem::PaAudioSystem()
{
	portaudio::System::initialize();

#ifndef NO_MP3
	mpg123_init();
#endif // NO_MP3

	this->device_id = -1;
}

PaAudioSystem::~PaAudioSystem()
{
	portaudio::System::terminate();

#ifndef NO_MP3
	mpg123_exit();
#endif // NO_MP3
}

std::vector<AudioSystem::Device> PaAudioSystem::GetDevicesInfo()
{
	auto &pa = portaudio::System::instance();
	std::vector<AudioSystem::Device> list;

	for (auto d = pa.devicesBegin(); d != pa.devicesEnd(); ++d) {
		if (!(*d).isInputOnlyDevice()) {
			list.push_back(std::make_pair((*d).index(), (*d).name()));
		}
	}
	return list;
}

bool PaAudioSystem::IsOutputDevice(int id)
{
	auto &pa = portaudio::System::instance();
	if (id < 0 || id >= pa.deviceCount()) return false;

	portaudio::Device &dev = pa.deviceByIndex(id);
	return !dev.isInputOnlyDevice();
}

void PaAudioSystem::SetDeviceID(int id)
{
	this->device_id = std::to_string(id);
}

Audio *PaAudioSystem::Load(const std::string &path) const
{
	AudioSource *source = this->LoadSource(path);
	assert(source != nullptr);

	auto sink = new AudioSink(*source, this->device_id);
	return new PipeAudio(source, sink);
}

AudioSource *PaAudioSystem::LoadSource(const std::string &path) const
{
	size_t extpoint = path.find_last_of('.');
	std::string ext = path.substr(extpoint + 1);

#ifndef NO_FLAC
	if (ext == "flac") {
		Debug() << "Using FlacAudioSource" << std::endl;
		return new FlacAudioSource(path);
	}
#endif

#ifndef NO_MP3
	if (ext == "mp3") {
		Debug() << "Using Mp3AudioSource" << std::endl;
		return new Mp3AudioSource(path);
	}
#endif

#ifndef NO_SNDFILE
	if (ext == "ogg" || ext == "wav" || ext == "flac") {
		Debug() << "Using SndfileAudioSource" << std::endl;
		return new SndfileAudioSource(path);
	}
#endif

	throw FileError("Unknown file format: " + ext);
}

