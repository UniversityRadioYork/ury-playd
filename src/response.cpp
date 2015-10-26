// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of client response classes.
 * @see response.hpp
 */

#include <cctype>
#include <initializer_list>
#include <sstream>

#include "errors.hpp"

#include "response.hpp"

const std::string Response::STRINGS[] = {
	"OHAI",   // Code::OHAI
	"ACK",    // Code::ACK
	"RES",    // Code::RES
	"UPDATE", // Code::UPDATE
	"END",
};

// Pre-made responses.
std::unique_ptr<Response> Response::Res(const std::string &tag,
                                        const std::string &path,
                                        const std::string &type,
                                        const std::string &value)
{
	auto res = std::unique_ptr<Response>(
		new Response(Response::Code::RES)
	);
	res->AddArg(tag).AddArg(path).AddArg(type).AddArg(value);
	return res;
}

std::unique_ptr<Response> Response::Update(const std::string &path, const std::string &type, const std::string &value)
{
	auto res = std::unique_ptr<Response>(
		new Response(Response::Code::UPDATE)
	);
	res->AddArg(path).AddArg(type).AddArg(value);
	return res;
}

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
		if (c == '\'') {
			escaped += R"('\'')";
		} else {
			escaped += c;
		}
	}

	// Only single-quote escape if necessary.
	// Otherwise, it wastes two characters!
	if (escaping) return "'" + escaped + "'";
	return escaped;
}

//
// ResponseSink
//

void ResponseSink::Respond(const Response &, size_t) const
{
	// By default, do nothing.
}
