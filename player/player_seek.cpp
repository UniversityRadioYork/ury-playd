#include "player.hpp"

#include <chrono>
#include <cstdint>
#include <sstream>
#include <string>

bool Player::Seek(const std::string &time_str)
{
	return IfCurrentStateIn({State::PLAYING, State::STOPPED},
	                        [this, &time_str] {
		bool success = true;
		std::chrono::microseconds position(0);

		try
		{
			position = this->time_parser.Parse(time_str);
		}
		catch (std::out_of_range)
		{
			success = false;
		}

		if (success) {
			this->audio->SeekToPosition(position);
			this->ResetPosition();
			this->UpdatePosition();
		}

		return success;
	});
}
