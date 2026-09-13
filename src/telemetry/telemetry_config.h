#ifndef DURIS_TELEMETRY_CONFIG_H
#define DURIS_TELEMETRY_CONFIG_H

#include "telemetry/telemetry_types.h"

#include <cstdint>
#include <type_traits>

/*
 * Config values are copied at publication and captured by value in later
 * records.  No producer hashes a live configuration per event.  The limits
 * in telemetry_types.h are bounded first-release proposals, not measured capacity claims;
 * active_window_usec defaults to 300 seconds but may be configured up to one
 * hour.  It is a bound, not a requirement to use the proposal value.
 */
enum class telemetry_config_apply_outcome : std::uint8_t
{
	applied = 0,
	unchanged = 1,
	stale_revision = 2,
	rejected_invalid = 3,
	flatfile_disabled = 4,
	unavailable = 5,
};

/* The bounded game/config publication entry is declared in telemetry_runtime.h. */
telemetry_config_snapshot telemetry_config_snapshot_copy(void);
telemetry_config_validation telemetry_config_status(void);

static_assert(std::is_trivially_copyable_v<telemetry_config_snapshot>);
static_assert(std::is_standard_layout_v<telemetry_config_snapshot>);
static_assert(sizeof(telemetry_config_snapshot) <= TELEMETRY_RECORD_MAX_BYTES,
	      "telemetry_config_snapshot must remain a bounded value");

#endif
