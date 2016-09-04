// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for the Player class.
 */

#include <sstream>

#include "catch.hpp"
#include "../errors.hpp"
#include "../messages.h"
#include "../player.hpp"
#include "dummy_audio_sink.hpp"
#include "dummy_audio_source.hpp"
#include "dummy_response_sink.hpp"

using namespace std::string_literals;

const std::map<std::string, Player::SourceFn> DUMMY_SRCS {
	{"mp3", &std::make_unique<DummyAudioSource, const std::string &>},
	{"ogg", [](const std::string &) -> std::unique_ptr<AudioSource> { throw FileError("test failure 1"); }},
	{"flac", [](const std::string &) -> std::unique_ptr<AudioSource> { throw InternalError("test failure 2"); }}
};


SCENARIO("Player announces changes in state correctly", "[player]") {
	GIVEN("a fresh Player using AudioSystem, DummyAudioSink and DummyAudioSource") {
		Player p(0,
			 &std::make_unique<DummyAudioSink, const AudioSource &, int>,
			 DUMMY_SRCS);
		
		WHEN("the player has nothing loaded") {
			GIVEN("a DummyResponseSink") {
				std::ostringstream os;
				DummyResponseSink drs(os);
				p.SetIo(drs);

				THEN("ejecting should emit nothing") {
					p.Eject("tag");
					REQUIRE(os.str() == "");
				}
	
				THEN("loading a file should emit all state") {
					p.Load("tag", "baz.mp3");
					REQUIRE(os.str() == "! STOP\n! FLOAD baz.mp3\n! POS 0\n");
				}
			}
		}
	}
}

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
				REQUIRE(res.Pack() == "tag ACK OK success");
			}
			THEN("Update returns false (the player is no longer running)") {
				REQUIRE_FALSE(p.Update());
			}
			THEN("Any future quits fail") {
				REQUIRE_FALSE(p.Quit("tag2").Pack() == "tag2 ACK OK success");
			}
		}
	}
}

SCENARIO("Player interacts correctly with the audio system", "[player]") {
	GIVEN("a fresh Player using AudioSystem, DummyAudioSink and DummyAudioSource") {
		Player p(0,
			 &std::make_unique<DummyAudioSink, const AudioSource &, int>,
			 DUMMY_SRCS);

		WHEN("there is no audio loaded") {
			THEN("playing returns failure") {
				REQUIRE_FALSE(p.SetPlaying("tag", true).Pack() == "tag ACK OK success");
			}
			THEN("stopping returns failure") {
				REQUIRE_FALSE(p.SetPlaying("tag", false).Pack() == "tag ACK OK success");
			}
			THEN("setting time to 1 returns failure") {
				REQUIRE_FALSE(p.Pos("tag", "0").Pack() == "tag ACK OK success");
			}
			THEN("setting state to 'Ejected' returns success") {
				// Telling an ejected player to eject is a
				// no-op.
				REQUIRE(p.Eject("tag").Pack() == "tag ACK OK success");
			}
			THEN("loading for a known file type returns success") {
				REQUIRE(p.Load("tag", "blah.mp3").Pack() == "tag ACK OK success");
			}
			THEN("loading for an unknown file type returns failure") {
				REQUIRE_FALSE(p.Load("tag", "blah.wav").Pack() == "tag ACK OK success");
			}
			THEN("loading for an empty filename returns failure") {
				auto r = "tag ACK WHAT '"s + MSG_LOAD_EMPTY_PATH + "'"s;
				REQUIRE(p.Load("tag", "").Pack() == r);
			}
		}

		WHEN("there is audio loaded") {
			p.Load("tag", "blah.mp3");

			AND_WHEN("the audio is stopped") {
				THEN("setting state to Playing returns success") {
					REQUIRE(p.SetPlaying("tag", true).Pack() == "tag ACK OK success");
				}
				THEN("setting state to Stopped returns success") {
					// Telling a stopped file to stop is a
					// no-op.
					REQUIRE(p.SetPlaying("tag", false).Pack() == "tag ACK OK success");
				}
				THEN("seeking to 0 returns success") {
					REQUIRE(p.Pos("tag", "0").Pack() == "tag ACK OK success");
				}
				THEN("setting state to Ejected returns success") {
					REQUIRE(p.Eject("tag").Pack() == "tag ACK OK success");
				}
				THEN("loading for a known file type returns success") {
					REQUIRE(p.Load("tag", "blah.mp3").Pack() == "tag ACK OK success");
				}
				THEN("loading for an unknown file type returns failure") {
					REQUIRE_FALSE(p.Load("tag", "blah.wav").Pack() == "tag ACK OK success");
				}
				THEN("loading for an empty filename returns failure") {
					auto r = "tag ACK WHAT '"s + MSG_LOAD_EMPTY_PATH + "'"s;
					REQUIRE(p.Load("tag", "").Pack() == r);
				}
			}

			AND_WHEN("the audio is playing") {
				p.SetPlaying("tag", true).Pack();

				THEN("setting state to Playing returns failure") {
					// Telling a playing file to play is a
					// no-op.
					REQUIRE(p.SetPlaying("tag", true).Pack() == "tag ACK OK success");
				}
				THEN("setting state to Stopped returns success") {
					REQUIRE(p.SetPlaying("tag", false).Pack() == "tag ACK OK success");
				}
				THEN("seeking to 0 returns success") {
					REQUIRE(p.Pos("tag", "0").Pack() == "tag ACK OK success");
				}
				THEN("setting state to Ejected returns success") {
					REQUIRE(p.Eject("tag").Pack() == "tag ACK OK success");
				}
				THEN("loading for a known file type returns success") {
					REQUIRE(p.Load("tag", "blah.mp3").Pack() == "tag ACK OK success");
				}
				THEN("loading for an unknown file type returns failure") {
					REQUIRE_FALSE(p.Load("tag", "blah.wav").Pack() == "tag ACK OK success");
				}
				THEN("loading for an empty filename returns failure") {
					auto r = "tag ACK WHAT '"s + MSG_LOAD_EMPTY_PATH + "'"s;
					REQUIRE(p.Load("tag", "").Pack() == r);
				}

			}
		}

	}
}

