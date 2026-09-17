#include "economy/collector_maintenance.h"

#include "economy/collector_catalog_cache.h"
#include "economy/collector_collection_preparation.h"
#include "economy/collector_config.h"
#include "economy/collector_expiry_preparation.h"
#include "economy/collector_listing_pipeline.h"
#include "economy/collector_notification.h"
#include "economy/collector_runtime.h"
#include "economy/collector_transaction.h"
#include "item/item_ownership_runtime.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <limits>
#include <memory>
#include <new>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
using clock_type = std::chrono::steady_clock;
constexpr size_t RECONCILE_SCAN_MULTIPLIER = 128;

clock_type::time_point next_audit = {};
clock_type::time_point next_due_audit = {};
uint64_t observed_config_revision = std::numeric_limits<uint64_t>::max();
bool observed_ready = false;
bool initial_ready_reconciliation = true;
uint64_t scan_cursor = 0;
bool cycle_changed = false;
collector_maintenance_health health = {};
std::unordered_map<uint64_t, uint64_t> pending_expiry_reads;

size_t scan_limit(size_t batch_limit)
{
	constexpr size_t maximum = collector::catalog_max_records;
	if (batch_limit > maximum / RECONCILE_SCAN_MULTIPLIER)
		return maximum;
	return std::min(maximum, batch_limit * RECONCILE_SCAN_MULTIPLIER);
}

