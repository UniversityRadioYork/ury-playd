// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Definition of the Tokeniser class.
 * @see io/tokeniser.hpp
 */

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>

#include "../cmd.hpp"
#include "io_response.hpp"
#include "tokeniser.hpp"

Tokeniser::Tokeniser()
    : escape_next_character(false),
      quote_type(Tokeniser::QuoteType::NONE)
{
}

Tokeniser::Lines Tokeniser::Feed(const std::string &raw_string)
{
	// The list of ready lines should be cleared by any previous Feed.
	assert(this->ready_lines.empty());

	for (unsigned char c : raw_string) {
		if (this->escape_next_character) {
			// TODO: Make this UTF-8 safe.
			Push(c);
			continue;
		}

		switch (this->quote_type) {
			case QuoteType::SINGLE:
				if (c == '\'') {
					this->quote_type = QuoteType::NONE;
				} else {
					Push(c);
				}
				break;

			case QuoteType::DOUBLE:
				switch (c) {
					case '\"':
						this->quote_type =
						                QuoteType::NONE;
						break;

					case '\\':
						this->escape_next_character =
						                true;
						break;

					default:
						Push(c);
						break;
				}
				break;

			case QuoteType::NONE:
				switch (c) {
					case '\n':
						Emit();
						break;

					case '\'':
						this->quote_type = QuoteType::
						                SINGLE;
						break;

					case '\"':
						this->quote_type = QuoteType::
						                DOUBLE;
						break;

					case '\\':
						this->escape_next_character =
						                true;
						break;

					default:
						isspace(c) ? EndWord()
						           : Push(c);
						break;
				}
				break;
		}
	}

	Lines lines = this->ready_lines;
	this->ready_lines.clear();

	// The ready line list must be empty, to establish the precondition
	// in the earlier assert().
	assert(this->ready_lines.empty());

	return lines;
}

void Tokeniser::Push(unsigned char c)
{
	assert(this->escape_next_character ||
	       !(this->quote_type == QuoteType::NONE && isspace(c)));
	this->current_word.push_back(c);
	this->escape_next_character = false;
	assert(!this->current_word.empty());
}

void Tokeniser::EndWord()
{
	// Ignore consecutive runs of whitespace.
	if (this->current_word.empty()) return;

	this->words.push_back(this->current_word);

	this->current_word.clear();
}

void Tokeniser::Emit()
{
	// Since we assume these, we don't need to set them later.
	assert(this->quote_type == QuoteType::NONE);
	assert(!this->escape_next_character);

	// We might still be in a word, in which case we treat the end of a
	// line as the end of the word too.
	EndWord();

	this->ready_lines.push_back(this->words);

	this->words.clear();

	// The state should now be clean and ready for another command.
	assert(this->quote_type == QuoteType::NONE);
	assert(!this->escape_next_character);
	assert(this->current_word.empty());
	assert(this->words.empty());
}
