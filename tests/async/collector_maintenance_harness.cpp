#include "economy/collector_maintenance.h"

#include "economy/collector_catalog_cache.h"
#include "economy/collector_collection_preparation.h"
#include "economy/collector_config.h"
#include "economy/collector_expiry_preparation.h"
#include "economy/collector_listing_pipeline.h"
#include "economy/collector_runtime.h"
#include "economy/collector_transaction.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <ctime>
#include <deque>
#include <set>
#include <vector>

namespace
{
collector_feature_config config;
bool cache_ready = false;
bool scan_succeeds = true;
std::vector<collector::record> records;
std::set<uint64_t> busy;
std::vector<collector_action> submitted_actions;
std::vector<uint64_t> submitted_listings;
std::vector<uint64_t> submitted_times;
unsigned int publication_error = 0;
size_t refreshes = 0;
std::vector<uint64_t> due_once;
std::deque<collector_listing_result> listing_results;
uint64_t next_request_id = 100;
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
	if (!scan_succeeds)
		return false;
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

std::vector<uint64_t> collector_runtime_lease_due(uint64_t, size_t limit, uint64_t)
{
	std::vector<uint64_t> leased;
	while (!due_once.empty() && leased.size() < limit)
	{
		leased.push_back(due_once.front());
		due_once.erase(due_once.begin());
	}
	return leased;
}

bool collector_runtime_find(uint64_t listing, collector::record *entry)
{
	const auto found = std::find_if(records.begin(), records.end(), [&](const auto &candidate)
					{ return candidate.listing == listing; });
	if (!entry || found == records.end())
		return false;
	*entry = *found;
	return true;
}

uint64_t collector_listing_pipeline_next_request_id()
{
	return ++next_request_id;
}

collector_listing_submit_outcome
collector_listing_pipeline_submit(const collector_listing_request &request)
{
	collector_listing_result result = {};
	result.request_id = request.request_id;
	result.listing = request.listing;
	result.consumer = request.consumer;
	auto found = std::find_if(records.begin(), records.end(), [&](const auto &entry)
				  { return entry.listing == request.listing; });
	if (found == records.end())
		result.outcome = collector_listing_outcome::not_found;
	else
	{
		result.outcome = collector_listing_outcome::found;
		result.detail.entry = *found;
		result.detail.item_blob = { 1, 2, 3 };
	}
	listing_results.push_back(std::move(result));
	return collector_listing_submit_outcome::accepted;
}

size_t collector_listing_pipeline_pulse_for(collector_listing_consumer consumer,
					    collector_listing_result *results, size_t capacity)
{
	size_t count = 0;
	for (auto current = listing_results.begin();
	     current != listing_results.end() && count < capacity;)
	{
		if (current->consumer != consumer)
		{
			++current;
			continue;
		}
		results[count++] = std::move(*current);
		current = listing_results.erase(current);
	}
	return count;
}

bool collector_listing_pipeline_cancel(uint64_t request_id)
{
	const auto found = std::find_if(listing_results.begin(), listing_results.end(),
					[&](const auto &result)
					{ return result.request_id == request_id; });
	if (found == listing_results.end())
		return false;
	listing_results.erase(found);
	return true;
}

bool item_ownership_runtime_lookup(uint64_t item_uid, item_ownership_runtime_entry *entry)
{
	if (!entry)
		return false;
	const auto found = std::find_if(records.begin(), records.end(),
					[&](const auto &record) { return record.uid == item_uid; });
	if (found == records.end())
		return false;
	*entry = { found->uid,
		   found->uid,
		   0,
		   { item_owner_type::collector, item_collector_owner_id(found->listing), 0 },
		   found->item_revision,
		   4,
		   501,
		   item_custody_state::active };
	return true;
}

uint64_t item_collector_owner_id(uint64_t listing_id)
{
	return listing_id;
}

bool item_ownership_runtime_owner_revision(const item_owner_identity &owner, uint64_t *revision)
{
	if (!revision || owner.type != item_owner_type::destruction)
		return false;
	*revision = 7;
	return true;
}

collector_expiry_prepare_outcome collector_expiry_prepare(
	const collector::record &runtime_entry, const collector_listing_detail &detail,
	const item_ownership_runtime_entry &held_item, uint64_t destruction_owner_revision,
	uint64_t observed_at, std::unique_ptr<collector_command_payload> *payload)
{
	if (!payload || detail.entry.listing != runtime_entry.listing ||
	    held_item.item_uid != runtime_entry.uid || observed_at < runtime_entry.expires_at)
		return collector_expiry_prepare_outcome::stale_listing;
	auto candidate = std::make_unique<collector_command_payload>();
	candidate->action = collector_action::expire;
	candidate->target_state = item_custody_state::destroyed;
	candidate->listing = runtime_entry.listing;
	candidate->expected_listing_revision = runtime_entry.revision;
	candidate->observed_at = observed_at;
	candidate->expected_from_owner_revision = held_item.owner_revision;
	candidate->expected_to_owner_revision = destruction_owner_revision;
	candidate->selected_item_uid = held_item.item_uid;
	candidate->item_count = 1;
	*payload = std::move(candidate);
	return collector_expiry_prepare_outcome::prepared;
}

collector_collection_prepare_outcome
collector_collection_prepare(const collector::record &, uint64_t,
			     std::unique_ptr<collector_command_payload> *)
{
	return collector_collection_prepare_outcome::invalid_request;
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
	    !payload.observed_at || payload.actor_pid)
		return false;
	if (payload.action == collector_action::pause)
	{
		if (found->holding_paused || payload.observed_at < found->available_at ||
		    payload.observed_at >= found->expires_at)
			return false;
		found->holding_paused = true;
		found->paused_at = payload.observed_at;
	}
	else if (payload.action == collector_action::resume)
	{
		if (!found->holding_paused || payload.observed_at < found->paused_at)
			return false;
		found->expires_at += payload.observed_at - found->paused_at;
		found->holding_paused = false;
		found->paused_at = 0;
	}
	else if (payload.action == collector_action::activate)
	{
		if (found->status != collector::state::collected ||
		    payload.observed_at < found->sale_at)
			return false;
		found->status = collector::state::available;
		found->available_at = payload.observed_at;
		found->expires_at = payload.observed_at + config.policy.holding_duration;
	}
	else if (payload.action == collector_action::expire)
	{
		if (found->status != collector::state::available || found->holding_paused ||
		    payload.observed_at < found->expires_at || payload.item_count != 1)
			return false;
		found->status = collector::state::expired;
		found->closed_reason = collector::reason::holding_elapsed;
		++found->item_revision;
	}
	else
		return false;
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
	const uint64_t now = static_cast<uint64_t>(time(nullptr));
	config.policy.enabled = false;
	config.policy.holding_duration = 7 * 24 * 60 * 60;
	config.maintenance_interval_seconds = 60;
	config.maintenance_batch_limit = 2;
	config.enabled_changed_at = now;
	config.revision = 1;
	for (uint64_t listing = 1; listing <= 3; ++listing)
	{
		collector::record entry;
		entry.listing = listing;
		entry.status = collector::state::available;
		entry.available_at = now - 500;
		entry.expires_at = now + 5000;
		entry.revision = 1;
		records.push_back(entry);
	}
	collector::record already_expired = records.back();
	already_expired.listing = 4;
	already_expired.expires_at = now - 1;
	records.push_back(already_expired);

