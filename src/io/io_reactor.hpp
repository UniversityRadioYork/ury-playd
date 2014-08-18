// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the IoReactor class.
 * @see io/io_reactor.cpp
 */

#ifndef PS_IO_REACTOR_HPP
#define PS_IO_REACTOR_HPP

#include <chrono>
#include <boost/asio.hpp>
#include "io_response.hpp"
#include <deque>          // std::deque
#include <functional>     // std::function
#include <ostream>        // std::ostream
#include <set>            // std::set
#include "io_reactor.hpp" // IoReactor
#include "../player/player.hpp"

class Player;
class CommandHandler;

extern "C" {
#include <uv.h>
}

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>

class CommandHandler;

class TcpConnectionManager;

/**
 * A connection using the TCP server.
 */
class TcpConnection : public std::enable_shared_from_this<TcpConnection>,
                      public ResponseSink {
public:
	/// A shared pointer to a TcpConnection.
	using Pointer = std::shared_ptr<TcpConnection>;

	/**
	 * Creates a new TcpConnection.
	 * @param cmd A function that sends a command line to be handled.
	 * @param manager The manager that is handling this connection.
	 * @param io_service The IO service to be used for this connection.
	 * @param player A const reference to the player.
	 * @return A TcpConnection.
	 */
	explicit TcpConnection(std::function<void(const std::string &)> cmd,
	                       TcpConnectionManager &manager,
	                       boost::asio::io_service &io_service,
	                       const Player &player);

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
	const Player &player;
	ResponseSink::Callback new_client_callback;

	bool closing;
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
 * The IO reactor, which services input, routes responses, and executes the
 * Player update routine periodically.
 */
class IoReactor : public ResponseSink {
public:
	/**
	 * Constructs an IoReactor.
	 * @param player The player to which periodic update requests shall be
	 * sent.
	 * @param handler The handler to which command inputs shall be sent.
	 * @param address The address to which IoReactor will bind.
	 * @param port The port on which IoReactor will listen for clients.
	 */
	explicit IoReactor(Player &player, CommandHandler &handler,
	                   const std::string &address, const std::string &port);

	/// Deleted copy constructor.
	IoReactor(const IoReactor &) = delete;

	/// Deleted copy-assignment.
	IoReactor &operator=(const IoReactor &) = delete;

	/**
	 * Runs the reactor.
	 * It will block until terminated.
	 */
	void Run();

	/**
	 * Ends the reactor.
	 * This should be called by the parent object when the player is
	 * quitting.
	 */
	void End();

private:
	void RespondRaw(const std::string &string) override;
	void DoAccept();
	void InitAcceptor(const std::string &address, const std::string &port);
	void DoUpdateTimer();
	void InitSignals();

	/// The period between player updates.
	static const std::chrono::nanoseconds PLAYER_UPDATE_PERIOD;

	/**
	 * Sends a command to the command handler.
	 * The result of the command is responded on as per the playslave API.
	 * @param line The command line received by the IO reactor.
	 */
	void HandleCommand(const std::string &line);

	uv_loop_t *loop;

	Player &player;                     ///< The player.
	CommandHandler &handler;            ///< The command handler.
	boost::asio::io_service io_service; ///< The ASIO IO service.

	/// The acceptor used to listen for incoming connections.
	boost::asio::ip::tcp::acceptor acceptor;

	/// The signal set used to shut the server down on terminations.
	boost::asio::signal_set signals;

	/// The object responsible for managing live connections.
	TcpConnectionManager manager;

	TcpConnection::Pointer new_connection;

	ResponseSink::Callback new_client_callback;
};

#endif // PS_IO_REACTOR_HPP
