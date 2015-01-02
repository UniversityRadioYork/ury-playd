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

#include "audio.hpp"
#include "audio_sink.hpp"
#include "audio_source.hpp"

/**
 * An AudioSystem represents the entire audio stack used by playd.
 *
 * The AudioSystem is responsible for creating Audio instances,
 * enumerating and resolving device IDs, and initialising and terminating the
 * audio libraries.
 *
 * This is an abstract class, implemented primarily by SoxPaAudioSystem.
 *
 * @see PaAudioSystem
 * @see Audio
 */
class AudioSystem
{
public:
	/// Type for device entries.
	typedef std::pair<int, std::string> Device;

	/**
	 * Creates an Audio for a lack of audio.
	 * @return A unique pointer to a dummy Audio.
	 */
	virtual std::unique_ptr<Audio> Null() const = 0;

	/**
	 * Loads a file, creating an Audio for it.
	 * @param path The path to a file.
	 * @return A unique pointer to the Audio for that file.
	 */
	virtual std::unique_ptr<Audio> Load(const std::string &path) const = 0;

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
class PaAudioSystem : public AudioSystem
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
	std::unique_ptr<Audio> Null() const override;
	std::unique_ptr<Audio> Load(const std::string &path) const override;
	void SetDeviceID(int id) override;
	std::vector<AudioSystem::Device> GetDevicesInfo() override;
	bool IsOutputDevice(int id) override;

private:
	int device_id; ///< The current device ID.

	/**
	 * Loads a file, creating an AudioSource.
	 * @param path The path to the file to load.
	 * @return An AudioSource pointer (may be nullptr, if no available
	 *   and suitable AudioSource was found).
	 * @see Load
	 */
	AudioSource *LoadSource(const std::string &path) const;
};

#endif // PLAYD_AUDIO_SYSTEM_HPP
