/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#include <string>

#include <portaudio.h>
#include "portaudiocpp/PortAudioCpp.hxx"

#include "audio_output.hpp"
#include "audio_system.hpp"

AudioSystem::AudioSystem()
{
	if (Pa_Initialize() != (int)paNoError) {
		throw Error(ErrorCode::AUDIO_INIT_FAIL,
		            "couldn't init portaudio");
	}
	av_register_all();

	SetDeviceID("0");
}

AudioSystem::~AudioSystem()
{
	Pa_Terminate();
}

void AudioSystem::OnDevices(std::function<void(const AudioSystem::Device &)> f)
                const
{
	PaDeviceIndex num_devices = Pa_GetDeviceCount();
	for (PaDeviceIndex i = 0; i < num_devices; i++) {
		const PaDeviceInfo *dev = Pa_GetDeviceInfo(i);
		f(Device(std::to_string(i), std::string(dev->name)));
	}
}

void AudioSystem::SetDeviceID(const std::string &id)
{
	this->device_id = std::string(id);
}

AudioOutput *AudioSystem::Load(const std::string &path) const
{
	return new AudioOutput(path, this->device_id);
}
