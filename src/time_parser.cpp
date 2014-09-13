// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the TimeParser class.
 * @see time_parser.hpp
 */

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "errors.hpp"
#include "messages.h"
#include "time_parser.hpp"

TimeParser::Time TimeParser::Parse(const std::string &time_str) const
{
	auto seek = Split(time_str);
	std::string unit = seek.first;
	Time num_units = seek.second;

	return static_cast<Time>(UnitMultiplier(unit)) * num_units;
}

TimeParser::Multiplier TimeParser::UnitMultiplier(const std::string &unit) const
{
	// Default to microseconds if no unit is given.
	if (unit.empty()) return Multiplier::US;

	if (IsIn(unit, {"us", "usec", "usecs"})) return Multiplier::US;
	if (IsIn(unit, {"ms", "msec", "msecs"})) return Multiplier::MS;
	if (IsIn(unit, {"s", "sec", "secs"})) return Multiplier::S;
	if (IsIn(unit, {"m", "min", "mins"})) return Multiplier::M;
	if (IsIn(unit, {"h", "hour", "hours"})) return Multiplier::H;

	throw std::out_of_range(unit);
}

bool TimeParser::IsIn(const std::string &unit, std::initializer_list<std::string> list) const
{
	return std::find(list.begin(), list.end(), unit) != list.end();
}

std::pair<std::string, TimeParser::Time> TimeParser::Split(const std::string &time_str) const
{
	std::istringstream is(time_str);
	Time raw_time;
	std::string rest;

	is >> raw_time;

	// Make sure we actually get a time of some form.
	if (is.fail()) throw SeekError(MSG_SEEK_FAIL);

	is >> rest;
	return std::make_pair(rest, raw_time);
}
