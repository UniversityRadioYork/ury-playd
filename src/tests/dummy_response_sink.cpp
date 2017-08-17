// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of Dummy_response_sink.
 */

#include <ostream>
#include <string>
#include "dummy_response_sink.h"

Dummy_response_sink::Dummy_response_sink(std::ostream &os) : os(os)
{
}

void Dummy_response_sink::Respond(size_t, const Response &response) const
{
	this->os << response.Pack() << std::endl;
}
