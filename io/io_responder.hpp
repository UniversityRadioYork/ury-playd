// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the Responder abstract class.
 * @see io/io_responder.cpp
 */

#ifndef PS_IO_RESPONDER_HPP
#define PS_IO_RESPONDER_HPP

#include <functional>    // std::function
#include <map>           // std::map
#include <string>        // std::string
#include <ostream>       // std::ostream etc.
#include "../errors.hpp" // Error

/**
 * Four-character response codes.
 * @note If you're adding new responses here, update RESPONSES.
 * @see RESPONSES
 */
enum class Response {
	/* 'Pull' responses (initiated by client command) */
	OKAY, /* Request was valid and produced an answer */
	WHAT, /* Request was invalid/user error */
	FAIL, /* Error, pointing blame at environment */
	OOPS, /* Error, pointing blame at programmer */
	NOPE, /* Request was probably valid, but forbidden. */
	/* 'Push' responses (initiated by server) */
	OHAI, /* Server starting up */
	TTFN, /* Server shutting down */
	STAT, /* Server changing state */
	TIME, /* Server sending current song time */
	DBUG, /* Debug information */
	/* Queue-specific responses */
	QENT, /* Requested information about a Queue ENTry */
	QMOD, /* A command caused a Queue MODification */
	QPOS, /* The current Queue POSition has changed */
	QNUM  /* Reminder of current number of queue items */
};

/**
 * A map from Response codes to their string equivalents.
 * @see Response
 */
extern const std::map<Response, std::string> RESPONSES;

/**
 * Abstract class for anything that can be sent a response.
 */
class Responder {
public:
	/**
	 * Outputs a response.
	 * @param code The response code to emit.
	 * @param message The response message.
	 */
	void Respond(Response code, const std::string &message);

	/**
	 * Emits an error as a response.
	 * @param error The error to convert to a response.
	 */
	void RespondWithError(const Error &error);

protected:
	/**
	 * Outputs a raw response string.
	 * @param string The response string, of the form "CODE message".
	 */
	virtual void RespondRaw(const std::string &string) = 0;
};

#endif // PS_IO_RESPONDER_HPP
