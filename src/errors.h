// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declarations of the playd Error exception set.
 * @see errors.cpp
 */

#ifndef PLAYD_ERRORS_H
#define PLAYD_ERRORS_H

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
class Config_error : public Error
{
public:
	/**
	 * Constructs an ConfigError.
	 * @param msg The human-readable message of the error.
	 */
	Config_error(const std::string &msg) : Error(msg)
	{
	}
};

/**
 * An Error signifying that playd has hit an internal snag.
 */
class Internal_error : public Error
{
public:
	/**
	 * Constructs an InternalError.
	 * @param msg The human-readable message of the error.
	 */
	Internal_error(const std::string &msg) : Error(msg)
	{
	}
};

/**
 * An Error signifying that playd can't read a file.
 */
class File_error : public Error
{
public:
	/**
	 * Constructs a FileError.
	 * @param msg The human-readable message of the error.
	 */
	File_error(const std::string &msg) : Error(msg)
	{
	}
};

/**
 * An Error signifying that playd can't seek in a file.
 */
class Seek_error : public Error
{
public:
	/**
	 * Constructs a SeekError.
	 * @param msg The human-readable message of the error.
	 */
	Seek_error(const std::string &msg) : Error(msg)
	{
	}
};

/**
 * A network error.
 */
class Net_error : public Error
{
public:
	/**
	 * Constructs a NetError.
	 * @param msg The human-readable message of the error.
	 */
	Net_error(const std::string &msg) : Error(msg)
	{
	}
};

/**
 * An Error signifying that no audio is loaded.
 */
class Null_audio_error : public Error
{
public:
	/**
	 * Constructs an NoAudioError.
	 * @param msg The human-readable message of the error.
	 */
	Null_audio_error(const std::string &msg) : Error(msg)
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
