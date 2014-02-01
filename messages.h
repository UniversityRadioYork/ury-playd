/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
 */

#ifndef MESSAGES_H
#define MESSAGES_H

/* All of these are defined in messages.c.
 *
 * NOTE: Some messages are defined in cuppa, see cuppa/messages.[hc].
 *
 * HOUSEKEEPING: Keep these and their messages.c counterparts in ASCIIbetical
 * order if possible?
 */

extern const char     *MSG_DEV_BADID;  /* Incorrect device ID given */ 
extern const char     *MSG_DEV_NOID;	/* No device ID given */
extern const char     *MSG_OHAI;	/* Greeting message */
extern const char     *MSG_TTFN;	/* Parting message */

#endif				/* not MESSAGES_H  */
