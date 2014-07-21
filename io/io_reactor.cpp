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

#include <csignal>              // SIG*
#include <string>               // std::string
#include "../player/player.hpp" // Player
#include "../cmd.hpp"           // CommandHandler
#include "../errors.hpp"
#include "../messages.h"                        // MSG_*
#include "io_reactor.hpp"                       // IoReactor
#include "io_responder.hpp"                     // Response
#include <boost/asio.hpp>                       // boost::asio::*
#include <boost/asio/high_resolution_timer.hpp> // boost::asio::high_resolution_timer

const std::chrono::nanoseconds IoReactor::PLAYER_UPDATE_PERIOD(1000);

IoReactor::IoReactor(Player &player, CommandHandler &handler,
                     const std::string &address, const std::string &port)
    : player(player),
      handler(handler),
      io_service(),
      acceptor(io_service),
      signals(io_service),
      manager(),
      reactor_running(true),
      new_connection()
{
	InitSignals();
	InitAcceptor(address, port);
	DoAccept();
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

void IoReactor::InitAcceptor(const std::string &address,
                             const std::string &port)
{
	boost::asio::ip::tcp::resolver resolver(io_service);
	boost::asio::ip::tcp::endpoint endpoint =
	                *resolver.resolve({address, port});
	this->acceptor.open(endpoint.protocol());
	this->acceptor.set_option(
	                boost::asio::ip::tcp::acceptor::reuse_address(true));
	this->acceptor.bind(endpoint);
	this->acceptor.listen();
}

void IoReactor::DoAccept()
{
	auto cmd = [this](const std::string &line) { HandleCommand(line); };
	this->new_connection.reset(new TcpConnection(cmd, this->manager,
	                                             this->io_service));
	auto on_accept = [this](boost::system::error_code ec) {
		if (!ec) {
			this->manager.Start(this->new_connection);
		}
		if (this->acceptor.is_open()) {
			DoAccept();
		}
	};
	this->acceptor.async_accept(this->new_connection->Socket(), on_accept);
}

void IoReactor::RespondRaw(const std::string &string)
{
	this->manager.Send(string);
}

void IoReactor::End()
{
	this->acceptor.close();
	this->manager.StopAll();
	this->io_service.stop();
}

//
// TcpConnection
//

TcpConnection::TcpConnection(std::function<void(const std::string &)> cmd,
                             TcpConnectionManager &manager,
                             boost::asio::io_service &io_service)
    : socket(io_service),
      strand(io_service),
      outbox(),
      cmd(cmd),
      manager(manager),
      closing(false)
{
}

void TcpConnection::Start()
{
	Respond(Response::OHAI, MSG_OHAI);
	DoRead();
}

void TcpConnection::Stop()
{
	this->closing = true;
	boost::system::error_code ignored_ec;
	this->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
	                      ignored_ec);
	this->socket.close();
}

boost::asio::ip::tcp::socket &TcpConnection::Socket()
{
	return socket;
}

void TcpConnection::DoRead()
{
	// This is needed to keep the TcpConnection alive while it performs the
	// read.  Otherwise, it'd be destructed as soon as it goes out of the
	// connection manager's list and cause illegal memory accesses.
	auto self(shared_from_this());

	boost::asio::async_read_until(socket, data, "\n",
	                              [this,
	                               self](const boost::system::error_code &
	                                                     ec,
	                                     std::size_t) {
		if (!ec) {
			std::istream is(&data);
			std::string s;
			std::getline(is, s);

			// If the line ended with CRLF, getline will miss the
			// CR, so we need to remove it from the line here.
			if (s.back() == '\r') {
				s.pop_back();
			}

			this->cmd(s);

			DoRead();
		} else if (ec != boost::asio::error::operation_aborted) {
			this->manager.Stop(shared_from_this());
		}
	});
}

void TcpConnection::Send(const std::string &string)
{
	if (this->closing) {
		return;
	}

	// This somewhat complex combination of a strand and queue ensure that
	// only one process actually writes to a TcpConnection at a given time.
	// Otherwise, writes could interrupt each other.
	this->strand.post([this, string]() {
		this->outbox.push_back(string);
		// If this string is the only one in the queue, then the last
		// chain
		// of DoWrite()s will have ended and we need to start a new one.
		if (this->outbox.size() == 1) {
			DoWrite();
		}
	});
}

void TcpConnection::DoWrite()
{
	// This is needed to keep the TcpConnection alive while it performs the
	// write.  Otherwise, it'd be destructed as soon as it goes out of the
	// connection manager's list and cause illegal memory accesses.
	auto self(shared_from_this());

	const std::string &string = this->outbox[0];
	// This is called after the write has finished.
	auto write_cb = [this, self](const boost::system::error_code &ec,
	                             std::size_t) {
		if (!ec) {
			this->outbox.pop_front();
			// Keep writing until and unless the outbox is emptied.
			// After that, the next Send will start DoWrite()ing
			// again.
			if (!this->outbox.empty()) {
				DoWrite();
			}
		} else if (ec != boost::asio::error::operation_aborted) {
			this->manager.Stop(shared_from_this());
		}
	};

	boost::asio::async_write(
	                this->socket,
	                boost::asio::buffer(string.c_str(), string.size()),
	                this->strand.wrap(write_cb));
}

void TcpConnection::RespondRaw(const std::string &string)
{
	Send(string);
}

//
// TcpConnectionManager
//

TcpConnectionManager::TcpConnectionManager()
{
}

void TcpConnectionManager::Start(TcpConnection::Pointer c)
{
	this->connections.insert(c);
	c->Start();
}

void TcpConnectionManager::Stop(TcpConnection::Pointer c)
{
	c->Stop();
	this->connections.erase(c);
}

void TcpConnectionManager::StopAll()
{
	for (auto c : this->connections) {
		c->Stop();
	}
	this->connections.clear();
}

void TcpConnectionManager::Send(const std::string &string)
{
	Debug("Send to all:", string);
	for (auto c : this->connections) {
		c->Send(string);
	}
}
