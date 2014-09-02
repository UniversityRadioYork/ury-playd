// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the IoReactor class.
 * @see io/io_reactor.cpp
 */

#ifndef PS_IO_REACTOR_HPP
#define PS_IO_REACTOR_HPP

#include <deque>
#include <functional>
#include <ostream>
#include <set>

extern "C" {
#include <uv.h>
}

#include "../player/player.hpp"
#include "io_reactor.hpp"
#include "io_response.hpp"
#include "tokeniser.hpp"

class CommandHandler;
class Player;
class TcpResponseSink;

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

	/**
	 * Accepts a new connection.
	 *
	 * This accepts the connection, and adds it to this IoReactor's
	 * connection pool.
	 *
	 * This should be called with a server that has just received a new
	 * connection.
	 *
	 * @param server Pointer to the libuv server accepting connections.
	 *
	 * @todo This isn't a great fit for the public interface of IoReactor -
	 *   separate into a ConnectionPool class?
	 */
	void NewConnection(uv_stream_t *server);

	/**
	 * Removes a connection.
	 *
	 * @param sink The connection to remove.
	 *
	 * @todo Rename sink?
	 * @todo This isn't a great fit for the public interface of IoReactor -
	 *   separate into a ConnectionPool class?
	 */
	void RemoveConnection(TcpResponseSink &sink);

private:
	/// The set of connections currently serviced by the IoReactor.
	std::set<std::shared_ptr<TcpResponseSink>> connections;

	/// The period between player updates.
	static const uint16_t PLAYER_UPDATE_PERIOD;

	uv_tcp_t server;         ///< The libuv handle for the TCP server.
	uv_timer_t updater;      ///< The libuv handle for the update timer.

	Player &player;          ///< The player.
	CommandHandler &handler; ///< The command handler.

	void RespondRaw(const std::string &string) const override;

	/**
	 * Initialises a TCP acceptor on the given address and port.
	 *
	 * @param address The IPv4 address on which the TCP server should
	 *   listen.
	 * @param port The TCP port on which the TCP server should listen.
	 */
	void InitAcceptor(const std::string &address, const std::string &port);

	/// Sets up a periodic timer to run the playslave++ update loop.
	void DoUpdateTimer();

};

/**
 * A TCP connection from a client.
 * @todo Rename to Connection?
 */
class TcpResponseSink : public ResponseSink {
public:
	/**
	 * Constructs a TcpResponseSink.
	 * @param parent The IoReactor that is the parent of this connection.
	 * @param tcp The underlying libuv TCP stream.
	 * @param handler The handler to which read commands should be sent.
	 */
	TcpResponseSink(IoReactor &parent, uv_tcp_t *tcp,
	                CommandHandler &handler);

	// Note: This is made public so that the IoReactor can send raw data
	// to the connection.
	void RespondRaw(const std::string &response) const override;

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
	/// The parent IoReactor on which this connection is running.
	IoReactor &parent;

	/// The libuv handle for the TCP connection.
	uv_tcp_t *tcp;

	/// The Tokeniser to which data read on this connection should be sent.
	Tokeniser tokeniser;
};

#endif // PS_IO_REACTOR_HPP
