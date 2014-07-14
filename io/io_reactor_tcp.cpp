// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
* @file
* Implementation of the TcpIoReactor class.
* @see io/io_reactor_tcp.hpp
* @see io/io_reactor.hpp
* @see io/io_reactor.cpp
* @see io/io_reactor_posix.hpp
* @see io/io_reactor_posix.cpp
* @see io/io_reactor_win.hpp
* @see io/io_reactor_win.cpp
*/

#include <iostream>
#include <map>
#include <string>
#include <thread>

#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <ctime>

#include <boost/asio.hpp>

#include "../cmd.hpp"
#include "../errors.hpp"
#include "../messages.h"
#include "../player/player.hpp"
#include "io_reactor_tcp.hpp"

//
// TcpIoReactor
//

TcpIoReactor::TcpIoReactor(Player &player, CommandHandler &handler,
                           const std::string &address, const std::string &port)
    : IoReactor(player, handler), acceptor(io_service), manager()
{
	InitAcceptor(address, port);
	DoAccept();
}

void TcpIoReactor::InitAcceptor(const std::string &address,
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

void TcpIoReactor::DoAccept()
{
	auto cmd = [this](const std::string &line) { HandleCommand(line); };
	TcpConnection *con =
	                new TcpConnection(cmd, this->manager, this->io_service);
	TcpConnection::Pointer connection(con);
	auto on_accept = [this, connection](boost::system::error_code ec) {
		if (!ec) {
			this->manager.Start(connection);
		}
		DoAccept();
	};
	this->acceptor.async_accept(connection->Socket(), on_accept);
}

void TcpIoReactor::RespondRaw(const std::string &string)
{
	this->manager.Send(string);
}

void TcpIoReactor::End()
{
	this->acceptor.close();
	this->manager.StopAll();

	IoReactor::End();
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
      manager(manager)
{
}

void TcpConnection::Start()
{
	Respond(Response::OHAI, MSG_OHAI);
	DoRead();
}

void TcpConnection::Stop()
{
	this->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
	this->socket.close();
}

boost::asio::ip::tcp::socket &TcpConnection::Socket() { return socket; }

void TcpConnection::DoRead()
{
	boost::asio::async_read_until(socket, data, "\n",
	                              [this](const boost::system::error_code &
	                                                     ec,
	                                     std::size_t) {
		if (!ec) {
			std::istream is(&data);
			std::string s;
			std::getline(is, s);

			this->cmd(s);

			DoRead();
		} else if (ec != boost::asio::error::operation_aborted) {
			this->manager.Stop(shared_from_this());
		}
	});
}

void TcpConnection::Send(const std::string &string)
{
	// This somewhat complex combination of a strand and queue ensure that
	// only one process actually writes to a TcpConnection at a given time.
	// Otherwise, writes could interrupt each other.
	this->strand.post([this, string]() {
		this->outbox.push_back(string);
		// If this string is the only one in the queue, then the last chain
		// of DoWrite()s will have ended and we need to start a new one.
		if (this->outbox.size() == 1) {
			DoWrite();
		}
	});
}

void TcpConnection::DoWrite()
{
	const std::string &string = this->outbox[0];
	// This is called after the write has finished.
	auto write_cb = [this](const boost::system::error_code &, std::size_t) {
		this->outbox.pop_front();
		// Keep writing until and unless the outbox is emptied.
		// After that, the next Send will start DoWrite()ing again.
		if (!this->outbox.empty()) {
			DoWrite();
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

TcpConnectionManager::TcpConnectionManager() {}

void TcpConnectionManager::Start(TcpConnection::Pointer c)
{
	this->connections.insert(c);
	c->Start();
}

void TcpConnectionManager::Stop(TcpConnection::Pointer c)
{
	this->connections.erase(c);
	c->Stop();
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
