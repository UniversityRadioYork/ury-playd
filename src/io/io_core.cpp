// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of the non-virtual aspects of the IoCore class.
 *
 * The implementation of IoCore is based on [libuv][], and also makes use
 * of various techniques mentioned in [the uvbook][].
 *
 * [libuv]: https://github.com/joyent/libuv
 * [the uvbook]: https://nikhilm.github.io/uvbook
 *
 * @see io/io_core.hpp
 */

#include <algorithm>
#include <csignal>
#include <cstring>
#include <string>

extern "C" {
#include <uv.h>
}

#include "../cmd.hpp"
#include "../errors.hpp"
#include "../messages.h"
#include "../player/player.hpp"
#include "io_core.hpp"
#include "io_response.hpp"

const std::uint16_t IoCore::PLAYER_UPDATE_PERIOD = 5; // ms

//
// libuv callbacks
//
// These should generally trampoline back into class methods.
//

/**
 * A structure used to associate a write buffer with a write handle.
 *
 * The WriteReq can appear to libuv code as a `uv_write_t`, as it includes a
 * `uv_write_t` at the start of its memory footprint.  This is a slightly
 * nasty use of low-level C, but works well.
 *
 * This tactic comes from the [Buffers and Streams][b] section of the uvbook;
 * see the _Write to pipe_ sub-section.
 *
 * [b]: https://nikhilm.github.io/uvbook/filesystem.html#buffers-and-streams
 */
struct WriteReq {
	uv_write_t req; ///< The main libuv write handle.
	uv_buf_t buf;   ///< The associated write buffer.
};

/// The function used to allocate and initialise buffers for client reading.
void UvAlloc(uv_handle_t *, size_t suggested_size, uv_buf_t *buf)
{
	*buf = uv_buf_init(new char[suggested_size](), suggested_size);
}

/// The callback fired when a client connection closes.
void UvCloseCallback(uv_handle_t *handle)
{
	delete handle;
}

/// The callback fired when some bytes are read from a client connection.
void UvReadCallback(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
	Connection *tcp = static_cast<Connection *>(stream->data);
	tcp->Read(nread, buf);
}

/// The callback fired when a new client connection is acquired by the listener.
void UvListenCallback(uv_stream_t *server, int status)
{
	if (status < 0) return;

	ConnectionPool *pool = static_cast<ConnectionPool *>(server->data);
	pool->Accept(server);
}

/// The callback fired when a response has been sent to a client.
void UvRespondCallback(uv_write_t *req, int status)
{
	if (status) {
		Debug() << "UvRespondCallback: got status:" << status
		        << std::endl;
	}

	WriteReq *wr = reinterpret_cast<WriteReq *>(req);
	delete[] wr -> buf.base;
	delete wr;
}

/// The callback fired when the update timer fires.
void UvUpdateTimerCallback(uv_timer_t *handle)
{
	Player *player = static_cast<Player *>(handle->data);
	bool running = player->Update();

	// If the player is ready to terminate, we need to kill the event loop
	// in order to disconnect clients and stop the updating.
	if (!running) IoCore::End();
}

//
// ConnectionPool
//

ConnectionPool::ConnectionPool(Player &player, CommandHandler &handler)
    : player(player), handler(handler), connections()
{
}

void ConnectionPool::Accept(uv_stream_t *server)
{
	uv_tcp_t *client = new uv_tcp_t();
	uv_tcp_init(uv_default_loop(), client);

	if (uv_accept(server, (uv_stream_t *)client) == 0) {
		this->connections.emplace_back(
		                new Connection(*this, client, this->handler));

		this->player.WelcomeClient(*this->connections.back());
		client->data = static_cast<void *>(
		                this->connections.back().get());

		uv_read_start((uv_stream_t *)client, UvAlloc, UvReadCallback);
	}
	else {
		uv_close((uv_handle_t *)client, UvCloseCallback);
	}
}

void ConnectionPool::Remove(Connection &conn)
{
	this->connections.erase(std::remove_if(
	                this->connections.begin(), this->connections.end(),
	                [&](const std::unique_ptr<Connection> &p) {
		                return p.get() == &conn;
		        }));
}

void ConnectionPool::Broadcast(const std::string &message) const
{
	if (this->connections.empty()) return;

	Debug() << "Sending command:" << message << std::endl;
	for (const auto &conn : this->connections) conn->RespondRaw(message);
}

void ConnectionPool::RespondRaw(const std::string &string) const
{
	this->Broadcast(string);
}

//
// IoCore
//

