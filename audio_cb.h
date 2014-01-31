/*
 * =============================================================================
 *
 *       Filename:  audio_cb.h
 *
 *    Description:  Interface to the audio callback
 *
 *        Version:  1.0
 *        Created:  25/12/2012 05:34:58
 *       Revision:  none
 *       Compiler:  clang
 *
 *         Author:  Matt Windsor (CaptainHayashi), matt.windsor@ury.org.uk
 *        Company:  University Radio York Computing Team
 *
 * =============================================================================
 */
/*-
 * Copyright (C) 2012  University Radio York Computing Team
 *
 * This file is a part of playslave.
 *
 * playslave is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * playslave is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * playslave; if not, write to the Free Software Foundation, Inc., 51 Franklin
 * Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef AUDIO_CB_H
#define AUDIO_CB_H

/**  INCLUDES  ****************************************************************/

#include <stdint.h>		/* uint64_t */

#include "contrib/pa_ringbuffer.h"	/* PaUtilRingBuffer */

/**  FUNCTIONS  ***************************************************************/

int
audio_cb_play(const void *in,
	      void *out,
	      unsigned long frames_per_buf,
	      const PaStreamCallbackTimeInfo *timeInfo,
	      PaStreamCallbackFlags statusFlags,
	      void *v_au);

#endif				/* not AUDIO_CB_H */
