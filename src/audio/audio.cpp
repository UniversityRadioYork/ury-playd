// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the Null_audio and Audio classes.
 * @see audio/audio.h
 */

#include "audio.h"

#include <chrono>
#include <cstdint>
#include <gsl/gsl>

#include "../errors.h"
#include "../messages.h"
#include "audio_sink.h"
#include "audio_source.h"
#include "sample_format.h"

//
// Null_audio
//

/// The error thrown if a Null_audio is asked to do something it can't do.
Error NotSupportedInNullAudio()
{
	return Null_audio_error{MSG_CMD_NEEDS_LOADED};
}

Audio::State Null_audio::Update()
{
	return State::none;
}

Audio::State Null_audio::CurrentState() const
{
	return State::none;
}

void Null_audio::SetPlaying(bool)
{
	throw NotSupportedInNullAudio();
}

void Null_audio::SetPosition(std::chrono::microseconds)
{
	throw NotSupportedInNullAudio();
}

std::chrono::microseconds Null_audio::Position() const
{
	throw NotSupportedInNullAudio();
}

std::chrono::microseconds Null_audio::Length() const
{
	throw NotSupportedInNullAudio();
}

const std::string &Null_audio::File() const
{
	throw NotSupportedInNullAudio();
}

//
// Basic_audio
//

Basic_audio::Basic_audio(std::unique_ptr<Audio_source> src,
                         std::unique_ptr<Audio_sink> sink)
    : src{std::move(src)}, sink{std::move(sink)}
{
	this->ClearFrame();
}

const std::string &Basic_audio::File() const
{
	return this->src->Path();
}

void Basic_audio::SetPlaying(bool playing)
{
	Expects(this->sink != nullptr);

	if (playing) {
		this->sink->Start();

		// It's ok for the sink to be playing, ejected or at-end here.
		Ensures(this->sink->CurrentState() != Audio::State::stopped);
	} else {
		this->sink->Stop();

		// It's ok for the sink to be stopped, ejected or at-end here.
		Ensures(this->sink->CurrentState() != Audio::State::playing);
	}
}

Audio::State Basic_audio::CurrentState() const
{
	return this->sink->CurrentState();
}

std::chrono::microseconds Basic_audio::Position() const
{
	Expects(this->sink != nullptr);
	Expects(this->src != nullptr);

	return this->src->MicrosFromSamples(this->sink->Position());
}

std::chrono::microseconds Basic_audio::Length() const
{
	Expects(this->sink != nullptr);
	Expects(this->src != nullptr);

	return this->src->MicrosFromSamples(this->src->Length());
}

void Basic_audio::SetPosition(std::chrono::microseconds position)
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

void Basic_audio::ClearFrame()
{
	this->frame.clear();
	this->frame_span = gsl::span<uint8_t, 0>();
}

Audio::State Basic_audio::Update()
{
	Expects(this->sink != nullptr);
	Expects(this->src != nullptr);

	const auto more_available = this->DecodeIfFrameEmpty();
	if (!more_available) this->sink->SourceOut();

	if (!this->FrameFinished()) this->TransferFrame();

	return this->sink->CurrentState();
}

void Basic_audio::TransferFrame()
{
	Expects(!this->frame.empty());
	Expects(this->sink != nullptr);
	Expects(this->src != nullptr);

	auto written = this->sink->Transfer(this->frame_span);
	this->frame_span =
	        this->frame_span.last(this->frame_span.size() - written);

	// We empty the frame once we're done with it.  This
	// maintains FrameFinished(), as an empty frame is a finished one.
	if (this->FrameFinished()) {
		this->ClearFrame();
		Ensures(this->FrameFinished());
	}

	// The frame span should not be empty unless the frame was emptied.
	Ensures(this->frame.empty() || !this->frame_span.empty());
}

bool Basic_audio::DecodeIfFrameEmpty()
{
	// Either the current frame is in progress, or has been emptied.
	// AdvanceFrameIterator() establishes this assertion by emptying a
	// frame as soon as it finishes.
	Expects(this->frame.empty() || !this->FrameFinished());

	// If we still have a frame, don't bother decoding yet.
	if (!this->FrameFinished()) return true;

	Expects(this->src != nullptr);
	Audio_source::Decode_result result = this->src->Decode();

	this->frame = result.second;
	this->frame_span = this->frame;

	return result.first != Audio_source::Decode_state::eof;
}

inline bool Basic_audio::FrameFinished() const
{
	return this->frame_span.empty();
}
