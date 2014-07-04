// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the AsioIoReactor class.
 * @see io/io_reactor_asio.cpp
 * @see io/io_reactor.hpp
 * @see io/io_reactor.cpp
 * @see io/io_reactor_std.hpp
 * @see io/io_reactor_std.cpp
 */

#ifndef PS_IO_REACTOR_ASIO_HPP
#define PS_IO_REACTOR_ASIO_HPP

#include <deque>          // std::deque
#include <functional>     // std::function
#include <ostream>        // std::ostream
#include <set>            // std::set
#include "io_reactor.hpp" // IoReactor

class Player;
class CommandHandler;

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>

class TcpConnectionManager;

/**
 * A connection using the TCP server.
 */
class TcpConnection : public std::enable_shared_from_this<TcpConnection>,
                      public Responder {
public:
	/// A shared pointer to a TcpConnection.
	using Pointer = std::shared_ptr<TcpConnection>;

	/**
	 * Creates a new TcpConnection.
	 * @param cmd A function that sends a command line to be handled.
	 * @param manager The manager that is handling this connection.
	 * @param io_service The IO service to be used for this connection.
	 * @return A TcpConnection.
	 */
	explicit TcpConnection(std::function<void(const std::string &)> cmd,
	                       TcpConnectionManager &manager,
	                       boost::asio::io_service &io_service);

	/// Deleted copy constructor.
	TcpConnection(const TcpConnection &) = delete;

	/// Deleted copy-assignment.
	TcpConnection &operator=(const TcpConnection &) = delete;

	/**
	 * Starts this TcpConnection.
	 */
	void Start();

	/**
	 * Stops this TcpConnection.
	 */
	void Stop();

	/**
	 * Sends a message to this connection.
	 * @param string The message to send.
	 */
	void Send(const std::string &string);

	/**
	 * Gets the socket to which this connection is attached.
	 * @return A reference to the TCP socket.
	 */
	boost::asio::ip::tcp::socket &Socket();

protected:
	void ResponseViaOstream(std::function<void(std::ostream &)> f) override;

private:
	void DoRead();
	void DoWrite();

	boost::asio::ip::tcp::socket socket;
	boost::asio::streambuf data;
	boost::asio::io_service::strand strand;
	std::deque<std::string> outbox;
	std::function<void(const std::string &)> cmd;
	TcpConnectionManager &manager;
};

/**
 * A manager for TcpConnection objects.
 */
class TcpConnectionManager {
public:
	/// Deleted copy constructor.
	TcpConnectionManager(const TcpConnectionManager &) = delete;

	/// Deleted copy-assignment.
	TcpConnectionManager &operator=(const TcpConnectionManager &) = delete;

	explicit TcpConnectionManager();

	/**
	 * Starts a TcpConnection, registering it with this manager.
	 * @param c A shared pointer to the TcpConnection.
	 */
	void Start(TcpConnection::Pointer c);

	/**
	 * Stops a TcpConnection, unregistering it with this manager.
	 * @param c A shared pointer to the TcpConnection.
	 */
	void Stop(TcpConnection::Pointer c);

	/**
	 * Stops and unregisters all connections.
	 */
	void StopAll();

	/**
	 * Sends a message to all connections.
	 * @param string The message to send.
	 */
	void Send(const std::string &string);

private:
	std::set<TcpConnection::Pointer> connections;
};

/**
 * An IoReactor using boost::asio.
 *
 * The AsioIoReactor uses asynchronous, socket-based IO.  It exposes a server on
 * a given TCP port and address, which takes commands as input and emits
 * responses as output.  An AsioIoReactor can support multiple clients at once,
 * all of whom are broadcast each response and all of whom may send commands.
 *
 * Similarly to StdIoReactor, the player is polled at a frequent rate to update
 * itself.  Unlike StdIoReactor, the polling and TCP communications occur
 * asynchronously.
 *
 * AsioIoReactor is several orders of magnitude more complex than StdIoReactor,
 * and is less well-tested.  It also requires more resources (a port and address
 * to which it will bind).  However, it is more flexible and portable.
 *
 * @see IoReactor
 * @see StdIoReactor
 */
class AsioIoReactor : public IoReactor {
public:
	/**
	 * Constructs an AsioIoReactor.
	 * @param player The player to which periodic update requests shall be
	 *   sent.
	 * @param handler The handler to which command inputs shall be sent.
	 * @param address The address to which AsioIoReactor will bind.
	 * @param port The port on which AsioIoReactor will listen for clients.
	 */
	explicit AsioIoReactor(Player &player, CommandHandler &handler,
	                       const std::string &address,
	                       const std::string &port);

	/// Deleted copy constructor.
	AsioIoReactor(const AsioIoReactor &) = delete;

	/// Deleted copy-assignment.
	AsioIoReactor &operator=(const AsioIoReactor &) = delete;

	void End() override;

protected:
	void ResponseViaOstream(std::function<void(std::ostream &)> f) override;

private:
	void MainLoop() override;

	void DoAccept();

	void DoUpdateTimer();

	void InitAcceptor(const std::string &address, const std::string &port);
	void InitSignals();

	/// The IO service used by the reactor.
	boost::asio::io_service io_service;

	/// The signal set used to shut the server down on terminations.
	boost::asio::signal_set signals;

	/// The acceptor used to listen for incoming connections.
	boost::asio::ip::tcp::acceptor acceptor;

	/// The object responsible for managing live connections.
	TcpConnectionManager manager;
};

#endif // PS_IO_REACTOR_ASIO_HPP
