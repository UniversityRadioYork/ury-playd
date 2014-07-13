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

/* This is forced because we require C++11, which has std::chrono, anyway.
   Some environments fail Boost's std::chrono detection, and don't set this
   flag, which causes compilation failures. */
#ifndef BOOST_ASIO_HAS_STD_CHRONO
#define BOOST_ASIO_HAS_STD_CHRONO
#endif

#include <csignal>                              // SIG*
#include <string>                               // std::string
#include "../player/player.hpp"                 // Player
#include "../cmd.hpp"                           // CommandHandler
#include "../messages.h"                        // MSG_*
#include "io_reactor.hpp"                       // IoReactor
#include "io_responder.hpp"                     // Response
#include <boost/asio.hpp>                       // boost::asio::*
#include <boost/asio/high_resolution_timer.hpp> // boost::asio::high_resolution_timer

const std::chrono::nanoseconds IoReactor::PLAYER_UPDATE_PERIOD(1000);

IoReactor::IoReactor(Player &player, CommandHandler &handler)
    : player(player), handler(handler), io_service(), signals(io_service)
{
	InitSignals();
	DoUpdateTimer();
}

void IoReactor::Run()
{
	Respond(Response::OHAI, MSG_OHAI);
	io_service.run();
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
	auto tick = std::chrono::duration_cast<
	                std::chrono::high_resolution_clock::duration>(
	                PLAYER_UPDATE_PERIOD);
	boost::asio::high_resolution_timer t(this->io_service, tick);
	t.async_wait([this](boost::system::error_code) {
		this->player.Update();
		DoUpdateTimer();
	});
}

void IoReactor::End()
{
	this->io_service.stop();
}
