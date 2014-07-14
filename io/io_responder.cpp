// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the non-virtual aspects of the Responder abstract class.
 * @see io/io_responder.hpp
 */

#include <sstream>          // std::ostringstream
#include "io_responder.hpp" // Responder, Response
#include "../errors.hpp"    // Error

const std::map<Response, std::string> RESPONSES = {{Response::OKAY, "OKAY"},
                                                   {Response::WHAT, "WHAT"},
                                                   {Response::FAIL, "FAIL"},
                                                   {Response::OOPS, "OOPS"},
                                                   {Response::NOPE, "NOPE"},
                                                   {Response::OHAI, "OHAI"},
                                                   {Response::TTFN, "TTFN"},
                                                   {Response::STAT, "STAT"},
                                                   {Response::TIME, "TIME"},
                                                   {Response::DBUG, "DBUG"},
                                                   {Response::QPOS, "QPOS"},
                                                   {Response::QENT, "QENT"},
                                                   {Response::QMOD, "QMOD"},
                                                   {Response::QNUM, "QNUM"}};

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
