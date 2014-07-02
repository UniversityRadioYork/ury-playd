// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of input/output related code.
 * @see io.hpp
 */

#include <iostream>
#include <map>
#include <string>

#include <cstdarg>
#include <cstdio>
#include <ctime>

#ifdef WIN32
#include <conio.h>
#else
#include <unistd.h>
#include <sys/select.h> /* select */
#endif

#include "io.hpp"

/* Data for the responses. */
const std::map<Response, std::string> RESPONSES = {{Response::OKAY, "OKAY"},
                                                   {Response::WHAT, "WHAT"},
                                                   {Response::FAIL, "FAIL"},
                                                   {Response::OOPS, "OOPS"},
                                                   {Response::NOPE, "NOPE"},
                                                   {Response::OHAI, "OHAI"},
                                                   {Response::TTFN, "TTFN"},
                                                   {Response::STAT, "STAT"},
                                                   {Response::TIME, "TIME"},
                                                   {Response::DBUG, "DBUG"},
                                                   {Response::QPOS, "QPOS"},
                                                   {Response::QENT, "QENT"},
                                                   {Response::QMOD, "QMOD"},
                                                   {Response::QNUM, "QNUM"}};

/* Returns true if input is waiting on standard in. */
int input_waiting(void)
{
#ifdef WIN32
	return _kbhit();
#else
	fd_set rfds;
	struct timeval tv;

	/* Watch stdin (fd 0) to see when it has input. */
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	/* Stop checking immediately. */
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	return select(1, &rfds, NULL, NULL, &tv);
#endif
}
