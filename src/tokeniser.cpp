// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Definition of the Tokeniser class.
 * @see tokeniser.h
 */

#include <algorithm>
#include <cassert>
#include <locale>

#include "response.h"

#include "tokeniser.h"

Tokeniser::Tokeniser()
    : escape_next(false), in_word(false), quote_type(QuoteType::NONE)
{
}

std::vector<std::vector<std::string>> Tokeniser::Feed(const std::string &raw)
{
	// The list of ready lines should be cleared by any previous Feed.
	assert(this->ready_lines.empty());

	for (char c : raw) {
		if (this->escape_next) {
			this->Push(c);
			continue;
		}

		switch (this->quote_type) {
			case QuoteType::SINGLE:
				if (c == '\'') {
					this->quote_type = QuoteType::NONE;
				} else {
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
						this->escape_next = true;
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
						this->in_word = true;
						this->quote_type =
						        QuoteType::SINGLE;
						break;

					case '\"':
						this->in_word = true;
						this->quote_type =
						        QuoteType::DOUBLE;
						break;

					case '\\':
						this->escape_next = true;
						break;

					default:
						isspace(c, std::locale::classic())
						        ? this->EndWord()
						        : this->Push(c);
						break;
				}
				break;
		}
	}

	auto lines = this->ready_lines;
	this->ready_lines.clear();

	return lines;
}

void Tokeniser::Push(const char c)
{
	assert(this->escape_next || !(this->quote_type == QuoteType::NONE &&
	                              isspace(c, std::locale::classic())));
	this->in_word = true;
	this->current_word.push_back(c);
	this->escape_next = false;
	assert(!this->current_word.empty());
}

void Tokeniser::EndWord()
{
	// Don't add a word unless we're in one.
	if (!this->in_word) return;
	this->in_word = false;

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
