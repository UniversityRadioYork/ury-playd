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
	GIVEN("a fresh Player using PipeAudioSystem, DummyAudioSink and DummyAudioSource") {
		PipeAudioSystem ds(0);
		Player p(ds);

		ds.SetSink(&DummyAudioSink::Build);
		ds.AddSource("mp3", &DummyAudioSource::Build);

		WHEN("quit has not been sent") {
			THEN("Update returns true (the player is running)") {
				REQUIRE(p.Update());
			}
		}
		WHEN("quit has been sent") {
			auto res = p.RunCommand(std::vector<std::string>{"quit"});
			THEN("The quit was a success") {
				REQUIRE(res.IsSuccess());
			}
			THEN("Update returns false (the player is no longer running)") {
				REQUIRE_FALSE(p.Update());
			}
			THEN("Any future quits fail") {
				REQUIRE_FALSE(p.RunCommand(std::vector<std::string>{"quit"}).IsSuccess());
			}
		}
	}
}

SCENARIO("Player interacts correctly with a PipeAudioSystem", "[player][dummy-audio-system]") {
	GIVEN("a fresh Player using PipeAudioSystem, DummyAudioSink and DummyAudioSource") {
		PipeAudioSystem ds(0);
		Player p(ds);

		ds.SetSink(&DummyAudioSink::Build);
		ds.AddSource("mp3", &DummyAudioSource::Build);

		WHEN("there is no audio loaded") {
			THEN("'play' returns failure") {
				REQUIRE_FALSE(p.RunCommand(std::vector<std::string>{"play"}).IsSuccess());
			}
			THEN("'stop' returns failure") {
				REQUIRE_FALSE(p.RunCommand(std::vector<std::string>{"stop"}).IsSuccess());
			}
			THEN("'seek' returns failure") {
				REQUIRE_FALSE(p.RunCommand(std::vector<std::string>{"seek", "0"}).IsSuccess());
			}
			THEN("'eject' returns success") {
				// Telling an ejected player to eject is a
				// no-op.
				REQUIRE(p.RunCommand(std::vector<std::string>{"eject"}).IsSuccess());
			}
			THEN("'load' for a known file type returns success") {
				REQUIRE(p.RunCommand(std::vector<std::string>{"load", "blah.mp3"}).IsSuccess());
			}
			THEN("'load' for an unknown file type returns failure") {
				REQUIRE_FALSE(p.RunCommand(std::vector<std::string>{"load", "blah.wav"}).IsSuccess());
			}
		}

		WHEN("there is audio loaded") {
			p.RunCommand(std::vector<std::string>{"load", "blah.mp3"});

			AND_WHEN("the audio is stopped") {
				THEN("'play' returns success") {
					REQUIRE(p.RunCommand(std::vector<std::string>{"play"}).IsSuccess());
				}
				THEN("'stop' returns failure") {
					// Telling a stopped file to stop is a
					// no-op.
					REQUIRE(p.RunCommand(std::vector<std::string>{"stop"}).IsSuccess());
				}
				THEN("'seek' returns success") {
					REQUIRE(p.RunCommand(std::vector<std::string>{"seek", "0"}).IsSuccess());
				}
				THEN("'eject' returns success") {
					REQUIRE(p.RunCommand(std::vector<std::string>{"eject"}).IsSuccess());
				}
				THEN("'load' for a known file type returns success") {
					REQUIRE(p.RunCommand(std::vector<std::string>{"load", "blah.mp3"}).IsSuccess());
				}
				THEN("'load' for an unknown file type returns failure") {
					REQUIRE_FALSE(p.RunCommand(std::vector<std::string>{"load", "blah.wav"}).IsSuccess());
				}
			}

			AND_WHEN("the audio is playing") {
				p.RunCommand(std::vector<std::string>{"play"});

				THEN("'play' returns failure") {
					// Telling a playing file to play is a
					// no-op.
					REQUIRE(p.RunCommand(std::vector<std::string>{"play"}).IsSuccess());
				}
				THEN("'stop' returns success") {
					REQUIRE(p.RunCommand(std::vector<std::string>{"stop"}).IsSuccess());
				}
				THEN("'seek' returns success") {
					REQUIRE(p.RunCommand(std::vector<std::string>{"seek", "0"}).IsSuccess());
				}
				THEN("'eject' returns success") {
					REQUIRE(p.RunCommand(std::vector<std::string>{"eject"}).IsSuccess());
				}
				THEN("'load' for a known file type returns success") {
					REQUIRE(p.RunCommand(std::vector<std::string>{"load", "blah.mp3"}).IsSuccess());
				}
				THEN("'load' for an unknown file type returns failure") {
					REQUIRE_FALSE(p.RunCommand(std::vector<std::string>{"load", "blah.wav"}).IsSuccess());
				}
			}
		}

	}
}
