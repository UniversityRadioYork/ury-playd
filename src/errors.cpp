// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the playd Error exception set.
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
