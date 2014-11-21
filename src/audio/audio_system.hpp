// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the AudioSystem class.
 * @see audio/audio_system.cpp
 */

#ifndef PLAYD_AUDIO_SYSTEM_HPP
#define PLAYD_AUDIO_SYSTEM_HPP

#include <string>
#include <utility>

#include "portaudiocpp/SampleDataFormat.hxx"
#include "portaudiocpp/Stream.hxx"
namespace portaudio
{
class CallbackInterface;
class Device;
}

#include "../sample_formats.hpp"
#include "audio_sink.hpp"
#include "audio_source.hpp"
#include "audio.hpp"

/**
 * An AudioSystem represents the entire audio stack used by playd.
 *
 * The AudioSystem is responsible for creating Audio instances,
 * enumerating and resolving device IDs, and initialising and terminating the
 * audio libraries.
 *
 * AudioSystem is a RAII-style class: it loads the audio libraries on
 * construction and unloads them on termination.  As such, it's probably not
 * wise to construct multiple AudioSystem instances.
 */
class AudioSystem : public AudioSinkConfigurator
{
public:
	/// Type for device entries.
	typedef std::pair<int, std::string> Device;

	/**
	 * Constructs an AudioSystem, initialising its libraries.
	 * This sets the current device ID to a sane default; use SetDeviceID
	 * to change it.
	 */
	AudioSystem();

	/**
	 * Destructs an AudioSystem, uninitialising its libraries.
	 */
	virtual ~AudioSystem();

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
	static std::vector<AudioSystem::Device> GetDevicesInfo();

	/**
	 * Can a sound device output sound?
	 * @param id Device ID.
	 * @return If the device can handle outputting sound.
	 */
	static bool IsOutputDevice(int id);

	virtual portaudio::Stream *Configure(
	                const AudioSource &source,
	                portaudio::CallbackInterface &cb) const override;

private:
	std::string device_id; ///< The current device ID.

	/**
	 * Converts a string device ID to a PortAudio device.
	 * @param id_string The device ID, as a string.
	 * @return The device.
	 */
	static const portaudio::Device &PaDeviceFrom(const std::string &id_string);

	/**
	 * Converts a sample format identifier from playd to PortAudio.
	 * @param fmt The playd sample format identifier.
	 * @return The PortAudio equivalent of the given SampleFormat.
	 */
	static portaudio::SampleDataFormat PaSampleFormatFrom(SampleFormat fmt);
};

#endif // PLAYD_AUDIO_SYSTEM_HPP
