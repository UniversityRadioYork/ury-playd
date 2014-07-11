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

#include "../contrib/stdin_stream.hpp"
#include "../cmd.hpp"
#include "../player/player.hpp"
#include "io_reactor_win.hpp"
#include <Windows.h>

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

void WinIoReactor::ResponseViaOstream(std::function<void(std::ostream &)> f)
{
	// There's no need for this to be asynchronous, so we just use iostream.
	f(std::cout);
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