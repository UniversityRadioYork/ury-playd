// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the TimeParser class.
 * @see time_parser.cpp
 */

#ifndef PLAYD_TIME_PARSER_HPP
#define PLAYD_TIME_PARSER_HPP

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
	static MicrosecondPosition Parse(const std::string &time_str);

private:
	// Conversion rates from various position units to microseconds.
	enum class Multiplier : MicrosecondPosition {
		MICROSECONDS = 1,    ///< Microseconds in one microsecond.
		MILLISECONDS = 1000, ///< Microseconds in one millisecond.
		SECONDS = 1000000,   ///< Microseconds in one second.
		MINUTES = 60000000,  ///< Microseconds in one minute.
		HOURS = 3600000000   ///< Microseconds in one hour.
	};

	/**
	 * Find the unit multiplier for a given unit suffix.
	 * The unit multiplier is the multiplier that converts from a time in
	 * @a unit units to a time in microseconds.
	 * @param unit The unit suffix.
	 * @return The unit multiplier.
	 * @throws std::out_of_range if the unit is not recognised.
	 */
	static Multiplier UnitMultiplier(const std::string &unit);

	/**
	 * Splits a time string into a pair of unit and amount.
	 * @param time_str  The time string.
	 * @return          A pair of unit string and time amount, as an integer
	 *                  in terms of the named unit.
	 */
	static std::pair<std::string, MicrosecondPosition> Split(
	                const std::string &time_str);
};

#endif // PLAYD_TIME_PARSER_HPP
