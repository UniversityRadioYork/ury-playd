// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the non-virtual aspects of the IoReactor class.
 * @see io/io_reactor.hpp
 */

#include <csignal>
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

const std::uint16_t IoReactor::PLAYER_UPDATE_PERIOD = 10; // ms

//
// libuv callbacks
//
// These should generally trampoline back into class methods.
//

struct WriteReq {
	uv_write_t req;
	uv_buf_t buf;
};

void UvAlloc(uv_handle_t *, size_t suggested_size, uv_buf_t *buf)
{
	*buf = uv_buf_init((char *)malloc(suggested_size), suggested_size);
}

void UvCloseCallback(uv_handle_t *handle)
{
	TcpResponseSink *tcp = static_cast<TcpResponseSink *>(handle->data);
	tcp->Close();
}

void UvReadCallback(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
	TcpResponseSink *tcp = static_cast<TcpResponseSink *>(stream->data);
	tcp->Read(stream, nread, buf);
}

void UvListenCallback(uv_stream_t *server, int status)
{
	if (status == -1) {
		return;
	}
	IoReactor *reactor = static_cast<IoReactor *>(server->data);
	reactor->NewConnection(server);
}

void UvRespondCallback(uv_write_t *req, int)
{
	// TODO: Handle the int status?
	WriteReq *wr = (WriteReq *)req;
	delete[] wr->buf.base;
	delete wr;
}

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
	uv_tcp_t *client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
	uv_tcp_init(uv_default_loop(), client);

	if (uv_accept(server, (uv_stream_t *)client) == 0) {
		auto tcp = std::make_shared<TcpResponseSink>(*this, client,
		                                             this->handler);
		this->player.WelcomeClient(*tcp);
		this->connections.insert(tcp);
		client->data = static_cast<void *>(tcp.get());

		uv_read_start((uv_stream_t *)client, UvAlloc, UvReadCallback);
	} else {
		uv_close((uv_handle_t *)client, nullptr);
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

	this->tokeniser.Feed(buf->base, (buf->base) + nread);
}

void TcpResponseSink::Close()
{
	Debug() << "Closing client connection" << std::endl;
	this->parent.RemoveConnection(*this);
}
