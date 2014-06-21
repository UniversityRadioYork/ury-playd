/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#include <map>
#include <string>
#include <sstream>

#include "portaudiocpp/PortAudioCpp.hxx"

#include "audio_output.hpp"
#include "audio_system.hpp"

AudioSystem::AudioSystem()
{
	portaudio::System::initialize();
	av_register_all();

	SetDeviceID("0");
}

AudioSystem::~AudioSystem()
{
	portaudio::System::terminate();
}

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
	                PaSampleFormatFrom(av.SampleFormat()), true,
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
		throw Error(ErrorCode::BAD_CONFIG, "Bad PortAudio ID.");
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
	try
	{
		return pa_from_sf.at(fmt);
	}
	catch (std::out_of_range)
	{
		throw Error(ErrorCode::BAD_FILE, "unusable sample rate");
	}
}
