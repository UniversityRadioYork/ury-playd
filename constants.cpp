#include <stdint.h>

extern "C" {
#ifdef WIN32
#define inline __inline
#endif
#include <libavcodec/avcodec.h>
}

/* See constants.c for more constants (especially macro-based ones) */
