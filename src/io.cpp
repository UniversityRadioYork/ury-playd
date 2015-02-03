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
 * @see io.hpp
 */

#include <algorithm>
#include <cassert>
#include <csignal>
#include <cstring>
#include <sstream>
#include <string>

// If UNICODE is defined on Windows, it'll select the wide-char gai_strerror.
// We don't want this.
#undef UNICODE
#include <uv.h>

#include "errors.hpp"
#include "messages.h"
#include "player.hpp"
#include "response.hpp"

#include "io.hpp"

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
struct WriteReq
{
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
	assert(handle != nullptr);
	delete handle;
}

/// The callback fired when some bytes are read from a client connection.
void UvReadCallback(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
	assert(stream != nullptr);

	Connection *tcp = static_cast<Connection *>(stream->data);
	assert(tcp != nullptr);

	tcp->Read(nread, buf);
}

/// The callback fired when a new client connection is acquired by the listener.
void UvListenCallback(uv_stream_t *server, int status)
{
	if (status < 0) return;
	assert(server != nullptr);

	ConnectionPool *pool = static_cast<ConnectionPool *>(server->data);
	assert(pool != nullptr);

	pool->Accept(server);
}

/// The callback fired when a response has been sent to a client.
void UvRespondCallback(uv_write_t *req, int status)
{
	if (status) {
		Debug() << "UvRespondCallback: got status:" << status
		        << std::endl;
	}

	auto *wr = reinterpret_cast<WriteReq *>(req);
	assert(wr != nullptr);

	delete[] wr -> buf.base;
	delete wr;
}

/// The callback fired when the update timer fires.
void UvUpdateTimerCallback(uv_timer_t *handle)
{
	assert(handle != nullptr);

	Player *player = static_cast<Player *>(handle->data);
	assert(player != nullptr);

	bool running = player->Update();

	// If the player is ready to terminate, we need to kill the event loop
	// in order to disconnect clients and stop the updating.
	if (!running) uv_stop(uv_default_loop());
}

//
// ConnectionPool
//

ConnectionPool::ConnectionPool(Player &player) : player(player), pool()
{
}

void ConnectionPool::Accept(uv_stream_t *server)
{
	assert(server != nullptr);

	auto client = new uv_tcp_t();
	uv_tcp_init(uv_default_loop(), client);

	if (uv_accept(server, (uv_stream_t *)client)) {
		uv_close((uv_handle_t *)client, UvCloseCallback);
		return;
	}

	// We'll want to try and use an existing, empty slot in the connection
	// pool.  If there aren't any (we've exceeded the maximum-so-far number
	// of simultaneous connections), we expand the pool.
	//
	// If we already have SIZE_MAX-1 simultaneous connections, we bail out.
	// Since this is at least 65,534, and likely to be 2^32-2 or 2^64-2,
	// this is incredibly unlikely to happen and probably means someone's
	// trying to denial-of-service an audio player.
	//
	// Why -1?  Because slot 0 in the connection pool is reserved for
	// broadcasts.
	if (this->free_list.empty()) {
		if (this->pool.size() == (SIZE_MAX - 1)) {
			throw InternalError("too many simultaneous connections");
		}

		this->pool.emplace_back(nullptr);
		// This isn't an off-by-one error; slots index from 1.
		this->free_list.push_back(this->pool.size());
	}

	assert(!this->free_list.empty());
	size_t client_slot = this->free_list.back();
	this->free_list.pop_back();

	// client_slot should be at least 1, because of the above.
	assert(0 < client_slot);
	assert(client_slot <= this->pool.size());

	std::unique_ptr<Connection> conn(new Connection(*this, client, this->player, client_slot));
	client->data = static_cast<void *>(conn.get());
	this->pool[client_slot - 1] = std::move(conn);

	// The player will already have been told to send responses to the
	// IoCore, so all it needs to know is the slot.
	this->player.WelcomeClient(client_slot);

	uv_read_start((uv_stream_t *)client, UvAlloc, UvReadCallback);
}

