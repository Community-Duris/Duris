#pragma once

#include "net/output_channel.h"
#include <cstddef>
#include <cstdint>

inline constexpr size_t OUTPUT_PREFERENCE_MAX_BYTES = 512;

// All-zero is inherited defaults with motion allowed. Plain storage is safe in
// pc_only_data, whose existing allocator does not invoke C++ constructors.
struct OutputPreferenceState
{
	// 0 inherits; 1/2/3 preserve/static/animated; 17..31 are named foreground IDs.
	uint8_t choices[(size_t)OutputChannel::Count];
	bool motion_off;
	bool operator==(const OutputPreferenceState &) const = default;
};
