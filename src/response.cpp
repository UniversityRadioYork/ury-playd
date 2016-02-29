// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of client response classes.
 * @see response.hpp
 */

#include <array>
#include <cctype>
#include <sstream>

#include "errors.hpp"

#include "response.hpp"

/* static */ const std::string Response::NOREQUEST = "!";

/* static */ const std::array<std::string, Response::CODE_COUNT> Response::STRINGS = {
        "OHAI",  // Code::OHAI
        "IAMA",  // Code::IAMA
        "FLOAD", // Code::FLOAD
        "EJECT", // Code::EJECT
        "POS",   // Code::POS
        "END",   // Code::END
        "PLAY",  // Code::PLAY
        "STOP",  // Code::STOP
        "ACK"    // Code::ACK
};

Response::Response(const std::string &tag, Response::Code code)
{
	this->string = Response::EscapeArg(tag) + " " +
	               STRINGS[static_cast<uint8_t>(code)];
}

Response &Response::AddArg(const std::string &arg)
{
	this->string += " " + Response::EscapeArg(arg);
	return *this;
}

std::string Response::Pack() const
{
	return this->string;
}

/* static */ Response Response::Success(const std::string &tag)
{
	return Response(tag, Response::Code::ACK)
	        .AddArg("OK")
	        .AddArg("success");
}

/* static */ Response Response::Invalid(const std::string &tag,
                                        const std::string &msg)
{
	return Response(tag, Response::Code::ACK).AddArg("WHAT").AddArg(msg);
}

/* static */ Response Response::Failure(const std::string &tag,
                                        const std::string &msg)
{
	return Response(tag, Response::Code::ACK).AddArg("FAIL").AddArg(msg);
}

/* static */ std::string Response::EscapeArg(const std::string &arg)
{
	bool escaping = false;
	std::string escaped;

	for (char c : arg) {
		// These are the characters (including all whitespace, via
		// isspace())  whose presence means we need to single-quote
		// escape the argument.
		bool is_escaper = c == '"' || c == '\'' || c == '\\';
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
// ResponseSink
//

void ResponseSink::Respond(size_t, const Response &) const
{
	// By default, do nothing.
}
