// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the NullAudio and BasicAudio classes.
 * @see audio/audio.h
 */

#include "audio.h"

#include <chrono>
#include <gsl/gsl>

#include "../errors.h"
#include "../messages.h"
#include "sink.h"
#include "source.h"

namespace Playd::Audio
{
//
// NullAudio
//

/// The error thrown if a Null_audio is asked to do something it can't do.
NullAudioError NotSupportedInNullAudio()
{
	return NullAudioError{MSG_CMD_NEEDS_LOADED};
}

Audio::State NullAudio::Update()
{
	return State::NONE;
}

Audio::State NullAudio::CurrentState() const
{
	return State::NONE;
}

void NullAudio::SetPlaying(bool)
{
	throw NotSupportedInNullAudio();
}

void NullAudio::SetPosition(std::chrono::microseconds)
{
	throw NotSupportedInNullAudio();
}

std::chrono::microseconds NullAudio::Position() const
{
	throw NotSupportedInNullAudio();
}

std::chrono::microseconds NullAudio::Length() const
{
	throw NotSupportedInNullAudio();
}

std::string_view NullAudio::File() const
{
	throw NotSupportedInNullAudio();
}

//
// BasicAudio
//

BasicAudio::BasicAudio(std::unique_ptr<Source> src, std::unique_ptr<Sink> sink)
    : src{std::move(src)}, sink{std::move(sink)}
{
	this->ClearFrame();
}

std::string_view BasicAudio::File() const
{
	return this->src->Path();
}

void BasicAudio::SetPlaying(bool playing)
{
	Expects(this->sink != nullptr);

	if (playing) {
		this->sink->Start();

		// It's ok for the sink to be playing, ejected or at-end here.
		Ensures(this->sink->CurrentState() != Audio::State::STOPPED);
	} else {
		this->sink->Stop();

		// It's ok for the sink to be stopped, ejected or at-end here.
		Ensures(this->sink->CurrentState() != Audio::State::PLAYING);
	}
}

Audio::State BasicAudio::CurrentState() const
{
	return this->sink->CurrentState();
}

std::chrono::microseconds BasicAudio::Position() const
{
	Expects(this->sink != nullptr);
	Expects(this->src != nullptr);

	return this->src->MicrosFromSamples(this->sink->Position());
}

std::chrono::microseconds BasicAudio::Length() const
{
	Expects(this->sink != nullptr);
	Expects(this->src != nullptr);

	return this->src->MicrosFromSamples(this->src->Length());
}

void BasicAudio::SetPosition(std::chrono::microseconds position)
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

void BasicAudio::ClearFrame()
{
	this->frame.clear();
	this->frame_span = gsl::span<std::byte, 0>();
}

Audio::State BasicAudio::Update()
{
	Expects(this->sink != nullptr);
	Expects(this->src != nullptr);

	const auto more_available = this->DecodeIfFrameEmpty();
	if (!more_available) this->sink->SourceOut();

	if (!this->FrameFinished()) this->TransferFrame();

	return this->sink->CurrentState();
}

void BasicAudio::TransferFrame()
{
	Expects(!this->frame.empty());
	Expects(this->sink != nullptr);
	Expects(this->src != nullptr);

	auto written = this->sink->Transfer(this->frame_span);
	this->frame_span = this->frame_span.last(this->frame_span.size() - written);

	// We empty the frame once we're done with it.  This
	// maintains FrameFinished(), as an empty frame is a finished one.
	if (this->FrameFinished()) {
		this->ClearFrame();
		Ensures(this->FrameFinished());
	}

	// The frame span should not be empty unless the frame was emptied.
	Ensures(this->frame.empty() || !this->frame_span.empty());
}

bool BasicAudio::DecodeIfFrameEmpty()
{
	// Either the current frame is in progress, or has been emptied.
	// AdvanceFrameIterator() establishes this assertion by emptying a
	// frame as soon as it finishes.
	Expects(this->frame.empty() || !this->FrameFinished());

	// If we still have a frame, don't bother decoding yet.
	if (!this->FrameFinished()) return true;

	Expects(this->src != nullptr);
	auto result = this->src->Decode();

	this->frame = result.second;
	this->frame_span = this->frame;

	return result.first != Source::DecodeState::END_OF_FILE;
}

inline bool BasicAudio::FrameFinished() const
{
	return this->frame_span.empty();
}

} // namespace Playd::Audio
