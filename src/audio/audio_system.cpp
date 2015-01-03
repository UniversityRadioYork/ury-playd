// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the PaAudioSystem class.
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

PaAudioSystem::PaAudioSystem() : sink([](const AudioSource &) -> std::unique_ptr<AudioSink> {
	throw InternalError("No audio sink!");
}) {}

std::vector<AudioSystem::Device> PaAudioSystem::GetDevicesInfo()
{
	return AudioSink::GetDevicesInfo();
}

bool PaAudioSystem::IsOutputDevice(int id)
{
	return AudioSink::IsOutputDevice(id);
}

std::unique_ptr<Audio> PaAudioSystem::Null() const
{
	return std::unique_ptr<Audio>(new NoAudio());
}

std::unique_ptr<Audio> PaAudioSystem::Load(const std::string &path) const
{
	std::unique_ptr<AudioSource> source = this->LoadSource(path);
	assert(source != nullptr);

	auto sink = this->sink(*source);
	return std::unique_ptr<Audio>(new PipeAudio(std::move(source), std::move(sink)));
}

std::unique_ptr<AudioSource> PaAudioSystem::LoadSource(const std::string &path) const
{
	size_t extpoint = path.find_last_of('.');
	std::string ext = path.substr(extpoint + 1);

	auto ibuilder = this->sources.find(ext);
	if (ibuilder == this->sources.end()) {
		throw FileError("Unknown file format: " + ext);
	}

	return (ibuilder->second)(path);
}

void PaAudioSystem::SetSink(PaAudioSystem::SinkBuilder sink, int device_id)
{
	this->sink = std::bind(sink, std::placeholders::_1, device_id);
}

void PaAudioSystem::AddSource(std::initializer_list<std::string> exts, PaAudioSystem::SourceBuilder source)
{
	for (auto &ext : exts) this->sources.emplace(ext, source);
}
