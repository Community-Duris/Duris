#ifndef DURIS_COLLECTOR_CONFIG_H
#define DURIS_COLLECTOR_CONFIG_H

#include "economy/collector_policy.h"

#include <cstddef>
#include <cstdint>

struct collector_feature_config
{
	collector::rules policy = {};
	uint64_t maintenance_interval_seconds = 60;
	uint64_t maintenance_lease_seconds = 120;
	size_t maintenance_batch_limit = 32;
	// Wall-clock second at which the current enabled state became effective.
	// A single value is shared by every bounded pause/resume batch so a large
	// catalog does not lose holding time merely because reconciliation spans
	// several game pulses.
	uint64_t enabled_changed_at = 0;
	uint64_t revision = 0;
};

// Reloaded with duris.properties on the game thread. The effective revision
// advances only when a validated value changes, allowing runtime services to
// react immediately to enable/disable transitions without polling the file.
void collector_config_reload(void);
const collector_feature_config *collector_config_get(void);
bool collector_config_enabled(void);
uint64_t collector_config_revision(void);
void collector_config_reset_for_tests(void);

#endif
