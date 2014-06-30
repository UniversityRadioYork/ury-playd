/* errors.c - error reporting */

/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#ifndef PS_ERRORS_HPP
#define PS_ERRORS_HPP

#include <string>
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

	void ToResponse();

	/**
	 * The human-readable message for this error.
	 * @return A reference to the string describing this Error.
	 */
	const std::string &Message();

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
	ConfigError(const std::string &message) : Error(message) {};
};

/**
 * An Error signifying that Playslave has hit an internal snag.
 */
class InternalError : public Error {
public:
	InternalError(const std::string &message) : Error(message) {};
};

/**
 * An Error signifying that Playslave can't read a file.
 */
class FileError : public Error {
public:
	FileError(const std::string &message) : Error(message) {};
};

//
// Debugging
//

inline void DebugArgs()
{
}

template <typename T1, typename... Ts>
inline void DebugArgs(T1 &t1, Ts &... ts)
{
	std::cerr << " " << t1;
	DebugArgs(ts...);
}

template <typename... Ts>
inline void Debug(Ts &... ts)
{
	std::cerr << "DEBUG:";
	DebugArgs(ts...);
	std::cerr << std::endl;
}

#endif // PS_ERRORS_HPP
