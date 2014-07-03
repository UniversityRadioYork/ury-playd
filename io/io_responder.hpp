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
	 * Base case for the RespondArgs template, for when there are no
	 * arguments.
	 */
	inline void RespondArgs(std::ostream &)
	{
	}

	/**
	 * Outputs a response body, with a variadic number of arguments.
	 * This is defined inductively, with RespondArgs() being the base case.
	 * @tparam Arg1 The type of the leftmost argument.
	 * @tparam Args Parameter pack of remaining arguments.
	 * @param stream The stream onto which the response body shall be
	 *   output.
	 * @param arg1 The leftmost argument.
	 * @param args The remaining arguments.
	 */
	template <typename Arg1, typename... Args>
	inline void RespondArgs(std::ostream &stream, const Arg1 &arg1,
	                        const Args &... args)
	{
		stream << " " << arg1;
		RespondArgs(stream, args...);
	}

	/**
	 * Base case for the Respond template, for when there are no arguments.
	 * @param code The response code to emit.
	 */
	inline void Respond(Response code)
	{
		ResponseViaOstream([&](std::ostream &stream) {
			stream << RESPONSES.at(code) << std::endl;
		});
	}

	/**
	 * Outputs a response, with a variadic number of arguments.
	 * This is defined on RespondArgs.
	 * @tparam Args Parameter pack of arguments.
	 * @param code The response code to emit.
	 * @param args The arguments, if any.
	 */
	template <typename... Args>
	inline void Respond(Response code, Args &... args)
	{
		ResponseViaOstream([&](std::ostream &stream) {
			stream << RESPONSES.at(code);
			RespondArgs(stream, args...);
			stream << std::endl;
		});
	}

	/**
	 * Emits an error as a response.
	 * @param error The error to convert to a response.
	 */
	void RespondWithError(const Error &error);

protected:
	/**
	 * Provides an ostream for building a response.
	 * The ostream is provided through a lambda, and the response will be
	 * sent
	 * either during or after the lambda's lifetime.
	 * @param f The lambda to invoke with the ostream.
	 */
	virtual void ResponseViaOstream(
	                std::function<void(std::ostream &)> f) = 0;
};

#endif // PS_IO_RESPONDER_HPP
