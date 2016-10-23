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
 * @see io.h
 */

#include <algorithm>
#include <cassert>
#include <csignal>
#include <string>

// If UNICODE is defined on Windows, it'll select the wide-char gai_strerror.
// We don't want this.
#undef UNICODE
// Use the same ssize_t as libmpg123 on Windows.
#ifdef _MSC_VER
typedef ptrdiff_t ssize_t;
#define _SSIZE_T_
#define _SSIZE_T_DEFINED
#endif
#include <uv.h>

#include "errors.h"
#include "messages.h"
#include "player.h"
#include "response.h"

#include "io.h"

const std::uint16_t IoCore::PLAYER_UPDATE_PERIOD = 5; // ms

//
// libuv callbacks
//
// These should generally trampoline back into class methods.
//

/// The function used to allocate and initialise buffers for client reading.
void UvAlloc(uv_handle_t *, size_t suggested_size, uv_buf_t *buf)
{
	// Since we used new here, we need to use delete when we finish reading.
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

	auto *tcp = static_cast<Connection *>(stream->data);
	assert(tcp != nullptr);

	// NB: Read delete[]s buf->base, so we don't.
	tcp->Read(nread, buf);

	// We don't delete the handle.
	// It will be used for future reads on this client!
}

/// The callback fired when a new client connection is acquired by the listener.
void UvListenCallback(uv_stream_t *server, int status)
{
	assert(server != nullptr);
	if (status < 0) return;

	auto *io = static_cast<IoCore *>(server->data);
	assert(io != nullptr);

	io->Accept(server);
}

/// The callback fired when a response has been sent to a client.
void UvWriteCallback(uv_write_t *req, int status)
{
	assert(req != nullptr);

	if (status) {
		Debug() << "UvRespondCallback: got status:" << status
		        << std::endl;
	}

	// We receive the write buffer as the write_t's data pointer.
	// This is because something has to delete[] it,
	// and we drew the short straw.
	auto *buf = static_cast<char *>(req->data);
	assert(buf != nullptr);

	delete[] buf;

	// This handle was created specifically for this shutdown.
	// We thus have to delete it.
	delete req;
}

/// The callback fired when the update timer fires.
void UvUpdateTimerCallback(uv_timer_t *handle)
{
	assert(handle != nullptr);

	auto *io = static_cast<IoCore *>(handle->data);
	assert(io != nullptr);

	io->UpdatePlayer();

	// We don't delete the handle.
	// It is being used for other timer fires.
}

/// The callback fired when SIGINT occurs.
void UvSigintCallback(uv_signal_t *handle, int signum)
{
	assert(handle != nullptr);

	auto *player = static_cast<Player *>(handle->data);
	assert(player != nullptr);

	if (signum != SIGINT) return;

	Debug() << "Caught SIGINT, closing..." << std::endl;
	player->Quit(Response::NOREQUEST);

	// We don't delete the handle.
	// It is being used for other signals.
}

/// The callback fired when a client is shut down.
void UvShutdownCallback(uv_shutdown_t *handle, int status)
{
	assert(handle != nullptr);

	if (status) {
		Debug() << "UvShutdownCallback: got status:" << status
		        << std::endl;
	}

	auto *conn = static_cast<Connection *>(handle->data);
	assert(conn != nullptr);

	// Now actually tell the client to die off.
	conn->Depool();

	// This handle was created specifically for this shutdown.
	// We thus have to delete it.
	delete handle;
}

//
// IoCore
//

IoCore::IoCore(Player &player) : loop(nullptr), player(player)
{
}

void IoCore::Run(const std::string &host, const std::string &port)
{
	this->loop = uv_default_loop();
	if (this->loop == nullptr) throw InternalError(MSG_IO_CANNOT_ALLOC);

	this->InitAcceptor(host, port);
	this->InitSignals();
	this->InitUpdateTimer();

	uv_run(this->loop, UV_RUN_DEFAULT);

	// We presume all of the open handles have been closed in Shutdown().
	// We need only close the loop.
	uv_loop_close(this->loop);
}

void IoCore::Accept(uv_stream_t *server)
{
	assert(server != nullptr);
	assert(this->loop != nullptr);

	auto client = new uv_tcp_t();
	uv_tcp_init(this->loop, client);

	// libuv does the 'nonzero is error' thing here
	if (uv_accept(server, reinterpret_cast<uv_stream_t *>(client))) {
		uv_close(reinterpret_cast<uv_handle_t *>(client), UvCloseCallback);
		return;
	}

	auto id = this->NextConnectionID();
	auto conn = std::make_shared<Connection>(*this, client, this->player, id);
	client->data = static_cast<void *>(conn.get());
	this->pool[id - 1] = move(conn);

	// Begin initial responses
	this->Respond(id, Response(Response::NOREQUEST, Response::Code::OHAI)
	                          .AddArg(std::to_string(id))
	                          .AddArg(MSG_OHAI_BIFROST)
	                          .AddArg(MSG_OHAI_PLAYD));
	this->Respond(id, Response(Response::NOREQUEST, Response::Code::IAMA)
	                          .AddArg("player/file"));
	this->player.Dump(id, Response::NOREQUEST);
	this->Respond(id, Response::Success(Response::NOREQUEST));
	// End initial responses

	uv_read_start(reinterpret_cast<uv_stream_t *>(client), UvAlloc, UvReadCallback);
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
	Debug() << "Shutting down..." << std::endl;

	// If the player is ready to terminate, we need to kill the event loop
	// in order to disconnect clients and stop the updating.
	// We do this by stopping everything using the loop.

	// First, the update timer:
	uv_timer_stop(&this->updater);

	// Then, the TCP server (as far as we can tell, this does *not* close
	// down the connections):
	uv_close(reinterpret_cast<uv_handle_t *>(&this->server), nullptr);

	// Next, ask each connection to stop.
	for (const auto conn : this->pool) {
		if (conn) conn->Shutdown();
	}

	// Finally, unregister signal processing.
	uv_signal_stop(&this->sigint);
	uv_close(reinterpret_cast<uv_handle_t *>(&this->sigint), nullptr);
}

