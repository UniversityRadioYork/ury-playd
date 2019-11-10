// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file player.cpp
 * Main implementation file for the Player class.
 * @see player.h
 */

#include <cassert>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "audio/audio.h"
#include "audio/sink.h"
#include "audio/source.h"
#include "errors.h"
#include "messages.h"
#include "player.h"
#include "response.h"

namespace Playd {

    Response PlayerDead(std::string_view tag) {
        return Response::Failure(tag, MSG_CMD_PLAYER_CLOSING);
    }

//
// Player
//

    Player::Player(int device_id, SinkFn sink, std::map<std::string, SourceFn> sources)
            : device_id{device_id},
              sink{std::move(sink)},
              sources{std::move(sources)},
              file{std::make_unique<Audio::NullAudio>()},
              dead{false},
              io{nullptr},
              last_pos{0} {
    }

    void Player::SetIo(const ResponseSink &new_io) {
        this->io = &new_io;
    }

    bool Player::Update() {
        assert(this->file != nullptr);
        const auto as = this->file->Update();

        if (as == Audio::Audio::State::AT_END) this->End(Response::NOREQUEST);
        if (as == Audio::Audio::State::PLAYING) {
            // Since the audio is currently playing, the position may have
            // advanced since last update.  So we need to update it.
            auto pos = this->file->Position();
            if (this->CanBroadcastPos(pos))
                this->BroadcastPos(Response::NOREQUEST, pos);
        }

        return !this->dead;
    }

//
// Commands
//

    Response Player::Dump(size_t id, Response::Tag tag) const {
        if (this->dead) return PlayerDead(tag);

        this->DumpState(id, tag);
        this->DumpFileInfo(id, tag);

        return Response::Success(tag);
    }

    void Player::DumpFileInfo(size_t id, Response::Tag tag) const {
        // This information won't exist if there is no file.
        if (this->file->CurrentState() == Audio::Audio::State::NONE) return;

        auto filename = this->file->File();
        Respond(id, Response(tag, Response::Code::FLOAD).AddArg(filename));

        auto pos = this->file->Position();
        AnnounceTimestamp(Response::Code::POS, id, tag, pos);

        auto len = this->file->Length();
        AnnounceTimestamp(Response::Code::LEN, id, tag, len);
    }

    Response Player::Eject(Response::Tag tag) {
        if (this->dead) return PlayerDead(tag);

        // Silently ignore ejects on ejected files.
        // Concurrently speaking, this should be fine, as we are the only
        // thread that can eject or un-eject files.
        if (this->file->CurrentState() == Audio::Audio::State::NONE) {
            return Response::Success(tag);
        }

        assert(this->file != nullptr);
        this->file = std::make_unique<Audio::NullAudio>();

        this->DumpState(0, tag);

        return Response::Success(tag);
    }

    Response Player::End(Response::Tag tag) {
        if (this->dead) return PlayerDead(tag);

        // Let upstream know that the file ended by itself.
        // This is needed for auto-advancing playlists, etc.
        this->Respond(0, Response(Response::NOREQUEST, Response::Code::END));

        this->SetPlaying(tag, false);

        // Rewind the file back to the start.  We can't use Player::Pos() here
        // in case End() is called from Pos(); a seek failure could start an
        // infinite loop.
        this->PosRaw(Response::NOREQUEST, std::chrono::microseconds{0});

        return Response::Success(tag);
    }

    Response Player::Load(Response::Tag tag, std::string_view path) {
        if (this->dead) return PlayerDead(tag);

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
        this->last_pos = std::chrono::seconds{0};

        // A load will change all of the player's state in one go,
        // so just send a Dump() instead of writing out all of the responses
        // here.
        // Don't take the response from here, though, because it has the wrong
        // tag.
        this->Dump(0, Response::NOREQUEST);

        return Response::Success(tag);
    }

