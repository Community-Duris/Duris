#include "economy/collector_runtime.h"

#include <cassert>
#include <cstring>
#include <vector>

namespace
{
size_t authority_publications = 0;
std::vector<item_ownership_runtime_entry> published_authority;
}

bool item_ownership_runtime_reconcile_collector(const item_ownership_runtime_entry *batch,
						size_t count)
{
	++authority_publications;
	if (count)
		published_authority.assign(batch, batch + count);
	else
		published_authority.clear();
	return true;
}

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
	std::vector<collector::record> pause_entries;
	uint64_t pause_cursor = 0;
	bool pause_end = false;
	assert(collector_runtime_pause_mismatches(true, 0, 1, 1, &pause_entries, &pause_cursor,
						  &pause_end));
	assert(pause_entries.size() == 1 && pause_entries[0].listing == 4 && !pause_end &&
	       pause_cursor == 4);
	assert(collector_runtime_pause_mismatches(true, pause_cursor, 1, 1, &pause_entries,
						  &pause_cursor, &pause_end));
	assert(pause_entries.empty() && pause_end && pause_cursor == 0);

	auto paused_entry = first;
	assert(collector::pause(&paused_entry, paused_entry.revision,
				paused_entry.available_at + 5) == collector::outcome::applied);
	assert(collector_runtime_publish(result(collector_action::pause, 12, paused_entry)));
	assert(collector_runtime_available_count() == 0);
	assert(collector_runtime_pause_mismatches(false, 0, 8, 8, &pause_entries, &pause_cursor,
						  &pause_end));
	assert(pause_entries.size() == 1 && pause_entries[0].listing == 4 && pause_end);
	assert(collector::resume(&paused_entry, paused_entry.revision,
				 paused_entry.paused_at + 5) == collector::outcome::applied);
	assert(collector_runtime_publish(result(collector_action::resume, 13, paused_entry)));
	assert(collector_runtime_available_count() == 1);
	first = paused_entry;

	auto purchased_entry = first;
	assert(collector::purchase(&purchased_entry, purchased_entry.revision, 42,
				   purchased_entry.price_value, true,
				   purchased_entry.available_at) == collector::outcome::applied);
	auto purchased = result(collector_action::purchase, 14, purchased_entry);
	assert(collector_runtime_publish(purchased));
	assert(collector_runtime_available_count() == 0 &&
	       collector_runtime_catalog_revision() == 14);
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
	assert(collector_runtime_size() == 3 && collector_runtime_catalog_revision() == 14 &&
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
	const item_ownership_runtime_entry held = {
		third.uid,
		third.uid,
		0,
		{ item_owner_type::collector, third.listing, 0 },
		third.item_revision,
		1,
		9012,
		item_custody_state::active,
	};
	assert(!collector_runtime_rebuild_authoritative(snapshot, nullptr, 0));
	assert(authority_publications == 0);
	auto mismatched = held;
	mismatched.item_uid++;
	mismatched.root_item_uid++;
	assert(!collector_runtime_rebuild_authoritative(snapshot, &mismatched, 1));
	assert(authority_publications == 0);
	assert(collector_runtime_rebuild_authoritative(snapshot, &held, 1));
	assert(authority_publications == 1 && published_authority.size() == 1 &&
	       published_authority[0].item_uid == third.uid);

	collector_runtime_reset();
	assert(collector_runtime_size() == 0 && collector_runtime_catalog_revision() == 0 &&
	       collector_runtime_next_listing() == 1);
}
