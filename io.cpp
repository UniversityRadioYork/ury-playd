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
#include <thread>

#include <cstdarg>
#include <cstdio>
#include <ctime>

#ifdef WIN32
#include <conio.h>
#else
#include <unistd.h>
#include <sys/select.h> /* select */
#endif

#include "constants.h" // LOOP_PERIOD
#include "cmd.hpp"
#include "errors.hpp"
#include "io.hpp"
#include "messages.h"
#include "player/player.hpp"

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

IoReactor::IoReactor(Player &player, CommandHandler &handler)
    : player(player), handler(handler) {};

void IoReactor::Run()
{
	Respond(Response::OHAI, MSG_OHAI);
	MainLoop();
	Respond(Response::TTFN, MSG_TTFN);
}

void IoReactor::RespondWithError(const Error &error)
{
	Respond(Response::FAIL, error.Message());
}

//
// StdIoReactor
//

void StdIoReactor::ResponseViaOstream(std::function<void(std::ostream &)> f)
{
    f(std::cout);
}

void StdIoReactor::MainLoop()
{
	while (this->player.IsRunning()) {
		/* Possible Improvement: separate command checking and player
		 * updating into two threads.  Player updating is quite
		 * intensive and thus impairs the command checking latency.
		 * Do this if it doesn't make the code too complex.
		 */
		CheckInput();
		this->player.Update();

		std::this_thread::sleep_for(LOOP_PERIOD);
	}
}

void StdIoReactor::CheckInput()
{
	if (InputWaiting()) {
		std::string line;
		std::getline(std::cin, line);

		bool valid = this->handler.Handle(line);
		if (valid) {
			Respond(Response::OKAY, line);
		} else {
			Respond(Response::WHAT, MSG_CMD_INVALID);
		}
	}
}

bool StdIoReactor::InputWaiting()
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
