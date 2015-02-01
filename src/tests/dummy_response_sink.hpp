// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Dummy classes for response class tests.
 */

#ifndef PLAYD_TESTS_IO_RESPONSE_HPP
#define PLAYD_TESTS_IO_RESPONSE_HPP

#include <ostream>

#include "../response.hpp"

// A dummy class for testing the ResponseSink abstract class methods.
class DummyResponseSink : public ResponseSink {
public:
	/**
	 * Constructs a DummyResponseSink.
	 * @param os The ostream to which responses should be sent.
	 */
	DummyResponseSink(std::ostream &os);

protected:
	virtual void Respond(const Response &response) const override;

private:
	/// Reference to the output stream.
	std::ostream &os;
};

#endif // PLAYD_TESTS_IO_RESPONSE_HPP
