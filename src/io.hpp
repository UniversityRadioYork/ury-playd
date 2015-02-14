// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the I/O classes used in playd.
 * @see io.cpp
 */

#ifndef PLAYD_IO_CORE_HPP
#define PLAYD_IO_CORE_HPP

#include <ostream>
#include <set>

#include <uv.h>

#include "player.hpp"
#include "response.hpp"
#include "tokeniser.hpp"

class Player;
class Connection;

/**
 * The IO core, which services input, routes responses, and executes the
 * Player update routine periodically.
 *
 * The IO core also maintains a pool of connections which can be sent responses
 * via their IDs inside the pool.  It ensures that each connection is given an
 * ID that is unique up until the removal of said connection.
 */
class IoCore : public ResponseSink
{
public:
	/**
	 * Constructs an IoCore.
	 * @param player The player to which update requests, commands, and new
	 *   connection state dump requests shall be sent.
	 */
	explicit IoCore(Player &player);

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

	//
	// Connection API
	//

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
	 * As the IoCore owns the Connection, it will be destroyed by this
	 * operation.
	 * @param id The ID of the connection to remove.
	 */
	void Remove(size_t id);

	void Respond(const Response &response, size_t id = 0) const override;

private:
	/// The period between player updates.
	static const uint16_t PLAYER_UPDATE_PERIOD;

	uv_tcp_t server;    ///< The libuv handle for the TCP server.
	uv_timer_t updater; ///< The libuv handle for the update timer.
	Player &player;     ///< The player.

	/// The set of connections inside this IoCore.
	std::vector<std::shared_ptr<Connection>> pool;

	/// A list of free 1-indexed slots inside pool.
	/// These slots may be re-used instead of creating a new slot.
	std::vector<size_t> free_list;

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

/**
 * A TCP connection from a client.
 *
 * This class wraps a libuv TCP stream representing a client connection,
 * allowing it to be sent responses (directly, or via a broadcast), removed
 * from its IoCore, and queried for its name.
 */
class Connection
{
public:
	/**
	 * Constructs a Connection.
	 * @param parent The connection pool to which this Connection belongs.
	 * @param tcp The underlying libuv TCP stream.
	 * @param player The player to which read commands should be sent.
	 * @param id The ID of this Connection in the IoCore.
	 */
	Connection(IoCore &parent, uv_tcp_t *tcp, Player &player, size_t id);

	/**
	 * Destructs a Connection.
	 * This causes libuv to close and free the libuv TCP stream.
	 */
	~Connection();

	/// Connection cannot be copied.
	Connection(const Connection &) = delete;

	/// Connection cannot be copy-assigned.
	Connection &operator=(const Connection &) = delete;

	/**
	 * Emits a Response via this Connection.
	 * @param response The response to send.
	 */
	void Respond(const Response &response) const;

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
	IoCore &parent;

	/// The libuv handle for the TCP connection.
	uv_tcp_t *tcp;

	/// The Tokeniser to which data read on this connection should be sent.
	Tokeniser tokeniser;

	/// The Player to which finished commands should be sent.
	Player &player;

	/// The Connection's ID in the connection pool.
	size_t id;

	/**
	 * Handles a tokenised command line.
	 * @param msg A vector of command words representing a command line.
	 */
	void RunCommand(const std::vector<std::string> &msg);
};

#endif // PLAYD_IO_CORE_HPP
