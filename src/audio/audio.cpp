// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the PipeAudio class.
 * @see audio/pipe_audio.hpp
 */

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdint>
#include <string>

#include "../errors.hpp"
#include "../messages.h"
#include "../response.hpp"
#include "audio.hpp"
#include "audio_sink.hpp"
#include "audio_source.hpp"
#include "sample_formats.hpp"

const std::string Audio::STATE_STRINGS[] {
	"ejected",  // Audio::State::NONE
	"stopped",  // Audio::State::STOPPED
	"playing",  // Audio::State::PLAYING
	"finished", // Audio::State::FINISHED
};

//
// NoAudio
//

Audio::State NoAudio::Update()
{
	return Audio::State::NONE;
}

std::pair<std::string, std::string> NoAudio::Emit(const std::string &path)
{
	if (path == "/player/state/current" || path == "/player/state/available") {
		// Only possible value is the set value
		return std::make_pair("Entry", "ejected");
	}

	throw FileError(MSG_NOT_FOUND);
}

void NoAudio::SetPlaying(bool)
{
	throw NoAudioError(MSG_CMD_NEEDS_LOADED);
}

void NoAudio::Seek(std::uint64_t)
{
	throw NoAudioError(MSG_CMD_NEEDS_LOADED);
}

std::uint64_t NoAudio::Position() const
{
	throw NoAudioError(MSG_CMD_NEEDS_LOADED);
}

//
// PipeAudio
//

PipeAudio::PipeAudio(std::unique_ptr<AudioSource> &&src,
                     std::unique_ptr<AudioSink> &&sink)
    : src(std::move(src)), sink(std::move(sink))
{
	this->ClearFrame();
}

std::pair<std::string, std::string> PipeAudio::Emit(const std::string &path)
{
	assert(this->src != nullptr);
	assert(this->sink != nullptr);

	std::unique_ptr<Response> ret;

	std::string value;

	if (path == "/player/state/current") {
		auto state = this->sink->State();
		value = Audio::STATE_STRINGS[static_cast<int>(state)];
	} else if (path == "/player/state/available") {
		// TODO: Use STATE_STRINGS?
		value = "playing,stopped,ejected,finished";
	} else if (path == "/player/file") {
		value = this->src->Path();
	} else if (path == "/player/time/elapsed") {
		std::uint64_t micros = this->Position();
		value = std::to_string(micros);
	}

	return std::make_pair("Entry", value);
}

void PipeAudio::SetPlaying(bool playing)
{
	assert(this->sink != nullptr);

	if (playing) {
		this->sink->Start();
	} else {
		this->sink->Stop();
	}
}

std::uint64_t PipeAudio::Position() const
{
	assert(this->sink != nullptr);
	assert(this->src != nullptr);

	return this->src->MicrosFromSamples(this->sink->Position());
}

void PipeAudio::Seek(std::uint64_t position)
{
	assert(this->sink != nullptr);
	assert(this->src != nullptr);

	auto in_samples = this->src->SamplesFromMicros(position);
	auto out_samples = this->src->Seek(in_samples);
	this->sink->SetPosition(out_samples);

	// We might still have decoded samples from the old position in
	// our frame, so clear them out.
	this->ClearFrame();
}

void PipeAudio::ClearFrame()
{
	this->frame.clear();
	this->frame_iterator = this->frame.end();
}

Audio::State PipeAudio::Update()
{
	assert(this->sink != nullptr);
	assert(this->src != nullptr);

	bool more_available = this->DecodeIfFrameEmpty();
	if (!more_available) this->sink->SourceOut();

	if (!this->FrameFinished()) this->TransferFrame();

	return this->sink->State();
}

void PipeAudio::TransferFrame()
{
	assert(!this->frame.empty());
	assert(this->sink != nullptr);
	assert(this->src != nullptr);

	this->sink->Transfer(this->frame_iterator, this->frame.end());

	// We empty the frame once we're done with it.  This
	// maintains FrameFinished(), as an empty frame is a finished one.
	if (this->FrameFinished()) {
		this->ClearFrame();
		assert(this->FrameFinished());
	}

	// The frame iterator should be somewhere between the beginning and
	// end of the frame, unless the frame was emptied.
	assert(this->frame.empty() ||
	       (this->frame.begin() <= this->frame_iterator &&
	        this->frame_iterator < this->frame.end()));
}

bool PipeAudio::DecodeIfFrameEmpty()
{
	// Either the current frame is in progress, or has been emptied.
	// AdvanceFrameIterator() establishes this assertion by emptying a
	// frame as soon as it finishes.
	assert(this->frame.empty() || !this->FrameFinished());

	// If we still have a frame, don't bother decoding yet.
	if (!this->FrameFinished()) return true;

	assert(this->src != nullptr);
	AudioSource::DecodeResult result = this->src->Decode();

	this->frame = result.second;
	this->frame_iterator = this->frame.begin();

	return result.first != AudioSource::DecodeState::END_OF_FILE;
}

bool PipeAudio::FrameFinished() const
{
	return this->frame.end() <= this->frame_iterator;
}
