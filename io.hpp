/* io.h - input/output  */

/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
 */

#ifndef CUPPA_IO_H
#define CUPPA_IO_H

#include <map>
#include <iostream>
#include <string>
#include <stdarg.h>		/* vresponse */

/* Four-character response codes.
 *
 * NOTE: If you're adding new responses here, PLEASE update RESPONSES in io.c.
 */
enum class Response {
	/* 'Pull' responses (initiated by client command) */
	OKAY,			/* Request was valid and produced an answer */
	WHAT,			/* Request was invalid/user error */
	FAIL,			/* Error, pointing blame at environment */
	OOPS,			/* Error, pointing blame at programmer */
	NOPE,			/* Request was probably valid, but forbidden. */
	/* 'Push' responses (initiated by server) */
	OHAI,			/* Server starting up */
	TTFN,			/* Server shutting down */
	STAT,			/* Server changing state */
	TIME,			/* Server sending current song time */
	DBUG,			/* Debug information */
	/* Queue-specific responses */
	QENT,			/* Requested information about a Queue ENTry */
	QMOD,			/* A command caused a Queue MODification */
	QPOS,			/* The current Queue POSition has changed */
	QNUM			/* Reminder of current number of queue items */
};

extern const std::map<Response, std::string> RESPONSES;

inline void RespondArgs()
{
}

template<typename Arg1, typename... Args>
inline void RespondArgs(const Arg1 &arg1, const Args&... args)
{
	std::cout << " " << arg1;
	RespondArgs(args...);
}

inline void Respond(Response code)
{
	std::cout << RESPONSES.at(code) << std::endl;
}

template<typename... Args>
inline void Respond(Response code, Args&... args)
{
	std::cout << RESPONSES.at(code);
	RespondArgs(args...);
	std::cout << std::endl;
}

int		input_waiting(void);

#endif				/* !CUPPA_IO_H */
