// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

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
#include "libavformat/avformat.h"
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
namespace portaudio {
class CallbackInterface;
}

#include "../errors.hpp"
#include "../messages.h"
#include "../sample_formats.hpp"

#include "audio_decoder.hpp"
#include "audio_output.hpp"
#include "audio_system.hpp"

AudioSystem::AudioSystem()
{
	portaudio::System::initialize();
	av_register_all();

	SetDeviceID("0");
}

AudioSystem::~AudioSystem() { portaudio::System::terminate(); }

void AudioSystem::OnDevices(std::function<void(const AudioSystem::Device &)> f)
                const
{
	auto &pa = portaudio::System::instance();

	std::for_each(pa.devicesBegin(), pa.devicesEnd(),
	              [&f](const portaudio::Device &d) {
		f(Device(std::to_string(d.index()), std::string(d.name())));
	});
}

void AudioSystem::SetDeviceID(const std::string &id)
{
	this->device_id = std::string(id);
}

AudioOutput *AudioSystem::Load(const std::string &path) const
{
	return new AudioOutput(path, *this);
}

portaudio::Stream *AudioSystem::Configure(portaudio::CallbackInterface &cb,
                                          const AudioDecoder &av) const
{
	const portaudio::Device &device = PaDeviceFrom(this->device_id);

	portaudio::DirectionSpecificStreamParameters out_pars(
	                device, av.ChannelCount(),
	                PaSampleFormatFrom(av.OutputSampleFormat()), true,
	                device.defaultLowOutputLatency(), nullptr);

	portaudio::StreamParameters pars(
	                portaudio::DirectionSpecificStreamParameters::null(),
	                out_pars, av.SampleRate(), av.BufferSampleCapacity(),
	                paClipOff);

	return new portaudio::InterfaceCallbackStream(pars, cb);
}

const portaudio::Device &AudioSystem::PaDeviceFrom(const std::string &id_string)
                const
{
	auto &pa = portaudio::System::instance();

	PaDeviceIndex id_pa = 0;

	std::istringstream is(id_string);
	is >> id_pa;

	if (id_pa >= pa.deviceCount()) {
		throw ConfigError(MSG_DEV_BADID);
	}

	return pa.deviceByIndex(id_pa);
}

/// Mappings from SampleFormats to their equivalent PaSampleFormats.
static std::map<SampleFormat, portaudio::SampleDataFormat> pa_from_sf = {
                {SampleFormat::PACKED_UNSIGNED_INT_8, portaudio::UINT8},
                {SampleFormat::PACKED_SIGNED_INT_16, portaudio::INT16},
                {SampleFormat::PACKED_SIGNED_INT_32, portaudio::INT32},
                {SampleFormat::PACKED_FLOAT_32, portaudio::FLOAT32}};

portaudio::SampleDataFormat AudioSystem::PaSampleFormatFrom(SampleFormat fmt)
                const
{
	try { return pa_from_sf.at(fmt); }
	catch (std::out_of_range) { throw FileError(MSG_DECODE_BADRATE); }
}