IoCore::IoCore(Player &player, CommandHandler &handler,
               const std::string &address, const std::string &port)
    : player(player), pool(player, handler)
{
	this->InitAcceptor(address, port);
	this->DoUpdateTimer();
}

/* static */ void IoCore::Run()
{
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

void IoCore::DoUpdateTimer()
{
	uv_timer_init(uv_default_loop(), &this->updater);
	this->updater.data = static_cast<void *>(&this->player);
	uv_timer_start(&this->updater, UvUpdateTimerCallback, 0,
	               PLAYER_UPDATE_PERIOD);
}

void IoCore::InitAcceptor(const std::string &address, const std::string &port)
{
	int uport = std::stoi(port);

	uv_tcp_init(uv_default_loop(), &this->server);
	this->server.data = static_cast<void *>(&this->pool);

	struct sockaddr_in bind_addr;
	uv_ip4_addr(address.c_str(), uport, &bind_addr);
	uv_tcp_bind(&this->server, (const sockaddr *)&bind_addr, 0);

	int r = uv_listen((uv_stream_t *)&this->server, 128, UvListenCallback);
	if (r) {
		throw NetError("Could not listen on " + address + ":" + port +
		               " (" + std::string(uv_err_name(r)) + ")");
	}

	Debug() << "Listening at" << address << "on" << port << std::endl;
}

void IoCore::RespondRaw(const std::string &string) const
{
	this->pool.Broadcast(string);
}

/* static */ void IoCore::End()
{
	uv_stop(uv_default_loop());
}

//
// Connection
//

Connection::Connection(ConnectionPool &parent, uv_tcp_t *tcp,
                       CommandHandler &handler)
    : parent(parent), tcp(tcp), tokeniser(), handler(handler)
{
	Debug() << "Opening connection from" << Name() << std::endl;
}

Connection::~Connection()
{
	Debug() << "Closing connection from" << Name() << std::endl;
	uv_close((uv_handle_t *)this->tcp, UvCloseCallback);
}

void Connection::RespondRaw(const std::string &string) const
{
	unsigned int l = string.length();
	const char *s = string.c_str();

	WriteReq *req = new WriteReq;
	req->buf = uv_buf_init(new char[l + 1], l + 1);
	memcpy(req->buf.base, s, l);
	req->buf.base[l] = '\n';

	uv_write((uv_write_t *)req, (uv_stream_t *)this->tcp, &req->buf, 1,
	         UvRespondCallback);
}

std::string Connection::Name()
{
	// Warning: fairly low-level Berkeley sockets code ahead!
	// (Thankfully, libuv makes sure the appropriate headers are included.)

	// Using this instead of struct sockaddr is advised by the libuv docs,
	// for IPv6 compatibility.
	struct sockaddr_storage s;
	int namelen;

	if (uv_tcp_getpeername(this->tcp, (struct sockaddr *)&s, &namelen) <
	    0) {
		return "(error)";
	}

	// Now, split the sockaddr into host and service.
	char host[NI_MAXHOST];
	char serv[NI_MAXSERV];

	// We use NI_NUMERICSERV to ensure a port number comes out.
	// Otherwise, we could get a (likely erroneous) string description of
	// what
	// the network stack *thinks* the port is used for.
	if (getnameinfo((struct sockaddr *)&s, namelen, host, sizeof(host),
	                serv, sizeof(serv), NI_NUMERICSERV)) {
		return "(error)";
	}

	return std::string(host) + ":" + std::string(serv);
}

void Connection::Read(ssize_t nread, const uv_buf_t *buf)
{
	// Did the connection hang up?  If so, de-pool it.
	// De-pooling the connection will usually lead to the connection being
	// destroyed.
	if (nread == UV_EOF) {
		this->Depool();
		return;
	}

	// Did we hit any other read errors?  Also de-pool, but log the error.
	if (nread < 0) {
		Debug() << "Error on" << Name() << "-" << uv_err_name(nread)
		        << std::endl;
		this->Depool();
		return;
	}

	auto chars = buf->base;

	// Make sure we actually have some data to read!
	if (chars == nullptr) return;

	// Everything looks okay for reading.
	auto lines = this->tokeniser.Feed(std::string(chars, nread));
	for (auto line : lines) HandleCommand(line);
	delete[] chars;
}

void Connection::HandleCommand(const std::vector<std::string> &words)
{
	if (words.empty()) return;

	Debug() << "Received command:";
	for (const auto &word : words) std::cerr << ' ' << '"' << word << '"';
	std::cerr << std::endl;

	CommandResult res = this->handler.Handle(words);
	res.Emit(this->parent, words);
}

void Connection::Depool()
{
	this->parent.Remove(*this);
}
