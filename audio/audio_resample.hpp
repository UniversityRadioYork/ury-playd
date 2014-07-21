// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declarations of the audio resampler classes.
 * @see audio/audio_resample.cpp
 */

#ifndef PS_AUDIO_RESAMPLE_HPP
#define PS_AUDIO_RESAMPLE_HPP

#include <functional>
#include <memory>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "../swr.hpp"

/**
 * A class for performing resampling.
 *
 * A resampler takes a ffmpeg frame from the decoder and returns a vector of raw
 * bytes representing the frame's samples in a form the audio output can
 * understand.
 *
 * Each resampler implemented thus far outputs packed samples (each channel is
 * interleaved in one byte stream), so we don't need to worry about returning
 * a vector for each channel.  This may change in the future.
 *
 * As the only resampling that takes place is converting from planar to packed,
 * we make heavy use of the original input codec context data (channel count,
 * sample rate, etc).  If the Resampler ever does more than this conversion, a
 * corresponding refactoring is necessary.
 */
class Resampler {
public:
	/// Type for output byte vectors from this Resampler.
	using ResultVector = std::vector<char>;

	/// Type for the count of bytes per sample.
	using SampleByteCount = int;

	/**
	 * Constructs a Resampler.
	 * @param output_format The AVSampleFormat this Resampler outputs.
	 */
	Resampler(AVSampleFormat output_format);

	/**
	 * Virtual destructor for Resampler.
	 */
	virtual ~Resampler() {};

	/**
	 * Resamples the contents of an ffmpeg frame.
	 * @param frame A pointer to the frame to resample.
	 * @return A vector of packed sample data, containing the results of
	 *   resampling the frame's contents.
	 */
	virtual ResultVector Resample(AVFrame *frame) = 0;

	/**
	 * The FFmpeg sample format this Resampler will output.
	 * @return The AVSampleFormat of this Resampler's output.
	 */
	virtual AVSampleFormat AVOutputFormat();

protected:
	/**
	 * Makes a frame vector from a sample data array.
	 * @param start A pointer to the start of a sample data array.
	 * @param channels The number of channels in the array.
	 * @param samples The number of samples (not bytes) in the array.
	 * @return A vector containing a copy of the sample data.
	 */
	ResultVector MakeFrameVector(char *start, int channels, int samples);

	AVSampleFormat output_format; ///< ffmpeg output format.
};

/**
 * A class for performing resampling on a planar sample format.
 *
 * Since most of playslave++ deals with packed samples, the PlanarResampler
 * converts from the planar format to its corresponding packed format using
 * a Swr.  This incurs a small delay.
 */
class PlanarResampler : public Resampler {
public:
	/**
	 * Constructs a PlanarResampler.
	 * @param codec The codec context for the stream being resampled.
	 */
	PlanarResampler(AVCodecContext *codec);

	ResultVector Resample(AVFrame *frame) override;

private:
	Swr swr; ///< The software resampler objct.
};

/**
 * A class for performing resampling on a packed sample format.
 *
 * Technically, this resampler does nothing other than taking the packed
 * samples from the ffmpeg frame and copying them into a vector.  At the time
 * of writing, this is all that is necessary.
 */
class PackedResampler : public Resampler {
public:
	/**
	 * Constructs a PackedResampler.
	 * @param codec The codec context for the stream being resampled.
	 */
	PackedResampler(AVCodecContext *codec);

	ResultVector Resample(AVFrame *frame) override;
};

#endif // PS_AUDIO_RESAMPLE_HPP
