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
 * This is an abstract class, implemented by AsioTcpIoReactor, AsioWinIoReactor,
 * and AsioPosixIoReactor.
 *
 * Similarly to StdIoReactor, the player is polled at a frequent rate to update
 * itself.  Unlike StdIoReactor, the polling and TCP communications occur
 * asynchronously.
 *
 * @see IoReactor
 * @see AsioTcpIoReactor
 * @see AsioWinIoReactor
 * @see AsioPosixIoReactor
 */
class AsioIoReactor : public IoReactor {
public:
	/**
	* Constructs an AsioIoReactor.
	* @param player The player to which periodic update requests shall be
	*   sent.
	* @param handler The handler to which command inputs shall be sent.
	*/
	explicit AsioIoReactor(Player &player, CommandHandler &handler);

	/// Deleted copy constructor.
	AsioIoReactor(const AsioIoReactor &) = delete;

	/// Deleted copy-assignment.
	AsioIoReactor &operator=(const AsioIoReactor &) = delete;

	void End() override;

protected:
	/// The IO service used by the reactor.
	boost::asio::io_service io_service;

private:
	void MainLoop() override;
	void DoUpdateTimer();
	void InitSignals();

	/// The signal set used to shut the server down on terminations.
	boost::asio::signal_set signals;
};

/**
 * An IoReactor using boost::asio and TCP/IP.
 *
 * The AsioTcpIoReactor uses asynchronous, socket-based IO.  It exposes a server on
 * a given TCP port and address, which takes commands as input and emits
 * responses as output.  An AsioTcpIoReactor can support multiple clients at once,
 * all of whom are broadcast each response and all of whom may send commands.
 *
 * @see IoReactor
 * @see AsioIoReactor
 * @see AsioWinReactor
 * @see AsioPosixReactor
 */
class AsioTcpIoReactor : public AsioIoReactor {
public:
	/**
	 * Constructs an AsioTcpIoReactor.
	 * @param player The player to which periodic update requests shall be
	 *   sent.
	 * @param handler The handler to which command inputs shall be sent.
	 * @param address The address to which AsioIoReactor will bind.
	 * @param port The port on which AsioIoReactor will listen for clients.
	 */
	explicit AsioTcpIoReactor(Player &player, CommandHandler &handler,
	                       const std::string &address,
	                       const std::string &port);

	/// Deleted copy constructor.
	AsioTcpIoReactor(const AsioTcpIoReactor &) = delete;

	/// Deleted copy-assignment.
	AsioTcpIoReactor &operator=(const AsioTcpIoReactor &) = delete;

	void End() override;

protected:
	void ResponseViaOstream(std::function<void(std::ostream &)> f) override;

private:
	void DoAccept();
	void InitAcceptor(const std::string &address, const std::string &port);

	/// The acceptor used to listen for incoming connections.
	boost::asio::ip::tcp::acceptor acceptor;

	/// The object responsible for managing live connections.
	TcpConnectionManager manager;
};

#ifdef BOOST_ASIO_HAS_WINDOWS_OBJECT_HANDLE

//
// Windows-specific code
//

#include <windows.h>
#include "../contrib/stdin_stream.hpp"

/**
 * An IoReactor using standard input/output.
 *
 * The AsioWinIoReactor allows Playslave to take input from, and send output to,
 * a Windows console or POSIX .
 *
 * @see IoReactor
 * @see AsioIoReactor
 * @see AsioTcpIoReactor
 * @see AsioPosixIoReactor
 */
class AsioWinIoReactor : public AsioIoReactor {
public:
	/**
	 * Constructs an AsioWinIoReactor.
	 * @param player The player to which periodic update requests shall be
	 *   sent.
	 * @param handler The handler to which command inputs shall be sent.
	 */
	explicit AsioWinIoReactor(Player &player, CommandHandler &handler);

	/// Deleted copy constructor.
	AsioWinIoReactor(const AsioWinIoReactor &) = delete;

	/// Deleted copy-assignment.
	AsioWinIoReactor &operator=(const AsioWinIoReactor &) = delete;
protected:
	void ResponseViaOstream(std::function<void(std::ostream &)> f) override;

private:
	void SetupWaitForInput();

	/// The handle pointing to the console input.
	HANDLE input_handle;

	/// The ASIO wrapper for the console input handle.
	stdin_stream input;

	/// The data buffer for the console input stream.
	boost::asio::streambuf data;
};

#endif // BOOST_ASIO_HAS_WINDOWS_OBJECT_HANDLE

//
// POSIX-specific code
//

#ifdef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR

/**
 * An IoReactor using POSIX streams.
 *
 * The AsioWinIoReactor allows Playslave to take input from, and send output to,
 * standard input/output on a POSIX-compliant OS.
 *
 * @see IoReactor
 * @see AsioIoReactor
 * @see AsioTcpIoReactor
 * @see AsioWinIoReactor
 */
class AsioPosixIoReactor : public AsioIoReactor {
public:
	/**
	 * Constructs an AsioPosixIoReactor.
	 * @param player The player to which periodic update requests shall be
	 *   sent.
	 * @param handler The handler to which command inputs shall be sent.
	 */
	explicit AsioPosixIoReactor(Player &player, CommandHandler &handler);

	/// Deleted copy constructor.
	AsioPosixIoReactor(const AsioPosixIoReactor &) = delete;

	/// Deleted copy-assignment.
	AsioPosixIoReactor &operator=(const AsioPosixIoReactor &) = delete;

protected:
	void ResponseViaOstream(std::function<void(std::ostream &)> f) override;

private:
	void SetupWaitForInput();

	/// The ASIO wrapper for POSIX stdin.
	boost::asio::posix::stream_descriptor input;

	/// The data buffer for the console input stream.
	boost::asio::streambuf data;
};

#endif // BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR

#endif // PS_IO_REACTOR_ASIO_HPP