void IoCore::Respond(size_t id, const Response &response) const
{
	if (this->pool.empty()) return;

	if (id == 0) {
		this->Broadcast(response);
	} else {
		this->Unicast(id, response);
	}
}

void IoCore::Broadcast(const Response &response) const
{
	Debug() << "broadcast:" << response.Pack() << std::endl;

	// Copy the connection by value, so that there's at least one
	// active reference to it throughout.
	for (const auto c : this->pool) {
		if (c) c->Respond(response);
	}
}

void IoCore::Unicast(size_t id, const Response &response) const
{
	assert(0 < id && id <= this->pool.size());

	Debug() << "unicast @" << std::to_string(id) << ":" << response.Pack()
	        << std::endl;

	auto c = this->pool.at(id - 1);
	if (c) c->Respond(response);
}

void IoCore::InitUpdateTimer()
{
	assert(this->loop != nullptr);

	uv_timer_init(this->loop, &this->updater);
	this->updater.data = static_cast<void *>(this);

	uv_timer_start(&this->updater, UvUpdateTimerCallback, 0,
	               PLAYER_UPDATE_PERIOD);
}

void IoCore::InitAcceptor(const std::string &address, const std::string &port)
{
	assert(this->loop != nullptr);

	if (uv_tcp_init(this->loop, &this->server)) {
		throw InternalError(MSG_IO_CANNOT_ALLOC);
	}
	this->server.data = static_cast<void *>(this);
	assert(this->server.data != nullptr);

	struct sockaddr_in bind_addr;
	uv_ip4_addr(address.c_str(), stoi(port), &bind_addr);
	uv_tcp_bind(&this->server, reinterpret_cast<const sockaddr *>(&bind_addr), 0);

	auto r = uv_listen(reinterpret_cast<uv_stream_t *>(&this->server), 128, UvListenCallback);
	if (r) {
		throw NetError("Could not listen on " + address + ":" + port +
		               " (" + std::string(uv_err_name(r)) + ")");
	}

	Debug() << "Listening at" << address << "on" << port << std::endl;
}

void IoCore::InitSignals()
{
	auto r = uv_signal_init(this->loop, &this->sigint);
	if (r) {
		throw InternalError(MSG_IO_CANNOT_ALLOC + ": " +
		                    std::string(uv_err_name(r)));
	}

	// We pass the player, not the IoCore.
	// This is so the SIGINT handler can tell the player to quit,
	// which then tells us to shutdown
	this->sigint.data = static_cast<void *>(&this->player);
	assert(this->sigint.data != nullptr);
	uv_signal_start(&this->sigint, UvSigintCallback, SIGINT);
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
	uv_close(reinterpret_cast<uv_handle_t *>(this->tcp), UvCloseCallback);
}

void Connection::Respond(const Response &response)
{
	// Pack provides us the response's wire format, except the newline.
	// We can provide that here.
	auto string = response.Pack();
	string.push_back('\n');

	unsigned int l = string.length();

	// Make a libuv buffer and pour the request into it.
	// The onus is on UvRespondCallback to free buf.base.
	auto buf = uv_buf_init(new char[l], l);
	assert(buf.base != nullptr);
	string.copy(buf.base, l);

	// Make a write request.
	// Since the callback must free the buffer, pass it through as data.
	// The callback will also free req.
	auto req = new uv_write_t;
	req->data = static_cast<void *>(buf.base);

	uv_write((uv_write_t *)req, (uv_stream_t *)this->tcp, &buf, 1,
	         UvWriteCallback);
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

	// Make sure we actually have some data to read!
	if (buf->base == nullptr) return;

	// Everything looks okay for reading.
	auto cmds = this->tokeniser.Feed(std::string(buf->base, nread));
	for (auto cmd : cmds) {
		if (cmd.empty()) continue;

		Response res = RunCommand(cmd);
		this->Respond(res);
	}

	delete[] buf->base;
}

Response Connection::RunCommand(const std::vector<std::string> &cmd)
{
	// First of all, figure out what the tag of this command is.
	// The first word is always the tag.
	auto tag = cmd[0];
	if (cmd.size() <= 1) return Response::Invalid(tag, MSG_CMD_SHORT);

	// The next words are the actual command, and any other arguments.
	auto word = cmd[1];
	auto nargs = cmd.size() - 2;

	if (nargs == 0) {
		if ("play" == word) return this->player.SetPlaying(tag, true);
		if ("stop" == word) return this->player.SetPlaying(tag, false);
		if ("end" == word) return this->player.End(tag);
		if ("eject" == word) return this->player.Eject(tag);
		if ("dump" == word) return this->player.Dump(id, tag);
	} else if (nargs == 1) {
		if ("fload" == word) return this->player.Load(tag, cmd[2]);
		if ("pos" == word) return this->player.Pos(tag, cmd[2]);
	}

	return Response::Invalid(tag, MSG_CMD_INVALID);
}

void Connection::Shutdown()
{
	auto req = new uv_shutdown_t;
	assert(req != nullptr);

	req->data = this;

	uv_shutdown(req, reinterpret_cast<uv_stream_t *>(this->tcp),
	            UvShutdownCallback);
}

void Connection::Depool()
{
	this->parent.Remove(this->id);
}
