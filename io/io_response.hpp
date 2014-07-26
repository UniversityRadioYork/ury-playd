// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of classes pertaining to responses to the client.
 * @see io/io_response.cpp
 */

#ifndef PS_IO_RESPONSE_HPP
#define PS_IO_RESPONSE_HPP

#include <functional>    // std::function
#include <map>           // std::map
#include <string>        // std::string
#include <ostream>       // std::ostream etc.

#include <boost/optional.hpp>

#include "../errors.hpp" // Error

/**
 * Four-character response codes.
 * @note If you're adding new responses here, update RESPONSES.
 * @see RESPONSES
 */
enum class Response {
	// 'Pull' responses (initiated by client command)
	OKAY, ///< Request was valid and produced an answer.
	WHAT, ///< Request was invalid/user error.
	FAIL, ///< Error, pointing blame at environment.
	OOPS, ///< Error, pointing blame at programmer.
	NOPE, ///< Request was probably valid, but forbidden.
	// 'Push' responses (initiated by server)
	OHAI, ///< Server starting up.
	TTFN, ///< Server shutting down.
	STAT, ///< Server changing state.
	TIME, ///< Server sending current song time.
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
	/// A type for callbacks taking a Responder.
	using Callback = std::function<void(Responder &)>;

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

/**
 * Abstract helper class for sources of responses.
 *
 * A ResponseSource can both 'push' responses to a registered Responder and
 * be 'polled' from outside to dump its current response to an external
 * Responder.  For example, PlayerPosition 'pushes' its position every few
 * milliseconds to the outside world, to keep the client aware of the time,
 * but is also 'polled' on a new client connection so that the client
 * immediately gets the current position on connect.
 */
class ResponseSource {
public:
	/**
	 * Emits a response to a given Responder.
	 * @param responder The Responder to which this ResponseSource's
	 *   current response should be emitted.
	 */
	virtual const void Emit(Responder &responder) const = 0;

	/**
	 * Registers a Responder with this ResponseSource.
	 * The ResponseSource will periodically send a Response to the given
	 * Responder.
	 * @param responder The responder to register.
	 */
	void SetResponder(Responder &responder);
protected:
	/**
	 * Calls an Emit on the registered Responder.
	 * If there is no registered Responder, the response is dropped.
	 * @param code The code for this response.
	 * @param message The message for this response.
	 */
	const void EmitToRegisteredSink() const;

private:
	/// A Responder to which 'push' responses are emitted.
	/// If the Responder is not present, responses are not emitted.
	boost::optional<std::reference_wrapper<Responder>> push_sink;
};

#endif // PS_IO_RESPONSE_HPP
