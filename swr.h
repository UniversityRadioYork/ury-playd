#ifndef PS_SWR
#define PS_SWR

#include <cstdint>

extern "C" {
#include <libswresample/swresample.h>
}

/**
 * A thin layer over the ffmpeg software resampler.
 */
class Swr {
public:
	Swr(std::int64_t out_ch_layout, AVSampleFormat out_sample_fmt,
	    int out_sample_rate, std::int64_t in_ch_layout,
	    AVSampleFormat in_sample_fmt, int in_sample_rate, int log_offset,
	    void* log_ctx);
	~Swr();

	std::int64_t GetDelay(std::int64_t base);
	int Convert(std::uint8_t* out_arg[SWR_CH_MAX], int out_count,
	            const std::uint8_t* in_arg[SWR_CH_MAX], int in_count);

private:
	SwrContext* context;
};

#endif // PS_SWR
