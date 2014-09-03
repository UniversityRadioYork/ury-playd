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
 * A Tokeniser is fed chunks of incoming data from the IO system, and emits any
 * fully-formed command lines it encounters to the command handler.
 *
 * @see CommandHandler
 * @see IoReactor
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
	 * @param nread The number of elements in the character buffer.
	 */
	void Feed(const char *start, unsigned int nread);

private:
	/// Enumeration of quotation types.
	enum class QuoteType : std::uint8_t {
		NONE,   ///< Not currently in a quote pair.
		SINGLE, ///< In single quotes ('').
		DOUBLE  ///< In double quotes ("").
	};

	/// The command handler to which complete lines should be sent.
	CommandHandler &handler;

	/// The response sink to which command acknowledgements should be sent.
	/// This includes OKAY (successful command) and WHAT (bad command).
	ResponseSink &response_sink;

	/// Whether the next character is to be interpreted as an escape code.
	/// This usually gets set to true when a backslash is detected.
	bool escape_next_character;

	/// The type of quotation currently being used in this Tokeniser.
	QuoteType quote_type;

	/// The current vector of completed, tokenised words.
	std::vector<std::string> words;

	/// The current, incomplete word to which new characters should be
	/// added.
	std::string current_word;

	/// Finishes the current word and sends the line to the CommandHandler.
	void Emit();

	/// Finishes the current word, adding it to the tokenised line.
	void EndWord();

	/**
	 * Pushes a raw character onto the end of the current word.
	 *
	 * This also clears the escape_next_character flag.
	 *
	 * @param c The character to push onto the current word.
	 */
	void Push(unsigned char c);
};

#endif // PS_TOKENISER_HPP
