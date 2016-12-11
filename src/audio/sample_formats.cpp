// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of sample format tables.
 * @see audio/sample_formats.h
 */

#include <array>
#include <cstddef>

#include "sample_formats.h"

const std::array<std::size_t, SAMPLE_FORMAT_COUNT> SAMPLE_FORMAT_BPS {{
        1, // PACKED_UNSIGNED_INT_8
        1, // PACKED_SIGNED_INT_8
        2, // PACKED_SIGNED_INT_16
        4, // PACKED_SIGNED_INT_32
        4  // PACKED_FLOAT_32
}};
