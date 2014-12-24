// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the PaSoxAudioSystem class.
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

#ifndef NO_FLAC
#include "sources/flac.hpp"
#endif // NO_FLAC

#ifndef NO_MP3
#include "sources/mp3.hpp"
#endif // NO_MP3

#include "sources/sox.hpp"

PaSoxAudioSystem::PaSoxAudioSystem()
{
	portaudio::System::initialize();
	sox_format_init();
	mpg123_init();

	this->device_id = -1;
}

PaSoxAudioSystem::~PaSoxAudioSystem()
{
	portaudio::System::terminate();
	sox_format_quit();
	mpg123_exit();
}

std::vector<AudioSystem::Device> PaSoxAudioSystem::GetDevicesInfo()
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

bool PaSoxAudioSystem::IsOutputDevice(int id)
{
	auto &pa = portaudio::System::instance();
	if (id < 0 || id >= pa.deviceCount()) return false;

	portaudio::Device &dev = pa.deviceByIndex(id);
	return !dev.isInputOnlyDevice();
}

void PaSoxAudioSystem::SetDeviceID(int id)
{
	this->device_id = std::to_string(id);
}

Audio *PaSoxAudioSystem::Load(const std::string &path) const
{
	AudioSource *source = this->LoadSource(path);
	assert(source != nullptr);

	auto sink = new AudioSink(*source, *this);
	return new PipeAudio(source, sink);
}

AudioSource *PaSoxAudioSystem::LoadSource(const std::string &path) const
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

	// TODO: Ogg

	Debug() << "Using SoXAudioSource" << std::endl;
	return new SoXAudioSource(path);
}

portaudio::Stream *PaSoxAudioSystem::Configure(const AudioSource &source,
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

/* static */ const portaudio::Device &PaSoxAudioSystem::PaDevice(
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

/* static */ portaudio::SampleDataFormat PaSoxAudioSystem::PaFormat(
                SampleFormat fmt)
{
	try {
		return pa_from_sf.at(fmt);
	} catch (std::out_of_range) {
		throw FileError(MSG_DECODE_BADRATE);
	}
}
