/* errors.c - error reporting */

/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#ifndef CUPPA_ERRORS_H
#define CUPPA_ERRORS_H

#include <string>
#include <iostream>

/* Categories of error.
 */
enum class ErrorCode {
	OK,               // No error
	NO_FILE,          // Tried to read nonexistent file
	BAD_STATE,        // State transition not allowed
	BAD_COMMAND,      // Command was malformed
	COMMAND_REJECTED, // Command was valid but refused
	COMMAND_IGNORED,  // Command was silently ignored
	BAD_FILE,         // Tried to read corrupt file
	BAD_CONFIG,       // Program improperly configured
	AUDIO_INIT_FAIL,  // Couldn't open audio backend
	INTERNAL_ERROR,   // General system error, usually fatal
	NO_MEM,           // Allocation of memory failed
	END_OF_FILE,      // Reached end of file while reading
	INCOMPLETE,       // Incomplete computation, try again
	UNKNOWN,          // Unknown error
};

class Error {
public:
	Error(ErrorCode error_code, std::string message);

	void ToResponse();
	ErrorCode Code();
	const std::string &Message();

private:
	ErrorCode error_code;
	std::string message;
};

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

#endif /* !CUPPA_ERRORS_H */
