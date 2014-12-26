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
#include "../io/io_response.hpp"
#include "audio.hpp"
#include "audio_sink.hpp"
#include "audio_source.hpp"
#include "sample_formats.hpp"

PipeAudio::PipeAudio(AudioSource *src, AudioSink *sink) : src(src), sink(sink)
{
	this->ClearFrame();
}

void PipeAudio::Emit(const ResponseSink &sink) const
{
	assert(this->src != nullptr);
	sink.Respond(Response(Response::Code::FILE).Arg(this->src->Path()));
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

	auto samples = this->src->Seek(position);
	this->sink->SetPosition(samples);

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
