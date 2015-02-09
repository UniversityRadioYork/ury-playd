// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for PipeAudioSystem.
 */

#include "catch.hpp"

#include "../audio/audio.hpp"
#include "../audio/audio_system.hpp"
#include "../errors.hpp"
#include "dummy_audio_sink.hpp"
#include "dummy_audio_source.hpp"

SCENARIO("PipeAudioSystem can provide null implementations of Audio", "[pipe-audio-system][no-audio]") {
	GIVEN("a fresh PipeAudioSystem") {
		PipeAudioSystem sys(0);

		WHEN("Null() is invoked") {
			std::unique_ptr<Audio> au = sys.Null();

			THEN("the result is a NoAudio") {
				REQUIRE_FALSE(dynamic_cast<NoAudio *>(au.get()) == nullptr);
			}
		}
	}
}

SCENARIO("PipeAudioSystem cannot create Audio without a registered Sink", "[pipe-audio-system]") {
	GIVEN("a PipeAudioSystem with dummy source loaded") {
		PipeAudioSystem sys(0);
		sys.AddSource("bar", &DummyAudioSource::Build);

		WHEN("Load() is invoked") {
			THEN("an InternalError is thrown") {
				REQUIRE_THROWS_AS(sys.Load("foo.bar"), InternalError);
			}
		}
	}
}


SCENARIO("PipeAudio can be constructed from a PipeAudioSystem", "[pipe-audio][pipe-audio-system]") {
	GIVEN("a fresh PipeAudioSystem") {
		PipeAudioSystem sys(0);
		WHEN("the PipeAudioSystem is assigned a dummy AudioSink and AudioSource") {
			sys.SetSink(&DummyAudioSink::Build);
			sys.AddSource("bar", &DummyAudioSource::Build);

			THEN("an attempt to create audio will succeed") {
				std::unique_ptr<Audio> au = sys.Load("foo.bar");
				REQUIRE(au->Position() == 0);
				REQUIRE_FALSE(dynamic_cast<PipeAudio *>(au.get()) == nullptr);
			}
		}
	}
}

