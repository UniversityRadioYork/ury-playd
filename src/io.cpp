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
	uv_write_t req;   ///< The main libuv write handle.
	uv_buf_t buf;     ///< The associated write buffer.
	Connection *conn; ///< The recipient Connection.
	bool fatal;       ///< Whether the Connection should now close.
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

	IoCore *pool = static_cast<IoCore *>(server->data);
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

	// Certain requests are intended to signal to the connection that it
	// should close.  These have the 'fatal' flag set.
	if (wr->fatal && wr->conn != nullptr) wr->conn->Depool();

	delete[] wr -> buf.base;
	delete wr;
}

/// The callback fired when the update timer fires.
void UvUpdateTimerCallback(uv_timer_t *handle)
{
	assert(handle != nullptr);

	IoCore *io = static_cast<IoCore *>(handle->data);
	assert(io != nullptr);
	io->UpdatePlayer();
}

//
// IoCore
//

IoCore::IoCore(Player &player) : player(player)
{
}

void IoCore::Run(const std::string &host, const std::string &port)
{
	this->InitAcceptor(host, port);
	this->DoUpdateTimer();
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

void IoCore::Accept(uv_stream_t *server)
{
	assert(server != nullptr);

	auto client = new uv_tcp_t();
	uv_tcp_init(uv_default_loop(), client);

	// libuv does the 'nonzero is error' thing here
	if (uv_accept(server, (uv_stream_t *)client)) {
		uv_close((uv_handle_t *)client, UvCloseCallback);
		return;
	}

	auto id = this->NextConnectionID();
	auto conn = std::make_shared<Connection>(*this, client, this->player, id);
	client->data = static_cast<void *>(conn.get());
	this->pool[id - 1] = std::move(conn);

	// The player will already have been told to send responses to the
	// IoCore, so all it needs to know is the slot.
	this->player.WelcomeClient(id);

	uv_read_start((uv_stream_t *)client, UvAlloc, UvReadCallback);
}

size_t IoCore::NextConnectionID()
{
	// We'll want to try and use an existing, empty ID in the connection
	// pool.  If there aren't any (we've exceeded the maximum-so-far number
	// of simultaneous connections), we expand the pool.
	if (this->free_list.empty()) this->ExpandPool();
	assert(!this->free_list.empty());

	// Acquire some free ID, and ensure that the ID cannot be re-used until
	// replaced onto the free list by the connection's removal.
	size_t id = this->free_list.back();
	this->free_list.pop_back();

	// client_slot should be at least 1, because of the above.
	assert(0 < id);
	assert(id <= this->pool.size());

	return id;
}

void IoCore::ExpandPool()
{
	// If we already have SIZE_MAX-1 simultaneous connections, we bail out.
	// Since this is at least 65,534, and likely to be 2^32-2 or 2^64-2,
	// this is incredibly unlikely to happen and probably means someone's
	// trying to denial-of-service an audio player.
	//
	// Why -1?  Because slot 0 in the connection pool is reserved for
	// broadcasts.
	bool full = this->pool.size() == (SIZE_MAX - 1);
	if (full) throw InternalError(MSG_TOO_MANY_CONNS);

	this->pool.emplace_back(nullptr);
	// This isn't an off-by-one error; slots index from 1.
	this->free_list.push_back(this->pool.size());
}

void IoCore::Remove(size_t slot)
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

void IoCore::UpdatePlayer()
{
	bool running = this->player.Update();
	if (!running) this->Shutdown();
}

void IoCore::Shutdown()
{
	// If the player is ready to terminate, we need to kill the event loop
	// in order to disconnect clients and stop the updating.
	// We do this by stopping everything using the loop.

	// First, the update timer:
	uv_timer_stop(&this->updater);

	// Then, the TCP server (as far as we can tell, this does *not* close
	// down the connections):
	uv_close(reinterpret_cast<uv_handle_t *>(&this->server), nullptr);

	// Finally, kill off all of the connections with 'fatal' responses.
	for (const auto conn : this->pool) IoCore::TryShutdown(conn);
}

/* static */ void IoCore::TryShutdown(const std::shared_ptr<Connection> conn)
{
	if (!conn) return;

	auto response = Response(Response::Code::STATE).AddArg("Quitting");

	// The true at the end is for the 'fatal' argument to
	// Connection::Respond, telling it to close itself after processing
	// 'response'.
	conn->Respond(response, true);
}

/* static */ void IoCore::TryRespond(const std::shared_ptr<Connection> conn,
                                     const Response &response)
{
	if (conn) conn->Respond(response);
}

void IoCore::Respond(const Response &response, size_t id) const
{
	if (this->pool.empty()) return;

	if (id == 0) {
		this->Broadcast(response);
	} else {
		this->Unicast(response, id);
	}
}

void IoCore::Broadcast(const Response &response) const
{
	Debug() << "broadcast:" << response.Pack() << std::endl;

	// Copy the connection by value, so that there's at least one
	// active reference to it throughout.
	for (const auto c : this->pool) IoCore::TryRespond(c, response);
}

void IoCore::Unicast(const Response &response, size_t id) const
{
	assert(0 < id && id <= this->pool.size());

	Debug() << "unicast @" << std::to_string(id) << ":" << response.Pack()
	        << std::endl;

	IoCore::TryRespond(this->pool.at(id - 1), response);
}

void IoCore::DoUpdateTimer()
{
	uv_timer_init(uv_default_loop(), &this->updater);
	this->updater.data = static_cast<void *>(this);
	uv_timer_start(&this->updater, UvUpdateTimerCallback, 0,
	               PLAYER_UPDATE_PERIOD);
}

void IoCore::InitAcceptor(const std::string &address, const std::string &port)
{
	int uport = std::stoi(port);

	uv_tcp_init(uv_default_loop(), &this->server);
	this->server.data = static_cast<void *>(this);
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

//
// Connection
//

Connection::Connection(IoCore &parent, uv_tcp_t *tcp, Player &player, size_t id)
    : parent(parent), tcp(tcp), tokeniser(), player(player), id(id)
{
	Debug() << "Opening connection from" << Name() << std::endl;
}

Connection::~Connection()
{
	Debug() << "Closing connection from" << Name() << std::endl;
	uv_close((uv_handle_t *)this->tcp, UvCloseCallback);
}

void Connection::Respond(const Response &response, bool fatal)
{
	auto string = response.Pack();

	unsigned int l = string.length();
	const char *s = string.c_str();
	assert(s != nullptr);

	auto req = new WriteReq;
	req->conn = this;
	req->fatal = fatal;

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

	CommandResult res = this->player.RunCommand(cmd, this->id);
	res.Emit(this->parent, cmd, this->id);
}

void Connection::Depool()
{
	this->parent.Remove(this->id);
}
