// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Dummy classes for response class tests.
 */

#ifndef PS_TESTS_IO_RESPONSE_HPP
#define PS_TESTS_IO_RESPONSE_HPP

#include "../io/io_response.hpp"

// A dummy class for testing the ResponseSink abstract class methods.
class DummyResponseSink : public ResponseSink {
public:
	/**
	 * Retrieve the last response sent to this DummyResponseSink.
	 * @return The last raw response.
	 */
	std::string LastResponse() const;

protected:
	virtual void RespondRaw(const std::string &string);

private:
	/// The last response sent to this DummyResponseSink.
	std::string last_response;
};

#endif // PS_TESTS_IO_RESPONSE_HPP

