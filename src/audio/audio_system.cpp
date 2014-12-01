// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the AudioSystem class.
 * @see audio/audio_system.hpp
 */

#include <algorithm>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

extern "C" {
#include <sox.h>
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
#include "../sample_formats.hpp"
#include "audio.hpp"
#include "audio_sink.hpp"
#include "audio_source.hpp"
#include "audio_system.hpp"

AudioSystem::AudioSystem()
{
	portaudio::System::initialize();
	sox_format_init();

	this->device_id = -1;
}

AudioSystem::~AudioSystem()
{
	portaudio::System::terminate();
	sox_format_quit();
}

/* static */ std::vector<AudioSystem::Device> AudioSystem::GetDevicesInfo()
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

/* static */ bool AudioSystem::IsOutputDevice(int id)
{
	auto &pa = portaudio::System::instance();
	if (id < 0 || id >= pa.deviceCount()) return false;

	portaudio::Device &dev = pa.deviceByIndex(id);
	return !dev.isInputOnlyDevice();
}

void AudioSystem::SetDeviceID(int id)
{
	this->device_id = std::to_string(id);
}

Audio *AudioSystem::Load(const std::string &path) const
{
	auto source = new AudioSource(path);
	auto sink = new AudioSink(*source, *this);
	return new PipeAudio(source, sink);
}

portaudio::Stream *AudioSystem::Configure(const AudioSource &source,
                                          portaudio::CallbackInterface &cb) const
{
	std::uint8_t channel_count = source.ChannelCount();
	SampleFormat sample_format = source.OutputSampleFormat();
	double sample_rate = source.SampleRate();
	size_t buffer_size = source.BufferSampleCapacity();
	const portaudio::Device &device = PaDeviceFrom(this->device_id);

	portaudio::DirectionSpecificStreamParameters out_pars(
	                device, channel_count, PaSampleFormatFrom(sample_format),
	                true, device.defaultLowOutputLatency(), nullptr);

	portaudio::StreamParameters pars(
	                portaudio::DirectionSpecificStreamParameters::null(),
	                out_pars, sample_rate, buffer_size, paClipOff);

	return new portaudio::InterfaceCallbackStream(pars, cb);
}

/* static */ const portaudio::Device &AudioSystem::PaDeviceFrom(
                const std::string &id_string)
{
	auto &pa = portaudio::System::instance();

	PaDeviceIndex id_pa = 0;

	std::istringstream is(id_string);
	is >> id_pa;

	if (id_pa >= pa.deviceCount()) throw ConfigError(MSG_DEV_BADID);

	return pa.deviceByIndex(id_pa);
}

/// Mappings from SampleFormats to their equivalent PaSampleFormats.
static const std::map<SampleFormat, portaudio::SampleDataFormat> pa_from_sf = {
	{ SampleFormat::PACKED_UNSIGNED_INT_8, portaudio::UINT8 },
	{ SampleFormat::PACKED_SIGNED_INT_16, portaudio::INT16 },
	{ SampleFormat::PACKED_SIGNED_INT_32, portaudio::INT32 },
	{ SampleFormat::PACKED_FLOAT_32, portaudio::FLOAT32 }
};

/* static */ portaudio::SampleDataFormat AudioSystem::PaSampleFormatFrom(
                SampleFormat fmt)
{
	try {
		return pa_from_sf.at(fmt);
	} catch (std::out_of_range) {
		throw FileError(MSG_DECODE_BADRATE);
	}
}
