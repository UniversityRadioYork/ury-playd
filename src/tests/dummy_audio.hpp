// This file is part of playd.
// playd is licensed under the MIT licence: see LICENCE.txt.

/**
 * @file
 * Declaration of DummyAudio and related classes.
 */

#ifndef PLAYD_DUMMY_AUDIO_HPP
#define PLAYD_DUMMY_AUDIO_HPP

#include <vector>

#include "../audio/audio.hpp"
#include "../audio/audio_system.hpp"
#include "../io/io_response.hpp"
#include "../time_parser.hpp"

/**
 * A dummy Audio implementation, for testing.
 * @see Audio
 */
class DummyAudio : public Audio
{
public:
	/// Constructs a DummyAudio.
	DummyAudio();

	void Start() override;
	void Stop() override;
	void Seek(TimeParser::MicrosecondPosition position) override;
	Audio::State Update() override;

	void Emit(ResponseSink &sink) const override;
	TimeParser::MicrosecondPosition Position() const override;

	// These fields left public for purposes of easy testing.

	bool started;                        ///< Whether the audio is started.
	std::string path;                    ///< The path.
	TimeParser::MicrosecondPosition pos; ///< The position.
	Audio::State state;                  ///< The state.
};

/**
 * A dummy AudioSystem implementation, for testing.
 * @see AudioSystem
 */
class DummyAudioSystem : public AudioSystem
{
public:
	/**
	 * Constructs a new DummyAudioSystem.
	 * @param audio A pointer to the Audio returned by this
	 *   DummyAudioSystem when performing loads.
	 */
	DummyAudioSystem(DummyAudio *audio) : audio(audio)
	{
	}

	Audio *Load(const std::string &path) const override;
	void SetDeviceID(int id) override;
	std::vector<AudioSystem::Device> GetDevicesInfo() override;
	bool IsOutputDevice(int id) override;

	/// The Audio held by this DummyAudioSystem.
	DummyAudio *audio;
};

#endif // PLAYD_DUMMY_AUDIO_HPP
