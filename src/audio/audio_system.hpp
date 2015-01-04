// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the AudioSystem class.
 * @see audio/audio_system.cpp
 */

#ifndef PLAYD_AUDIO_SYSTEM_HPP
#define PLAYD_AUDIO_SYSTEM_HPP

#include <functional>
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
 * This is an abstract class, implemented primarily by PipeAudioSystem.
 *
 * @see PipeAudioSystem
 * @see Audio
 */
class AudioSystem
{
public:
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
};

/**
 * A modular AudioSystem that constructs PipeAudio.
 *
 * PipeAudioSystem creates Audio by chaining together audio _sources_, selected
 * by file extension, and an audio _sink_.
 *
 * @see PipeAudio
 * @see AudioSink
 * @see AudioSource
 */
class PipeAudioSystem : public AudioSystem
{
public:
	/// Type for functions that construct sinks.
	using SinkBuilder = std::function<
	                std::unique_ptr<AudioSink>(const AudioSource &, int)>;

	/// Type for functions that construct sources.
	using SourceBuilder = std::function<
	                std::unique_ptr<AudioSource>(const std::string &)>;

	/// Constructs a PipeAudioSystem.
	PipeAudioSystem();

	// AudioSystem implementation
	std::unique_ptr<Audio> Null() const override;
	std::unique_ptr<Audio> Load(const std::string &path) const override;

	/**
	 * Sets the sink to use for outputting sound.
	 * @param sink The function to use when building sinks.
	 * @param device_id The device ID to pass to any constructed sinks.
	 */
	void SetSink(SinkBuilder sink, int device_id);

	/**
	 * Assign an AudioSource for a series of file extensions.
	 * @param exts The list of file extensions to associate with this
	 *   source.
	 * @param source The function to use when building source.
	 * @note If two AddSource invocations name the first file extension,
	 *   the first is used for said extension.
	 */
	void AddSource(std::initializer_list<std::string> exts,
	               SourceBuilder source);

private:
	/// The current sink builder.
	std::function<std::unique_ptr<AudioSink>(const AudioSource &)> sink;

	/// Map from file extensions to source builders.
	std::map<std::string, SourceBuilder> sources;

	/**
	 * Loads a file, creating an AudioSource.
	 * @param path The path to the file to load.
	 * @return An AudioSource pointer (may be nullptr, if no available
	 *   and suitable AudioSource was found).
	 * @see Load
	 */
	std::unique_ptr<AudioSource> LoadSource(const std::string &path) const;
};

#endif // PLAYD_AUDIO_SYSTEM_HPP
