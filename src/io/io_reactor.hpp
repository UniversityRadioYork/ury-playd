// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the IoReactor class.
 * @see io/io_reactor.cpp
 */

#ifndef PS_IO_REACTOR_HPP
#define PS_IO_REACTOR_HPP

#include "io_response.hpp"
#include <deque>          // std::deque
#include <functional>     // std::function
#include <ostream>        // std::ostream
#include <set>            // std::set
#include "io_reactor.hpp" // IoReactor
#include "tokeniser.hpp"
#include "../player/player.hpp"

class Player;
class CommandHandler;

extern "C" {
#include <uv.h>
}

class CommandHandler;
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

	void NewConnection(uv_stream_t *server);
	void Read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
	void RemoveConnection(TcpResponseSink &sink);

private:
	void RespondRaw(const std::string &string) const override;
	void InitAcceptor(const std::string &address, const std::string &port);
	void DoUpdateTimer();

	std::set<std::shared_ptr<TcpResponseSink>> connections;

	/// The period between player updates.
	static const uint16_t PLAYER_UPDATE_PERIOD;

	uv_tcp_t server;
	uv_timer_t updater;

	Player &player;          ///< The player.
	CommandHandler &handler; ///< The command handler.
};

/**
 * A TCP connection from a client.
 */
class TcpResponseSink : public ResponseSink {
public:
	TcpResponseSink(IoReactor &parent, uv_tcp_t *tcp,
	                CommandHandler &handler);
	void RespondRaw(const std::string &response) const override;
	void Read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
	void Close();

private:
	IoReactor &parent;
	uv_tcp_t *tcp;
	Tokeniser tokeniser;
};

#endif // PS_IO_REACTOR_HPP
