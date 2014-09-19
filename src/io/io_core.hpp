// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the IoCore class.
 * @see io/io_core.cpp
 */

#ifndef PS_IO_CORE_HPP
#define PS_IO_CORE_HPP

#include <ostream>
#include <set>

extern "C" {
#include <uv.h>
}

#include "../cmd.hpp"
#include "../player/player.hpp"
#include "io_core.hpp"
#include "io_response.hpp"
#include "tokeniser.hpp"

class Player;
class Connection;

/**
 * The IO core, which services input, routes responses, and executes the
 * Player update routine periodically.
 */
class IoCore : public ResponseSink {
public:
	/**
	 * Constructs an IoCore.
	 * @param player The player to which periodic update requests shall be
	 * sent.
	 * @param handler The handler to which command inputs shall be sent.
	 * @param address The address to which IoCore will bind.
	 * @param port The port on which IoCore will listen for clients.
	 */
	explicit IoCore(Player &player, CommandHandler &handler,
	                const std::string &address, const std::string &port);

	/// Deleted copy constructor.
	IoCore(const IoCore &) = delete;

	/// Deleted copy-assignment.
	IoCore &operator=(const IoCore &) = delete;

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
	 *
	 * @todo This isn't a great fit for the public interface of IoCore -
	 *   separate into a ConnectionPool class?
	 */
	void NewConnection(uv_stream_t *server);

	/**
	 * Removes a connection.
	 *
	 * @param sink The connection to remove.
	 *
	 * @todo Rename sink?
	 * @todo This isn't a great fit for the public interface of IoCore -
	 *   separate into a ConnectionPool class?
	 */
	void RemoveConnection(Connection &sink);

private:
	/// The set of connections currently serviced by the IoCore.
	std::set<std::shared_ptr<Connection>> connections;

	/// The period between player updates.
	static const uint16_t PLAYER_UPDATE_PERIOD;

	uv_tcp_t server;    ///< The libuv handle for the TCP server.
	uv_timer_t updater; ///< The libuv handle for the update timer.

	Player &player;          ///< The player.
	CommandHandler &handler; ///< The command handler.

	void RespondRaw(const std::string &string) const;

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

/// A TCP connection from a client.
class Connection : public ResponseSink {
public:
	/**
	 * Constructs a Connection.
	 * @param parent The IoCore that is the parent of this connection.
	 * @param tcp The underlying libuv TCP stream.
	 * @param handler The handler to which read commands should be sent.
	 */
	Connection(IoCore &parent, uv_tcp_t *tcp, CommandHandler &handler);

	// Note: This is made public so that the IoCore can send raw data
	// to the connection.
	void RespondRaw(const std::string &response) const;

	/**
	 * Processes a data read on this connection.
	 *
	 * @param stream The libuv TCP/IP stream providing the data.
	 * @param nread The number of bytes read.
	 * @param buf The buffer containing the read data.
	 */
	void Read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

	/**
	 * Closes this connection.
	 * @todo Roll into the destructor/use RAII?
	 */
	void Close();

private:
	/// The parent IoCore on which this connection is running.
	IoCore &parent;

	/// The libuv handle for the TCP connection.
	uv_tcp_t *tcp;

	/// The Tokeniser to which data read on this connection should be sent.
	Tokeniser tokeniser;

	/// The CommandHandler to which finished commands should be sent.
	CommandHandler &handler;

	/// Has the connection authenticated correctly?
	bool authed;

	/**
	 * Handles a tokenised command line.
	 *
	 * @param line A vector of command words representing a command line.
	 */
	void HandleCommand(const std::vector<std::string> &words);
};

#endif // PS_IO_CORE_HPP
