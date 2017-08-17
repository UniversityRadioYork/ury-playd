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

// A dummy class for testing the Response_sink abstract class methods.
class Dummy_response_sink : public Response_sink {
public:
	/**
	 * Constructs a Dummy_response_sink.
	 * @param os The ostream to which responses should be sent.
	 */
	Dummy_response_sink(std::ostream &os);

protected:
	virtual void Respond(size_t id, const Response &response) const override;

private:
	/// Reference to the output stream.
	std::ostream &os;
};

#endif // PLAYD_TESTS_DUMMY_RESPONSE_SINK_H
