// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the non-virtual aspects of the IoReactor class.
 * @see io/io_reactor.hpp
 * @see io/io_reactor_asio.hpp
 * @see io/io_reactor_asio.cpp
 * @see io/io_reactor_std.hpp
 * @see io/io_reactor_std.cpp
 */

#include <string>               // std::string
#include "../player/player.hpp" // Player
#include "../cmd.hpp"           // CommandHandle
#include "../messages.h"        // MSG_*
#include "io_reactor.hpp"       // IoReactor
#include "io_responder.hpp"     // Response

IoReactor::IoReactor(Player &player, CommandHandler &handler)
    : player(player), handler(handler)
{
}

void IoReactor::Run()
{
	Respond(Response::OHAI, MSG_OHAI);
	MainLoop();
	Respond(Response::TTFN, MSG_TTFN);
}

void IoReactor::HandleCommand(const std::string &line)
{
	bool valid = this->handler.Handle(line);
	if (valid) {
		Respond(Response::OKAY, line);
	} else {
		Respond(Response::WHAT, MSG_CMD_INVALID);
	}
}
