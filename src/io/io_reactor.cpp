// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the non-virtual aspects of the IoReactor class.
 *
 * The implementation of IoReactor is based on [libuv][], and also makes use
 * of various techniques mentioned in [the uvbook][].
 *
 * [libuv]: https://github.com/joyent/libuv
 * [the uvbook]: https://nikhilm.github.io/uvbook
 *
 * @see io/io_reactor.hpp
 */

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
#include "io_reactor.hpp"
#include "io_response.hpp"

const std::uint16_t IoReactor::PLAYER_UPDATE_PERIOD = 5; // ms

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
	Debug() << "Closing client connection" << std::endl;
	if (handle->data != nullptr) {
		auto tcp = static_cast<TcpResponseSink *>(handle->data);
		tcp->Close();
	}
	delete handle;
}

/// The callback fired when some bytes are read from a client connection.
void UvReadCallback(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
	TcpResponseSink *tcp = static_cast<TcpResponseSink *>(stream->data);
	tcp->Read(stream, nread, buf);
}

/// The callback fired when a new client connection is acquired by the listener.
void UvListenCallback(uv_stream_t *server, int status)
{
	if (status == -1) {
		return;
	}
	IoReactor *reactor = static_cast<IoReactor *>(server->data);
	reactor->NewConnection(server);
}

/// The callback fired when a response has been sent to a client.
void UvRespondCallback(uv_write_t *req, int)
{
	// TODO: Handle the int status?
	WriteReq *wr = (WriteReq *)req;
	delete[] wr->buf.base;
	delete wr;
}

/// The callback fired when the update timer fires.
void UvUpdateTimerCallback(uv_timer_t *handle)
{
	Player *player = static_cast<Player *>(handle->data);
	bool running = player->Update();
	if (!running) {
		uv_stop(uv_default_loop());
	}
}

//
// IoReactor
//

void IoReactor::NewConnection(uv_stream_t *server)
{
	uv_tcp_t *client = new uv_tcp_t();
	uv_tcp_init(uv_default_loop(), client);

	if (uv_accept(server, (uv_stream_t *)client) == 0) {
		Debug() << "New connection" << std::endl;
		auto tcp = std::make_shared<TcpResponseSink>(*this, client,
		                                             this->handler);
		this->player.WelcomeClient(*tcp);
		this->connections.insert(tcp);
		client->data = static_cast<void *>(tcp.get());

		uv_read_start((uv_stream_t *)client, UvAlloc, UvReadCallback);
	} else {
		uv_close((uv_handle_t *)client, UvCloseCallback);
	}
}

void IoReactor::RemoveConnection(TcpResponseSink &conn)
{
	this->connections.erase(std::make_shared<TcpResponseSink>(conn));
}

IoReactor::IoReactor(Player &player, CommandHandler &handler,
                     const std::string &address, const std::string &port)
    : player(player), handler(handler)
{
	InitAcceptor(address, port);
	DoUpdateTimer();
}

void IoReactor::Run()
{
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

void IoReactor::DoUpdateTimer()
{
	uv_timer_init(uv_default_loop(), &this->updater);
	this->updater.data = static_cast<void *>(&this->player);
	uv_timer_start(&this->updater, UvUpdateTimerCallback, 0,
	               PLAYER_UPDATE_PERIOD);
}

void IoReactor::InitAcceptor(const std::string &address,
                             const std::string &port)
{
	int uport = std::stoi(port);

	uv_tcp_init(uv_default_loop(), &this->server);
	this->server.data = static_cast<void *>(this);

	struct sockaddr_in bind_addr;
	uv_ip4_addr(address.c_str(), uport, &bind_addr);
	uv_tcp_bind(&this->server, (const sockaddr *)&bind_addr, 0);

	// TODO: Handle errors from uv_listen.
	uv_listen((uv_stream_t *)&this->server, 128, UvListenCallback);
	Debug() << "Listening at" << address << "on" << port << std::endl;
}

void IoReactor::RespondRaw(const std::string &string) const
{
	for (const auto &conn : this->connections) {
		conn->RespondRaw(string);
	}
}

void IoReactor::End()
{
	uv_stop(uv_default_loop());
}

//
// TcpResponseSink
//

TcpResponseSink::TcpResponseSink(IoReactor &parent, uv_tcp_t *tcp,
                                 CommandHandler &handler)
    : parent(parent), tcp(tcp), tokeniser(handler, *this)
{
}

void TcpResponseSink::RespondRaw(const std::string &string) const
{
	Debug() << "Sending command:" << string << std::endl;
	unsigned int l = string.length();
	const char *s = string.c_str();

	WriteReq *req = new WriteReq;
	req->buf = uv_buf_init(new char[l + 1], l + 1);
	memcpy(req->buf.base, s, l);
	req->buf.base[l] = '\n';

	uv_write((uv_write_t *)req, (uv_stream_t *)tcp, &req->buf, 1,
	         UvRespondCallback);
}

void TcpResponseSink::Read(uv_stream_t *stream, ssize_t nread,
                           const uv_buf_t *buf)
{
	if (nread < 0) {
		if (nread == UV_EOF) {
			uv_close((uv_handle_t *)stream, UvCloseCallback);
		}

		return;
	}

	if (buf->base != nullptr) {
		this->tokeniser.Feed(buf->base, nread);
		delete[] buf->base;
	}
}

void TcpResponseSink::Close()
{
	this->parent.RemoveConnection(*this);
}
