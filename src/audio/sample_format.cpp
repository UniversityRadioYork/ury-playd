// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of sample format tables.
 * @see audio/sample_formats.h
 */

#include "sample_format.h"

#include <array>
#include <cstddef>

namespace Playd::Audio
{
const std::array<std::size_t, SAMPLE_FORMAT_COUNT> sample_format_bps{{
        1, // UINT8
        1, // SINT8
        2, // UINT16
        4, // SINT32
        4  // FLOAT32
}};

} // namespace Playd::Audio