void ConnectionPool::Remove(size_t slot)
{
	assert(0 < slot && slot <= this->pool.size());

	// Don't remove if it's already a nullptr, because we'd end up with the
	// slot on the free list twice.
	if (this->pool.at(slot - 1)) {
		this->pool[slot - 1] = nullptr;
		this->free_list.push_back(slot);
	}

	assert(!this->pool.at(slot - 1));
}

void ConnectionPool::Respond(size_t id, const Response &response) const
{
	if (this->pool.empty()) return;

	if (id == 0) {
		Debug() << "broadcast:" << response.Pack() << std::endl;
		for (const auto &conn : this->pool) conn->Respond(response);
	} else {
		Debug() << "unicast @" << std::to_string(id) << ":" << response.Pack() << std::endl;

		assert(0 < id && id <= this->pool.size());
		if (this->pool.at(id - 1)) this->pool.at(id - 1)->Respond(response);
	}
}

//
// IoCore
//

IoCore::IoCore(Player &player) : player(player), pool(player)
{
}

void IoCore::Run(const std::string &host, const std::string &port)
{
	this->InitAcceptor(host, port);
	this->DoUpdateTimer();
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
	assert(this->server.data != nullptr);

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

void IoCore::Respond(size_t id, const Response &response) const
{
	this->pool.Respond(id, response);
}

//
// Connection
//

Connection::Connection(ConnectionPool &parent, uv_tcp_t *tcp, Player &player, size_t id)
    : parent(parent), tcp(tcp), tokeniser(), player(player), id(id)
{
	Debug() << "Opening connection from" << Name() << std::endl;
}

Connection::~Connection()
{
	Debug() << "Closing connection from" << Name() << std::endl;
	uv_close((uv_handle_t *)this->tcp, UvCloseCallback);
}

void Connection::Respond(const Response &response) const
{
	auto string = response.Pack();

	unsigned int l = string.length();
	const char *s = string.c_str();
	assert(s != nullptr);

	auto req = new WriteReq;
	req->buf = uv_buf_init(new char[l + 1], l + 1);
	assert(req->buf.base != nullptr);
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
	auto sp = (struct sockaddr *)&s;

	// Turns out if you don't do this, Windows (and only Windows?) is upset.
	socklen_t namelen = sizeof(s);

	int pe = uv_tcp_getpeername(this->tcp, sp, (int *)&namelen);
	// These std::string()s are needed as, otherwise, the compiler would
	// think we're trying to add const char*s together.  We need AT LEAST
	// ONE of the sides of the first + to be a std::string.
	if (pe) return "<error@peer: " + std::string(uv_strerror(pe)) + ">";

	// Now, split the sockaddr into host and service.
	char host[NI_MAXHOST];
	char serv[NI_MAXSERV];

	// We use NI_NUMERICSERV to ensure a port number comes out.
	// Otherwise, we could get a (likely erroneous) string description of
	// what the network stack *thinks* the port is used for.
	int ne = getnameinfo(sp, namelen, host, sizeof(host), serv,
	                     sizeof(serv), NI_NUMERICSERV);
	// See comment for above error.
	if (ne) return "<error@name: " + std::string(gai_strerror(ne)) + ">";

	auto id = std::to_string(this->id);
	return id + std::string("!") + host + std::string(":") + serv;
}

void Connection::Read(ssize_t nread, const uv_buf_t *buf)
{
	assert(buf != nullptr);

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
	auto cmds = this->tokeniser.Feed(std::string(chars, nread));
	for (auto cmd : cmds) RunCommand(cmd);
	delete[] chars;
}

void Connection::RunCommand(const std::vector<std::string> &cmd)
{
	if (cmd.empty()) return;

	Debug() << "Received command:";
	for (const auto &word : cmd) std::cerr << ' ' << '"' << word << '"';
	std::cerr << std::endl;

	CommandResult res = this->player.RunCommand(cmd);
	res.Emit(this->parent, cmd);
}

void Connection::Depool()
{
	this->parent.Remove(this->id);
}
