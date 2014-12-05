// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the PlayerPosition class.
 * @see player/player_position.cpp
 */

#ifndef PLAYD_PLAYER_POSITION_HPP
#define PLAYD_PLAYER_POSITION_HPP

#include <set>

#include "../audio/audio_source.hpp"
#include "../io/io_response.hpp"
#include "../time_parser.hpp"

/**
 * Tracker and broadcaster for the Player's current position in a song.
 * @see Player
 * @see PlayerState
 */
class PlayerPosition : public ResponseSource
{
private:
	/// The period between each push of the position to the response sink.
	TimeParser::MicrosecondPosition period;

	/// The current position of the player.
	TimeParser::MicrosecondPosition current;

	/// The position of the player at the last push.
	TimeParser::MicrosecondPosition last;

	/**
	 * Whether the position was reset since the last push.
	 * This is also set high if the position has never been emitted.
	 */
	bool has_reset;

public:
	/**
	 * Constructs a PlayerPosition.
	 * @param period The period to wait between responses.
	 */
	PlayerPosition(TimeParser::MicrosecondPosition period);

	/**
	 * Updates the position tracker with the new position.
	 * This pushes the position to the registered response sink, if
	 * necessary.
	 * @param position The new position, in @a PositionUnit units.
	 * @see Reset
	 */
	void Update(TimeParser::MicrosecondPosition position);

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
	bool IsReadyToSend() const;

	/**
	 * Sends a position signal to the outside environment, if ready.
	 * This only sends a signal if the requested amount of time has passed
	 * since the last one.
	 */
	void Send();
};

#endif // PLAYD_PLAYER_POSITION_HPP
