// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

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
#include <boost/optional.hpp>

#include "../io/io_response.hpp"

/**
 * Tracker and broadcaster for the Player's current position in a song.
 */
class PlayerPosition : public ResponseSource {
public:
	/// Unit used for positions.
	using Unit = std::chrono::microseconds;
private:
	/// The period between each firing of the listeners.
	Unit period;

	/// The current position of the player.
	Unit current;

	/**
	 * The position of the player at the last firing of the callbacks.
	 * This may be empty, if the callbacks have never fired.
	 */
	boost::optional<Unit> last;

public:
	/**
	 * Constructs a PlayerPosition.
	 */
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
	 * This fires the position listeners if necessary.
	 * @param position The new position, in @a PositionUnit units.
	 * @see Reset
	 */
	void Update(Unit position);

	/**
	 * Resets the position tracker's position data.
	 * This does not deregister the listeners.
	 * Call this whenever the song changes, or before a skip.
	 * @see Update
	 */
	void Reset();

	/**
	 * Emits the current position to a responder.
	 * @param target The responder to which a TIME response shall be sent.
	 */
	const void Emit(ResponseSink &target) const override;
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
