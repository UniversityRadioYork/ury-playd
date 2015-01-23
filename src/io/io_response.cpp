// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of client response classes.
 * @see io/io_response.hpp
 */

#include <cctype>
#include <initializer_list>
#include <sstream>

#include "../errors.hpp"
#include "io_response.hpp"

const std::string Response::STRINGS[] = {
        "OK",       // Code::OK
        "WHAT",     // Code::WHAT
        "FAIL",     // Code::FAIL
        "OHAI",     // Code::OHAI
        "STATE",    // Code::STATE
        "TIME",     // Code::TIME
        "FILE",     // Code::FILE
        "FEATURES", // Code::FEATURES
        "END"       // Code::END
};

Response::Response(Response::Code code)
{
	this->string = Response::STRINGS[static_cast<int>(code)];
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

/* static */ std::string Response::EscapeArg(const std::string &arg)
{
	bool escaping = false;
	std::string escaped;

	for (unsigned char c : arg) {
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

void ResponseSink::Respond(const Response &) const
{
	// By default, do nothing.
}
