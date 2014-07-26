// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of client response classes.
 * @see io/io_response.hpp
 */

#include <sstream>         // std::ostringstream
#include "io_response.hpp" // Responder, Response
#include "../errors.hpp"   // Error

const std::map<Response, std::string> RESPONSES = {{Response::OKAY, "OKAY"},
                                                   {Response::WHAT, "WHAT"},
                                                   {Response::FAIL, "FAIL"},
                                                   {Response::OOPS, "OOPS"},
                                                   {Response::NOPE, "NOPE"},
                                                   {Response::OHAI, "OHAI"},
                                                   {Response::TTFN, "TTFN"},
                                                   {Response::STAT, "STAT"},
                                                   {Response::TIME, "TIME"}};

void Responder::Respond(Response code, const std::string &message)
{
	// Responses are formatted as "CODE message\n".
	std::ostringstream os;
	os << RESPONSES.at(code) << " " << message << std::endl;

	// Delegate the actual sending of the response string to the concrete
	// implementation.
	RespondRaw(os.str());
}

void Responder::RespondWithError(const Error &error)
{
	Respond(Response::FAIL, error.Message());
}

//
// ResponseSource
//

void ResponseSource::SetResponder(Responder &responder)
{
	this->push_sink = std::ref(responder);
}

const void ResponseSource::EmitToRegisteredSink() const
{
	if (this->push_sink.is_initialized()) {
		Emit(this->push_sink.get().get());
	}
}