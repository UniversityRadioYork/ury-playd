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
#include <sstream>

/**
 * A Playslave exception.
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

/** Class for telling the human what playslave is doing. */
class Debug {
public:
	/** Constructor. */
	inline Debug()
	{
		oss << "DEBUG:";
	}

	/** Destructor. Actually shoves things to the screen. */
	inline ~Debug()
	{
		std::cerr << oss.str();
	}

	/**
	 * Stream operator for shoving objects onto a screen somewhere.
	 * @tparam T Type of parameter.
	 * @param x Object to write to the stream.
	 * @return Chainable reference.
	 */
	template <typename T>
	inline Debug &operator<<(const T &x)
	{
		oss << " ";
		oss << x;
		return *this;
	}

	/**
	 * Specialisation for std::endl, which is actually a function pointer.
	 * @param pf Function pointer.
	 * @return Chainable reference.
	 */
	inline Debug &operator<<(std::ostream &(*pf)(std::ostream &))
	{
		oss << pf;
		return *this;
	}

private:
	std::ostringstream oss; ///< Stream buffer (avoids theoretical threading
	                        /// issues).
};

#endif // PS_ERRORS_HPP
