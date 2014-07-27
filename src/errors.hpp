// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declarations of the Playslave Error exception set.
 * @see errors.cpp
 */

#ifndef PS_ERRORS_HPP
#define PS_ERRORS_HPP

#include <iostream>

/**
 * A Playslave exception.
 * @todo Replace the enum with subclasses.
 */
class Error {
public:
	/**
	 * Constructs an Error.
	 * @param message The human-readable message of the error.
	 */
	Error(const std::string &message);

	/**
	 * The human-readable message for this error.
	 * @return A reference to the string describing this Error.
	 */
	const std::string &Message() const;

private:
	std::string message; ///< The human-readable message for this Error.
};

//
// Error sub-categories
//

/**
 * An Error signifying that Playslave has been improperly configured.
 */
class ConfigError : public Error {
public:
	/**
	 * Constructs an ConfigError.
	 * @param message The human-readable message of the error.
	 */
	ConfigError(const std::string &message) : Error(message) {};
};

/**
 * An Error signifying that Playslave has hit an internal snag.
 */
class InternalError : public Error {
public:
	/**
	 * Constructs an InternalError.
	 * @param message The human-readable message of the error.
	 */
	InternalError(const std::string &message) : Error(message) {};
};

/**
 * An Error signifying that Playslave can't read a file.
 */
class FileError : public Error {
public:
	/**
	 * Constructs a FileError.
	 * @param message The human-readable message of the error.
	 */
	FileError(const std::string &message) : Error(message) {};
};

//
// Debugging
//

/**
 * Base case for DebugArgs, when there are no arguments.
 */
inline void DebugArgs()
{
}

/**
 * Outputs a debug message, with a variadic number of arguments (at least one).
 * This is defined inductively, with DebugArgs() being the base case.
 * @tparam Arg1 The type of the leftmost argument.
 * @tparam Args Parameter pack of remaining arguments.
 * @param arg1 The leftmost argument.
 * @param args The remaining arguments.
 */
template <typename Arg1, typename... Args>
inline void DebugArgs(Arg1 &arg1, Args &... args)
{
	std::cerr << " " << arg1;
	DebugArgs(args...);
}

/**
 * Outputs a debug message, with a variadic number of arguments.
 * @tparam Args Parameter pack of arguments.
 * @param args The arguments.
 * @see DebugArgs
 */
template <typename... Args>
inline void Debug(Args &... args)
{
	std::cerr << "DEBUG:";
	DebugArgs(args...);
	std::cerr << std::endl;
}

#endif // PS_ERRORS_HPP
