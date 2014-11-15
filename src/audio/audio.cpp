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
#include "audio.hpp"
#include "audio_sink.hpp"
#include "audio_source.hpp"

Audio::Audio(AudioSource *source, AudioSink *sink) : source(source), sink(sink)
{
	this->ClearFrame();
}

std::string Audio::Path() const
{
	assert(this->source != nullptr);
	return this->source->Path();
}

void Audio::Start()
{
	this->sink->Start();
}

void Audio::Stop()
{
	this->sink->Stop();
}

bool Audio::IsStopped()
{
	return this->sink->IsStopped();
}

TimeParser::MicrosecondPosition Audio::CurrentPositionMicroseconds()
{
	return this->source->MicrosecondPositionFromSamples(
	                this->sink->Position());
}

void Audio::SeekToPositionMicroseconds(
                TimeParser::MicrosecondPosition microseconds)
{
	auto samples = this->source->Seek(microseconds);
	this->sink->SetPosition(samples);
	this->ClearFrame();
}

void Audio::ClearFrame()
{
	this->frame.clear();
	this->frame_iterator = this->frame.end();
	this->file_ended = false;
}

bool Audio::Update()
{
	bool more_frames_available = this->DecodeIfFrameEmpty();

	if (!this->FrameFinished()) {
		this->TransferFrame();
	}

	this->sink->SetInputReady(more_frames_available);
	this->file_ended = !more_frames_available;
	return more_frames_available;
}

void Audio::TransferFrame()
{
	assert(!this->frame.empty());

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

bool Audio::DecodeIfFrameEmpty()
{
	// Either the current frame is in progress, or has been emptied.
	// AdvanceFrameIterator() establishes this assertion by emptying a
	// frame as soon as it finishes.
	assert(this->frame.empty() || !this->FrameFinished());

	bool more_frames_available = true;

	if (this->FrameFinished()) {
		AudioSource::DecodeResult result = this->source->Decode();

		this->frame = result.second;
		this->frame_iterator = this->frame.begin();

		more_frames_available = result.first !=
		                        AudioSource::DecodeState::END_OF_FILE;
	}

	return more_frames_available;
}

bool Audio::FileEnded() const
{
	return this->file_ended;
}

bool Audio::FrameFinished() const
{
	return this->frame.end() <= this->frame_iterator;
}
