#include "economy/collector_maintenance.h"

#include "economy/collector_catalog_cache.h"
#include "economy/collector_config.h"
#include "economy/collector_runtime.h"
#include "economy/collector_transaction.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <set>
#include <vector>

namespace
{
collector_feature_config config;
bool cache_ready = false;
std::vector<collector::record> records;
std::set<uint64_t> busy;
std::vector<collector_action> submitted_actions;
std::vector<uint64_t> submitted_listings;
std::vector<uint64_t> submitted_times;
unsigned int publication_error = 0;
size_t refreshes = 0;
}

const collector_feature_config *collector_config_get()
{
	return &config;
}

bool collector_catalog_cache_ready()
{
	return cache_ready;
}

bool collector_catalog_cache_refresh()
{
	++refreshes;
	return true;
}

bool collector_runtime_pause_mismatches(bool should_pause, uint64_t after_listing,
					size_t scan_limit, size_t result_limit,
					std::vector<collector::record> *entries,
					uint64_t *next_after_listing, bool *reached_end)
{
	assert(entries && next_after_listing && reached_end && scan_limit && result_limit);
	entries->clear();
	auto current = std::find_if(records.begin(), records.end(), [&](const auto &entry)
				    { return entry.listing > after_listing; });
	size_t scanned = 0;
	uint64_t cursor = after_listing;
	while (current != records.end() && scanned < scan_limit && entries->size() < result_limit)
	{
		cursor = current->listing;
		if (current->status == collector::state::available &&
		    current->holding_paused != should_pause)
			entries->push_back(*current);
		++current;
		++scanned;
	}
	*reached_end = current == records.end();
	*next_after_listing = *reached_end ? 0 : cursor;
	return true;
}

bool collector_transaction_listing_busy(uint64_t listing)
{
	return busy.count(listing) != 0;
}

bool collector_transaction_submit_background(const collector_command_payload &payload,
					     collector_completion_fn completion)
{
	auto found = std::find_if(records.begin(), records.end(), [&](const auto &entry)
				  { return entry.listing == payload.listing; });
	if (found == records.end() || found->revision != payload.expected_listing_revision ||
	    !payload.observed_at || payload.actor_pid ||
	    (payload.action != collector_action::pause &&
	     payload.action != collector_action::resume))
		return false;
	if (payload.action == collector_action::pause)
	{
		if (found->holding_paused || payload.observed_at < found->available_at ||
		    payload.observed_at >= found->expires_at)
			return false;
		found->holding_paused = true;
		found->paused_at = payload.observed_at;
	}
	else
	{
		if (!found->holding_paused || payload.observed_at < found->paused_at)
			return false;
		found->expires_at += payload.observed_at - found->paused_at;
		found->holding_paused = false;
		found->paused_at = 0;
	}
	++found->revision;
	submitted_actions.push_back(payload.action);
	submitted_listings.push_back(payload.listing);
	submitted_times.push_back(payload.observed_at);
	collector_command_result result;
	result.action = payload.action;
	result.record_present = true;
	result.catalog_revision = submitted_actions.size();
	result.entry = *found;
	const unsigned int error = publication_error;
	publication_error = 0;
	completion(nullptr, true, result, error, payload);
	return true;
}

int main()
{
	collector_maintenance_reset_for_tests();
	config.policy.enabled = false;
	config.maintenance_interval_seconds = 60;
	config.maintenance_batch_limit = 2;
	config.enabled_changed_at = 1000;
	config.revision = 1;
	for (uint64_t listing = 1; listing <= 3; ++listing)
	{
		collector::record entry;
		entry.listing = listing;
		entry.status = collector::state::available;
		entry.available_at = 500;
		entry.expires_at = 5000;
		entry.revision = 1;
		records.push_back(entry);
	}
	collector::record already_expired = records.back();
	already_expired.listing = 4;
	already_expired.expires_at = 900;
	records.push_back(already_expired);

	collector_maintenance_pulse();
	assert(!collector_maintenance_health_copy().ready && submitted_actions.empty());

	cache_ready = true;
	busy.insert(2);
	collector_maintenance_pulse();
	assert(submitted_actions.size() == 1 && submitted_listings[0] == 1 &&
	       submitted_times[0] == 1000);
	assert(collector_maintenance_health_copy().reconciling);
	busy.clear();
	collector_maintenance_pulse();
	collector_maintenance_pulse();
	collector_maintenance_pulse();
	assert(!collector_maintenance_health_copy().reconciling);
	assert(submitted_actions.size() == 3);
	assert(std::all_of(submitted_actions.begin(), submitted_actions.end(),
			   [](auto action) { return action == collector_action::pause; }));
	assert(std::all_of(submitted_times.begin(), submitted_times.end(),
			   [](uint64_t value) { return value == 1000; }));
	for (size_t index = 0; index < 3; ++index)
		assert(records[index].holding_paused && records[index].paused_at == 1000);
	assert(!records[3].holding_paused && records[3].expires_at == 900);

	collector_maintenance_pulse();
	assert(submitted_actions.size() == 3);

	config.policy.enabled = true;
	config.enabled_changed_at = 2000;
	config.revision = 2;
	publication_error = ESTALE;
	collector_maintenance_pulse();
	collector_maintenance_pulse();
	collector_maintenance_pulse();
	assert(!collector_maintenance_health_copy().reconciling);
	assert(submitted_actions.size() == 6);
	for (size_t index = 3; index < submitted_actions.size(); ++index)
	{
		assert(submitted_actions[index] == collector_action::resume);
		assert(submitted_times[index] == 2000);
	}
	for (size_t index = 0; index < 3; ++index)
		assert(!records[index].holding_paused && !records[index].paused_at &&
		       records[index].expires_at == 6000);
	assert(!records[3].holding_paused && records[3].expires_at == 900);

	const collector_maintenance_health health = collector_maintenance_health_copy();
	assert(health.ready && health.enabled && health.config_revision == 2);
	assert(health.submitted == 6 && health.completions == 6 && health.committed == 6);
	assert(health.listing_busy == 1 && health.publication_failures == 1 &&
	       health.recovery_refreshes == 1 && refreshes == 1);
	assert(health.ineligible > 0);
	assert(!health.rejected && !health.submit_failures && !health.scan_failures);
	collector_maintenance_shutdown();
	assert(!collector_maintenance_health_copy().ready);
}
