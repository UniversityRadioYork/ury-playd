// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of client response classes.
 * @see io/io_response.hpp
 */

#include <sstream>         // std::ostringstream
#include "io_response.hpp" // ResponseSink, ResponseCode
#include "../errors.hpp"   // Error

const std::map<ResponseCode, std::string> RESPONSES = {
                {ResponseCode::OKAY, "OKAY"},
                {ResponseCode::WHAT, "WHAT"},
                {ResponseCode::FAIL, "FAIL"},
                {ResponseCode::OOPS, "OOPS"},
                {ResponseCode::NOPE, "NOPE"},
                {ResponseCode::OHAI, "OHAI"},
                {ResponseCode::TTFN, "TTFN"},
                {ResponseCode::STAT, "STAT"},
                {ResponseCode::TIME, "TIME"}};

void ResponseSink::Respond(ResponseCode code, const std::string &message)
{
	// ResponseCodes are formatted as "CODE message\n".
	std::ostringstream os;
	os << RESPONSES.at(code) << " " << message << std::endl;

	// Delegate the actual sending of the response string to the concrete
	// implementation.
	RespondRaw(os.str());
}

void ResponseSink::RespondWithError(const Error &error)
{
	Respond(ResponseCode::FAIL, error.Message());
}

//
// ResponseSource
//

void ResponseSource::SetResponseSink(ResponseSink &responder)
{
	this->push_sink = std::ref(responder);
}

void ResponseSource::Push() const
{
	if (this->push_sink.is_initialized()) {
		Emit(this->push_sink.get().get());
	}
}
