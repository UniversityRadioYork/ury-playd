// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of classes pertaining to responses to the client.
 * @see response.cpp
 */

#ifndef PLAYD_IO_RESPONSE_HPP
#define PLAYD_IO_RESPONSE_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "errors.hpp"

/// A response.
class Response
{
public:
	/**
	 * Enumeration of all possible response codes.
	 * @note If you're adding new responses here, update
	 * ResponseSink::STRINGS.
	 * @see ResponseSink::STRINGS
	 */
	enum class Code : std::uint8_t {
		OHAI,   ///< Server starting up.
		ACK,    ///< Command result.
		RES,    ///< Resource result.
		UPDATE, ///< Resource changed.
		END,
	};

	/**
	 * Constructs a Response with no arguments.
	 * @param code The Response::Code representing the response command.
	 */
	Response(Response::Code code);

	/**
	 * Constructs a RES response.
	 * @param path The path to the resource.
	 * @param type The type of resource being emitted.
	 * @param value The string representing the resource's value.
	 */
	static std::unique_ptr<Response> Res(const std::string &path,
	                                     const std::string &type,
	                                     const std::string &value);

	/**
	 * Adds an argument to this Response.
	 * @param arg The argument to add.  The argument must not be escaped.
	 * @return A reference to this Response, for chaining.
	 */
	Response &AddArg(const std::string &arg);

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
	 * @param arg The argument to escape.
	 * @return The escaped argument.
	 */
	static std::string EscapeArg(const std::string &arg);

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
	/// Empty virtual destructor for ResponseSink.
	virtual ~ResponseSink() = default;

	/**
	 * Outputs a response.
	 * @param response The Response to output.
	 * @param id The ID, if pertinent, of the sub-component of the
	 *   ResponseSink to receive a Response, or 0, which signifies that the
	 *   entire sub-component should receive the Response.  Defaults to 0.
	 */
	virtual void Respond(const Response &response, size_t id = 0) const;
};

#endif // PLAYD_IO_RESPONSE_HPP
