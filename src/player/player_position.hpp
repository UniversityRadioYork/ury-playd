// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the PlayerPosition class.
 * @see player/player_position.cpp
 */

#ifndef PS_PLAYER_POSITION_HPP
#define PS_PLAYER_POSITION_HPP

#include <chrono>
#include <functional>
#include <set>

#include "../io/io_response.hpp"

/**
 * Tracker and broadcaster for the Player's current position in a song.
 * @see Player
 * @see PlayerState
 */
class PlayerPosition : public ResponseSource {
public:
	/// Unit used for positions.
	using Unit = std::chrono::microseconds;

private:
	/// The period between each push of the position to the response sink.
	Unit period;

	/// The current position of the player.
	Unit current;

	/// The position of the player at the last push.
	Unit last;

	/**
	 * Whether the position was reset since the last push.
	 * This is also set high if the position has never been emitted.
	 */
	bool has_reset;

public:
	/// Constructs a PlayerPosition.
	PlayerPosition();

	/**
	 * Sets the period between position signals.
	 * This is shared across all listeners.
	 * @param period The period to wait between responses.
	 * @see SetResponseSink
	 */
	void SetResponsePeriod(Unit period);

	/**
	 * Updates the position tracker with the new position.
	 * This pushes the position to the registered response sink, if
	 * necessary.
	 * @param position The new position, in @a PositionUnit units.
	 * @see Reset
	 */
	void Update(Unit position);

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
	void Emit(ResponseSink &sink) const override;

private:
	/**
	 * Figures out whether it's time to send a position signal.
	 * @return True if enough time has elapsed for a signal to be sent;
	 *   false otherwise.
	 */
	bool IsReadyToSend();

	/**
	 * Sends a position signal to the outside environment, if ready.
	 * This only sends a signal if the requested amount of time has passed
	 * since the last one.
	 */
	void Send();
};

#endif // PS_PLAYER_POSITION_HPP
