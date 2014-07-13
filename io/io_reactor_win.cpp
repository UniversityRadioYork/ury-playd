// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the WinIoReactor class.
 * @see io/io_reactor_win.hpp
 * @see io/io_reactor.hpp
 * @see io/io_reactor.cpp
 * @see io/io_reactor_posix.hpp
 * @see io/io_reactor_posix.cpp
 * @see io/io_reactor_tcp.hpp
 * @see io/io_reactor_tcp.cpp
 */

#ifdef _WIN32

#include <iostream>                    // std::cout
#include <string>                      // std::string
#include "../contrib/stdin_stream.hpp" // stdin_stream
#include "../cmd.hpp"                  // CommandHandler
#include "../player/player.hpp"        // Player
#include "io_reactor_win.hpp"          // WinIoReactor
#include <Windows.h>                   // Windows API

WinIoReactor::WinIoReactor(Player &player, CommandHandler &handler)
    : IoReactor(player, handler),
      input_handle(GetStdHandle(STD_INPUT_HANDLE)),
      input(io_service, input_handle)
{
	if (this->input_handle == INVALID_HANDLE_VALUE) {
		throw new InternalError("Couldn't get input console handle");
	}

	SetupWaitForInput();
}

void WinIoReactor::RespondRaw(const std::string &string)
{
	// There's no need for this to be asynchronous, so we just use iostream.
        std::cout << string << std::flush;
}

void WinIoReactor::SetupWaitForInput()
{
	boost::asio::async_read_until(
	                input, data, "\r\n",
	                [this](const boost::system::error_code &ec,
	                       std::size_t) {
		                if (!ec) {
			                std::istream is(&data);
			                std::string s;
			                std::getline(is, s);

			                // Windows uses CRLF endings, but
			                // std::getline can ignore the CR.
			                // Let's fix that in an awful way
			                // fitting of Windows's awfulness.
			                if (s.back() == '\r') {
				                s.pop_back();
			                }

			                HandleCommand(s);

			                SetupWaitForInput();
		                }
		        });
}

#endif // _WIN32
