// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * The TimeParser template class.
 */

#ifndef PS_TIME_PARSER_HPP
#define PS_TIME_PARSER_HPP

#include <cstdint>
#include <sstream>

#include "errors.hpp"
#include "messages.h"

/**
 * A class for parsing time strings consisting of an integer and unit into
 * a flat integer representing the number of OutUnit units that time string
 * represents.
 */
template <typename OutUnit, typename IntType = std::uint64_t>
class TimeParser {
public:
	/**
	 * The type of the map from unit suffixes to multipliers that convert
	 * from microseconds to those units.
	 */
	using UnitMap = std::map<std::string, OutUnit>;

	/**
	 * Constructs a TimeParser.
	 * @param unit_map  The map from units to their parsers.
	 * @see   MkTime
	 */
	TimeParser(const UnitMap &unit_map) : unit_map(unit_map)
	{
	}

	/**
	 * Parses a time.
	 * @param time_str  The time string.
	 * @return          The time, as an instance of OutUnit.
	 */
	OutUnit Parse(const std::string &time_str) const
	{
		auto seek = Split(time_str);
		return unit_map.at(seek.first) * seek.second;
	}

private:
	UnitMap unit_map; ///< Map from units to handling functions.

	/**
	 * Splits a time string into a pair of unit and amount.
	 * @param time_str  The time string.
	 * @return          A pair of unit string and time amount, as an integer
	 *                  in terms of the named unit.
	 */
	std::pair<std::string, IntType> Split(const std::string &time_str) const
	{
		std::istringstream is(time_str);
		IntType raw_time;
		std::string rest;

		is >> raw_time;

		// Make sure we actually get a time of some form.
		if (is.fail()) {
			throw SeekError(MSG_SEEK_FAIL);
		}

		is >> rest;
		return std::make_pair(rest, raw_time);
	}
};

#endif // PS_TIME_PARSER_HPP
