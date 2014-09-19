// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

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

const std::string RESPONSES[] = {
	/* ResponseCode::OKAY     */ "OKAY",
	/* ResponseCode::WHAT     */ "WHAT",
	/* ResponseCode::FAIL     */ "FAIL",
	/* ResponseCode::OHAI     */ "OHAI",
	/* ResponseCode::STATE    */ "STATE",
	/* ResponseCode::TIME     */ "TIME",
	/* ResponseCode::FILE     */ "FILE",
	/* ResponseCode::FEATURES */ "FEATURES",
	/* ResponseCode::END      */ "END"
};

void ResponseSink::Respond(ResponseCode code, const std::string &message) const
{
	RespondArgs(code, { message });
}

void ResponseSink::RespondArgs(ResponseCode code, const std::initializer_list<std::string> arguments) const
{
	std::ostringstream os;
	os << RESPONSES[static_cast<int>(code)];
	for (auto argument : arguments) os << " " << EscapeArgument(argument);

	// Delegate the actual sending of the response string to the concrete
	// implementation.
	RespondRaw(os.str());
}

void ResponseSink::RespondWithError(const Error &error) const
{
	Respond(ResponseCode::FAIL, error.Message());
}

std::string ResponseSink::EscapeArgument(const std::string &argument) const
{
	bool escaping = false;
	std::string escaped;

	for (unsigned char c : argument) {
		bool is_escaper = c == '"' || c == '\'' || c == '\\';
		if (isspace(c) || is_escaper) escaping = true;

		// Since we use single-quote escaping, the only thing we need
		// to escape by itself is single quotes, which are replaced by
		// the sequence '\'' (break out of single quotes, escape a
		// single quote, then re-enter single quotes).
		escaped += (c == '\'') ? R"('\'')" : std::string(1, c);
	}

	if (escaping) return "'" + escaped + "'";
	return escaped;
}

//
// ResponseSource
//

void ResponseSource::SetResponseSink(ResponseSink &responder)
{
	this->push_sink = &responder;
}

void ResponseSource::Push() const
{
	if (this->push_sink != nullptr) {
		Emit(*this->push_sink);
	}
}
