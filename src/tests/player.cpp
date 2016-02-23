// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for the Player class.
 */

#include <sstream>

#include "catch.hpp"
#include "../errors.hpp"
#include "../player.hpp"
#include "dummy_audio_sink.hpp"
#include "dummy_audio_source.hpp"
#include "dummy_response_sink.hpp"

const std::map<std::string, Player::SourceFn> DUMMY_SRCS {
	{"mp3", &std::make_unique<DummyAudioSource, const std::string &>}
};

SCENARIO("Player accurately represents whether it is running", "[player]") {
	GIVEN("a fresh Player using AudioSystem, DummyAudioSink and DummyAudioSource") {
		Player p(0,
			 &std::make_unique<DummyAudioSink, const AudioSource &, int>,
			 DUMMY_SRCS);

		WHEN("the player has not been asked to quit") {
			THEN("Update returns true (the player is running)") {
				REQUIRE(p.Update());
			}
		}
		WHEN("the player has been asked to quit") {
			auto res = p.Quit("tag");
			THEN("The quit was a success") {
				REQUIRE(res.Pack() == "tag ACK OK Success");
			}
			THEN("Update returns false (the player is no longer running)") {
				REQUIRE_FALSE(p.Update());
			}
			THEN("Any future quits fail") {
				REQUIRE_FALSE(p.Quit("tag2").Pack() == "tag2 ACK OK Success");
			}
		}
	}
}

SCENARIO("Player interacts correctly with a AudioSystem", "[player][dummy-audio-system]") {
	GIVEN("a fresh Player using AudioSystem, DummyAudioSink and DummyAudioSource") {
		Player p(0,
			 &std::make_unique<DummyAudioSink, const AudioSource &, int>,
			 DUMMY_SRCS);

		WHEN("there is no audio loaded") {
			THEN("playing returns failure") {
				REQUIRE_FALSE(p.SetPlaying("tag", true).Pack() == "tag ACK OK Success");
			}
			THEN("stopping returns failure") {
				REQUIRE_FALSE(p.SetPlaying("tag", false).Pack() == "tag ACK OK Success");
			}
			THEN("setting time to 0 returns failure") {
				REQUIRE_FALSE(p.Pos("tag", 0).Pack() == "tag ACK OK Success");
			}
			THEN("setting state to 'Ejected' returns success") {
				// Telling an ejected player to eject is a
				// no-op.
				REQUIRE(p.Eject("tag").Pack() == "ACK OK Success");
			}
			THEN("loading for a known file type returns success") {
				REQUIRE(p.Load("tag", "blah.mp3").Pack() == "tag ACK OK Success");
			}
			THEN("loading for an unknown file type returns failure") {
				REQUIRE_FALSE(p.Load("tag", "blah.wav").Pack() == "tag ACK OK Success");
			}
		}

		WHEN("there is audio loaded") {
			p.Load("tag", "blah.mp3");

			AND_WHEN("the audio is stopped") {
				THEN("setting state to Playing returns success") {
					REQUIRE(p.SetPlaying("tag", true).Pack() == "tag ACK OK Success");
				}
				THEN("setting state to Stopped returns success") {
					// Telling a stopped file to stop is a
					// no-op.
					REQUIRE(p.SetPlaying("tag", false).Pack() == "tag ACK OK Success");
				}
				THEN("seeking to 0 returns success") {
					REQUIRE(p.Pos("tag", 0).Pack() == "tag ACK OK Success");
				}
				THEN("setting state to Ejected returns success") {
					REQUIRE(p.Eject("tag").Pack() == "tag ACK OK Success");
				}
				THEN("loading for a known file type returns success") {
					REQUIRE(p.Load("tag", "blah.mp3").Pack() == "tag ACK OK Success");
				}
				THEN("loading for an unknown file type returns failure") {
					REQUIRE_FALSE(p.Load("tag", "blah.wav").Pack() == "tag ACK OK Success");
				}
			}

			AND_WHEN("the audio is playing") {
				p.SetPlaying("tag", true).Pack();

				THEN("setting state to Playing returns failure") {
					// Telling a playing file to play is a
					// no-op.
					REQUIRE(p.SetPlaying("tag", true).Pack() == "tag ACK OK Success");
				}
				THEN("setting state to Stopped returns success") {
					REQUIRE(p.SetPlaying("tag", false).Pack() == "tag ACK OK Success");
				}
				THEN("seeking to 0 returns success") {
					REQUIRE(p.Pos("tag", 0).Pack() == "tag ACK OK Success");
				}
				THEN("setting state to Ejected returns success") {
					REQUIRE(p.Eject("tag").Pack() == "tag ACK OK Success");
				}
				THEN("loading for a known file type returns success") {
					REQUIRE(p.Load("tag", "blah.mp3").Pack() == "tag ACK OK Success");
				}
				THEN("loading for an unknown file type returns failure") {
					REQUIRE_FALSE(p.Load("tag", "blah.wav").Pack() == "tag ACK OK Success");
				}
			}
		}

	}
}
