// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the AudioSystem class.
 * @see audio/audio_system.cpp
 */

#ifndef PS_AUDIO_SYSTEM_HPP
#define PS_AUDIO_SYSTEM_HPP

#include <functional>
#include <string>
#include <utility>

#include "portaudiocpp/SampleDataFormat.hxx"
#include "portaudiocpp/Stream.hxx"
namespace portaudio {
class CallbackInterface;
class Device;
}

#include "../sample_formats.hpp"
#include "audio_sink.hpp"
#include "audio_source.hpp"
#include "audio.hpp"

/**
 * An AudioSystem represents the entire audio stack used by Playslave++.
 *
 * The AudioSystem is responsible for creating Audio instances,
 * enumerating and resolving device IDs, and initialising and terminating the
 * audio libraries.
 *
 * AudioSystem is a RAII-style class: it loads the audio libraries on
 * construction and unloads them on termination.  As such, it's probably not
 * wise to construct multiple AudioSystem instances.
 */
class AudioSystem {
public:
	/// Type for device entries.
	using Device = std::pair<int, std::string>;

	/**
	 * Constructs an AudioSystem, initialising its libraries.
	 * This sets the current device ID to a sane default; use SetDeviceID
	 * to change it.
	 */
	AudioSystem();

	/**
	 * Destructs an AudioSystem, uninitialising its libraries.
	 */
	~AudioSystem();

	/**
	 * Loads a file, creating an Audio for it.
	 * @param path The path to a file.
	 * @return The Audio for that file.
	 */
	Audio *Load(const std::string &path) const;

	/**
	 * Sets the current device ID.
	 * @param id The device ID to use for subsequent Audios.
	 */
	void SetDeviceID(int id);

	/**
	 * Gets the number and name of each output device entry in the
	 * AudioSystem.
	 * @return List of output devices, as strings.
	 */
	std::vector<AudioSystem::Device> GetDevicesInfo();

	/**
	 * Can a sound device output sound?
	 * @param id Device ID.
	 * @return If the device can handle outputting sound.
	 */
	bool IsOutputDevice(int id);

	/**
	 * Configures and returns a PortAudio stream.
	 * @param channel_count The number of channels of the stream will
	 *   receive.
	 * @param sample_format The format of the samples the stream will
	 *   receive.
	 * @param sample_rate The rate of the samples the stream will receive.
	 * @param buffer_size The size of the buffer the stream should allocate.
	 * @param cb The object that PortAudio will call to receive audio.
	 * @return The configured PortAudio stream.
	 */
	portaudio::Stream *Configure(std::uint8_t channel_count,
	                             SampleFormat sample_format,
	                             double sample_rate, size_t buffer_size,
	                             portaudio::CallbackInterface &cb) const;

private:
	std::string device_id; ///< The current device ID.

	/**
	 * Converts a string device ID to a PortAudio device.
	 * @param id_string The device ID, as a string.
	 * @return The device.
	 */
	const portaudio::Device &PaDeviceFrom(const std::string &id_string)
	                const;

	/**
	 * Converts a sample format identifier from playslave++ to PortAudio.
	 * @param fmt The playslave++ sample format identifier.
	 * @return The PortAudio equivalent of the given SampleFormat.
	 */
	portaudio::SampleDataFormat PaSampleFormatFrom(SampleFormat fmt) const;
};

#endif // PS_AUDIO_SYSTEM_HPP
