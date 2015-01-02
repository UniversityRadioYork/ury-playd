// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the PlayerPosition class.
 * @see player/player_position.cpp
 */

#ifndef PLAYD_PLAYER_POSITION_HPP
#define PLAYD_PLAYER_POSITION_HPP

#include <cstdint>
#include <set>

#include "../audio/audio_source.hpp"
#include "../io/io_response.hpp"

/**
 * Tracker and broadcaster for the Player's current position in a song.
 * @see Player
 */
class PlayerPosition : public ResponseSource
{
private:
	/// The period between each push of the position to the response sink.
	std::uint64_t period;

	/// The current position of the player.
	std::uint64_t current;

	/// The position of the player at the last push.
	std::uint64_t last;

	/**
	 * Whether the position was reset since the last push.
	 * This is also set high if the position has never been emitted.
	 */
	bool has_reset;

public:
	/**
	 * Constructs a PlayerPosition.
	 * @param time_sink The ResponseSink to which TIME responses are pushed.
	 * @param period The period to wait between responses.
	 */
	PlayerPosition(const ResponseSink *time_sink, std::uint64_t period);

	/**
	 * Updates the position tracker with the new position.
	 * This pushes the position to the registered response sink, if
	 * necessary.
	 * @param position The new position, in @a PositionUnit units.
	 * @see Reset
	 */
	void Update(std::uint64_t position);

	/**
	 * Resets the position tracker's position data.
	 * This does not remove any registered push response sink.
	 * Call this whenever the song changes, or before a skip.
	 * @see Update
	 */
	void Reset();

	/**
	 * Emits the current position to a ResponseSink.
	 * @param sink The ResponseSink to which a TIME response shall be
	 *   sent.
	 */
	void Emit(const ResponseSink &sink) const override;

private:
	/**
	 * Figures out whether it's time to send a position signal.
	 * @return True if enough time has elapsed for a signal to be sent;
	 *   false otherwise.
	 */
	bool IsReadyToSend() const;

	/**
	 * Sends a position signal to the outside environment, if ready.
	 * This only sends a signal if the requested amount of time has passed
	 * since the last one.
	 */
	void Send();
};

#endif // PLAYD_PLAYER_POSITION_HPP
