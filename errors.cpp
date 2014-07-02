// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the Playslave Error exception set.
 * @see errors.hpp
 */

#include <string>

#include "errors.hpp"
#include "io.hpp"

Error::Error(const std::string &message)
{
	this->message = std::string(message);
}

void Error::ToResponse()
{
	Respond(Response::FAIL, Message());
}

const std::string &Error::Message()
{
	return this->message;
}