void transition_completed(P_char, bool committed, const collector_command_result &result,
			  unsigned int error_code, const collector_command_payload &)
{
	++health.completions;
	if (!committed)
	{
		++health.rejected;
		return;
	}
	++health.committed;
	if (!error_code && result.action == collector_action::activate && result.record_present)
		collector_notification_on_available(result.entry);
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

bool expiry_read_pending(uint64_t listing)
{
	return std::any_of(pending_expiry_reads.begin(), pending_expiry_reads.end(),
			   [listing](const auto &entry) { return entry.second == listing; });
}

bool submit_timed_command(const collector_command_payload &payload)
{
	if (collector_transaction_listing_busy(payload.listing))
	{
		++health.listing_busy;
		return false;
	}
	if (!collector_transaction_submit_background(payload, transition_completed))
	{
		++health.submit_failures;
		return false;
	}
	++health.submitted;
	return true;
}

void handle_expiry_detail(const collector_feature_config &config, bool ready,
			  collector_listing_result result)
{
	++health.expiry_results;
	const auto pending = pending_expiry_reads.find(result.request_id);
	if (pending == pending_expiry_reads.end())
	{
		++health.due_stale;
		return;
	}
	const uint64_t expected_listing = pending->second;
	pending_expiry_reads.erase(pending);
	if (expected_listing != result.listing ||
	    result.consumer != collector_listing_consumer::maintenance)
	{
		++health.due_stale;
		return;
	}
	if (!ready || !config.policy.enabled || result.outcome != collector_listing_outcome::found)
	{
		if (result.outcome == collector_listing_outcome::invalid_data)
			++health.rejected;
		return;
	}
	collector::record runtime_entry;
	item_ownership_runtime_entry held_item = {};
	uint64_t destruction_revision = 0;
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	if (!collector_runtime_find(result.listing, &runtime_entry) ||
	    !item_ownership_runtime_lookup(runtime_entry.uid, &held_item) ||
	    !item_ownership_runtime_owner_revision(destruction, &destruction_revision))
	{
		++health.due_stale;
		return;
	}
	std::unique_ptr<collector_command_payload> payload;
	const uint64_t now = static_cast<uint64_t>(time(nullptr));
	const collector_expiry_prepare_outcome prepared = collector_expiry_prepare(
		runtime_entry, result.detail, held_item, destruction_revision, now, &payload);
	if (prepared != collector_expiry_prepare_outcome::prepared || !payload)
	{
		if (prepared == collector_expiry_prepare_outcome::allocation_failure)
			++health.submit_failures;
		else
			++health.due_stale;
		return;
	}
	if (submit_timed_command(*payload))
		++health.expiry_submissions;
}

void drain_expiry_details(const collector_feature_config &config, bool ready)
{
	collector_listing_result results[COLLECTOR_LISTING_MAX_COMPLETIONS] = {};
	const size_t count =
		collector_listing_pipeline_pulse_for(collector_listing_consumer::maintenance,
						     results, COLLECTOR_LISTING_MAX_COMPLETIONS);
	for (size_t index = 0; index < count; ++index)
		handle_expiry_detail(config, ready, std::move(results[index]));
	health.pending_expiry_reads = pending_expiry_reads.size();
}

void request_expiry_detail(const collector::record &entry)
{
	if (expiry_read_pending(entry.listing))
	{
		++health.listing_busy;
		return;
	}
	const uint64_t request_id = collector_listing_pipeline_next_request_id();
	try
	{
		if (!pending_expiry_reads.emplace(request_id, entry.listing).second)
		{
			++health.submit_failures;
			return;
		}
	}
	catch (const std::bad_alloc &)
	{
		++health.submit_failures;
		return;
	}
	const collector_listing_submit_outcome submitted = collector_listing_pipeline_submit(
		{ request_id, entry.listing, collector_listing_consumer::maintenance });
	if (submitted != collector_listing_submit_outcome::accepted)
	{
		pending_expiry_reads.erase(request_id);
		++health.submit_failures;
		return;
	}
	++health.expiry_reads;
	health.pending_expiry_reads = pending_expiry_reads.size();
}

void submit_activation(const collector::record &entry, uint64_t now)
{
	std::unique_ptr<collector_command_payload> payload;
	try
	{
		payload = std::make_unique<collector_command_payload>();
	}
	catch (const std::bad_alloc &)
	{
		++health.submit_failures;
		return;
	}
	payload->action = collector_action::activate;
	payload->listing = entry.listing;
	payload->expected_listing_revision = entry.revision;
	payload->observed_at = now;
	if (submit_timed_command(*payload))
		++health.activation_submissions;
}

void submit_candidate_cancellation(const collector::record &entry, collector::reason why,
				   uint64_t now)
{
	std::unique_ptr<collector_command_payload> payload;
	try
	{
		payload = std::make_unique<collector_command_payload>();
	}
	catch (const std::bad_alloc &)
	{
		++health.submit_failures;
		return;
	}
	payload->action = collector_action::cancel;
	payload->cancel_reason = why;
	payload->listing = entry.listing;
	payload->expected_listing_revision = entry.revision;
	payload->observed_at = now;
	if (submit_timed_command(*payload))
		++health.candidate_cancellations;
}

void process_candidate(const collector::record &entry, uint64_t now)
{
	std::unique_ptr<collector_command_payload> payload;
	const collector_collection_prepare_outcome prepared =
		collector_collection_prepare(entry, now, &payload);
	if (prepared == collector_collection_prepare_outcome::prepared)
	{
		if (payload && submit_timed_command(*payload))
			++health.collection_submissions;
		else if (!payload)
			++health.submit_failures;
		return;
	}
	switch (prepared)
	{
	case collector_collection_prepare_outcome::missing_item:
	case collector_collection_prepare_outcome::destroyed:
		submit_candidate_cancellation(entry, collector::reason::destroyed, now);
		return;
	case collector_collection_prepare_outcome::claimed:
		submit_candidate_cancellation(entry, collector::reason::claimed, now);
		return;
	case collector_collection_prepare_outcome::excluded:
		submit_candidate_cancellation(entry, collector::reason::excluded, now);
		return;
	case collector_collection_prepare_outcome::stale_custody:
	case collector_collection_prepare_outcome::invalid_topology:
		submit_candidate_cancellation(entry, collector::reason::quarantined, now);
		return;
	case collector_collection_prepare_outcome::limit_exceeded:
	case collector_collection_prepare_outcome::allocation_failure:
		++health.submit_failures;
		return;
	case collector_collection_prepare_outcome::invalid_request:
	case collector_collection_prepare_outcome::not_due:
		++health.due_stale;
		return;
	case collector_collection_prepare_outcome::prepared:
		return;
	}
}

void process_due(const collector_feature_config &config)
{
	const auto steady_now = clock_type::now();
	if (next_due_audit != clock_type::time_point{} && steady_now < next_due_audit)
		return;
	next_due_audit = steady_now + std::chrono::seconds(config.maintenance_interval_seconds);
	++health.due_passes;
	const uint64_t now = static_cast<uint64_t>(time(nullptr));
	const uint64_t lease_until =
		now > std::numeric_limits<uint64_t>::max() - config.maintenance_lease_seconds ?
			std::numeric_limits<uint64_t>::max() :
			now + config.maintenance_lease_seconds;
	const std::vector<uint64_t> due =
		collector_runtime_lease_due(now, config.maintenance_batch_limit, lease_until);
	health.due_leased += due.size();
	for (uint64_t listing : due)
	{
		if (collector_transaction_listing_busy(listing))
		{
			++health.listing_busy;
			continue;
		}
		collector::record entry;
		if (!collector_runtime_find(listing, &entry))
		{
			++health.due_stale;
			continue;
		}
		switch (entry.status)
		{
		case collector::state::collected:
			submit_activation(entry, now);
			break;
		case collector::state::available:
			if (entry.holding_paused || now < entry.expires_at)
				++health.due_stale;
			else
				request_expiry_detail(entry);
			break;
		case collector::state::candidate:
			process_candidate(entry, now);
			break;
		case collector::state::purchased:
		case collector::state::cancelled:
		case collector::state::expired:
			++health.due_stale;
			break;
		}
	}
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
		health.reconciling = false;
		next_audit = clock_type::now() +
			     std::chrono::seconds(config.maintenance_interval_seconds);
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
		if (submit_transition(entry, !config.policy.enabled, observed_at))
			cycle_changed = true;
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
	const bool initial_ready = became_ready && initial_ready_reconciliation;
	observed_config_revision = config.revision;
	observed_ready = ready;
	if (ready)
		initial_ready_reconciliation = false;
	health.ready = ready;
	health.enabled = config.policy.enabled;
	health.config_revision = config.revision;
	drain_expiry_details(config, ready);
	// The first ready edge starts the initial audit.  Later ready edges are
	// expected after catalog-cache invalidation and must resume normal scheduling
	// without resetting an in-progress/next-due audit back to listing zero.
	if (config_changed || initial_ready)
	{
		start_reconciliation();
		next_due_audit = {};
	}
	if (!ready)
		return;
	if (!health.reconciling &&
	    (next_audit == clock_type::time_point{} || clock_type::now() >= next_audit))
		start_reconciliation();
	if (health.reconciling)
		reconcile_chunk(config);
	if (config.policy.enabled)
		process_due(config);
}

void collector_maintenance_shutdown(void)
{
	next_audit = {};
	next_due_audit = {};
	observed_config_revision = std::numeric_limits<uint64_t>::max();
	observed_ready = false;
	initial_ready_reconciliation = true;
	scan_cursor = 0;
	cycle_changed = false;
	for (const auto &[request_id, listing] : pending_expiry_reads)
	{
		(void)listing;
		collector_listing_pipeline_cancel(request_id);
	}
	pending_expiry_reads.clear();
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
