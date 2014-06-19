/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#ifndef PS_AUDIO_RESAMPLE_HPP
#define PS_AUDIO_RESAMPLE_HPP

#include <functional>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "swr.hpp"

/**
 * Abstract class for things that convert between sample counts and byte counts.
 *
 * This class defines an interface only.  It exists mainly for the Resampler's
 * benefit as it needs access to something that can do this conversion for it
 * but doesn't need to know exactly what.
 *
 * The conversions depend on whatever bytes-per-sample ratio this
 * SampleByteConverter holds.  For instance, for an AudioDecoder, the ratio
 * is that of whatever the AudioDecoder is outputting.  Note that we can't
 * plug an AudioDecoder directly into a Resampler, due to cyclic dependencies.
 */
class SampleByteConverter {
public:
	/**
	 * Converts from a byte count to a sample count.
	 *
	 * This involves dividing by the bytes-per-sample ratio. and is the
	 * inverse of SampleCountForByteCount up to rounding and overflowing.
	 * @param bytes  The byte count to convert to a sample count.
	 * @return       The corresponding sample count.
	 * @see          ByteCountForSampleCount
	 */
	virtual size_t SampleCountForByteCount(size_t bytes) const = 0;

	/**
	 * Converts from a sample count to a byte count.
	 *
	 * This involves multiplying by the bytes-per-sample ratio, and is the
	 * inverse of SampleCountForByteCount up to rounding and overflowing.
	 * @param samples  The sample count to convert to a byte count.
	 * @return         The corresponding byte count.
	 * @see            SampleCountForByteCount
	 */
	virtual size_t ByteCountForSampleCount(size_t samples) const = 0;
};

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
 * A resampler uses an external SampleByteConverter to provide conversions
 * between sample counts and byte counts.  Only one is sufficient at the moment
 * because the resampler only converts from planar to packed.  If the Resampler
 * ever does more, separate input and output conversions may be necessary.
 */
class Resampler : protected SampleByteConverter {
public:
	/**
	 * Constructs a Resampler.
	 * @param conv  A SampleByteConverter providing conversions between
	 *              samples and bytes for the output format.
	 */
	Resampler(const SampleByteConverter &conv);

	/**
	 * Resamples the contents of an ffmpeg frame.
	 * @param frame  A pointer to the frame to resample.
	 * @return       A vector of packed sample data, containing the results
	 *               of resampling the frame's contents.
	 */
	virtual std::vector<char> Resample(AVFrame *frame) = 0;

	/**
	 * The ffmpeg sample format this Resampler will output.
	 * @return The AVSampleFormat of this Resampler's output.
	 */
	virtual AVSampleFormat AVOutputFormat();

protected:
	size_t SampleCountForByteCount(size_t bytes) const;
	size_t ByteCountForSampleCount(size_t samples) const;

	/**
	 * Makes a frame vector from a sample data array and sample count.
	 * @param start         A pointer to the start of a sample data array.
	 * @param sample_count  The number of samples (not bytes) in the array.
	 * @return              A vector containing a copy of the sample data.
	 */
	std::vector<char> MakeFrameVector(char *start, int sample_count);

	AVSampleFormat output_format;    ///< ffmpeg output format.
	const SampleByteConverter &out;  ///< Output sample rate converter.
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
	 * @param conv   A SampleByteConverter providing conversions between
	 *               samples and bytes for the output format.
	 * @param codec  The codec context for the stream being resampled.
	 */
	PlanarResampler(const SampleByteConverter &conv, AVCodecContext *codec);

	std::vector<char> Resample(AVFrame *frame);

private:
	std::unique_ptr<Swr> swr; ///< The software resampler objct.
	std::unique_ptr<uint8_t, std::function<void(uint8_t *)>>
	                resample_buffer;  ///< The buffer used for resampling.
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
	 * @param conv   A SampleByteConverter providing conversions between
	 *               samples and bytes for the output format.
	 * @param codec  The codec context for the stream being resampled.
	 */
	PackedResampler(const SampleByteConverter &conv, AVCodecContext *codec);

	std::vector<char> Resample(AVFrame *frame);
};

#endif // PS_AUDIO_RESAMPLE_HPP
