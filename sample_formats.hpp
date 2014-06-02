#include <cstdint>

#ifndef PS_SAMPLE_FORMATS
#define PS_SAMPLE_FORMATS

/* Sample formats available in playslave++.
 * This is the intersection of those sample formats available in ffmpeg and
 * PortAudio.
 */
enum class SampleFormat : std::uint8_t {
	PACKED_UNSIGNED_INT_8,
	PACKED_SIGNED_INT_16,
	PACKED_SIGNED_INT_32,
	PACKED_FLOAT_32
};

#endif // PS_SAMPLE_FORMATS
