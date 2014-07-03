// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the StdIoReactor class.
 * @see io/io_reactor_std.hpp
 * @see io/io_reactor.hpp
 * @see io/io_reactor.cpp
 * @see io/io_reactor_asio.hpp
 * @see io/io_reactor_asio.cpp
 */

#include <string>               // std::string
#include <thread>               // std::thread
#include "../player/player.hpp" // Player
#include "../constants.h"       // LOOP_PERIOD
#include "../cmd.hpp"           // CommandHandle
#include "../messages.h"        // MSG_*
#include "io_reactor_std.hpp"   // StdIoReactor

#ifdef WIN32
// TODO: Replace conio with something that doesn't block the entire program
// while keyboard input is ready.
#include <conio.h> // _kbhit
#else
#include <unistd.h>
#include <sys/select.h> // select
#endif

StdIoReactor::StdIoReactor(Player &player, CommandHandler &handler)
    : IoReactor(player, handler), running(true)
{
}

void StdIoReactor::ResponseViaOstream(std::function<void(std::ostream &)> f)
{
	f(std::cout);
}

void StdIoReactor::MainLoop()
{
	while (this->running) {
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

		HandleCommand(line);
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

void StdIoReactor::End()
{
	running = false;
}
