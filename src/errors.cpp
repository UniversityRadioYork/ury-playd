// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the playd Error exception set.
 * @see errors.h
 */

#include "errors.h"

Error::Error(const std::string_view msg) : std::runtime_error{std::string {msg}}, message{msg}
{
}

std::string_view Error::Message() const
{
	return this->message;
}
