// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the Tokeniser class.
 * @see tokeniser.cpp
 */

#ifndef PLAYD_TOKENISER_HPP
#define PLAYD_TOKENISER_HPP

#include "response.hpp"

/**
 * A string tokeniser.
 *
 * A Tokeniser is fed chunks of incoming data from the IO system, and emits any
 * fully-formed command lines it encounters to the command handler.
 *
 * @see CommandHandler
 * @see IoCore
 */
class Tokeniser
{
public:
	/// Constructs a new Tokeniser.
	Tokeniser();

	/**
	 * Feeds a string into a Tokeniser.
	 * @param raw Const reference to the raw string to feed.  The string
	 *   need not contain complete lines.
	 * @return The vector of lines that have been successfully tokenised in
	 *   this tokenising pass.  This vector may be empty.
	 * @note Escaping a multi-byte UTF-8 character is undefined behaviour.
	 */
	std::vector<std::vector<std::string>> Feed(const std::string &raw);

private:
	/// Enumeration of quotation types.
	enum class QuoteType : std::uint8_t {
		NONE,   ///< Not currently in a quote pair.
		SINGLE, ///< In single quotes ('').
		DOUBLE  ///< In double quotes ("").
	};

	/// The current vector of completed, tokenised lines.
	/// This is cleared at the end of every Tokeniser::Feed.
	std::vector<std::vector<std::string>> ready_lines;

	/// The current vector of completed, tokenised words.
	std::vector<std::string> words;

	/// The current, incomplete word to which new characters should be
	/// added.
	std::string current_word;

	/// Whether the next character is to be interpreted as an escape code.
	/// This usually gets set to true when a backslash is detected.
	bool escape_next = false;

	/// Whether the tokeniser is currently in a word.
	bool in_word = false;

	/// The type of quotation currently being used in this Tokeniser.
	QuoteType quote_type;

	/// Finishes the current word and sends the line to the CommandHandler.
	void Emit();

	/// Finishes the current word, adding it to the tokenised line.
	void EndWord();

	/**
	 * Pushes a raw character onto the end of the current word.
	 * This also clears the escape_next flag.
	 * @param c The character to push onto the current word.
	 */
	void Push(char c);
};

#endif // PLAYD_TOKENISER_HPP
