/* errors.cpp - error reporting */

/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#include <string>

#include "errors.hpp"
#include "io.hpp"

Error::Error(ErrorCode error_code, std::string message)
{
	this->error_code = error_code;
	this->message = message;
}

ErrorCode Error::Code()
{
	return this->error_code;
}

void Error::ToResponse()
{
	Respond(Response::FAIL, Message());
}

const std::string &Error::Message()
{
	return this->message;
}
