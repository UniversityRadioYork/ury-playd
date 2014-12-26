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

	auto sink = new AudioSink(*source, *this);
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

portaudio::Stream *PaAudioSystem::Configure(const AudioSource &source,
                                            portaudio::CallbackInterface &cb) const
{
	std::uint8_t channel_count = source.ChannelCount();
	SampleFormat sample_format = source.OutputSampleFormat();
	double sample_rate = source.SampleRate();
	const portaudio::Device &device = PaDevice(this->device_id);

	portaudio::DirectionSpecificStreamParameters out_pars(
	                device, channel_count, PaFormat(sample_format), true,
	                device.defaultLowOutputLatency(), nullptr);

	portaudio::StreamParameters pars(
	                portaudio::DirectionSpecificStreamParameters::null(),
	                out_pars, sample_rate, paFramesPerBufferUnspecified,
	                paClipOff);

	return new portaudio::InterfaceCallbackStream(pars, cb);
}

/* static */ const portaudio::Device &PaAudioSystem::PaDevice(
                const std::string &id)
{
	auto &pa = portaudio::System::instance();

	PaDeviceIndex id_pa = std::stoi(id);
	if (pa.deviceCount() <= id_pa) throw ConfigError(MSG_DEV_BADID);
	return pa.deviceByIndex(id_pa);
}

/// Mappings from SampleFormats to their equivalent PaSampleFormats.
static const std::map<SampleFormat, portaudio::SampleDataFormat> pa_from_sf = {
	{ SampleFormat::PACKED_UNSIGNED_INT_8, portaudio::UINT8 },
	{ SampleFormat::PACKED_SIGNED_INT_8, portaudio::INT8 },
	{ SampleFormat::PACKED_SIGNED_INT_16, portaudio::INT16 },
	{ SampleFormat::PACKED_SIGNED_INT_24, portaudio::INT24 },
	{ SampleFormat::PACKED_SIGNED_INT_32, portaudio::INT32 },
	{ SampleFormat::PACKED_FLOAT_32, portaudio::FLOAT32 }
};

/* static */ portaudio::SampleDataFormat PaAudioSystem::PaFormat(SampleFormat fmt)
{
	try {
		return pa_from_sf.at(fmt);
	} catch (std::out_of_range) {
		throw FileError(MSG_DECODE_BADRATE);
	}
}
