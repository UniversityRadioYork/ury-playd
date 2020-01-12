// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Definition of the Tokeniser class.
 * @see tokeniser.h
 */

#include <locale>
#include <gsl/gsl>

#include "response.h"

#include "tokeniser.h"

namespace Playd {

    Tokeniser::Tokeniser()
            : escape_next{false}, in_word{false}, quote_type{QuoteType::NONE} {
    }

    std::vector<std::vector<std::string>> Tokeniser::Feed(const std::string &raw) {
        // The list of ready lines should be cleared by any previous Feed.
        Expects(this->ready_lines.empty());

        for (const char c : raw) {
            if (this->escape_next) {
                this->Push(c);
                continue;
            }

            switch (this->quote_type) {
                case QuoteType::SINGLE:
                    FeedSingleQuotedChar(c);
                    break;

                case QuoteType::DOUBLE:
                    FeedDoublyQuotedChar(c);
                    break;

                case QuoteType::NONE:
                    FeedUnquotedChar(c);
                    break;
            }
        }

        auto lines = this->ready_lines;
        this->ready_lines.clear();

        return lines;
    }

    void Tokeniser::FeedUnquotedChar(char c) {
        switch (c) {
            case '\n':
                Emit();
                break;

            case '\'':
                in_word = true;
                quote_type =
                        QuoteType::SINGLE;
                break;

            case '\"':
                in_word = true;
                quote_type =
                        QuoteType::DOUBLE;
                break;

            case '\\':
                escape_next = true;
                break;

            default:
                isspace(c, std::locale::classic())
                ? EndWord()
                : Push(c);
                break;
        }
    }

    void Tokeniser::FeedDoublyQuotedChar(char c) {
        switch (c) {
            case '\"':
                quote_type =
                        QuoteType::NONE;
                break;

            case '\\':
                escape_next = true;
                break;

            default:
                Push(c);
                break;
        }
    }

    void Tokeniser::FeedSingleQuotedChar(char c) {
        if (c == '\'') {
            quote_type = QuoteType::NONE;
        } else {
            Push(c);
        }
    }

    void Tokeniser::Push(const char c) {
        Expects(this->escape_next || !(this->quote_type == QuoteType::NONE &&
                                       isspace(c, std::locale::classic())));
        this->in_word = true;
        this->current_word.push_back(c);
        this->escape_next = false;

        Ensures(!this->current_word.empty());
    }

    void Tokeniser::EndWord() {
        // Don't add a word unless we're in one.
        if (!this->in_word) return;
        this->in_word = false;

        this->words.push_back(this->current_word);

        this->current_word.clear();
    }

    void Tokeniser::Emit() {
        // Since we assume these, we don't need to set them later.
        Expects(this->quote_type == QuoteType::NONE);
        Expects(!this->escape_next);

        // We might still be in a word, in which case we treat the end of a
        // line as the end of the word too.
        this->EndWord();

        this->ready_lines.push_back(this->words);

        this->words.clear();

        // The state should now be clean and ready for another command.
        Ensures(this->quote_type == QuoteType::NONE);
        Ensures(!this->escape_next);
        Ensures(this->current_word.empty());
    }

} // namespace Playd
