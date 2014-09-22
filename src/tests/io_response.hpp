// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Dummy classes for response class tests.
 */

#ifndef PS_TESTS_IO_RESPONSE_HPP
#define PS_TESTS_IO_RESPONSE_HPP

#include <ostream>

#include "../io/io_response.hpp"

// A dummy class for testing the ResponseSink abstract class methods.
class DummyResponseSink : public ResponseSink {
public:
	/**
	 * Constructs a DummyResponseSink.
	 * @param os The ostream to which responses should be sent.
	 */
	DummyResponseSink(std::ostream &os);

protected:
	virtual void RespondRaw(const std::string &string) const;

private:
	/// Reference to the output stream.
	std::ostream &os;
};

#endif // PS_TESTS_IO_RESPONSE_HPP

