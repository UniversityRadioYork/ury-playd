/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more
 * details.
 */

#ifndef PS_MESSAGES_H
#define PS_MESSAGES_H

#include <string>

const std::string MSG_CMD_INVALID = "Bad command or file name";

const std::string MSG_DECODE_FAIL = "Decoding failure";
const std::string MSG_DECODE_NOAUDIO = "This doesn't seem to be an audio file";
const std::string MSG_DECODE_NOSTREAM = "Couldn't acquire stream";
const std::string MSG_DECODE_NOCODEC = "Couldn't acquire codec";
const std::string MSG_DECODE_BADRATE = "Unsupported or invalid sample rate";

const std::string MSG_SEEK_FAIL = "Seek failed";

const std::string MSG_DEV_BADID = "Incorrect device ID";
const std::string MSG_DEV_NOID = "Expected a device ID as an argument";

const std::string MSG_OUTPUT_RINGWRITE = "Ring buffer write error";
const std::string MSG_OUTPUT_RINGINIT = "Ring buffer init error";

const std::string MSG_OHAI = "URY playslave at your service";
const std::string MSG_TTFN = "Sleep now";


#endif // PS_MESSAGES_H
