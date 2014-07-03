// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declarations of input/output related code.
 * @see io.cpp
 */

#ifndef PS_IO_HPP
#define PS_IO_HPP

#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>

class CommandHandler;
class Error;
class Player;

/**
 * Four-character response codes.
 * @note If you're adding new responses here, update RESPONSES.
 * @see RESPONSES
 */
enum class Response {
	/* 'Pull' responses (initiated by client command) */
	OKAY, /* Request was valid and produced an answer */
	WHAT, /* Request was invalid/user error */
	FAIL, /* Error, pointing blame at environment */
	OOPS, /* Error, pointing blame at programmer */
	NOPE, /* Request was probably valid, but forbidden. */
	/* 'Push' responses (initiated by server) */
	OHAI, /* Server starting up */
	TTFN, /* Server shutting down */
	STAT, /* Server changing state */
	TIME, /* Server sending current song time */
	DBUG, /* Debug information */
	/* Queue-specific responses */
	QENT, /* Requested information about a Queue ENTry */
	QMOD, /* A command caused a Queue MODification */
	QPOS, /* The current Queue POSition has changed */
	QNUM  /* Reminder of current number of queue items */
};

/**
 * A map from Response codes to their string equivalents.
 * @see Response
 */
extern const std::map<Response, std::string> RESPONSES;

/**
 * Abstract class for anything that can be sent a response.
 */
class Responder {
public:
	/**
	 * Base case for the RespondArgs template, for when there are no
	 * arguments.
	 * @param stream The stream onto which the response body shall be
	 *   output.
	 */
	inline void RespondArgs(std::ostream &)
	{
	}

	/**
	 * Outputs a response body, with a variadic number of arguments.
	 * This is defined inductively, with RespondArgs() being the base case.
	 * @tparam Arg1 The type of the leftmost argument.
	 * @tparam Args Parameter pack of remaining arguments.
	 * @param stream The stream onto which the response body shall be
	 *   output.
	 * @param arg1 The leftmost argument.
	 * @param args The remaining arguments.
	 */
	template <typename Arg1, typename... Args>
	inline void RespondArgs(std::ostream &stream, const Arg1 &arg1,
	                        const Args &... args)
	{
		stream << " " << arg1;
		RespondArgs(stream, args...);
	}

	/**
	 * Base case for the Respond template, for when there are no arguments.
	 * @param code The response code to emit.
	 */
	inline void Respond(Response code)
	{
		ResponseViaOstream([&](std::ostream &stream) {
			stream << RESPONSES.at(code) << std::endl;
		});
	}

	/**
	 * Outputs a response, with a variadic number of arguments.
	 * This is defined on RespondArgs.
	 * @tparam Args Parameter pack of arguments.
	 * @param code The response code to emit.
	 * @param args The arguments, if any.
	 */
	template <typename... Args>
	inline void Respond(Response code, Args &... args)
	{
		ResponseViaOstream([&](std::ostream &stream) {
			stream << RESPONSES.at(code);
			RespondArgs(stream, args...);
			stream << std::endl;
		});
	}

	/**
	 * Emits an error as a response.
	 * @param error The error to convert to a response.
	 */
	void RespondWithError(const Error &error);

protected:
	/**
	 * Provides an ostream for building a response.
	 * The ostream is provided through a lambda, and the response will be
	 * sent
	 * either during or after the lambda's lifetime.
	 * @param f The lambda to invoke with the ostream.
	 */
	virtual void ResponseViaOstream(
	                std::function<void(std::ostream &)> f) = 0;
};

/**
 * The reactor, which services input, routes responses, and executes the
 * Player update routine periodically.
 */
class IoReactor : public Responder {
public:
	/**
	 * Constructs an IoReactor.
	 * @param player The player to which periodic update requests shall be
	 *   sent.
	 * @param handler The handler to which command inputs shall be sent.
	 */
	IoReactor(Player &player, CommandHandler &handler);

	/**
	 * Runs the reactor.
	 * It will block until terminated.
	 */
	void Run();

	/**
	 * Ends the reactor.
	 * This should be called by the parent object when the player is
	 * quitting.
	 */
	virtual void End() = 0;

protected:
	Player &player;          ///< The player.
	CommandHandler &handler; ///< The command handler.

	/**
	 * Sends a command to the command handler.
	 * The result of the command is responded on as per the playslave API.
	 * @param line The command line received by the IO reactor.
	 */
	void HandleCommand(const std::string &line);

	/**
	 * The reactor's main loop.
	 * It will block until a quit command is received.
	 * @return The exit code of the main loop.
	 */
	virtual void MainLoop() = 0;
};

/**
 * An IoReactor using stdin and stdout.
 */
class StdIoReactor : public IoReactor {
public:
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

class TcpConnectionManager;

/**
 * A connection using the TCP server.
 */
class TcpConnection : public std::enable_shared_from_this<TcpConnection>,
                      public Responder {
public:
	/// A shared pointer to a TcpConnection.
	using Pointer = std::shared_ptr<TcpConnection>;

	/**
	 * Creates a new TcpConnection.
	 * @param io_service The IO service to be used for this connection.
	 * @return A TcpConnection.
	 */
	explicit TcpConnection(std::function<void(const std::string &)> cmd,
	                       TcpConnectionManager &manager,
	                       boost::asio::io_service &io_service);

	TcpConnection(const TcpConnection &) = delete;
	TcpConnection &operator=(const TcpConnection &) = delete;

	void Start();

	void Stop();

	void Send(const std::string &string);

	boost::asio::ip::tcp::socket &Socket();

protected:
	void ResponseViaOstream(std::function<void(std::ostream &)> f) override;

private:
	void DoRead();
	void DoWrite();

	boost::asio::ip::tcp::socket socket;
	boost::asio::streambuf data;
	boost::asio::io_service::strand strand;
	std::deque<std::string> outbox;
	std::function<void(const std::string &)> cmd;
	TcpConnectionManager &manager;
};

/**
 * A manager for TcpConnection objects.
 */
class TcpConnectionManager {
public:
	TcpConnectionManager(const TcpConnectionManager &) = delete;
	TcpConnectionManager &operator=(const TcpConnectionManager &) = delete;

	explicit TcpConnectionManager();
	void Start(TcpConnection::Pointer c);
	void Stop(TcpConnection::Pointer c);
	void StopAll();
	void Send(const std::string &string);

private:
	std::set<TcpConnection::Pointer> connections;
};
/**
 * An IoReactor using boost::asio.
 */
class AsioIoReactor : public IoReactor {
public:
	/**
	 * Constructs an AsioIoReactor.
	 * @param player The player to which periodic update requests shall be
	 *   sent.
	 * @param handler The handler to which command inputs shall be sent.
	 * @param port The port on which AsioIoReactor will listen for clients.
	 */
	explicit AsioIoReactor(Player &player, CommandHandler &handler,
	                       std::string address, std::string port);

	AsioIoReactor(const AsioIoReactor &) = delete;
	AsioIoReactor &operator=(const AsioIoReactor &) = delete;

	void End() override;

protected:
	void ResponseViaOstream(std::function<void(std::ostream &)> f) override;

private:
	void MainLoop() override;

	void DoAccept();

	void DoAwaitStop();

	void DoUpdateTimer();

	/// The IO service used by the reactor.
	boost::asio::io_service io_service;

	/// The signal set used to shut the server down on terminations.
	boost::asio::signal_set signals;

	/// The acceptor used to listen for incoming connections.
	boost::asio::ip::tcp::acceptor acceptor;

	/// The object responsible for managing live connections.
	TcpConnectionManager manager;
};

#endif // PS_IO_HPP
