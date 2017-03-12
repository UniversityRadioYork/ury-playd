// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file player.cpp
 * Main implementation file for the Player class.
 * @see player.hpp
 */

#include <cassert>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "audio/audio_sink.hpp"
#include "audio/audio_source.hpp"
#include "audio/audio.hpp"
#include "errors.hpp"
#include "response.hpp"
#include "messages.h"
#include "player.hpp"

Player::Player(int device_id, SinkFn sink, std::map<std::string, SourceFn> sources)
    : device_id(device_id),
      sink(std::move(sink)),
      sources(std::move(sources)),
      file(std::make_unique<NoAudio>()),
      dead(false),
      io(nullptr),
      last_pos(0)
{
}

void Player::SetIo(ResponseSink &io)
{
	this->io = &io;
}

bool Player::Update()
{
	assert(this->file != nullptr);
	auto as = this->file->Update();

	if (as == Audio::State::AT_END) this->End(Response::NOREQUEST);
	if (as == Audio::State::PLAYING) {
		// Since the audio is currently playing, the position may have
		// advanced since last update.  So we need to update it.
		auto pos = this->file->Position();
		if (this->CanAnnounceTime(pos)) {
			this->Respond(0, Response(Response::NOREQUEST,
			                          Response::Code::POS)
			                         .AddArg(std::to_string(pos)));
		}
	}

	return !this->dead;
}

//
// Commands
//

Response Player::Dump(size_t id, const std::string &tag) const
{
	if (this->dead) return Response::Failure(tag, MSG_CMD_PLAYER_CLOSING);

	this->DumpState(id, tag);

	// This information won't exist if there is no file.
	if (this->file->CurrentState() != Audio::State::NONE) {
		auto file = this->file->File();
		this->Respond(
		        id, Response(tag, Response::Code::FLOAD).AddArg(file));

		auto pos = this->file->Position();
		this->Respond(id, Response(tag, Response::Code::POS)
		                          .AddArg(std::to_string(pos)));

	auto len = this->file->Length();
	this->Respond(id, Response(tag, Response::Code::LEN)
	                         .AddArg(std::to_string(len)));
	}

	return Response::Success(tag);
}

Response Player::Eject(const std::string &tag)
{
	if (this->dead) return Response::Failure(tag, MSG_CMD_PLAYER_CLOSING);

	// Silently ignore ejects on ejected files.
	// Concurrently speaking, this should be fine, as we are the only
	// thread that can eject or un-eject files.
	if (this->file->CurrentState() == Audio::State::NONE) {
		return Response::Success(tag);
	}

	assert(this->file != nullptr);
	this->file = std::make_unique<NoAudio>();

	this->DumpState(0, tag);

	return Response::Success(tag);
}

Response Player::End(const std::string &tag)
{
	if (this->dead) return Response::Failure(tag, MSG_CMD_PLAYER_CLOSING);

	// Let upstream know that the file ended by itself.
	// This is needed for auto-advancing playlists, etc.
	this->Respond(0, Response(Response::NOREQUEST, Response::Code::END));

	this->SetPlaying(tag, false);

	// Rewind the file back to the start.  We can't use Player::Pos() here
	// in case End() is called from Pos(); a seek failure could start an
	// infinite loop.
	this->PosRaw(Response::NOREQUEST, 0);

	return Response::Success(tag);
}

Response Player::Load(const std::string &tag, const std::string &path)
{
	if (this->dead) return Response::Failure(tag, MSG_CMD_PLAYER_CLOSING);
	if (path.empty()) return Response::Invalid(tag, MSG_LOAD_EMPTY_PATH);

	assert(this->file != nullptr);

	// Bin the current file as soon as possible.
	// This ensures that we don't have any situations where two files are
	// contending over resources, or the current file spends a second or
	// two flushing its remaining audio.
	this->Eject(Response::NOREQUEST);

	try {
		this->file = this->LoadRaw(path);
	} catch (FileError &e) {
		// File errors aren't fatal, so catch them here.
		return Response::Failure(tag, e.Message());
	}

	assert(this->file != nullptr);
	this->last_pos = 0;

	// A load will change all of the player's state in one go,
	// so just send a Dump() instead of writing out all of the responses
	// here.
	// Don't take the response from here, though, because it has the wrong
	// tag.
	this->Dump(0, Response::NOREQUEST);

	return Response::Success(tag);
}