    Response Player::Pos(Response::Tag tag, std::string_view pos_str) {
        if (this->dead) return PlayerDead(tag);

        std::chrono::microseconds pos{0};
        try {
            pos = PosParse(pos_str);
        } catch (SeekError &e) {
            // Seek errors here are a result of clients sending weird times.
            // Thus, we tell them off.
            return Response::Invalid(tag, e.Message());
        }

        try {
            this->PosRaw(tag, pos);
        } catch (NullAudioError &) {
            return Response::Invalid(tag, MSG_CMD_NEEDS_LOADED);
        } catch (SeekError &) {
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

    Response Player::SetPlaying(Response::Tag tag, bool playing) {
        if (this->dead) return PlayerDead(tag);

        // Why is SetPlaying not split between Start() and Stop()?, I hear the
        // best practices purists amongst you say.  Quite simply, there is a
        // large amount of fiddly exception boilerplate here that would
        // otherwise be duplicated between the two methods.

        assert(this->file != nullptr);

        try {
            this->file->SetPlaying(playing);
        } catch (NullAudioError &e) {
            return Response::Invalid(tag, e.Message());
        }

        this->DumpState(0, Response::NOREQUEST);

        return Response::Success(tag);
    }

    Response Player::Quit(Response::Tag tag) {
        if (this->dead) return PlayerDead(tag);

        this->Eject(tag);
        this->dead = true;
        return Response::Success(tag);
    }

//
// Command implementations
//

/* static */ std::chrono::microseconds Player::PosParse(std::string_view pos_str) {
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
            pos = std::stoull(std::string{pos_str}, &cpos);
        } catch (...) {
            throw SeekError(MSG_SEEK_INVALID_VALUE);
        }

        // cpos will point to the first character in pos that wasn't a number.
        // We don't want any such characters here, so bail if the position isn't
        // at the end of the string.
        const auto sl = pos_str.length();
        if (cpos != sl) throw SeekError(MSG_SEEK_INVALID_VALUE);

        return std::chrono::microseconds{pos};
    }

    void Player::PosRaw(Response::Tag tag, std::chrono::microseconds pos) {
        Expects(this->file != nullptr);

        this->file->SetPosition(pos);
        this->BroadcastPos(tag, pos);
    }

    void Player::DumpState(size_t id, Response::Tag tag) const {
        Response::Code code = StateResponseCode();

        this->Respond(id, Response(tag, code));
    }

    Response::Code Player::StateResponseCode() const {
        switch (file->CurrentState()) {
            case Audio::Audio::State::AT_END:
                return Response::Code::END;
            case Audio::Audio::State::NONE:
                return Response::Code::EJECT;
            case Audio::Audio::State::PLAYING:
                return Response::Code::PLAY;
            case Audio::Audio::State::STOPPED:
                return Response::Code::STOP;
        }
        return Response::Code::EJECT;
    }

    void Player::Respond(int id, const Response &rs) const {
        if (this->io != nullptr) this->io->Respond(id, rs);
    }

    void Player::AnnounceTimestamp(Response::Code code, int id, Response::Tag tag,
                                   std::chrono::microseconds ts) const {
        this->Respond(id, Response(tag, code)
                .AddArg(std::to_string(ts.count())));
    }

    bool Player::CanBroadcastPos(std::chrono::microseconds pos) const {
        // Because last_pos is counted in seconds, this condition becomes
        // true whenever pos 'ticks over' to a different second from last_pos.
        // The result is that we broadcast at most roughly once a second.
        return this->last_pos < std::chrono::duration_cast<std::chrono::seconds>(pos);
    }

    void Player::BroadcastPos(Response::Tag tag, std::chrono::microseconds pos) {
        // This ensures we don't broadcast too often:
        // see CanBroadcastPos.
        this->last_pos = std::chrono::duration_cast<std::chrono::seconds>(pos);
        this->AnnounceTimestamp(Response::Code::POS, 0, tag, pos);
    }

    std::unique_ptr<Audio::Audio> Player::LoadRaw(std::string_view path) const {
        auto source = this->LoadSource(path);
        assert(source != nullptr);

        auto sink = this->sink(*source, this->device_id);
        return std::make_unique<Audio::BasicAudio>(std::move(source), std::move(sink));
    }

    std::unique_ptr<Audio::Source> Player::LoadSource(std::string_view path) const {
        size_t extpoint = path.find_last_of('.');
        auto ext = std::string{path.substr(extpoint + 1)};

        auto ibuilder = this->sources.find(ext);
        if (ibuilder == this->sources.end()) {
            throw FileError("Unknown file format: " + ext);
        }

        return (ibuilder->second)(path);
    }

} // namespace Playd
