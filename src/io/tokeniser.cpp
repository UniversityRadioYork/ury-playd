// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

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
    : escape_next(false), quote_type(Tokeniser::QuoteType::NONE)
{
}

std::vector<Tokeniser::Line> Tokeniser::Feed(const std::string &raw_string)
{
	// The list of ready lines should be cleared by any previous Feed.
	assert(this->ready_lines.empty());

	for (unsigned char c : raw_string) {
		if (this->escape_next) {
			this->Push(c);
			continue;
		}

		switch (this->quote_type) {
			case QuoteType::SINGLE:
				if (c == '\'') {
					this->quote_type = QuoteType::NONE;
				}
				else {
					this->Push(c);
				}
				break;

			case QuoteType::DOUBLE:
				switch (c) {
					case '\"':
						this->quote_type =
						                QuoteType::NONE;
						break;

					case '\\':
						this->escape_next =
						                true;
						break;

					default:
						this->Push(c);
						break;
				}
				break;

			case QuoteType::NONE:
				switch (c) {
					case '\n':
						this->Emit();
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
						this->escape_next =
						                true;
						break;

					default:
						isspace(c) ? this->EndWord()
						           : this->Push(c);
						break;
				}
				break;
		}
	}

	std::vector<Line> lines = this->ready_lines;
	this->ready_lines.clear();

	return lines;
}

void Tokeniser::Push(unsigned char c)
{
	assert(this->escape_next ||
	       !(this->quote_type == QuoteType::NONE && isspace(c)));
	this->current_word.push_back(c);
	this->escape_next = false;
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
	assert(!this->escape_next);

	// We might still be in a word, in which case we treat the end of a
	// line as the end of the word too.
	this->EndWord();

	this->ready_lines.push_back(this->words);

	this->words.clear();

	// The state should now be clean and ready for another command.
	assert(this->quote_type == QuoteType::NONE);
	assert(!this->escape_next);
	assert(this->current_word.empty());
}
