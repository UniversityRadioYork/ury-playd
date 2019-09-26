// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of client response classes.
 * @see response.h
 */

#include <array>
#include <cctype>
#include <sstream>

#include "response.h"

namespace playd {

/* static */ constexpr std::array<std::string_view, Response::CODE_COUNT> CODE_STRINGS{{
                                                                                               "OHAI",  // Code::ohai
                                                                                               "IAMA",  // Code::iama
                                                                                               "FLOAD", // Code::fload
                                                                                               "EJECT", // Code::eject
                                                                                               "POS",   // Code::pos
                                                                                               "END",   // Code::end
                                                                                               "PLAY",  // Code::play
                                                                                               "STOP",  // Code::stop
                                                                                               "ACK",   // Code::ack
                                                                                               "LEN"    // Code::len
                                                                                       }};

    Response::Response(std::string_view tag, Response::Code code) {
        this->string = Response::EscapeArg(tag) + " " + std::string{CODE_STRINGS[static_cast<uint8_t>(code)]};
    }

    Response &Response::AddArg(std::string_view arg) {
        this->string += " " + Response::EscapeArg(arg);
        return *this;
    }

    std::string Response::Pack() const {
        return this->string;
    }

/* static */ Response Response::Success(Response::Tag tag) {
        return Response(tag, Response::Code::ack)
                .AddArg("OK")
                .AddArg("success");
    }

/* static */ Response Response::Invalid(Response::Tag tag,
                                        std::string_view msg) {
        return Response(tag, Response::Code::ack).AddArg("WHAT").AddArg(msg);
    }

/* static */ Response Response::Failure(Response::Tag tag,
                                        std::string_view msg) {
        return Response(tag, Response::Code::ack).AddArg("FAIL").AddArg(msg);
    }

/* static */ std::string Response::EscapeArg(std::string_view arg) {
        bool escaping = false;
        std::string escaped;

        for (char c : arg) {
            // These are the characters (including all whitespace, via
            // isspace())  whose presence means we need to single-quote
            // escape the argument.
            const bool is_escaper = c == '"' || c == '\'' || c == '\\';
            if (isspace(c) || is_escaper) escaping = true;

            // Since we use single-quote escaping, the only thing we need
            // to escape by itself is single quotes, which are replaced by
            // the sequence '\'' (break out of single quotes, escape a
            // single quote, then re-enter single quotes).
            escaped += (c == '\'') ? R"('\'')" : std::string(1, c);
        }

        // Only single-quote escape if necessary.
        // Otherwise, it wastes two characters!
        if (escaping) return "'" + escaped + "'";
        return escaped;
    }

//
// Response_sink
//

    void Response_sink::Respond(size_t, const Response &) const {
        // By default, do nothing.
    }

} // namespace playd
