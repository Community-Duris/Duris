#include "economy/collector_maintenance.h"

#include "economy/collector_catalog_cache.h"
#include "economy/collector_config.h"
#include "economy/collector_runtime.h"
#include "economy/collector_transaction.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>
#include <new>
#include <vector>

namespace
{
using clock_type = std::chrono::steady_clock;
constexpr size_t RECONCILE_SCAN_MULTIPLIER = 128;

clock_type::time_point next_audit = {};
uint64_t observed_config_revision = std::numeric_limits<uint64_t>::max();
bool observed_ready = false;
uint64_t scan_cursor = 0;
bool cycle_changed = false;
collector_maintenance_health health = {};

size_t scan_limit(size_t batch_limit)
{
	constexpr size_t maximum = collector::catalog_max_records;
	if (batch_limit > maximum / RECONCILE_SCAN_MULTIPLIER)
		return maximum;
	return std::min(maximum, batch_limit * RECONCILE_SCAN_MULTIPLIER);
}

void transition_completed(P_char, bool committed, const collector_command_result &,
			  unsigned int error_code, const collector_command_payload &)
{
	++health.completions;
	if (!committed)
	{
		++health.rejected;
		return;
	}
	++health.committed;
	if (!error_code)
		return;
	++health.publication_failures;
	if (collector_catalog_cache_refresh())
		++health.recovery_refreshes;
}

bool transition_time(const collector::record &entry, bool should_pause, uint64_t boundary,
		     uint64_t *observed_at)
{
	if (!observed_at)
		return false;
	if (should_pause)
	{
		*observed_at = std::max(boundary, entry.available_at);
		return *observed_at < entry.expires_at;
	}
	*observed_at = std::max(boundary, entry.paused_at);
	return true;
}

bool submit_transition(const collector::record &entry, bool should_pause, uint64_t observed_at)
{
	if (collector_transaction_listing_busy(entry.listing))
	{
		++health.listing_busy;
		return false;
	}
	std::unique_ptr<collector_command_payload> payload;
	try
	{
		payload = std::make_unique<collector_command_payload>();
	}
	catch (const std::bad_alloc &)
	{
		++health.submit_failures;
		return false;
	}
	payload->action = should_pause ? collector_action::pause : collector_action::resume;
	payload->listing = entry.listing;
	payload->expected_listing_revision = entry.revision;
	payload->observed_at = observed_at;
	if (!collector_transaction_submit_background(*payload, transition_completed))
	{
		++health.submit_failures;
		return false;
	}
	++health.submitted;
	return true;
}

void start_reconciliation()
{
	scan_cursor = 0;
	cycle_changed = false;
	health.reconciling = true;
	next_audit = {};
}

void reconcile_chunk(const collector_feature_config &config)
{
	std::vector<collector::record> entries;
	uint64_t next_cursor = 0;
	bool reached_end = false;
	if (!collector_runtime_pause_mismatches(
		    !config.policy.enabled, scan_cursor, scan_limit(config.maintenance_batch_limit),
		    config.maintenance_batch_limit, &entries, &next_cursor, &reached_end))
	{
		++health.scan_failures;
		return;
	}
	health.selected += entries.size();
	for (const collector::record &entry : entries)
	{
		uint64_t observed_at = 0;
		if (!transition_time(entry, !config.policy.enabled, config.enabled_changed_at,
				     &observed_at))
		{
			++health.ineligible;
			continue;
		}
		cycle_changed = true;
		submit_transition(entry, !config.policy.enabled, observed_at);
	}
	scan_cursor = next_cursor;
	if (!reached_end)
		return;
	++health.passes;
	if (cycle_changed)
	{
		scan_cursor = 0;
		cycle_changed = false;
		return;
	}
	health.reconciling = false;
	next_audit = clock_type::now() + std::chrono::seconds(config.maintenance_interval_seconds);
}
} // namespace

void collector_maintenance_pulse(void)
{
	const collector_feature_config &config = *collector_config_get();
	const bool ready = collector_catalog_cache_ready();
	const bool config_changed = config.revision != observed_config_revision;
	const bool became_ready = ready && !observed_ready;
	observed_config_revision = config.revision;
	observed_ready = ready;
	health.ready = ready;
	health.enabled = config.policy.enabled;
	health.config_revision = config.revision;
	if (config_changed || became_ready)
		start_reconciliation();
	if (!ready)
		return;
	if (!health.reconciling &&
	    (next_audit == clock_type::time_point{} || clock_type::now() >= next_audit))
		start_reconciliation();
	if (health.reconciling)
		reconcile_chunk(config);
}

void collector_maintenance_shutdown(void)
{
	next_audit = {};
	observed_config_revision = std::numeric_limits<uint64_t>::max();
	observed_ready = false;
	scan_cursor = 0;
	cycle_changed = false;
	health = {};
}

collector_maintenance_health collector_maintenance_health_copy(void)
{
	return health;
}

void collector_maintenance_reset_for_tests(void)
{
	collector_maintenance_shutdown();
}
