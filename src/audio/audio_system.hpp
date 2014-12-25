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

#include "audio.hpp"
#include "audio_sink.hpp"
#include "audio_source.hpp"
#include "sample_formats.hpp"

/**
 * An AudioSystem represents the entire audio stack used by playd.
 *
 * The AudioSystem is responsible for creating Audio instances,
 * enumerating and resolving device IDs, and initialising and terminating the
 * audio libraries.
 *
 * This is an abstract class, implemented primarily by SoxPaAudioSystem.
 *
 * @see SoxPaAudioSystem
 * @see Audio
 */
class AudioSystem
{
public:
	/// Type for device entries.
	typedef std::pair<int, std::string> Device;

	/**
	 * Loads a file, creating an Audio for it.
	 * @param path The path to a file.
	 * @return The Audio for that file.
	 */
	virtual Audio *Load(const std::string &path) const = 0;

	/**
	 * Sets the current device ID.
	 * @param id The device ID to use for subsequent Audios.
	 */
	virtual void SetDeviceID(int id) = 0;

	/**
	 * Gets the number and name of each output device entry in the
	 * AudioSystem.
	 * @return List of output devices, as strings.
	 */
	virtual std::vector<AudioSystem::Device> GetDevicesInfo() = 0;

	/**
	 * Can a sound device output sound?
	 * @param id Device ID.
	 * @return If the device can handle outputting sound.
	 */
	virtual bool IsOutputDevice(int id) = 0;
};

/**
 * Implementation of AudioSystem using PortAudio and friends.
 *
 * PaAudioSystem is a RAII-style class: it loads the audio libraries on
 * construction and unloads them on termination.  As such, it's probably not
 * wise to construct multiple AudioSystem instances.
 */
class PaAudioSystem : public AudioSystem, public AudioSinkConfigurator
{
public:
	/**
	 * Constructs a PaAudioSystem, initialising its libraries.
	 * This sets the current device ID to a sane default; use SetDeviceID
	 * to change it.
	 */
	PaAudioSystem();

	/**
	 * Destructs an PaAudioSystem, uninitialising its libraries.
	 */
	virtual ~PaAudioSystem();

	// AudioSystem implementation
	Audio *Load(const std::string &path) const override;
	void SetDeviceID(int id) override;
	std::vector<AudioSystem::Device> GetDevicesInfo() override;
	bool IsOutputDevice(int id) override;

	// AudioSinkConfigurator implementation
	portaudio::Stream *Configure(
	                const AudioSource &source,
	                portaudio::CallbackInterface &cb) const override;

private:
	std::string device_id; ///< The current device ID.

	/**
	 * Loads a file, creating an AudioSource.
	 * @param path The path to the file to load.
	 * @return An AudioSource pointer (may be nullptr, if no available
	 *   and suitable AudioSource was found).
	 * @see Load
	 */
	AudioSource *LoadSource(const std::string &path) const;

	/**
	 * Converts a string device ID to a PortAudio device.
	 * @param id_string The device ID, as a string.
	 * @return The device.
	 */
	static const portaudio::Device &PaDevice(const std::string &id_string);

	/**
	 * Converts a sample format identifier from playd to PortAudio.
	 * @param fmt The playd sample format identifier.
	 * @return The PortAudio equivalent of the given SampleFormat.
	 */
	static portaudio::SampleDataFormat PaFormat(SampleFormat fmt);
};

#endif // PLAYD_AUDIO_SYSTEM_HPP
