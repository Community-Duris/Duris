#include "economy/collector_policy.h"

#include <algorithm>
#include <limits>

namespace collector
{
namespace
{
bool add(uint64_t left, uint64_t right, uint64_t *sum)
{
	if (right > std::numeric_limits<uint64_t>::max() - left)
		return false;
	*sum = left + right;
	return true;
}

outcome check(const record *entry, uint64_t expected, state required)
{
	if (!entry || entry->version != record_version || !entry->listing || !entry->uid ||
	    !entry->beneficiary || !valid_rules(entry->policy) ||
	    entry->status < state::candidate || entry->status > state::expired)
		return outcome::invalid;
	if (entry->revision != expected || entry->status != required)
		return outcome::conflict;
	if (entry->revision == std::numeric_limits<uint64_t>::max())
		return outcome::overflow;
	return outcome::applied;
}
}

bool valid_rules(const rules &policy)
{
	return policy.collection_delay && policy.sale_delay >= policy.collection_delay &&
	       policy.holding_duration && policy.price_percent && policy.minimum_value;
}

bool terminal(state status)
{
	return status == state::purchased || status == state::cancelled || status == state::expired;
}

outcome price(int64_t base_value, const rules &policy, uint64_t *value)
{
	if (!value || !valid_rules(policy))
		return outcome::invalid;
	// Negative/zero recorded values receive the minimum fee. Divide before
	// multiplying, then round fractional copper upward without overflowing.
	const uint64_t base = base_value > 0 ? static_cast<uint64_t>(base_value) : 0;
	const uint64_t whole = policy.price_percent / 100;
	const uint64_t remainder = policy.price_percent % 100;
	const uint64_t maximum = std::numeric_limits<uint64_t>::max();
	if (whole && base > maximum / whole)
		return outcome::overflow;
	uint64_t result = 0;
	const uint64_t fractional =
		(base / 100) * remainder + ((base % 100) * remainder + 99) / 100;
	if (!add(base * whole, fractional, &result))
		return outcome::overflow;
	*value = std::max(result, policy.minimum_value);
	return outcome::applied;
}

outcome enroll(uint64_t listing, const std::string &death_operation, uint32_t beneficiary,
	       uint64_t uid, uint64_t item_revision, uint64_t death_time, const rules &policy,
	       record *result)
{
	if (!result || !listing || death_operation.size() != 32 ||
	    death_operation.find_first_not_of("0123456789abcdef") != std::string::npos ||
	    death_operation == std::string(32, '0') || !beneficiary || !uid || !item_revision ||
	    !policy.enabled || !valid_rules(policy))
		return outcome::invalid;
	record candidate;
	if (!add(death_time, policy.collection_delay, &candidate.collect_at) ||
	    !add(death_time, policy.sale_delay, &candidate.sale_at))
		return outcome::overflow;
	candidate.listing = listing;
	candidate.death_operation = death_operation;
	candidate.beneficiary = beneficiary;
	candidate.uid = uid;
	candidate.item_revision = item_revision;
	candidate.death_time = death_time;
	candidate.revision = 1;
	candidate.policy = policy;
	*result = candidate;
	return outcome::applied;
}

outcome cancel(record *entry, uint64_t expected_revision, reason why)
{
	if (!entry || why <= reason::none || why >= reason::holding_elapsed)
		return outcome::invalid;
	const auto checked = check(entry, expected_revision, entry->status);
	if (checked != outcome::applied)
		return checked;
	if (terminal(entry->status) ||
	    (why == reason::claimed && entry->status != state::candidate))
		return outcome::conflict;
	entry->status = state::cancelled;
	entry->closed_reason = why;
	++entry->revision;
	return outcome::applied;
}

outcome collect(record *entry, uint64_t expected_revision, uint64_t expected_item_revision,
		uint64_t current_item_revision, bool active_unclaimed, int64_t current_base_value,
		uint64_t now)
{
	const auto checked = check(entry, expected_revision, state::candidate);
	if (checked != outcome::applied)
		return checked;
	if (now < entry->collect_at)
		return outcome::not_due;
	if (!active_unclaimed || !current_item_revision ||
	    expected_item_revision != current_item_revision ||
	    current_item_revision < entry->item_revision)
		return outcome::conflict;
	if (current_item_revision == std::numeric_limits<uint64_t>::max())
		return outcome::overflow;
	uint64_t gold = 0;
	const auto priced = price(current_base_value, entry->policy, &gold);
	if (priced != outcome::applied)
		return priced;
	entry->price_value = gold;
	entry->item_revision = current_item_revision + 1;
	entry->status = state::collected;
	++entry->revision;
	return outcome::applied;
}

outcome activate(record *entry, uint64_t expected_revision, uint64_t now)
{
	const auto checked = check(entry, expected_revision, state::collected);
	if (checked != outcome::applied)
		return checked;
	if (now < entry->sale_at)
		return outcome::not_due;
	uint64_t expiry = 0;
	if (!add(now, entry->policy.holding_duration, &expiry))
		return outcome::overflow;
	entry->available_at = now;
	entry->expires_at = expiry;
	entry->status = state::available;
	++entry->revision;
	return outcome::applied;
}

outcome purchase(record *entry, uint64_t expected_revision, uint32_t actor, uint64_t carried_value,
		 bool has_capacity, uint64_t now)
{
	const auto checked = check(entry, expected_revision, state::available);
	if (checked != outcome::applied)
		return checked;
	if (!actor || actor != entry->beneficiary)
		return outcome::forbidden;
	if (entry->holding_paused || now < entry->available_at || now >= entry->expires_at)
		return outcome::conflict;
	if (!has_capacity)
		return outcome::capacity;
	if (carried_value < entry->price_value)
		return outcome::insufficient_funds;
	if (entry->item_revision == std::numeric_limits<uint64_t>::max())
		return outcome::overflow;
	entry->status = state::purchased;
	++entry->item_revision;
	++entry->revision;
	return outcome::applied;
}

outcome expire(record *entry, uint64_t expected_revision, uint64_t now)
{
	const auto checked = check(entry, expected_revision, state::available);
	if (checked != outcome::applied)
		return checked;
	if (entry->holding_paused || now < entry->expires_at)
		return outcome::not_due;
	if (entry->item_revision == std::numeric_limits<uint64_t>::max())
		return outcome::overflow;
	entry->status = state::expired;
	entry->closed_reason = reason::holding_elapsed;
	++entry->item_revision;
	++entry->revision;
	return outcome::applied;
}

outcome pause(record *entry, uint64_t expected_revision, uint64_t now)
{
	const auto checked = check(entry, expected_revision, state::available);
	if (checked != outcome::applied)
		return checked;
	if (entry->holding_paused || now < entry->available_at)
		return outcome::conflict;
	entry->holding_paused = true;
	entry->paused_at = now;
	++entry->revision;
	return outcome::applied;
}

outcome resume(record *entry, uint64_t expected_revision, uint64_t now)
{
	const auto checked = check(entry, expected_revision, state::available);
	if (checked != outcome::applied)
		return checked;
	if (!entry->holding_paused || now < entry->paused_at)
		return outcome::conflict;
	uint64_t expiry = 0;
	if (!add(entry->expires_at, now - entry->paused_at, &expiry))
		return outcome::overflow;
	entry->expires_at = expiry;
	entry->holding_paused = false;
	entry->paused_at = 0;
	++entry->revision;
	return outcome::applied;
}

bool due_queue::update(const record &entry)
{
	if (!entry.listing || entry.version != record_version)
		return false;
	uint64_t deadline = 0;
	switch (entry.status)
	{
	case state::candidate:
		deadline = entry.collect_at;
		break;
	case state::collected:
		deadline = entry.sale_at;
		break;
	case state::available:
		if (entry.holding_paused)
		{
			erase(entry.listing);
			return true;
		}
		deadline = entry.expires_at;
		break;
	case state::purchased:
	case state::cancelled:
	case state::expired:
		erase(entry.listing);
		return true;
	default:
		return false;
	}
	// Insert before erasing the old deadline so allocation failure leaves the
	// existing work scheduled. The listing map must be restored on failure.
	const auto previous = by_listing.find(entry.listing);
	if (previous != by_listing.end() && previous->second == deadline)
		return true;
	by_deadline.emplace(std::make_pair(deadline, entry.listing), true);
	try
	{
		if (previous == by_listing.end())
			by_listing.emplace(entry.listing, deadline);
		else
		{
			by_deadline.erase({ previous->second, entry.listing });
			previous->second = deadline;
		}
	}
	catch (...)
	{
		by_deadline.erase({ deadline, entry.listing });
		throw;
	}
	return true;
}

void due_queue::erase(uint64_t listing)
{
	const auto found = by_listing.find(listing);
	if (found == by_listing.end())
		return;
	by_deadline.erase({ found->second, listing });
	by_listing.erase(found);
}

std::vector<uint64_t> due_queue::due(uint64_t now, size_t limit) const
{
	std::vector<uint64_t> result;
	for (auto it = by_deadline.begin();
	     it != by_deadline.end() && it->first.first <= now && result.size() < limit; ++it)
		result.push_back(it->first.second);
	return result;
}
}
