// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the Audio class.
 * @see audio/audio.hpp
 */

#include <algorithm>
#include <cassert>
#include <climits>
#include <string>

#include "../errors.hpp"
#include "../sample_formats.hpp"
#include "../messages.h"
#include "../time_parser.hpp"
#include "../io/io_response.hpp"
#include "audio.hpp"
#include "audio_sink.hpp"
#include "audio_source.hpp"

PipeAudio::PipeAudio(AudioSource *source, AudioSink *sink) : source(source), sink(sink)
{
	this->ClearFrame();
}

void PipeAudio::Emit(ResponseSink &sink) const
{
	assert(this->source != nullptr);
	sink.Respond(ResponseCode::FILE, this->source->Path());
}

void PipeAudio::Start()
{
	assert(this->sink != nullptr);
	this->sink->Start();
}

void PipeAudio::Stop()
{
	assert(this->sink != nullptr);
	this->sink->Stop();
}

TimeParser::MicrosecondPosition PipeAudio::Position() const
{
	assert(this->sink != nullptr);
	assert(this->source != nullptr);

	return this->source->MicrosecondPositionFromSamples(
	                this->sink->Position());
}

void PipeAudio::Seek(TimeParser::MicrosecondPosition position)
{
	assert(this->sink != nullptr);
	assert(this->source != nullptr);

	auto samples = this->source->Seek(position);
	this->sink->SetPosition(samples);

	// We might still have decoded samples from the old position in
	// our frame, so clear them out.
	this->ClearFrame();
}

void PipeAudio::ClearFrame()
{
	this->frame.clear();
	this->frame_iterator = this->frame.end();
	this->file_ended = false;
}

Audio::State PipeAudio::Update()
{
	assert(this->sink != nullptr);
	assert(this->source != nullptr);

	bool more_frames_available = this->DecodeIfFrameEmpty();

	if (!this->FrameFinished()) this->TransferFrame();

	this->sink->SetInputReady(more_frames_available);
	this->file_ended = !more_frames_available;

	if (this->file_ended) return Audio::State::AT_END;
	if (this->sink->IsStopped()) return Audio::State::STOPPED;
	return Audio::State::PLAYING;
}

void PipeAudio::TransferFrame()
{
	assert(!this->frame.empty());
	assert(this->sink != nullptr);
	assert(this->source != nullptr);

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

	assert(this->source != nullptr);
	AudioSource::DecodeResult result = this->source->Decode();

	this->frame = result.second;
	this->frame_iterator = this->frame.begin();

	return result.first != AudioSource::DecodeState::END_OF_FILE;
}

bool PipeAudio::FrameFinished() const
{
	return this->frame.end() <= this->frame_iterator;
}
