// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for the Player class.
 */

#include <sstream>

#include "catch.hpp"
#include "../audio/audio_system.hpp"
#include "../errors.hpp"
#include "../player.hpp"
#include "dummy_audio_sink.hpp"
#include "dummy_audio_source.hpp"
#include "dummy_response_sink.hpp"

SCENARIO("Player accurately represents whether it is running", "[player]") {
	GIVEN("a fresh Player using AudioSystem, DummyAudioSink and DummyAudioSource") {
		AudioSystem ds(0);
		Player p(ds);

		ds.SetSink(&DummyAudioSink::Build);
		ds.AddSource("mp3", &DummyAudioSource::Build);

		WHEN("the player has not been asked to quit") {
			THEN("Update returns true (the player is running)") {
				REQUIRE(p.Update());
			}
		}
		WHEN("the player has been asked to quit") {
			auto res = p.RunCommand(std::vector<std::string>{"write", "tag", "/control/state", "Quitting"});
			THEN("The quit was a success") {
				REQUIRE(res.IsSuccess());
			}
			THEN("Update returns false (the player is no longer running)") {
				REQUIRE_FALSE(p.Update());
			}
			THEN("Any future quits fail") {
				REQUIRE_FALSE(p.RunCommand(std::vector<std::string>{"write", "tag", "/control/state", "Quitting"}).IsSuccess());
			}
		}
	}
}

SCENARIO("Player interacts correctly with a AudioSystem", "[player][dummy-audio-system]") {
	GIVEN("a fresh Player using AudioSystem, DummyAudioSink and DummyAudioSource") {
		AudioSystem ds(0);
		Player p(ds);

		ds.SetSink(&DummyAudioSink::Build);
		ds.AddSource("mp3", &DummyAudioSource::Build);

		WHEN("there is no audio loaded") {
			THEN("setting state to 'Playing' returns failure") {
				REQUIRE_FALSE(p.RunCommand(std::vector<std::string>{"write", "tag", "/control/state", "Playing"}).IsSuccess());
			}
			THEN("setting state to 'Stopped' returns failure") {
				REQUIRE_FALSE(p.RunCommand(std::vector<std::string>{"write", "tag", "/control/state", "Stopped"}).IsSuccess());
			}
			THEN("setting time to 0 returns failure") {
				REQUIRE_FALSE(p.RunCommand(std::vector<std::string>{"write", "tag", "/player/time/elapsed", "0"}).IsSuccess());
			}
			THEN("setting state to 'Ejected' returns success") {
				// Telling an ejected player to eject is a
				// no-op.
				REQUIRE(p.RunCommand(std::vector<std::string>{"write", "tag", "/control/state", "Ejected"}).IsSuccess());
			}
			THEN("loading for a known file type returns success") {
				REQUIRE(p.RunCommand(std::vector<std::string>{"write", "tag", "/player/file", "blah.mp3"}).IsSuccess());
			}
			THEN("loading for an unknown file type returns failure") {
				REQUIRE_FALSE(p.RunCommand(std::vector<std::string>{"write", "tag", "/player/file", "blah.wav"}).IsSuccess());
			}
		}

		WHEN("there is audio loaded") {
			p.RunCommand(std::vector<std::string>{"write", "tag", "/player/file", "blah.mp3"});

			AND_WHEN("the audio is stopped") {
				THEN("setting state to Playing returns success") {
					REQUIRE(p.RunCommand(std::vector<std::string>{"write", "tag", "/control/state", "Playing"}).IsSuccess());
				}
				THEN("setting state to Stopped returns success") {
					// Telling a stopped file to stop is a
					// no-op.
					REQUIRE(p.RunCommand(std::vector<std::string>{"write", "tag", "/control/state", "Stopped"}).IsSuccess());
				}
				THEN("seeking to 0 returns success") {
					REQUIRE(p.RunCommand(std::vector<std::string>{"write", "tag", "/player/time/elapsed", "0"}).IsSuccess());
				}
				THEN("setting state to Ejected returns success") {
					REQUIRE(p.RunCommand(std::vector<std::string>{"write", "tag", "/control/state", "Ejected"}).IsSuccess());
				}
				THEN("loading for a known file type returns success") {
					REQUIRE(p.RunCommand(std::vector<std::string>{"write", "tag", "/player/file", "blah.mp3"}).IsSuccess());
				}
				THEN("loading for an unknown file type returns failure") {
					REQUIRE_FALSE(p.RunCommand(std::vector<std::string>{"write", "tag", "/player/file", "blah.wav"}).IsSuccess());
				}
			}

			AND_WHEN("the audio is playing") {
				p.RunCommand(std::vector<std::string>{"write", "tag", "/control/state", "Playing"});

				THEN("setting state to Playing returns failure") {
					// Telling a playing file to play is a
					// no-op.
					REQUIRE(p.RunCommand(std::vector<std::string>{"write", "tag", "/control/state", "Playing"}).IsSuccess());
				}
				THEN("setting state to Stopped returns success") {
					REQUIRE(p.RunCommand(std::vector<std::string>{"write", "tag", "/control/state", "Stopped"}).IsSuccess());
				}
				THEN("seeking to 0 returns success") {
					REQUIRE(p.RunCommand(std::vector<std::string>{"write", "tag", "/player/time/elapsed", "0"}).IsSuccess());
				}
				THEN("setting state to Ejected returns success") {
					REQUIRE(p.RunCommand(std::vector<std::string>{"write", "tag", "/control/state", "Ejected"}).IsSuccess());
				}
				THEN("loading for a known file type returns success") {
					REQUIRE(p.RunCommand(std::vector<std::string>{"write", "tag", "/player/file", "blah.mp3"}).IsSuccess());
				}
				THEN("loading for an unknown file type returns failure") {
					REQUIRE_FALSE(p.RunCommand(std::vector<std::string>{"/player/file", "blah.wav"}).IsSuccess());
				}
			}
		}

	}
}
