// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of client response classes.
 * @see io/io_response.hpp
 */

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
	// ResponseCodes are formatted as "CODE message\n".
	// Delegate the actual sending of the response string to the concrete
	// implementation.
	RespondRaw(RESPONSES[static_cast<int>(code)] + " " + message);
}

void ResponseSink::RespondWithError(const Error &error) const
{
	Respond(ResponseCode::FAIL, error.Message());
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