Response Player::Pos(const std::string &tag, const std::string &pos_str)
{
	if (this->dead) return Response::Failure(tag, MSG_CMD_PLAYER_CLOSING);

	std::uint64_t pos = 0;
	try {
		pos = PosParse(pos_str);
	} catch (SeekError &e) {
		// Seek errors here are a result of clients sending weird times.
		// Thus, we tell them off.
		return Response::Invalid(tag, e.Message());
	}

	try {
		this->PosRaw(tag, pos);
	} catch (NoAudioError) {
		return Response::Invalid(tag, MSG_CMD_NEEDS_LOADED);
	} catch (SeekError) {
		// Seek failures here are a result of the decoder not liking the
		// seek position (usually because it's outside the audio file!).
		// Thus, unlike above, we try to recover.

		Debug() << "Seek failure" << std::endl;

		// Make it look to the client as if the seek ran off the end of
		// the file.
		this->End(tag);
	}

	// If we've made it all the way down here, we deserve to succeed.
	return Response::Success(tag);
}

Response Player::SetPlaying(const std::string &tag, bool playing)
{
	if (this->dead) return Response::Failure(tag, MSG_CMD_PLAYER_CLOSING);

	// Why is SetPlaying not split between Start() and Stop()?, I hear the
	// best practices purists amongst you say.  Quite simply, there is a
	// large amount of fiddly exception boilerplate here that would
	// otherwise be duplicated between the two methods.

	assert(this->file != nullptr);

	try {
		this->file->SetPlaying(playing);
	} catch (NoAudioError &e) {
		return Response::Invalid(tag, e.Message());
	}

	this->DumpState(0, Response::NOREQUEST);

	return Response::Success(tag);
}

Response Player::Quit(const std::string &tag)
{
	if (this->dead) return Response::Failure(tag, MSG_CMD_PLAYER_CLOSING);

	this->Eject(tag);
	this->dead = true;
	return Response::Success(tag);
}

//
// Command implementations
//

/* static */ std::uint64_t Player::PosParse(const std::string &pos_str)
{
	size_t cpos = 0;

	// Try and see if this position string is negative.
	// Cheap and easy way: see if it has '-'.
	// This means we don't need to skip whitespace first, with no loss
	// of suction: no valid position string will contain '-'.
	if (pos_str.find('-') != std::string::npos) {
		throw SeekError(MSG_SEEK_INVALID_VALUE);
	}


	std::uint64_t pos;
	try {
		pos = std::stoull(pos_str, &cpos);
	} catch (...) {
		throw SeekError(MSG_SEEK_INVALID_VALUE);
	}

	// cpos will point to the first character in pos that wasn't a number.
	// We don't want any such characters here, so bail if the position isn't
	// at the end of the string.
	auto sl = pos_str.length();
	if (cpos != sl) throw SeekError(MSG_SEEK_INVALID_VALUE);

	return pos;
}

void Player::PosRaw(const std::string &tag, std::uint64_t pos)
{
	assert(this->file != nullptr);

	this->file->SetPosition(pos);

	// This is required to make CanAnnounceTime() continue working.
	this->last_pos = pos / 1000 / 1000;

	this->Respond(0, Response(tag, Response::Code::POS)
	                         .AddArg(std::to_string(pos)));
}

void Player::DumpState(size_t id, const std::string &tag) const
{
	Response::Code code = Response::Code::EJECT;

	switch (this->file->CurrentState()) {
		case Audio::State::AT_END:
			code = Response::Code::END;
			break;
		case Audio::State::NONE:
			code = Response::Code::EJECT;
			break;
		case Audio::State::PLAYING:
			code = Response::Code::PLAY;
			break;
		case Audio::State::STOPPED:
			code = Response::Code::STOP;
			break;
		default:
			// Just don't dump anything in this case.
			return;
	}

	this->Respond(id, Response(tag, code));
}

void Player::Respond(int id, Response rs) const
{
	if (this->io != nullptr) this->io->Respond(id, rs);
}

bool Player::CanAnnounceTime(std::uint64_t micros)
{
	std::uint64_t secs = micros / 1000 / 1000;

	// We can announce if the last announced pos was in a previous second.
	bool announce = this->last_pos < secs;
	if (announce) this->last_pos = secs;

	return announce;
}

std::unique_ptr<Audio> Player::LoadRaw(const std::string &path) const
{
	std::unique_ptr<AudioSource> source = this->LoadSource(path);
	assert(source != nullptr);

	auto sink = this->sink(*source, this->device_id);
	return std::make_unique<PipeAudio>(std::move(source), std::move(sink));
}

std::unique_ptr<AudioSource> Player::LoadSource(const std::string &path) const
{
	size_t extpoint = path.find_last_of('.');
	std::string ext = path.substr(extpoint + 1);

	auto ibuilder = this->sources.find(ext);
	if (ibuilder == this->sources.end()) {
		throw FileError("Unknown file format: " + ext);
	}

	return (ibuilder->second)(path);
}
