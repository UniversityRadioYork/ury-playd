// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the StdIoReactor class.
 * @see io/io_reactor_std.cpp
 * @see io/io_reactor.hpp
 * @see io/io_reactor.cpp
 * @see io/io_reactor_asio.hpp
 * @see io/io_reactor_asio.cpp
 */

#ifndef PS_IO_REACTOR_STD_HPP
#define PS_IO_REACTOR_STD_HPP

#include <functional>     // std::function
#include <ostream>        // std::ostream
#include "io_reactor.hpp" // IoReactor

class Player;
class CommandHandler;

/**
 * An IoReactor using stdin and stdout.
 *
 * The StdIoReactor listens for commands on standard input, using select()
 * on POSIX systems (or similar analogues on other platforms) to detect when
 * lines of input are waiting.  Responses are sent on standard output.
 *
 * When not consuming input, the StdIoReactor continuously pumps the Player to
 * make it decode and update its internal state.  This is simplistic and perhaps
 * inefficient, but works ok in practice.
 *
 * StdIoReactor is lightweight, simple, robust, and portable across POSIX, and
 * is the best IoReactor for debugging Playslave.  However, it does not work
 * well with other OSen (Windows, for instance), and requires elaborate process
 * acrobatics to hook up Playslave to a master program written in a low-level
 * language such as C/C++.  It also more or less forces Playslave to be launched
 * and owned by one master process.
 *
 * StdIoReactor does not require any resources (ports, files, etc).
 *
 * @see IoReactor
 * @see AsioIoReactor
 */
class StdIoReactor : public IoReactor {
public:
	/**
	 * Constructs a StdIoReactor.
	 * @param player The player to which periodic update requests shall be
	 *   sent.
	 * @param handler The handler to which command inputs shall be sent.
	 */
	StdIoReactor(Player &player, CommandHandler &handler);

	void End() override;

protected:
	void ResponseViaOstream(std::function<void(std::ostream &)> f) override;
	void MainLoop() override;

private:
	void CheckInput();
	bool InputWaiting();

	bool running; ///< Whether the IO routine should be running.
};

#endif // PS_IO_REACTOR_STD_HPP
