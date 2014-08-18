// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the non-virtual aspects of the IoReactor class.
 * @see io/io_reactor.hpp
 * @see io/io_reactor_asio.hpp
 * @see io/io_reactor_asio.cpp
 * @see io/io_reactor_std.hpp
 * @see io/io_reactor_std.cpp
 */

/* This is forced because we require C++11, which has std::chrono, anyway.
   Some environments fail Boost's std::chrono detection, and don't set this
   flag, which causes compilation failures. */
#ifndef BOOST_ASIO_HAS_STD_CHRONO
#define BOOST_ASIO_HAS_STD_CHRONO
#endif

#include <csignal>              // SIG*
#include <string>               // std::string

extern "C" {
#include <uv.h>
}

#include "../player/player.hpp" // Player
#include "../cmd.hpp"           // CommandHandler
#include "../errors.hpp"
#include "../messages.h"                        // MSG_*
#include "io_reactor.hpp"                       // IoReactor
#include "io_response.hpp"                      // ResponseCode
#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>                       // boost::asio::*
#include <boost/asio/high_resolution_timer.hpp> // boost::asio::high_resolution_timer

const std::uint16_t IoReactor::PLAYER_UPDATE_PERIOD = 20; // ms

void AllocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
	*buf = uv_buf_init((char *)malloc(suggested_size), suggested_size);
}

void Read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{

}

void OnNewConnection(uv_stream_t *server, int status)
{
	if (status == -1) {
		return;
	}
	IoReactor *reactor = static_cast<IoReactor *>(server->data);
	reactor->NewConnection(server);
}

void IoReactor::NewConnection(uv_stream_t *server)
{
	uv_tcp_t *client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
	uv_tcp_init(uv_default_loop(), client);
	if (uv_accept(server, (uv_stream_t *)client) == 0) {
		uv_read_start((uv_stream_t *)client, AllocBuffer, Read);
	} else {
		uv_close((uv_handle_t *)client, nullptr);
	}
}

IoReactor::IoReactor(Player &player, CommandHandler &handler,
                     const std::string &address, const std::string &port)
    : player(player),
      handler(handler)
{
	InitAcceptor(address, port);
	DoUpdateTimer();
}

void IoReactor::Run()
{
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

void IoReactor::HandleCommand(const std::string &line)
{
	bool valid = this->handler.Handle(line);
	if (valid) {
		Respond(ResponseCode::OKAY, line);
	} else {
		Respond(ResponseCode::WHAT, MSG_CMD_INVALID);
	}
}

void UpdateTimerCallback(uv_timer_t *handle)
{
	Player *player = static_cast<Player *>(handle->data);
	player->Update();
}

void IoReactor::DoUpdateTimer()
{
	uv_timer_init(uv_default_loop(), &this->updater);
	this->updater.data = static_cast<void *>(&this->player);
	uv_timer_start(&this->updater, UpdateTimerCallback, 0, 20);
}

void IoReactor::InitAcceptor(const std::string &address,
                             const std::string &port)
{
	std::uint16_t uport = boost::lexical_cast<std::uint16_t>(port);

	uv_tcp_init(uv_default_loop(), &this->server);
	this->server.data = static_cast<void *>(this);

	struct sockaddr_in bind_addr;
	uv_ip4_addr(address.c_str(), uport, &bind_addr);
	uv_tcp_bind(&this->server, (const sockaddr *)&bind_addr, 0);

	int r = uv_listen((uv_stream_t *)&this->server, 128, OnNewConnection);
}

void IoReactor::RespondRaw(const std::string &string)
{
	// todo
	//this->manager.Send(string);
}

void IoReactor::End()
{
	uv_stop(uv_default_loop());
}