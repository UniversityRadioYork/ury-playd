// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declarations of input/output related code.
 * @see io.cpp
 */

#ifndef PS_IO_HPP
#define PS_IO_HPP

#include <map>
#include <iostream>
#include <string>

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
 * Base case for the RespondArgs template, for when there are no arguments.
 */
inline void RespondArgs()
{
}

/**
 * Outputs a response body, with a variadic number of arguments.
 * This is defined inductively, with RespondArgs() being the base case.
 * @tparam Arg1 The type of the leftmost argument.
 * @tparam Args Parameter pack of remaining arguments.
 * @param arg1 The leftmost argument.
 * @param args The remaining arguments.
 */
template <typename Arg1, typename... Args>
inline void RespondArgs(const Arg1 &arg1, const Args &... args)
{
	std::cout << " " << arg1;
	RespondArgs(args...);
}

/**
 * Base case for the Respond template, for when there are no arguments.
 * @param code The response code to emit.
 */
inline void Respond(Response code)
{
	std::cout << RESPONSES.at(code) << std::endl;
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
	std::cout << RESPONSES.at(code);
	RespondArgs(args...);
	std::cout << std::endl;
}

/**
 * Checks whether input is waiting from the client.
 * @return A truthy value if input is waiting; a falsy value otherwise.
 * @todo Move into a class/replace with asynchronous I/O.
 */
int input_waiting(void);

#endif // PS_IO_HPP
