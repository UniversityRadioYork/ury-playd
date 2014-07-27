// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the Playslave Error exception set.
 * @see errors.hpp
 */

#include "errors.hpp"

Error::Error(const std::string &message)
{
	this->message = std::string(message);
}

const std::string &Error::Message() const
{
	return this->message;
}
