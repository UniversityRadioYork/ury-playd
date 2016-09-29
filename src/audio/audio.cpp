// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the PipeAudio class.
 * @see audio/pipe_audio.h
 */

#include <cassert>
#include <chrono>
#include <climits>
#include <cstdint>
#include <string>

#include "../gsl/gsl"
#include "../errors.h"
#include "../messages.h"
#include "../response.h"
#include "audio.h"
#include "audio_sink.h"
#include "audio_source.h"
#include "sample_formats.h"

using namespace std::chrono;


//
// NoAudio
//

Audio::State NoAudio::Update()
{
	return State::NONE;
}

Audio::State NoAudio::CurrentState() const
{
	return State::NONE;
}

void NoAudio::SetPlaying(bool)
{
	throw NoAudioError(MSG_CMD_NEEDS_LOADED);
}

void NoAudio::SetPosition(microseconds)
{
	throw NoAudioError(MSG_CMD_NEEDS_LOADED);
}

microseconds NoAudio::Position() const
{
	throw NoAudioError(MSG_CMD_NEEDS_LOADED);
}

microseconds NoAudio::Length() const
{
	throw NoAudioError(MSG_CMD_NEEDS_LOADED);
}

const std::string &NoAudio::File() const
{
	throw NoAudioError(MSG_CMD_NEEDS_LOADED);
}

//
// PipeAudio
//

PipeAudio::PipeAudio(std::unique_ptr<AudioSource> src,
                     std::unique_ptr<AudioSink> sink)
    : src(move(src)), sink(move(sink))
{
	this->ClearFrame();
}

const std::string &PipeAudio::File() const
{
	return this->src->Path();
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

Audio::State PipeAudio::CurrentState() const
{
	return this->sink->State();
}

microseconds PipeAudio::Position() const
{
	Expects(this->sink != nullptr);
	Expects(this->src != nullptr);

	return this->src->MicrosFromSamples(this->sink->Position());
}

microseconds PipeAudio::Length() const
{
	assert(this->sink != nullptr);
	assert(this->src != nullptr);

	return this->src->MicrosFromSamples(this->src->Length());
}

void PipeAudio::SetPosition(microseconds position)
{
	Expects(this->sink != nullptr);
	Expects(this->src != nullptr);

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
	this->frame_span = gsl::span<uint8_t, 0>();
}

Audio::State PipeAudio::Update()
{
	Expects(this->sink != nullptr);
	Expects(this->src != nullptr);

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

	auto written = this->sink->Transfer(this->frame_span);
	this->frame_span = this->frame_span.last(this->frame_span.length() - written);

	// We empty the frame once we're done with it.  This
	// maintains FrameFinished(), as an empty frame is a finished one.
	if (this->FrameFinished()) {
		this->ClearFrame();
		Ensures(this->FrameFinished());
	}

	// The frame span should not be empty unless the frame was emptied.
	Ensures(this->frame.empty() || !this->frame_span.empty());
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
	this->frame_span = this->frame;

	return result.first != AudioSource::DecodeState::END_OF_FILE;
}

inline bool PipeAudio::FrameFinished() const
{
	return this->frame_span.empty();
}
