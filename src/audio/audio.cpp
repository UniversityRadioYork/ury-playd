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

//
// NoAudio
//

Audio::State NoAudio::Update()
{
	return Audio::State::NONE;
}

void NoAudio::Emit(std::initializer_list<Response::Code> codes, const ResponseSink *sink)
{
	if (sink == nullptr) return;

	for (auto &code : codes) {
		if (code == Response::Code::STATE) {
			sink->Respond(Response(Response::Code::STATE).Arg("Ejected"));
		}
	}
}

void NoAudio::Start()
{
	throw NoAudioError(MSG_CMD_NEEDS_LOADED);
}

void NoAudio::Stop()
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

PipeAudio::PipeAudio(AudioSource *src, AudioSink *sink) : src(src), sink(sink)
{
	this->ClearFrame();
}

void PipeAudio::Emit(std::initializer_list<Response::Code> codes, const ResponseSink *sink)
{
	if (sink == nullptr) return;

	assert(this->src != nullptr);
	assert(this->sink != nullptr);

	for (auto &code : codes) {
		auto r = Response(code);

		if (code == Response::Code::STATE) {
			auto playing = this->sink->State() == Audio::State::PLAYING;
			r.Arg(playing ? "Playing" : "Stopped");
		} else if (code == Response::Code::FILE) {
			r.Arg(this->src->Path());
		} else if (code == Response::Code::TIME) {
			// To prevent spewing massive amounts of TIME
			// responses, we only send one if the number of seconds
			// has changed since the last request for this
			// response on this sink.
			std::uint64_t micros = this->Position();
			std::uint64_t secs = micros / 1000 / 1000;

			auto last_entry = this->last_times.find(sink);

			// We can announce if we haven't got a record for this
			// sink, or if the last record was in a previous
			// second.
			bool can_announce = true;
			if (last_entry != this->last_times.end()) {
				can_announce = (last_entry->second < secs);

				// This is so as to allow the emplace below to
				// work--it fails if there's already a value
				// under the same key.
				if (can_announce) this->last_times.erase(last_entry);
			}

			if (!can_announce) continue;
			this->last_times.emplace(sink, secs);
			r.Arg(std::to_string(micros));
		} else {
			continue;
		}

		sink->Respond(r);
	}
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

	auto in_samples = this->src->SamplesFromMicros(position);
	auto out_samples = this->src->Seek(in_samples);
	this->sink->SetPosition(out_samples);

	// Make sure we always announce the new position to all response sinks.
	this->last_times.clear();

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
