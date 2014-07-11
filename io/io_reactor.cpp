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

#include <csignal>                              // SIG*
#include <string>                               // std::string
#include "../player/player.hpp"                 // Player
#include "../cmd.hpp"                           // CommandHandler
#include "../messages.h"                        // MSG_*
#include "io_reactor.hpp"                       // IoReactor
#include "io_responder.hpp"                     // Response
#include <boost/asio.hpp>                       // boost::asio::*
#include <boost/asio/high_resolution_timer.hpp> // boost::asio::high_resolution_timer

IoReactor::IoReactor(Player &player, CommandHandler &handler)
    : player(player),
	  handler(handler),
      io_service(),
	  signals(io_service)
{
	InitSignals();
	DoUpdateTimer();
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

void IoReactor::MainLoop()
{
	io_service.run();
}

void IoReactor::InitSignals()
{
	this->signals.add(SIGINT);
	this->signals.add(SIGTERM);
#ifdef SIGQUIT
	this->signals.add(SIGQUIT);
#endif // SIGQUIT

	this->signals.async_wait([this](boost::system::error_code,
		int) { End(); });
}

void IoReactor::DoUpdateTimer()
{
	boost::asio::high_resolution_timer t(
		this->io_service,
		std::chrono::duration_cast<
		std::chrono::high_resolution_clock::
		duration>(LOOP_PERIOD));
	t.async_wait([this](boost::system::error_code) {
		this->player.Update();
		DoUpdateTimer();
	});
}

void IoReactor::End()
{
	this->io_service.stop();
}

