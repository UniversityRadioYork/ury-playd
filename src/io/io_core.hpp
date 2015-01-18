// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the IoCore class.
 * @see io/io_core.cpp
 */

#ifndef PLAYD_IO_CORE_HPP
#define PLAYD_IO_CORE_HPP

#include <ostream>
#include <set>

#include <uv.h>

#include "../cmd.hpp"
#include "../player/player.hpp"
#include "io_core.hpp"
#include "io_response.hpp"
#include "tokeniser.hpp"

class Player;
class Connection;

/**
 * A pool of TCP connections.
 *
 * A ConnectionPool contains, and holds ownership over, a set of
 * Connection objects.  Each Connection is created indirectly by asking
 * the pool to Accept() a connection from the libuv TCP server.
 *
 * The ConnectionPool can broadcast messages to each pooled Connection,
 * using the Broadcast() method, and Remove() a Connection at any time,
 * effectively destroying it.
 */
class ConnectionPool : public ResponseSink
{
public:
	/**
	 * Constructs a ConnectionPool.
	 * @param player The player that forms welcome responses for new
	 *   clients.
	 * @param handler The handler to which read commands should be sent.
	 */
	ConnectionPool(Player &player, CommandHandler &handler);

	/**
	 * Accepts a new connection.
	 *
	 * This accepts the connection, and adds it to this IoCore's
	 * connection pool.
	 *
	 * This should be called with a server that has just received a new
	 * connection.
	 *
	 * @param server Pointer to the libuv server accepting connections.
	 */
	void Accept(uv_stream_t *server);

	/**
	 * Removes a connection.
	 * As the ConnectionPool owns the Connection, it will be
	 * destroyed by this operation.
	 * @param conn The connection to remove.
	 */
	void Remove(Connection &conn);

	void Respond(const Response &response) const override;

private:
	Player &player;          ///< The player.
	CommandHandler &handler; ///< The command handler.

	/// The set of connections inside this ConnectionPool.
	std::vector<std::unique_ptr<Connection>> connections;
};

/**
 * A TCP connection from a client.
 *
 * This class wraps a libuv TCP stream representing a client connection,
 * allowing it to be sent responses (directly, or via a ConnectionPool
 * Broadcast()), removed from its ConnectionPool, and queried for its name.
 */
class Connection : public ResponseSink
{
public:
	/**
	 * Constructs a Connection.
	 * @param parent The connection pool to which this Connection belongs.
	 * @param tcp The underlying libuv TCP stream.
	 * @param handler The handler to which read commands should be sent.
	 */
	Connection(ConnectionPool &parent, uv_tcp_t *tcp,
	           CommandHandler &handler);

	/**
	 * Destructs a Connection.
	 * This causes libuv to close and free the libuv TCP stream.
	 */
	~Connection();

	/// Connection cannot be copied.
	Connection(const Connection &) = delete;

	/// Connection cannot be copy-assigned.
	Connection &operator=(const Connection &) = delete;

	void Respond(const Response &response) const override;

	/**
	 * Processes a data read on this connection.
	 * @param nread The number of bytes read.
	 * @param buf The buffer containing the read data.
	 */
	void Read(ssize_t nread, const uv_buf_t *buf);

	/**
	 * Removes this connection from its connection pool.
	 * Since the pool may contain a shared reference to this connection,
	 * calling this can result in the connection being destructed.
	 */
	void Depool();

	/**
	 * Retrieves a name for this connection.
	 * This will be of the form "HOST:PORT", unless errors occur.
	 * @return The Connection's name.
	 */
	std::string Name();

private:
	/// The pool on which this connection is running.
	ConnectionPool &parent;

	/// The libuv handle for the TCP connection.
	uv_tcp_t *tcp;

	/// The Tokeniser to which data read on this connection should be sent.
	Tokeniser tokeniser;

	/// The CommandHandler to which finished commands should be sent.
	CommandHandler &handler;

	/**
	 * Handles a tokenised command line.
	 * @param words A vector of command words representing a command line.
	 */
	void HandleCommand(const std::vector<std::string> &words);
};

/**
 * The IO core, which services input, routes responses, and executes the
 * Player update routine periodically.
 */
class IoCore : public ResponseSink
{
public:
	/**
	 * Constructs an IoCore.
	 * @param player The player to which periodic update requests shall be
	 * sent.
	 * @param handler The handler to which command inputs shall be sent.
	 */
	explicit IoCore(Player &player, CommandHandler &handler);

	/// Deleted copy constructor.
	IoCore(const IoCore &) = delete;

	/// Deleted copy-assignment.
	IoCore &operator=(const IoCore &) = delete;

	/**
	 * Runs the reactor.
	 * It will block until it terminates.
	 * @param host The IP host to which IoCore will bind.
	 * @param port The TCP port to which IoCore will bind.
	 * @exception NetError Thrown if IoCore cannot bind to @a host or @a
	 *   port.
	 */
	void Run(const std::string &host, const std::string &port);

	void Respond(const Response &response) const override;

private:
	/// The period between player updates.
	static const uint16_t PLAYER_UPDATE_PERIOD;

	uv_tcp_t server;     ///< The libuv handle for the TCP server.
	uv_timer_t updater;  ///< The libuv handle for the update timer.
	Player &player;      ///< The player.
	ConnectionPool pool; ///< The pool of client Connections.

	/**
	 * Initialises a TCP acceptor on the given address and port.
	 *
	 * @param address The IPv4 address on which the TCP server should
	 *   listen.
	 * @param port The TCP port on which the TCP server should listen.
	 */
	void InitAcceptor(const std::string &address, const std::string &port);

	/// Sets up a periodic timer to run the playd update loop.
	void DoUpdateTimer();
};

#endif // PLAYD_IO_CORE_HPP
