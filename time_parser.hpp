#ifndef PS_TIME_PARSER_HPP
#define PS_TIME_PARSER_HPP

#include <chrono>
#include <cstdint>
#include <sstream>

/**
 * A class for parsing time strings consisting of an integer and unit into
 * a flat integer representing the number of OutUnit units that time string
 * represents.
 */
template <typename OutUnit, typename IntType = std::uint64_t>
class TimeParser {
public:
	/**
	 * The type of the map from unit suffixes to functions that convert
	 * from an amount of that time unit to a chrono duration of type OutUnit
	 * capturing the same duration.
	 */
	using UnitMap = std::map<std::string, std::function<OutUnit(IntType)>>;

	/**
	 * Constructs a TimeParser.
	 * @param unit_map  The map from units to their parsers.
	 * @see   MkTime
	 */
	TimeParser(const UnitMap &unit_map) : unit_map(unit_map)
	{
	}

	/**
	 * A template for converting an IntType representation of a duration of
	 * InUnit units into a duration expressed as an OutUnit.
	 *
	 * For example, MkTime<std::chrono::seconds> takes in an IntType of
	 * seconds and returns a std::chrono::microseconds: applying this to
	 * the integer 1 will result in a microseconds variable representing
	 * 1,000,000 microseconds.
	 *
	 * @param raw_time The raw time, as an integer whose units are those
	 *   used by InUnit.
	 * @return An OutUnit capturing the input duration, but in terms of the
	 *   units used by OutUnit.
	 */
	template <typename InUnit>
	static OutUnit MkTime(IntType raw_time)
	{
		return std::chrono::duration_cast<OutUnit>(InUnit(raw_time));
	}

	/**
	 * Parses a time, returning it as a std::chrono duration.
	 * @param time_str  The time string.
	 * @return          The time, as an instance of OutUnit.
	 */
	OutUnit Parse(const std::string &time_str) const
	{
                auto seek = Split(time_str);
		return unit_map.at(seek.first)(seek.second);
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

		is >> raw_time >> rest;
		return std::make_pair(rest, raw_time);
	}
};

#endif // PS_TIME_PARSER_HPP
