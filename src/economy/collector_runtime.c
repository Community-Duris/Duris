#include "economy/collector_runtime.h"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <new>
#include <set>
#include <utility>

namespace
{
std::map<uint64_t, collector::record> records;
std::map<std::pair<uint32_t, uint64_t>, collector_death_snapshot> deaths_by_identity;
collector::due_queue due;
uint64_t catalog_revision = 0;
uint64_t next_listing = 1;
size_t available_count = 0;

bool available(const collector::record &entry)
{
	return entry.status == collector::state::available && !entry.holding_paused;
}

bool action_matches(const collector_command_result &result)
{
	switch (result.action)
	{
	case collector_action::collect:
		return result.entry.status == collector::state::collected;
	case collector_action::activate:
		return result.entry.status == collector::state::available &&
		       !result.entry.holding_paused;
	case collector_action::purchase:
		return result.entry.status == collector::state::purchased;
	case collector_action::expire:
		return result.entry.status == collector::state::expired;
	case collector_action::cancel:
		return result.entry.status == collector::state::cancelled;
	case collector_action::pause:
		return result.entry.status == collector::state::available &&
		       result.entry.holding_paused;
	case collector_action::resume:
		return result.entry.status == collector::state::available &&
		       !result.entry.holding_paused;
	case collector_action::unknown:
		return false;
	}
	return false;
}

bool same_record(const collector::record &left, const collector::record &right)
{
	std::array<uint8_t, collector::encoded_record_bytes> left_bytes = {}, right_bytes = {};
	return collector::record_encode(left, &left_bytes) == collector::codec_result::ok &&
	       collector::record_encode(right, &right_bytes) == collector::codec_result::ok &&
	       left_bytes == right_bytes;
}

struct catalog_projection
{
	std::map<uint64_t, collector::record> records;
	std::map<std::pair<uint32_t, uint64_t>, collector_death_snapshot> deaths;
	collector::due_queue due;
	size_t available = 0;
};

bool build_projection(const collector::catalog &catalog, catalog_projection *projection)
{
	if (!projection || !collector::valid_catalog(catalog))
		return false;
	try
	{
		for (const collector::record &entry : catalog.records)
		{
			if (!projection->records.emplace(entry.listing, entry).second ||
			    !projection->due.update(entry))
				return false;
			const auto previous = records.find(entry.listing);
			uint64_t previous_deadline = 0, rebuilt_deadline = 0;
			if (previous != records.end() && same_record(previous->second, entry) &&
			    due.deadline(entry.listing, &previous_deadline) &&
			    projection->due.deadline(entry.listing, &rebuilt_deadline) &&
			    previous_deadline > rebuilt_deadline &&
			    !projection->due.defer(entry.listing, previous_deadline))
				return false;
			if (available(entry))
				++projection->available;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

void install_projection(const collector::catalog &catalog, catalog_projection *projection) noexcept
{
	records.swap(projection->records);
	deaths_by_identity.swap(projection->deaths);
	due.swap(projection->due);
	catalog_revision = catalog.revision;
	next_listing = catalog.next_listing;
	available_count = projection->available;
	projection->available = 0;
}

bool build_death_projection(const collector_death_snapshot *deaths, size_t death_count,
			    catalog_projection *projection)
{
	if (!projection || (!deaths && death_count) || death_count > collector::catalog_max_records)
		return false;
	std::set<std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>> operation_ids;
	try
	{
		for (size_t index = 0; index < death_count; ++index)
		{
			const collector_death_snapshot &death = deaths[index];
			if (std::all_of(death.operation_id.bytes.begin(),
					death.operation_id.bytes.end(),
					[](uint8_t value) { return value == 0; }) ||
			    !death.beneficiary_pid || !death.death_time || !death.policy.enabled ||
			    !collector::valid_rules(death.policy) || death.hint_state > 2 ||
			    !operation_ids.insert(death.operation_id.bytes).second ||
			    !projection->deaths
				     .emplace(std::make_pair(death.beneficiary_pid, death.death_time),
					      death)
				     .second)
				return false;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool held_projection_matches(const catalog_projection &projection,
			     const item_ownership_runtime_entry *held_items, size_t held_count)
{
	if (!held_items && held_count)
		return false;
	size_t expected = 0;
	for (const auto &[listing, entry] : projection.records)
	{
		(void)listing;
		if (entry.status == collector::state::collected ||
		    entry.status == collector::state::available)
			++expected;
	}
	if (expected != held_count)
		return false;
	for (size_t index = 0; index < held_count; ++index)
	{
		const item_ownership_runtime_entry &held = held_items[index];
		const auto found = projection.records.find(held.owner.id);
		if (held.owner.type != item_owner_type::collector || held.owner.context_id ||
		    found == projection.records.end() || found->second.listing != held.owner.id ||
		    found->second.uid != held.item_uid ||
		    found->second.item_revision != held.item_revision ||
		    (found->second.status != collector::state::collected &&
		     found->second.status != collector::state::available))
			return false;
	}
	return true;
}
} // namespace

bool collector_runtime_rebuild(const collector::catalog &catalog)
{
	catalog_projection projection;
	if (!build_projection(catalog, &projection))
		return false;
	install_projection(catalog, &projection);
	return true;
}

bool collector_runtime_rebuild_authoritative(const collector::catalog &catalog,
					     const item_ownership_runtime_entry *held_items,
					     size_t held_count,
					     const collector_death_snapshot *deaths,
					     size_t death_count)
{
	catalog_projection projection;
	if (!build_projection(catalog, &projection) ||
	    !build_death_projection(deaths, death_count, &projection) ||
	    !held_projection_matches(projection, held_items, held_count) ||
	    !item_ownership_runtime_reconcile_collector(held_items, held_count))
		return false;
	install_projection(catalog, &projection);
	return true;
}

bool collector_runtime_publish(const collector_command_result &result)
{
	if (!result.record_present || !result.catalog_revision || !action_matches(result) ||
	    !collector::valid_record(result.entry) ||
	    result.entry.listing == std::numeric_limits<uint64_t>::max())
		return false;
	auto found = records.find(result.entry.listing);
	if (found != records.end())
	{
		if (found->second.revision > result.entry.revision)
		{
			catalog_revision = std::max(catalog_revision, result.catalog_revision);
			return true;
		}
		if (found->second.revision == result.entry.revision)
		{
			if (!same_record(found->second, result.entry))
				return false;
			catalog_revision = std::max(catalog_revision, result.catalog_revision);
			return true;
		}
	}
	if (found == records.end() && records.size() >= collector::catalog_max_records)
		return false;
	const bool was_available = found != records.end() && available(found->second);
	try
	{
		if (!due.update(result.entry))
			return false;
		if (found == records.end())
		{
			try
			{
				found = records.emplace(result.entry.listing, result.entry).first;
			}
			catch (...)
			{
				due.erase(result.entry.listing);
				throw;
			}
		}
		else
			found->second = result.entry;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	const bool now_available = available(result.entry);
	if (was_available != now_available)
		available_count = now_available ? available_count + 1 : available_count - 1;
	catalog_revision = std::max(catalog_revision, result.catalog_revision);
	next_listing = std::max(next_listing, result.entry.listing + 1);
	return true;
}

bool collector_runtime_find(uint64_t listing, collector::record *entry)
{
	if (!entry)
		return false;
	const auto found = records.find(listing);
	if (found == records.end())
		return false;
	*entry = found->second;
	return true;
}

bool collector_runtime_find_death(uint32_t beneficiary_pid, uint64_t death_time,
				  collector_death_snapshot *death)
{
	if (!beneficiary_pid || !death_time || !death)
		return false;
	const auto found = deaths_by_identity.find({ beneficiary_pid, death_time });
	if (found == deaths_by_identity.end())
		return false;
	*death = found->second;
	return true;
}

bool collector_runtime_snapshot(collector::catalog *catalog)
{
	if (!catalog)
		return false;
	collector::catalog candidate;
	candidate.revision = catalog_revision;
	candidate.next_listing = next_listing;
	try
	{
		candidate.records.reserve(records.size());
		for (const auto &[listing, entry] : records)
		{
			(void)listing;
			candidate.records.push_back(entry);
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (!collector::valid_catalog(candidate))
		return false;
	*catalog = std::move(candidate);
	return true;
}

bool collector_runtime_available_for(uint32_t beneficiary, size_t limit,
				     std::vector<collector::record> *entries)
{
	if (!beneficiary || !entries || !limit)
		return false;
	std::vector<collector::record> candidate;
	try
	{
		candidate.reserve(std::min(limit, available_count));
		for (const auto &[listing, entry] : records)
		{
			(void)listing;
			if (available(entry) && entry.beneficiary == beneficiary)
			{
				if (candidate.size() == limit)
					break;
				candidate.push_back(entry);
			}
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	*entries = std::move(candidate);
	return true;
}

std::vector<uint64_t> collector_runtime_lease_due(uint64_t now, size_t limit, uint64_t lease_until)
{
	try
	{
		return due.lease_due(now, limit, lease_until);
	}
	catch (const std::bad_alloc &)
	{
		return {};
	}
}

bool collector_runtime_pause_mismatches(bool should_pause, uint64_t after_listing,
					size_t scan_limit, size_t result_limit,
					std::vector<collector::record> *entries,
					uint64_t *next_after_listing, bool *reached_end)
{
	if (!scan_limit || !result_limit || !entries || !next_after_listing || !reached_end)
		return false;
	std::vector<collector::record> candidate;
	auto current = after_listing ? records.upper_bound(after_listing) : records.begin();
	size_t scanned = 0;
	uint64_t cursor = after_listing;
	try
	{
		candidate.reserve(std::min(scan_limit, result_limit));
		while (current != records.end() && scanned < scan_limit &&
		       candidate.size() < result_limit)
		{
			cursor = current->first;
			const collector::record &entry = current->second;
			++current;
			++scanned;
			if (entry.status == collector::state::available &&
			    entry.holding_paused != should_pause)
				candidate.push_back(entry);
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	*reached_end = current == records.end();
	*next_after_listing = *reached_end ? 0 : cursor;
	*entries = std::move(candidate);
	return true;
}

uint64_t collector_runtime_catalog_revision(void)
{
	return catalog_revision;
}

uint64_t collector_runtime_next_listing(void)
{
	return next_listing;
}

size_t collector_runtime_size(void)
{
	return records.size();
}

size_t collector_runtime_available_count(void)
{
	return available_count;
}

void collector_runtime_reset(void)
{
	records.clear();
	deaths_by_identity.clear();
	due = {};
	catalog_revision = 0;
	next_listing = 1;
	available_count = 0;
}

bool collector_publish_committed_event(const collector_command_result &result,
				       unsigned long long outbox_id)
{
	return outbox_id && collector_runtime_publish(result);
}
