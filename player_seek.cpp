#include "player.hpp"
#include "cmd.hpp"

#include <chrono>
#include <cstdint>
#include <sstream>
#include <string>

/**
 * A template for converting a uint64_t giving a duration in terms of T1 into
 * a duration expressed as a T2.
 *
 * For example, MkTime<std::chrono::seconds> takes in a uint64_t of seconds
 * and returns a std::chrono::microseconds.
 */
template <typename T1, typename T2 = std::chrono::microseconds>
T2 MkTime(std::uint64_t raw_time)
{
	return std::chrono::duration_cast<T2>(T1(raw_time));
}

typedef std::map<std::string,
                 std::function<std::chrono::microseconds(uint64_t)>>
                TimeSuffixMap;

/**
 * Mapping from unit suffixes for seek command times to functions converting
 * time integers to the appropriate duration in microseconds.
 */
static const TimeSuffixMap time_suffixes = {
                {"s", MkTime<std::chrono::seconds>},
                {"sec", MkTime<std::chrono::seconds>},
                {"secs", MkTime<std::chrono::seconds>},
                {"m", MkTime<std::chrono::minutes>},
                {"min", MkTime<std::chrono::minutes>},
                {"mins", MkTime<std::chrono::minutes>},
                {"h", MkTime<std::chrono::hours>},
                {"hour", MkTime<std::chrono::hours>},
                {"hours", MkTime<std::chrono::hours>},
                // Default when there is no unit
                {"", MkTime<std::chrono::microseconds>}};

std::pair<std::string, uint64_t> Player::ParseSeekTime(
                const std::string &time_str) const
{
	std::istringstream is(time_str);
	uint64_t raw_time;
	std::string rest;

	is >> raw_time >> rest;
	return std::make_pair(rest, raw_time);
}

CommandHandler::SingleRequiredWordAction Player::SeekAction()
{
	return [this] (std::string &time_str) { return this->Seek(time_str); };
}

bool Player::Seek(const std::string &time_str)
{
	return IfCurrentStateIn({State::PLAYING, State::STOPPED},
	                        [this, &time_str] {
		bool success = true;

		auto seek = ParseSeekTime(time_str);
		std::chrono::microseconds position(0);

		try
		{
			position = time_suffixes.at(seek.first)(seek.second);
		}
		catch (std::out_of_range)
		{
			success = false;
		}

		if (success) {
			this->au->SeekToPosition(position);
			this->position_last_invalid = true;
		}

		return success;
	});
}
