#include "economy/collector_runtime.h"

#include <cassert>
#include <cstring>
#include <vector>

namespace
{
constexpr char death_one[] = "11111111111111111111111111111111";
constexpr char death_two[] = "22222222222222222222222222222222";

collector::record candidate(uint64_t listing, const char *death, uint32_t beneficiary, uint64_t uid,
			    uint64_t death_time)
{
	collector::rules policy;
	policy.enabled = true;
	collector::record entry;
	assert(collector::enroll(listing, death, beneficiary, uid, 1, death_time, policy, &entry) ==
	       collector::outcome::applied);
	return entry;
}

collector_command_result result(collector_action action, uint64_t catalog_revision,
				collector::record entry)
{
	collector_command_result value;
	value.action = action;
	value.record_present = true;
	value.catalog_revision = catalog_revision;
	value.entry = entry;
	return value;
}
}

int main()
{
	collector_runtime_reset();
	auto first = candidate(4, death_one, 42, 100, 1000);
	auto second = candidate(8, death_two, 43, 200, 2000);
	collector::catalog initial;
	initial.revision = 9;
	initial.next_listing = 9;
	initial.records = { first, second };
	assert(collector_runtime_rebuild(initial));
	assert(collector_runtime_size() == 2 && collector_runtime_available_count() == 0 &&
	       collector_runtime_catalog_revision() == 9 && collector_runtime_next_listing() == 9);
	auto due = collector_runtime_lease_due(first.collect_at, 1, first.collect_at + 60);
	assert(due.size() == 1 && due[0] == 4);

	assert(collector::collect(&first, first.revision, 1, 1, true, 75, first.collect_at) ==
	       collector::outcome::applied);
	auto collected = result(collector_action::collect, 10, first);
	assert(collector_runtime_publish(collected));
	assert(collector_runtime_publish(collected));
	assert(collector_runtime_catalog_revision() == 10);
	collector::record found;
	assert(collector_runtime_find(4, &found) && found.status == collector::state::collected);

	assert(collector::activate(&first, first.revision, first.sale_at + 25) ==
	       collector::outcome::applied);
	auto activated = result(collector_action::activate, 11, first);
	assert(collector_runtime_publish(activated));
	assert(collector_runtime_available_count() == 1);
	std::vector<collector::record> visible;
	assert(collector_runtime_available_for(42, 20, &visible) && visible.size() == 1 &&
	       visible[0].listing == 4);
	assert(collector_runtime_available_for(43, 20, &visible) && visible.empty());

	auto purchased_entry = first;
	assert(collector::purchase(&purchased_entry, purchased_entry.revision, 42,
				   purchased_entry.price_value, true,
				   purchased_entry.available_at) == collector::outcome::applied);
	auto purchased = result(collector_action::purchase, 13, purchased_entry);
	assert(collector_runtime_publish(purchased));
	assert(collector_runtime_available_count() == 0 &&
	       collector_runtime_catalog_revision() == 13);
	// A delayed earlier outbox event is delivered idempotently without
	// regressing the listing that has already advanced.
	assert(collector_runtime_publish(activated));
	assert(collector_runtime_find(4, &found) && found.status == collector::state::purchased);
	assert(!collector_publish_committed_event(purchased, 0));
	assert(collector_publish_committed_event(purchased, 77));

	// A distinct older event still has to be inserted even though another
	// listing advanced the global catalog revision first.
	auto third = candidate(12, death_one, 42, 300, 3000);
	assert(collector::collect(&third, third.revision, 1, 1, true, 50, third.collect_at) ==
	       collector::outcome::applied);
	assert(collector_runtime_publish(result(collector_action::collect, 12, third)));
	assert(collector_runtime_size() == 3 && collector_runtime_catalog_revision() == 13 &&
	       collector_runtime_next_listing() == 13);

	collector::catalog snapshot;
	assert(collector_runtime_snapshot(&snapshot) && snapshot.records.size() == 3 &&
	       snapshot.records[0].listing == 4 && snapshot.records[1].listing == 8 &&
	       snapshot.records[2].listing == 12 && snapshot.next_listing == 13);
	auto conflicting = purchased;
	conflicting.entry.price_value++;
	assert(!collector_runtime_publish(conflicting));
	collector::catalog invalid = snapshot;
	invalid.next_listing = 12;
	assert(!collector_runtime_rebuild(invalid));
	assert(collector_runtime_find(4, &found) && found.status == collector::state::purchased);

	collector_runtime_reset();
	assert(collector_runtime_size() == 0 && collector_runtime_catalog_revision() == 0 &&
	       collector_runtime_next_listing() == 1);
}
