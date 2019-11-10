// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Dummy classes for response class tests.
 */

#ifndef PLAYD_TESTS_DUMMY_RESPONSE_SINK_H
#define PLAYD_TESTS_DUMMY_RESPONSE_SINK_H

#include <ostream>

#include "../response.h"

namespace Playd::Tests
{
// A dummy class for testing the ResponseSink abstract class methods.
class DummyResponseSink : public ResponseSink
{
public:
	/**
	 * Constructs a Dummy_response_sink.
	 * @param os The ostream to which responses should be sent.
	 */
	DummyResponseSink(std::ostream &os);

protected:
	virtual void Respond(size_t id, const Response &response) const override;

private:
	/// Reference to the output stream.
	std::ostream &os;
};

} // namespace Playd::Tests

#endif // PLAYD_TESTS_DUMMY_RESPONSE_SINK_H
