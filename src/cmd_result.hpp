// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the CommandResult class.
 */

#ifndef PLAYD_CMD_RESULT
#define PLAYD_CMD_RESULT

#include <cstdint>
#include <string>
#include <vector>
#include "response.hpp"

/**
 * A result from a command (an 'ACK' response).
 *
 * Commands may either succeed (with no failure message), or fail (with a
 * failure message).  The success (or not) of a command may be checked with
 * CommandResult::IsSuccess.
 */
class CommandResult
{
public:
	/**
	 * Enumeration of possible command result types.
	 * To be elaborated on pending specification.
	 * @note If you're adding new ack-codes here, update STRINGS.
	 * @see CommandResult::STRINGS
	 */
	enum class Code : std::uint8_t {
		OK,   ///< Request was valid and produced an answer.
		WHAT, ///< Request was invalid/user error.
		FAIL  ///< Error, pointing blame at environment.
	};

	/**
	 * Shortcut for constructing a successful CommandResult.
	 * @param tag The tag of the original command.
	 * @return A CommandResult denoting success.
	 */
	static CommandResult Success(const std::string &tag);

	/**
	 * Shortcut for constructing an invalid-response CommandResult.
	 * @param tag The tag of the original command.
	 * @param msg The failure message.  See CommandResult().
	 * @return A CommandResult denoting an invalid response.
	 */
	static CommandResult Invalid(const std::string &tag, const std::string &msg);

	/**
	 * Shortcut for constructing a failed CommandResult.
	 * @param tag The tag of the original command.
	 * @param msg The failure message.  See CommandResult().
	 * @return A CommandResult denoting failure.
	 */
	static CommandResult Failure(const std::string &tag, const std::string &msg);

	/**
	 * Constructs a CommandResult.
	 * @param tag The tag of the original command.
	 * @param type The type of command result.
	 * @param msg A message providing more information about the result.
	 *   The message will be copied into the CommandResult.
	 */
	CommandResult(const std::string &tag, CommandResult::Code type, const std::string &msg);

	/**
	 * Determines whether this CommandResult was successful.
	 * @return True if the result was a success; false otherwise.
	 */
	bool IsSuccess() const;

	/**
	 * Sends a response to a ResponseSink about this CommandResult.
	 *
	 * If the CommandResult was a success, then the response is 'OK cmd',
	 * where 'cmd' is the parameter named as such.  Otherwise, the response
	 * is 'X msg', where 'msg' is the result message and 'X' is whichever
	 * response code is most appropriate for the failure.
	 *
	 * @param id The connection ID in the sink to which the response will
	 *   be sent.  Defaults to 0 (broadcast).
	 * @param sink The ResponseSink to which the response will be sent.
	 */
	void Emit(size_t id, const ResponseSink &sink) const;

private:
	/**
	 * A map from CommandResult::Code codes to their string equivalents.
	 * @see CommandResult::Code
	 */
	static const std::string STRINGS[];

	std::string tag;          ///< The original command's tag.
	std::string msg;          ///< The command result's message.
	CommandResult::Code type; ///< The command result's ack code.
};

#endif // PLAYD_CMD_RESULT
