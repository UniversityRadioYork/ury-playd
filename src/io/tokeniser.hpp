// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
* @file
* Declaration of the Tokeniser class.
* @see io/tokeniser.cpp
*/

#ifndef PS_TOKENISER_HPP
#define PS_TOKENISER_HPP

#include "../cmd.hpp"
#include "io_response.hpp"

/**
 * A string tokeniser.
 *
 * A Tokeniser is fed chunks of incoming data from the IO system,
 * and emits any fully-formed command lines it encounters to the
 * command handler.
 */
class Tokeniser {
public:
	/**
	 * Constructs a new Tokeniser.
	 * @param handler The command handler to which full commands should be
	 *   sent.
	 * @param response_sink The response sink to which errors and
	 *   acknowledgements should be sent.
	 */
	Tokeniser(CommandHandler &handler, ResponseSink &response_sink);

	/**
	 * Feeds the contents of a character buffer into a Tokeniser.
	 * @param start The pointer to the start of the character buffer.
	 * @param end The pointer to the end of the character buffer.
	 */
	void Feed(const char *start, const char *end);

private:
	enum class QuoteType : std::uint8_t {
		NONE,
		SINGLE,
		DOUBLE
	};

	void Emit();
	void EndWord();
	void Push(char c);

	CommandHandler &handler;
	ResponseSink &response_sink;

	bool escape_next_character;
	QuoteType quote_type;

	std::vector<std::string> words;
	std::string current_word;
};

#endif // PS_TOKENISER_HPP
