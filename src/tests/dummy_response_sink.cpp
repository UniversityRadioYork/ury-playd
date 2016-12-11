// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of DummyResponseSink.
 */

#include <ostream>
#include <string>
#include "dummy_response_sink.h"

DummyResponseSink::DummyResponseSink(std::ostream &os) : os(os)
{
}

void DummyResponseSink::Respond(size_t, const Response &response) const
{
	this->os << response.Pack() << std::endl;
}
