#include "economy/collector_runtime.h"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <new>
#include <utility>

namespace
{
std::map<uint64_t, collector::record> records;
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
	due.swap(projection->due);
	catalog_revision = catalog.revision;
	next_listing = catalog.next_listing;
	available_count = projection->available;
	projection->available = 0;
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
					     size_t held_count)
{
	catalog_projection projection;
	if (!build_projection(catalog, &projection) ||
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
