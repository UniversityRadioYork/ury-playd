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

#include "audio_decoder.hpp"
#include "audio_output.hpp"

/**
 * An AudioSystem represents the entire audio stack used by Playslave++.
 *
 * The AudioSystem is responsible for creating AudioOutput instances,
 * enumerating and resolving device IDs, and initialising and terminating the
 * audio libraries.
 *
 * AudioSystem is a RAII-style class: it loads the audio libraries on
 * construction and unloads them on termination.  As such, it's probably not
 * wise to construct multiple AudioSystem instances.
 */
class AudioSystem : public StreamConfigurator {
public:
	/// Type for device entries.
	using Device = std::pair<std::string, std::string>;

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
	 * Loads a file, creating an AudioOutput for it.
	 * @param path  The path to a file.
	 * @return      The AudioOutput for that file.
	 */
	AudioOutput *Load(const std::string &path) const;

	/**
	 * Sets the current device ID.
	 * @param id  The device ID to use for subsequent AudioOutputs.
	 */
	void SetDeviceID(const std::string &id);

	/**
	 * Performs a function on each device entry in the AudioSystem.
	 * @param f  The function to call on each device.
	 */
	void OnDevices(std::function<void(const Device &)> f) const;

	portaudio::Stream *Configure(portaudio::CallbackInterface &cb,
	                             const AudioDecoder &av) const override;

private:
	std::string device_id; ///< The current device ID.

	/**
	 * Converts a string device ID to a PortAudio device.
	 * @param id_string The device ID, as a string.
	 * @return The device.
	 */
	const portaudio::Device &PaDeviceFrom(
	                const std::string &id_string) const;

	/**
	 * Converts a sample format identifier from playslave++ to PortAudio.
	 * @param fmt The playslave++ sample format identifier.
	 * @return The PortAudio equivalent of the given SampleFormat.
	 */
	portaudio::SampleDataFormat PaSampleFormatFrom(SampleFormat fmt) const;
};

#endif // PS_AUDIO_SYSTEM_HPP
