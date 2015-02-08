// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for PipeAudio.
 */

#include "catch.hpp"

#include "../audio/audio.hpp"
#include "dummy_audio_source.hpp"
#include "dummy_audio_sink.hpp"

SCENARIO("PipeAudioSystem can be constructed with a DummyAudioSink and DummyAudioSource", "[pipe-audio]") {
	GIVEN("unique pointers to a sink and source") {
		auto src = std::unique_ptr<AudioSource>(new DummyAudioSource("test"));
		auto sink = std::unique_ptr<AudioSink>(new DummyAudioSink());

		WHEN("a PipeAudio is created") {
			THEN("no exceptions are thrown and queries return the expected initial values") {
				PipeAudio pa(std::move(src), std::move(sink));
				REQUIRE(pa.Position() == 0);
			}
		}
	}
}
