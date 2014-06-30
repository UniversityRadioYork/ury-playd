/* errors.cpp - error reporting */

/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
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
