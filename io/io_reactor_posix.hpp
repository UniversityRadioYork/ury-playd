// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the PosixIoReactor class.
 * @see io/io_reactor_posix.cpp
 * @see io/io_reactor.hpp
 * @see io/io_reactor.cpp
 * @see io/io_reactor_tcp.hpp
 * @see io/io_reactor_tcp.cpp
 * @see io/io_reactor_win.hpp
 * @see io/io_reactor_win.cpp
 */

#ifndef PS_IO_REACTOR_POSIX_HPP
#define PS_IO_REACTOR_POSIX_HPP

#include <boost/asio.hpp>

#ifdef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR

#include <functional>                  // std::function
#include <ostream>                     // std::ostream
#include "io_reactor.hpp"              // IoReactor

/**
 * An IoReactor using POSIX streams.
 *
 * The WinIoReactor allows Playslave to take input from, and send output to,
 * standard input/output on a POSIX-compliant OS.
 *
 * @see IoReactor
 * @see IoReactor
 * @see TcpIoReactor
 * @see WinIoReactor
 */
class PosixIoReactor : public IoReactor {
public:
	/**
	* Constructs an PosixIoReactor.
	* @param player The player to which periodic update requests shall be
	*   sent.
	* @param handler The handler to which command inputs shall be sent.
	*/
	explicit PosixIoReactor(Player &player, CommandHandler &handler);

	/// Deleted copy constructor.
	PosixIoReactor(const PosixIoReactor &) = delete;

	/// Deleted copy-assignment.
	PosixIoReactor &operator=(const PosixIoReactor &) = delete;

protected:
	void ResponseViaOstream(std::function<void(std::ostream &)> f) override;

private:
	void SetupWaitForInput();

	/// The ASIO wrapper for POSIX stdin.
	boost::asio::posix::stream_descriptor input;

	/// The data buffer for the console input stream.
	boost::asio::streambuf data;
};

#endif // BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR

#endif // PS_IO_REACTOR_POSIX_HPP
