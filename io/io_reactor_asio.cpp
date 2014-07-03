// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the AsioIoReactor class.
 * @see io/io_reactor_asio.hpp
 * @see io/io_reactor.hpp
 * @see io/io_reactor.cpp
 * @see io/io_reactor_std.hpp
 * @see io/io_reactor_std.cpp
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

#define BOOST_ASIO_HAS_STD_CHRONO
#include <boost/asio/high_resolution_timer.hpp>


#include "../constants.h" // LOOP_PERIOD
#include "../cmd.hpp"
#include "../errors.hpp"
#include "../messages.h"
#include "../player/player.hpp"
#include "io_reactor_asio.hpp"


//
// AsioIoReactor
//

AsioIoReactor::AsioIoReactor(Player &player, CommandHandler &handler,
                             std::string address, std::string port)
    : IoReactor(player, handler),
      io_service(),
      signals(io_service),
      acceptor(io_service),
      manager()
{
	this->signals.add(SIGINT);
	this->signals.add(SIGTERM);
#ifdef SIGQUIT
	this->signals.add(SIGQUIT);
#endif // SIGQUIT

	DoAwaitStop();

	boost::asio::ip::tcp::resolver resolver(io_service);
	boost::asio::ip::tcp::endpoint endpoint =
	                *resolver.resolve({address, port});
	this->acceptor.open(endpoint.protocol());
	this->acceptor.set_option(
	                boost::asio::ip::tcp::acceptor::reuse_address(true));
	this->acceptor.bind(endpoint);
	this->acceptor.listen();

	DoAccept();

	DoUpdateTimer();
}

void AsioIoReactor::MainLoop()
{
	io_service.run();
}

void AsioIoReactor::DoAccept()
{
        auto cmd = [this](const std::string &line) { HandleCommand(line); };
	TcpConnection *con = new TcpConnection(cmd, this->manager, this->io_service);
        TcpConnection::Pointer connection(con);
	auto on_accept = [this, connection](boost::system::error_code ec) {
		if (!ec) {
			this->manager.Start(connection);
		}
		DoAccept();
	};
	this->acceptor.async_accept(connection->Socket(), on_accept);
}

void AsioIoReactor::DoUpdateTimer()
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

void AsioIoReactor::DoAwaitStop()
{
	this->signals.async_wait([this](boost::system::error_code,
	                                int) { End(); });
}

void AsioIoReactor::ResponseViaOstream(std::function<void(std::ostream &)> f)
{
	std::ostringstream os;
	f(os);

	this->manager.Send(os.str());
}

void AsioIoReactor::End()
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

boost::asio::ip::tcp::socket &TcpConnection::Socket()
{
        return socket;
}

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
	this->strand.post([this, string]() {
		this->outbox.push_back(string);
		if (this->outbox.size() == 1) {
			DoWrite();
		}
	});
}

void TcpConnection::DoWrite()
{
	const std::string &string = this->outbox[0];
	auto write_cb = [this](const boost::system::error_code &, std::size_t) {
		this->outbox.pop_front();
		if (!this->outbox.empty()) {
			DoWrite();
		}
	};

	boost::asio::async_write(
	                this->socket,
	                boost::asio::buffer(string.c_str(), string.size()),
	                this->strand.wrap(write_cb));
}

void TcpConnection::ResponseViaOstream(std::function<void(std::ostream &)> f)
{
	std::ostringstream os;
	f(os);

	Send(os.str());
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
