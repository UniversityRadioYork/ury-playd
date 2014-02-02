/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
 */

#ifndef AUDIO_CB_H
#define AUDIO_CB_H

#include <stdint.h>		/* uint64_t */

#include "contrib/pa_ringbuffer.h"	/* PaUtilRingBuffer */

int
audio_cb_play(const void *in,
	      void *out,
	      unsigned long frames_per_buf,
	      const PaStreamCallbackTimeInfo *timeInfo,
	      PaStreamCallbackFlags statusFlags,
	      void *v_au);

#endif				/* not AUDIO_CB_H */