	collector_maintenance_pulse();
	assert(!collector_maintenance_health_copy().ready && submitted_actions.empty());

	cache_ready = true;
	busy.insert(2);
	collector_maintenance_pulse();
	assert(submitted_actions.size() == 1 && submitted_listings[0] == 1 &&
	       submitted_times[0] == now);
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
			   [now](uint64_t value) { return value == now; }));
	for (size_t index = 0; index < 3; ++index)
		assert(records[index].holding_paused && records[index].paused_at == now);
	assert(!records[3].holding_paused && records[3].expires_at == now - 1);

	collector_maintenance_pulse();
	assert(submitted_actions.size() == 3);

	config.policy.enabled = true;
	config.enabled_changed_at = now + 1000;
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
		assert(submitted_times[index] == now + 1000);
	}
	for (size_t index = 0; index < 3; ++index)
		assert(!records[index].holding_paused && !records[index].paused_at &&
		       records[index].expires_at == now + 6000);
	assert(!records[3].holding_paused && records[3].expires_at == now - 1);

	collector::record collected = {};
	collected.listing = 10;
	collected.uid = 1010;
	collected.item_revision = 3;
	collected.revision = 1;
	collected.status = collector::state::collected;
	collected.sale_at = now - 1;
	records.push_back(collected);
	collector::record expired = {};
	expired.listing = 11;
	expired.uid = 1011;
	expired.item_revision = 5;
	expired.revision = 1;
	expired.status = collector::state::available;
	expired.available_at = now - 100;
	expired.expires_at = now - 1;
	records.push_back(expired);
	due_once = { 10, 11 };
	config.revision = 3;
	collector_maintenance_pulse();
	assert(records[4].status == collector::state::available);
	assert(collector_maintenance_health_copy().pending_expiry_reads == 1);
	collector_maintenance_pulse();
	assert(records[5].status == collector::state::expired &&
	       records[5].closed_reason == collector::reason::holding_elapsed);

	const collector_maintenance_health health = collector_maintenance_health_copy();
	assert(health.ready && health.enabled && health.config_revision == 3);
	assert(health.submitted == 8 && health.completions == 8 && health.committed == 8);
	assert(health.listing_busy == 1 && health.publication_failures == 1 &&
	       health.recovery_refreshes == 1 && refreshes == 1);
	assert(health.due_passes == 2 && health.due_leased == 2 &&
	       health.activation_submissions == 1 && health.expiry_reads == 1 &&
	       health.expiry_results == 1 && health.expiry_submissions == 1 &&
	       !health.pending_expiry_reads);
	assert(health.ineligible > 0);
	assert(!health.rejected && !health.submit_failures && !health.scan_failures);
	collector_maintenance_shutdown();
	assert(!collector_maintenance_health_copy().ready);

	// A projection read failure backs off for the configured interval instead of
	// retrying on every game pulse.
	scan_succeeds = false;
	config.revision = 4;
	collector_maintenance_pulse();
	assert(!collector_maintenance_health_copy().reconciling &&
	       collector_maintenance_health_copy().scan_failures == 1);
	collector_maintenance_pulse();
	assert(collector_maintenance_health_copy().scan_failures == 1);

	// Merely observing a mismatch is not progress. A permanently busy listing ends
	// this pass and is retried at the next audit rather than spinning immediately.
	collector_maintenance_shutdown();
	scan_succeeds = true;
	records.clear();
	busy.clear();
	collector::record blocked = {};
	blocked.listing = 99;
	blocked.status = collector::state::available;
	blocked.available_at = now - 1;
	blocked.expires_at = now + 1000;
	blocked.revision = 1;
	records.push_back(blocked);
	busy.insert(blocked.listing);
	config.policy.enabled = false;
	config.enabled_changed_at = now;
	config.revision = 5;
	collector_maintenance_pulse();
	assert(!collector_maintenance_health_copy().reconciling &&
	       collector_maintenance_health_copy().listing_busy == 1);
	collector_maintenance_pulse();
	assert(collector_maintenance_health_copy().listing_busy == 1);
	collector_maintenance_shutdown();
}
