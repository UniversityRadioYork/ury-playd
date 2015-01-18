// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the PipeAudioSystem class.
 * @see audio/audio_system.hpp
 */

#include <algorithm>
#include <cassert>
#include <functional>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

#include "../errors.hpp"
#include "../messages.h"
#include "audio.hpp"
#include "audio_sink.hpp"
#include "audio_source.hpp"
#include "audio_system.hpp"
#include "sample_formats.hpp"

#include "sources/flac.hpp"
#include "sources/mp3.hpp"
#include "sources/sndfile.hpp"

PipeAudioSystem::PipeAudioSystem(int device_id)
    : device_id(device_id),
      sink([](const AudioSource &, int) -> std::unique_ptr<AudioSink> {
	      throw InternalError("No audio sink!");
      })
{
}

std::unique_ptr<Audio> PipeAudioSystem::Null() const
{
	return std::unique_ptr<Audio>(new NoAudio());
}

std::unique_ptr<Audio> PipeAudioSystem::Load(const std::string &path) const
{
	std::unique_ptr<AudioSource> source = this->LoadSource(path);
	assert(source != nullptr);

	auto sink = this->sink(*source, this->device_id);
	return std::unique_ptr<Audio>(
	                new PipeAudio(std::move(source), std::move(sink)));
}

std::unique_ptr<AudioSource> PipeAudioSystem::LoadSource(
                const std::string &path) const
{
	size_t extpoint = path.find_last_of('.');
	std::string ext = path.substr(extpoint + 1);

	auto ibuilder = this->sources.find(ext);
	if (ibuilder == this->sources.end()) {
		throw FileError("Unknown file format: " + ext);
	}

	return (ibuilder->second)(path);
}

void PipeAudioSystem::SetSink(PipeAudioSystem::SinkBuilder sink)
{
	this->sink = sink;
}

void PipeAudioSystem::AddSource(const std::string &ext,
                                PipeAudioSystem::SourceBuilder source)
{
	this->sources.emplace(ext, source);
}
