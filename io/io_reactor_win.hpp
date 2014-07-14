// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the WinIoReactor class.
 * @see io/io_reactor_win.cpp
 * @see io/io_reactor.hpp
 * @see io/io_reactor.cpp
 * @see io/io_reactor_posix.hpp
 * @see io/io_reactor_posix.cpp
 * @see io/io_reactor_tcp.hpp
 * @see io/io_reactor_tcp.cpp
 */

#ifndef PS_IO_REACTOR_WIN_HPP
#define PS_IO_REACTOR_WIN_HPP

#ifdef _WIN32

#include <functional>                  // std::function
#include <ostream>                     // std::ostream
#include <Windows.h>                   // HANDLE
#include "../contrib/stdin_stream.hpp" // stdin_stream
#include "io_reactor.hpp"              // IoReactor

class Player;
class CommandHandler;

/**
 * An IoReactor using standard input/output.
 *
 * The WinIoReactor allows Playslave to take input from, and send output to,
 * a Windows console or POSIX .
 *
 * @see IoReactor
 * @see TcpIoReactor
 * @see PosixIoReactor
 */
class WinIoReactor : public IoReactor {
public:
	/**
	 * Constructs a WinIoReactor.
	 * @param player The player to which periodic update requests shall be
	 *   sent.
	 * @param handler The handler to which command inputs shall be sent.
	 */
	explicit WinIoReactor(Player &player, CommandHandler &handler);

	/// Deleted copy constructor.
	WinIoReactor(const WinIoReactor &) = delete;

	/// Deleted copy-assignment.
	WinIoReactor &operator=(const WinIoReactor &) = delete;

protected:
	void RespondRaw(const std::string &string) override;

private:
	void SetupWaitForInput();

	/// The handle pointing to the console input.
	HANDLE input_handle;

	/// The ASIO wrapper for the console input handle.
	stdin_stream input;

	/// The data buffer for the console input stream.
	boost::asio::streambuf data;
};

#endif // _WIN32

#endif // PS_IO_REACTOR_ASIO_HPP
