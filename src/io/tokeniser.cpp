// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Definition of the Tokeniser class.
 * @see io/tokeniser.hpp
 */

#include <algorithm>
#include <cctype>
#include <cstdint>

#include "../cmd.hpp"
#include "io_response.hpp"
#include "tokeniser.hpp"

Tokeniser::Tokeniser(CommandHandler &handler, ResponseSink &response_sink)
    : handler(handler),
      response_sink(response_sink),
      escape_next_character(false),
      quote_type(Tokeniser::QuoteType::NONE)
{
}

void Tokeniser::Feed(const char *start, const char *end)
{
	std::for_each(start, end, [this](char c) {
		if (this->escape_next_character) {
			this->escape_next_character = false;
			Push(c);
			return;
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
					default:
						if (isspace(c)) {
							EndWord();
						} else {
							Push(c);
						}
						break;
				}
				break;
		}
	});
}

void Tokeniser::Push(char c)
{
	this->current_word.push_back(c);
}

void Tokeniser::EndWord()
{
	// Ignore consecutive runs of whitespace.
	if (this->current_word.empty()) {
		return;
	}
	this->words.push_back(this->current_word);
	this->current_word.clear();
}

void Tokeniser::Emit()
{
	EndWord();

	bool valid = this->handler.Handle(words);
	if (valid) {
		this->response_sink.Respond(ResponseCode::OKAY, words[0]);
	} else {
		this->response_sink.Respond(ResponseCode::WHAT,
		                            MSG_CMD_INVALID);
	}

	this->words.clear();
}
