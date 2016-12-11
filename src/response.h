// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of classes pertaining to responses to the client.
 * @see response.cpp
 */

#ifndef PLAYD_IO_RESPONSE_H
#define PLAYD_IO_RESPONSE_H

#include <cstdint>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "errors.h"

/// A response.
class Response
{
public:
	/// The tag for unsolicited messages (not from responses).
	static const std::string NOREQUEST;

	/**
	 * Enumeration of all possible response codes.
	 * @note If you're adding new responses here, update
	 *       Response::STRINGS.
	 * @see Response::STRINGS
	 */
	enum class Code : std::uint8_t {
		OHAI,  ///< Server starting up.
		IAMA,  ///< Server sending its role.
		FLOAD, ///< The loaded file just changed.
		EJECT, ///< The loaded file just ejected.
		POS,   ///< Server sending current song time.
		END,   ///< The loaded file just ended.
		PLAY,  ///< The loaded file is playing.
		STOP,  ///< The loaded file has stopped.
		ACK,   ///< Command result.
        LEN
	};

	/// The number of codes, which should agree with Response::Code.
	static constexpr std::uint8_t CODE_COUNT = 10;

	/**
	 * Constructs a Response with no arguments.
	 * @param tag The tag of the response.
	 * @param code The Response::Code representing the response command.
	 */
	Response(const std::string &tag, Response::Code code);

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

	/**
	 * Shortcut for constructing a final response to a successful request.
	 * @param tag The tag of the original request.
	 * @return A Response acknowledging a successful request.
	 */
	static Response Success(const std::string &tag);

	/**
	* Shortcut for constructing a final response to a invalid request.
	* @param tag The tag of the original request.
	* @param msg The failure message.
	* @return A Response acknowledging an invalid request.
	*/
	static Response Invalid(const std::string &tag, const std::string &msg);

	/**
	* Shortcut for constructing a final response to a failed request.
	* @param tag The tag of the original request.
	* @param msg The failure message.
	* @return A Response acknowledging a failed request.
	*/
	static Response Failure(const std::string &tag, const std::string &msg);

private:

	/**
	 * A map from Response::Code codes to their string equivalents.
	 * @see Response::Code
	 */
	static const std::array<std::string, CODE_COUNT> STRINGS;

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
	 * @param id The ID of the client of the ResponseSink receiving this
	 * response.
	 *   Use 0 for broadcasts.
	 * @param response The Response to output.
	 */
	virtual void Respond(size_t id, const Response &response) const;
};

#endif // PLAYD_IO_RESPONSE_H
