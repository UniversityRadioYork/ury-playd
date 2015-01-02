// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of classes pertaining to responses to the client.
 * @see io/io_response.cpp
 */

#ifndef PLAYD_IO_RESPONSE_HPP
#define PLAYD_IO_RESPONSE_HPP

#include <cstdint>
#include <map>
#include <ostream>
#include <string>
#include <vector>

#include "../errors.hpp"

/// A response.
class Response
{
public:
	/**
	 * Four-character response codes.
	 * @note If you're adding new responses here, update
	 * ResponseSink::STRINGS.
	 * @see ResponseSink::STRINGS
	 */
	enum class Code : std::uint8_t {
		OK,       ///< Request was valid and produced an answer.
		WHAT,     ///< Request was invalid/user error.
		FAIL,     ///< Error, pointing blame at environment.
		OHAI,     ///< Server starting up.
		STATE,    ///< Server changing state.
		TIME,     ///< Server sending current song time,
		FILE,     ///< The loaded file just changed.
		FEATURES, ///< Server sending feature list.
		END       ///< The loaded file just ended on its own.
	};

	/**
	 * Constructs a Response with no arguments.
	 * @param code The Response::Code representing the response command.
	 */
	Response(Response::Code code);

	/**
	 * Adds an argument to this Response.
	 * @param arg The argument to add.  The argument must not be escaped.
	 * @return A reference to this Response, for chaining.
	 */
	Response &Arg(const std::string &arg);

	/**
	 * Packs the Response, converting it to a BAPS3 protocol message.
	 * Pack()ing does not alter the Response, which may be Pack()ed again.
	 * @return The BAPS3 message, sans newline, ready to send.
	 */
	std::string Pack() const;

private:
	/**
	 * A map from Response::Code codes to their string equivalents.
	 * @see Response::Code
	 */
	static const std::string STRINGS[];

	/**
	 * Escapes a single response argument.
	 * @param argument The argument to escape.
	 * @return The escaped argument.
	 */
	static std::string EscapeArgument(const std::string &argument);

	/// The current packed form of the response.
	/// @see Pack
	std::string string;
};

/**
 * Abstract class for anything that can be sent a response.
 */
class ResponseSink
{
public:
	/**
	 * Outputs a response.
	 * @param response The Response to output.
	 */
	virtual void Respond(const Response &response) const;
};

#endif // PLAYD_IO_RESPONSE_HPP
