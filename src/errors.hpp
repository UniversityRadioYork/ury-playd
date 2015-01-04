// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declarations of the playd Error exception set.
 * @see errors.cpp
 */

#ifndef PLAYD_ERRORS_HPP
#define PLAYD_ERRORS_HPP

#include <iostream>
#include <sstream>

/**
 * A playd exception.
 */
class Error
{
public:
	/**
	 * Constructs an Error.
	 * @param msg The human-readable message of the error.
	 */
	Error(const std::string &msg);

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
 * An Error signifying that playd has been improperly configured.
 */
class ConfigError : public Error
{
public:
	/**
	 * Constructs an ConfigError.
	 * @param msg The human-readable message of the error.
	 */
	ConfigError(const std::string &msg) : Error(msg)
	{
	}
};

/**
 * An Error signifying that playd has hit an internal snag.
 */
class InternalError : public Error
{
public:
	/**
	 * Constructs an InternalError.
	 * @param msg The human-readable message of the error.
	 */
	InternalError(const std::string &msg) : Error(msg)
	{
	}
};

/**
 * An Error signifying that playd can't read a file.
 */
class FileError : public Error
{
public:
	/**
	 * Constructs a FileError.
	 * @param msg The human-readable message of the error.
	 */
	FileError(const std::string &msg) : Error(msg)
	{
	}
};

/**
 * An Error signifying that playd can't seek in a file.
 */
class SeekError : public Error
{
public:
	/**
	 * Constructs a SeekError.
	 * @param msg The human-readable message of the error.
	 */
	SeekError(const std::string &msg) : Error(msg)
	{
	}
};

/**
 * A network error.
 */
class NetError : public Error
{
public:
	/**
	 * Constructs a NetError.
	 * @param msg The human-readable message of the error.
	 */
	NetError(const std::string &msg) : Error(msg)
	{
	}
};

/**
 * An Error signifying that no audio is loaded.
 */
class NoAudioError : public Error
{
public:
	/**
	 * Constructs an NoAudioError.
	 * @param msg The human-readable message of the error.
	 */
	NoAudioError(const std::string &msg) : Error(msg)
	{
	}
};

/** Class for telling the human what playd is doing. */
class Debug
{
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

#endif // PLAYD_ERRORS_HPP
