// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of Dummy_response_sink.
 */

#include "dummy_response_sink.h"

#include <ostream>
#include <string>

namespace playd::tests
{
Dummy_response_sink::Dummy_response_sink(std::ostream &os) : os(os)
{
}

void Dummy_response_sink::Respond(size_t, const Response &response) const
{
	this->os << response.Pack() << std::endl;
}

} // namespace playd::tests
