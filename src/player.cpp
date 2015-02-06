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
#include "cmd_result.hpp"
#include "errors.hpp"
#include "response.hpp"
#include "messages.h"
#include "player.hpp"

const std::vector<std::string> Player::FEATURES{"End", "FileLoad", "PlayStop",
                                                "Seek", "TimeReport"};

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

	if (as == Audio::State::AT_END) this->End();
	if (as == Audio::State::PLAYING) {
		// Since the audio is currently playing, the position may have
		// advanced since last update.  So we need to update it.
		this->file->Emit(Response::Code::TIME, this->sink);
	}

	return this->is_running;
}

void Player::EmitAllAudioState(size_t id) const
{
	this->file->Emit(Response::Code::FILE, sink, id);
	this->file->Emit(Response::Code::TIME, sink, id);
	this->file->Emit(Response::Code::STATE, sink, id);
}

void Player::WelcomeClient(size_t id) const
{
	this->sink->Respond(Response(Response::Code::OHAI).AddArg(MSG_OHAI), id);

	auto features = Response(Response::Code::FEATURES);
	for (auto &f : FEATURES) features.AddArg(f);
	this->sink->Respond(features, id);

	this->EmitAllAudioState(id);
}

void Player::End()
{
	this->Stop();

	// Rewind the file back to the start.  We can't use Player::Seek() here
	// in case End() is called from Seek(); a seek failure could start an
	// infinite loop.
	this->SeekRaw(0);

	// Let upstream know that the file ended by itself.
	// This is needed for auto-advancing playlists, etc.
	if (this->sink == nullptr) return;
	this->sink->Respond(Response(Response::Code::END));
}

//
// Commands
//

CommandResult Player::RunCommand(const std::vector<std::string> &cmd)
{
	switch (cmd.size()) {
		case 1:
			return this->RunNullaryCommand(cmd[0]);
		case 2:
			if (!cmd[1].empty()) {
				return this->RunUnaryCommand(cmd[0], cmd[1]);
			}
			return CommandResult::Invalid(MSG_CMD_INVALID);
		default:
			return CommandResult::Invalid(MSG_CMD_INVALID);
	}
}

CommandResult Player::RunNullaryCommand(const std::string &word)
{
	if ("play" == word) return this->Play();
	if ("stop" == word) return this->Stop();
	if ("eject" == word) return this->Eject();
	if ("quit" == word) return this->Quit();

	return CommandResult::Invalid(MSG_CMD_INVALID);
}

CommandResult Player::RunUnaryCommand(const std::string &word,
                                      const std::string &arg)
{
	if ("load" == word) return this->Load(arg);
	if ("seek" == word) return this->Seek(arg);

	return CommandResult::Invalid(MSG_CMD_INVALID);
}

CommandResult Player::Eject()
{
	assert(this->file != nullptr);
	this->file = this->audio.Null();
	this->file->Emit(Response::Code::STATE, this->sink);

	return CommandResult::Success();
}

CommandResult Player::Load(const std::string &path)
{
	if (path.empty()) return CommandResult::Invalid(MSG_LOAD_EMPTY_PATH);

	assert(this->file != nullptr);

	// Bin the current file as soon as possible.
	// This ensures that we don't have any situations where two files are
	// contending over resources, or the current file spends a second or
	// two flushing its remaining audio.
	this->file = this->audio.Null();

	try {
		assert(this->file != nullptr);
		this->file = this->audio.Load(path);
		this->EmitAllAudioState(0);
		assert(this->file != nullptr);
	} catch (FileError &e) {
		// File errors aren't fatal, so catch them here.
		this->Eject();
		return CommandResult::Failure(e.Message());
	} catch (Error &) {
		// Ensure a load failure doesn't leave a corrupted track
		// loaded.
		this->Eject();
		throw;
	}

	return CommandResult::Success();
}

CommandResult Player::Play()
{
	return this->SetPlaying(true);
}

CommandResult Player::Stop()
{
	return this->SetPlaying(false);
}

CommandResult Player::SetPlaying(bool playing)
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
		return CommandResult::Invalid(e.Message());
	}

	this->file->Emit(Response::Code::STATE, this->sink);

	return CommandResult::Success();
}

CommandResult Player::Quit()
{
	this->Eject();
	this->is_running = false;
	return CommandResult::Success();
}

CommandResult Player::Seek(const std::string &time_str)
{
	std::uint64_t pos = 0;
	try {
		pos = SeekParse(time_str);
	} catch (SeekError &e) {
		// Seek errors here are a result of clients sending weird times.
		// Thus, we tell them off.
		return CommandResult::Invalid(e.Message());
	}

	try {
		this->SeekRaw(pos);
	} catch (NoAudioError) {
		return CommandResult::Invalid(MSG_CMD_NEEDS_LOADED);
	} catch (SeekError) {
		// Seek failures here are a result of the decoder not liking the
		// seek position (usually because it's outside the audio file!).
		// Thus, unlike above, we try to recover.

		Debug() << "Seek failure" << std::endl;

		// Make it look to the client as if the seek ran off the end of
		// the file.
		this->End();
	}

	// If we've made it all the way down here, we deserve to succeed.
	return CommandResult::Success();
}

std::uint64_t Player::SeekParse(const std::string &time_str)
{
	size_t cpos = 0;

	// In previous versions, this used to parse a unit at the end.
	// This was removed for simplicity--use baps3-cli etc. instead.
	std::uint64_t pos;
	try {
		pos = std::stoull(time_str, &cpos);
	} catch (...) {
		throw SeekError(MSG_SEEK_INVALID_VALUE);
	}

	// cpos will point to the first character in pos that wasn't a number.
	// We don't want any such characters here, so bail if the position isn't
	// at the end of the string.
	auto sl = time_str.length();
	if (cpos != sl) throw SeekError(MSG_SEEK_INVALID_VALUE);

	return pos;
}

void Player::SeekRaw(std::uint64_t pos)
{
	assert(this->file != nullptr);

	this->file->Seek(pos);
	this->file->Emit(Response::Code::TIME, this->sink);
}