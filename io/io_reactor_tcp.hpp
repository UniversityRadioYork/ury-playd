// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the TcpIoReactor class.
 * @see io/io_reactor_tcp.cpp
 * @see io/io_reactor.hpp
 * @see io/io_reactor.cpp
 * @see io/io_reactor_posix.hpp
 * @see io/io_reactor_posix.cpp
 * @see io/io_reactor_win.hpp
 * @see io/io_reactor_win.cpp
 */

#ifndef PS_IO_REACTOR_TCP_HPP
#define PS_IO_REACTOR_TCP_HPP

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
	void RespondRaw(const std::string &string) override;

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
* An IoReactor using boost::asio and TCP/IP.
*
* The TcpIoReactor uses asynchronous, socket-based IO.  It exposes a server on
* a given TCP port and address, which takes commands as input and emits
* responses as output.  An TcpIoReactor can support multiple clients at once,
* all of whom are broadcast each response and all of whom may send commands.
*
* @see IoReactor
* @see IoReactor
* @see WinReactor
* @see PosixReactor
*/
class TcpIoReactor : public IoReactor {
public:
	/**
	* Constructs an TcpIoReactor.
	* @param player The player to which periodic update requests shall be
	*   sent.
	* @param handler The handler to which command inputs shall be sent.
	* @param address The address to which IoReactor will bind.
	* @param port The port on which IoReactor will listen for clients.
	*/
	explicit TcpIoReactor(Player &player, CommandHandler &handler,
	                      const std::string &address,
	                      const std::string &port);

	/// Deleted copy constructor.
	TcpIoReactor(const TcpIoReactor &) = delete;

	/// Deleted copy-assignment.
	TcpIoReactor &operator=(const TcpIoReactor &) = delete;

	void End() override;

protected:
	void RespondRaw(const std::string &string) override;

private:
	void DoAccept();
	void InitAcceptor(const std::string &address, const std::string &port);

	/// The acceptor used to listen for incoming connections.
	boost::asio::ip::tcp::acceptor acceptor;

	/// The object responsible for managing live connections.
	TcpConnectionManager manager;
};

#endif // PS_IO_REACTOR_TCP_HPP
