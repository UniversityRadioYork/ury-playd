// This file is part of playd.
// playd is licensed under the MIT licence: see LICENCE.txt.

/**
 * @file
 * Declaration of DummyAudio and related classes.
 */

#ifndef PLAYD_DUMMY_AUDIO_HPP
#define PLAYD_DUMMY_AUDIO_HPP

#include <cstdint>
#include <vector>

#include "../audio/audio.hpp"
#include "../audio/audio_system.hpp"
#include "../io/io_response.hpp"

/**
 * A dummy AudioSystem implementation, for testing.
 *
 * DummyAudioSystem contains several properties that are read and written by
 * DummyAudio items created by the system, for ease of testing.
 *
 * @see AudioSystem
 */
class DummyAudioSystem : public AudioSystem
{
public:
	/// Constructs a new DummyAudioSystem.
	DummyAudioSystem();

	std::unique_ptr<Audio> Null() const override;
	std::unique_ptr<Audio> Load(const std::string &path) const override;
	void SetDeviceID(int id) override;
	std::vector<AudioSystem::Device> GetDevicesInfo() override;
	bool IsOutputDevice(int id) override;

	// These fields left public for purposes of easy testing.

	bool started;        ///< Whether the audio is started.
	std::string path;    ///< The path.
	std::uint64_t pos;   ///< The position.
	Audio::State state;  ///< The state.
};

/**
 * A dummy Audio implementation, for testing.
 *
 * All actions on DummyAudio set/get properties on the parent AudioSystem,
 * so testing with these dummy objects is not thread-safe in any way.
 *
 * @see Audio
 * @see DummyAudioSystem
 */
class DummyAudio : public Audio
{
public:
	/**
	 * Constructs a DummyAudio.
	 * @param sys The DummyAudioSystem spawning this DummyAudio.
	 */
	DummyAudio(DummyAudioSystem &sys);

	void Start() override;
	void Stop() override;
	void Seek(std::uint64_t position) override;
	Audio::State Update() override;

	void EmitFile(const ResponseSink &sink) const override;
	void EmitState(const ResponseSink &sink) const override;
	std::uint64_t Position() const override;

private:
	/**
	 * The DummyAudioSystem storing the testable properties for this Audio.
	 *
	 * Why are they there, and not here?  Because this Audio is owned by
	 * the PlayerFile, hidden inside it, and deleted when ejected, so the
	 * signature of any actions on the DummyAudio needs to persist past its
	 * own lifetime.
	 */
	DummyAudioSystem &sys;
};

#endif // PLAYD_DUMMY_AUDIO_HPP
