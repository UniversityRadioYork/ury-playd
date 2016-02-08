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

#include "audio/audio_system.hpp"
#include "audio/audio.hpp"
#include "errors.hpp"
#include "response.hpp"
#include "messages.h"
#include "player.hpp"


Player::Player(AudioSystem &audio)
    : audio(audio), file(audio.Null()), is_running(true), sink(nullptr)
{
}

void Player::SetSink(ResponseSink &sink)
{
	this->sink = &sink;
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
			this->sink->Respond(0, Response(Response::NOREQUEST, Response::Code::POS).AddArg(std::to_string(pos)));
		}
	}

	return this->is_running;
}

void Player::WelcomeClient(size_t id) const
{
	this->sink->Respond(id, Response(Response::NOREQUEST, Response::Code::OHAI).AddArg(std::to_string(id)).AddArg(MSG_OHAI_BIFROST).AddArg(MSG_OHAI_PLAYD));
	this->sink->Respond(id, Response(Response::NOREQUEST, Response::Code::IAMA).AddArg("player/file"));
	this->Dump(id, Response::NOREQUEST);
}

Response Player::End(const std::string &tag)
{
	this->sink->Respond(0, Response(tag, Response::Code::END));

	this->SetPlaying(tag, false);

	// Rewind the file back to the start.  We can't use Player::Pos() here
	// in case End() is called from Pos(); a seek failure could start an
	// infinite loop.
	this->PosRaw(tag, 0);

	// Let upstream know that the file ended by itself.
	// This is needed for auto-advancing playlists, etc.
	if (this->sink == nullptr) return Response::Success(tag);

	return Response::Success(tag);
}

//
// Commands
//

Response Player::RunCommand(const std::vector<std::string> &cmd, size_t id)
{
	// First of all, figure out what the tag of this command is.
	// We make it by adding the id to the zeroth command word.
	if (cmd.size() == 0) return Response::Invalid(Response::NOREQUEST, MSG_CMD_SHORT);
	auto tag = cmd[0];
	if (cmd.size() <= 1) return Response::Invalid(tag, MSG_CMD_SHORT);

	if (!this->is_running) {
		// Refuse any and all commands when not running.
		// This is mainly to prevent the internal state from
		// going weird, but seems logical anyway.
		return Response::Failure(tag, MSG_CMD_PLAYER_CLOSING);
	}

	// The first word is always the tag; the second is the actual command word.

	auto word = cmd[1];
	auto nargs = cmd.size() - 2;

	if (nargs == 0 && "play" == word) return this->SetPlaying(tag, true);
	if (nargs == 0 && "stop" == word) return this->SetPlaying(tag, false);
	if (nargs == 0 && "end" == word) return this->End(tag);
	if (nargs == 0 && "eject" == word) return this->Eject(tag);
	if (nargs == 0 && "quit" == word) return this->Quit(tag);
	if (nargs == 0 && "dump" == word) return this->Dump(id, tag);
	if (nargs == 1 && "fload" == word) return this->Load(tag, cmd[2]);
	if (nargs == 1 && "pos" == word) return this->Pos(tag, cmd[2]);

	return Response::Invalid(tag, MSG_CMD_INVALID);
}

Response Player::Eject(const std::string &tag)
{
	assert(this->file != nullptr);
	this->file = this->audio.Null();

	this->DumpState(0, tag);

	return Response::Success(tag);
}

Response Player::Load(const std::string &tag, const std::string &path)
{
	if (path.empty()) return Response::Invalid(tag, MSG_LOAD_EMPTY_PATH);

	assert(this->file != nullptr);

	// Bin the current file as soon as possible.
	// This ensures that we don't have any situations where two files are
	// contending over resources, or the current file spends a second or
	// two flushing its remaining audio.
	this->file = this->audio.Null();

	try {
		assert(this->file != nullptr);
		this->file = this->audio.Load(path);
		this->last_pos = 0;
		this->DumpRaw(0, tag);
		assert(this->file != nullptr);
	} catch (FileError &e) {
		// File errors aren't fatal, so catch them here.
		this->Eject(tag);
		return Response::Failure(tag, e.Message());
	} catch (Error &) {
		// Ensure a load failure doesn't leave a corrupted track
		// loaded.
		this->Eject(tag);
		throw;
	}

	return Response::Success(tag);
}

Response Player::SetPlaying(const std::string &tag, bool playing)
{
	// Why is SetPlaying not split between Start() and Stop()?, I hear the
	// best practices purists amongst you say.  Quite simply, there is a
	// large amount of fiddly exception boilerplate here that would
	// otherwise be duplicated between the two methods.  The 'public'
	// interface Player gives is Start()/Stop(), anyway.

	assert(this->file != nullptr);

	try {
		this->file->SetPlaying(playing);
	} catch (NoAudioError &e) {
		return Response::Invalid(tag, e.Message());
	}

	this->DumpState(0, tag);

	return Response::Success(tag);
}

Response Player::Quit(const std::string &tag)
{
	this->Eject(tag);
	this->is_running = false;
	return Response::Success(tag);
}

Response Player::Pos(const std::string &tag, const std::string &pos_str)
{
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

/* static */ std::uint64_t Player::PosParse(const std::string &pos_str)
{
	size_t cpos = 0;

	// In previous versions, this used to parse a unit at the end.
	// This was removed for simplicity--use baps3-cli etc. instead.
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

	this->sink->Respond(0, Response(tag, Response::Code::POS).AddArg(std::to_string(pos)));
}

void Player::DumpRaw(size_t id, const std::string &tag) const
{
	auto rs = std::vector<Response>();

	this->DumpState(id, tag);

	// This information won't exist if there is no file.
	if (this->file->CurrentState() != Audio::State::NONE) {
		auto file = this->file->File();
		this->sink->Respond(id, Response(tag, Response::Code::FLOAD).AddArg(file));

		auto pos = this->file->Position();
		this->sink->Respond(id, Response(tag, Response::Code::POS).AddArg(std::to_string(pos)));
	}
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

	this->sink->Respond(id, Response(tag, code));
}

Response Player::Dump(size_t id, const std::string &tag) const
{
	this->DumpRaw(id, tag);
	this->sink->Respond(id, Response(tag, Response::Code::DUMP));
	return Response::Success(tag);
}

bool Player::CanAnnounceTime(std::uint64_t micros)
{
	std::uint64_t secs = micros / 1000 / 1000;

	// We can announce if the last announced pos was in a previous second.
	bool announce = this->last_pos < secs;
	if (announce) this->last_pos = secs;

	return announce;
}