SCENARIO("Player refuses absurd seek positions", "[seek]") {
	GIVEN("a loaded Player") {
		Player p(0,
			 &std::make_unique<DummyAudioSink, const AudioSource &, int>,
			 DUMMY_SRCS);
		
		p.Load("tag", "blah.mp3");

		auto response = "tag ACK WHAT '"s + MSG_SEEK_INVALID_VALUE + "'"s;

		WHEN("the player is told to seek to a negative time") {
			auto res = p.Pos("tag", "-5");

			THEN("it rejects the seek") {
				REQUIRE(res.Pack() == response);
			}
		}

		WHEN("the player is told to seek to a time with units") {
			auto res = p.Pos("tag", "10ms");

			THEN("it rejects the seek") {
				REQUIRE(res.Pack() == response);
			}
		}

		WHEN("the player is told to seek to a fractional time") {
			auto res = p.Pos("tag", "100.52");

			THEN("it rejects the seek") {
				REQUIRE(res.Pack() == response);
			}
		}

		WHEN("the player is told to seek to a non-numeric time") {
			auto res = p.Pos("tag", "PI");

			THEN("it rejects the seek") {
				REQUIRE(res.Pack() == response);
			}
		}

		WHEN("the player is told to seek to an empty time") {
			auto res = p.Pos("tag", "");

			THEN("it rejects the seek") {
				REQUIRE(res.Pack() == response);
			}
		}

		// Seeking to non-base-10 positions deliberately left untested.
		// It's harmless if allowed.
	}
}

SCENARIO("Player handles End requests correctly", "[player]") {
	GIVEN("a loaded Player") {
		Player p(0,
			 &std::make_unique<DummyAudioSink, const AudioSource &, int>,
			 DUMMY_SRCS);

		p.Load("tag", "blah.mp3");

		std::ostringstream os;
		DummyResponseSink drs(os);
		p.SetIo(drs);

		WHEN("End is sent to the player") {
			p.End("tag");

			THEN("the response contains END, STOP, and POS") {
				REQUIRE(os.str() == "! END\n"
						    "! STOP\n"
						    "! POS 0\n");
			}
		}
	}
}

SCENARIO("Player refuses commands when quitting", "[player]") {
	GIVEN("a loaded Player") {
		Player p(0,
			 &std::make_unique<DummyAudioSink, const AudioSource &, int>,
			 DUMMY_SRCS);
		
		p.Load("tag", "blah.mp3");

		auto response = "tag ACK FAIL '"s + MSG_CMD_PLAYER_CLOSING + "'"s;

		WHEN("the player is told to quit") {
			p.Quit("t");
			
			THEN("loading returns a player-closing failure") {
				REQUIRE(p.Load("tag", "barbaz.mp3").Pack() == response);
			}
			THEN("ejecting returns a player-closing failure") {
				REQUIRE(p.Eject("tag").Pack() == response);
			}
			THEN("seeking returns a player-closing failure") {
				REQUIRE(p.Pos("tag", "100").Pack() == response);
			}
			THEN("ending returns a player-closing failure") {
				REQUIRE(p.End("tag").Pack() == response);
			}
			THEN("playing returns a player-closing failure") {
				REQUIRE(p.SetPlaying("tag", true).Pack() == response);
			}
			THEN("stopping returns a player-closing failure") {
				REQUIRE(p.SetPlaying("tag", false).Pack() == response);
			}
			THEN("quitting returns a player-closing failure") {
				REQUIRE(p.Quit("tag").Pack() == response);
			}
			THEN("dumping returns a player-closing failure") {
				REQUIRE(p.Dump(5, "tag").Pack() == response);
			}
		}
	}
}

SCENARIO("Player handles load errors properly", "[seek]") {
	GIVEN("a Player") {
		Player p(0,
			 &std::make_unique<DummyAudioSink, const AudioSource &, int>,
			 DUMMY_SRCS);
		
		WHEN("no file is loaded") {
			std::ostringstream os;
			DummyResponseSink drs(os);
			p.SetIo(drs);

			AND_WHEN("a load fails with FileError") {
				auto rs = p.Load("tag", "blah.ogg");

				THEN("the load response is FAIL with the given message") {
					REQUIRE(rs.Pack() == "tag ACK FAIL 'test failure 1'");
				}

				THEN("the player does not eject") {
					REQUIRE(os.str() == "");
				}
			}

			AND_WHEN("a load fails with InternalError") {
				THEN("the error propagates outwards") {
					REQUIRE_THROWS_AS(p.Load("tag", "blah.flac"), InternalError);	
				}
			}
		}

		WHEN("a file is already loaded") {
			p.Load("tag", "foo.mp3");

			std::ostringstream os;
			DummyResponseSink drs(os);
			p.SetIo(drs);

			AND_WHEN("a load fails with FileError") {
				auto rs = p.Load("tag", "blah.ogg");

				THEN("the load response is FAIL with the given message") {
					REQUIRE(rs.Pack() == "tag ACK FAIL 'test failure 1'");
				}

				THEN("the player ejects") {
					REQUIRE(os.str() == "! EJECT\n");
				}
			}

			AND_WHEN("a load fails with InternalError") {
				THEN("the error propagates outwards") {
					REQUIRE_THROWS_AS(p.Load("tag", "blah.flac"), InternalError);	
				}
			}
		}
	}
}
