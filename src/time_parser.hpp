// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the TimeParser class.
 * @see time_parser.cpp
 */

#ifndef PS_TIME_PARSER_HPP
#define PS_TIME_PARSER_HPP

#include <cstdint>

/**
 * A class for parsing time strings consisting of an integer and unit into
 * a flat integer representing the number of OutUnit units that time string
 * represents.
 */
class TimeParser {
public:
	/// Type for microsecond positions emitted by this TimeParser.
	typedef std::uint64_t MicrosecondPosition;

	/**
	 * Parses a time.
	 * @param time_str  The time string.
	 * @return          The parsed time.
	 */
	MicrosecondPosition Parse(const std::string &time_str) const;

private:
	// Conversion rates from various position units to microseconds.
	enum class Multiplier : MicrosecondPosition {
		US = 1,  /// Microseconds in one microsecond.
		MS = 1000,  /// Microseconds in one millisecond.
		S = 1000000,  /// Microseconds in one second.
		M = 60000000,  /// Microseconds in one minute.
		H = 3600000000  /// Microseconds in one hour.
	};

	/**
	 * Find the unit multiplier for a given unit suffix.
	 * The unit multiplier is the multiplier that converts from a time in
	 * @a unit units to a time in microseconds.
	 * @param unit The unit suffix.
	 * @return The unit multiplier.
	 * @throws std::out_of_range if the unit is not recognised.
	 */
	Multiplier UnitMultiplier(const std::string &unit) const;

	/**
	 * Determines whether a unit suffix is in a list of unit suffixes.
	 * @param unit The unit suffix to check against @a list.
	 * @param list The list of unit suffixes.
	 * @return True if @a unit is in @a list; false otherwise.
	 */
	bool IsIn(const std::string &unit, std::initializer_list<std::string> list) const;

	/**
	 * Splits a time string into a pair of unit and amount.
	 * @param time_str  The time string.
	 * @return          A pair of unit string and time amount, as an integer
	 *                  in terms of the named unit.
	 */
	std::pair<std::string, MicrosecondPosition> Split(const std::string &time_str) const;
};

#endif // PS_TIME_PARSER_HPP
