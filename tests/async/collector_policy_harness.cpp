#include "economy/collector_policy.h"

#include <cassert>
#include <iostream>
#include <limits>

using namespace collector;

namespace
{
record candidate(uint64_t listing = 1, uint64_t uid = 101)
{
	rules policy;
	policy.enabled = true;
	record entry;
	assert(enroll(listing, "123456789abcdef0123456789abcdef0", 42, uid, 5, 1000, policy,
		      &entry) == outcome::applied);
	return entry;
}

record saleable()
{
	auto entry = candidate();
	assert(collect(&entry, 1, 5, 5, true, 75, entry.collect_at) == outcome::applied);
	assert(activate(&entry, 2, entry.sale_at) == outcome::applied);
	return entry;
}
}

int main()
{
	const auto maximum = std::numeric_limits<uint64_t>::max();
	rules defaults;
	assert(!defaults.enabled && defaults.collection_delay == 43200 &&
	       defaults.sale_delay == 86400 && defaults.holding_duration == 604800 &&
	       defaults.price_percent == 200 && defaults.minimum_value == 100);
	record untouched;
	assert(enroll(1, "123456789abcdef0123456789abcdef0", 42, 101, 5, 1000, defaults,
		      &untouched) == outcome::invalid);
	assert(!untouched.listing);
	defaults.enabled = true;
	assert(enroll(1, "123456789abcdef0123456789abcdef0", 42, 101, 5, maximum, defaults,
		      &untouched) == outcome::overflow);
	assert(!untouched.listing);

	uint64_t gold = 0;
	assert(price(0, defaults, &gold) == outcome::applied && gold == 100);
	assert(price(-7, defaults, &gold) == outcome::applied && gold == 100);
	assert(price(75, defaults, &gold) == outcome::applied && gold == 150);
	defaults.price_percent = 101;
	assert(price(1, defaults, &gold) == outcome::applied && gold == 100);
	defaults.price_percent = maximum;
	assert(price(100, defaults, &gold) == outcome::applied && gold == maximum);
	assert(price(101, defaults, &gold) == outcome::overflow && gold == maximum);
	assert(price(std::numeric_limits<int64_t>::max(), defaults, &gold) == outcome::overflow);

	// The sword was looted, but the untouched shield keeps its own entitlement.
	auto sword = candidate();
	auto shield = candidate(2, 102);
	assert(cancel(&sword, 1, reason::claimed) == outcome::applied);
	assert(collect(&sword, 2, 5, 5, true, 75, sword.collect_at) == outcome::conflict);
	assert(cancel(&sword, 2, reason::claimed) == outcome::conflict);
	assert(shield.status == state::candidate && shield.revision == 1);
	// A later death of the same UID gets a distinct record; the old one stays closed.
	record later;
	assert(enroll(3, "223456789abcdef0123456789abcdef0", 42, sword.uid, 8, 2000, shield.policy,
		      &later) == outcome::applied);
	assert(later.uid == sword.uid && later.status == state::candidate &&
	       sword.status == state::cancelled);

	assert(collect(&shield, 1, 5, 5, true, 75, shield.collect_at - 1) == outcome::not_due);
	assert(collect(&shield, 0, 5, 5, true, 75, shield.collect_at) == outcome::conflict);
	assert(collect(&shield, 1, 5, 6, true, 75, shield.collect_at) == outcome::conflict);
	assert(collect(&shield, 1, 5, 5, false, 75, shield.collect_at) == outcome::conflict);
	assert(shield.revision == 1 && !shield.price_value);
	// Environmental release may advance custody revision without cancelling.
	assert(collect(&shield, 1, 6, 6, true, 75, shield.collect_at) == outcome::applied);
	assert(shield.item_revision == 7 && shield.price_value == 150);
	assert(collect(&shield, 1, 6, 6, true, 1, shield.collect_at) == outcome::conflict);
	assert(shield.price_value == 150);
	assert(activate(&shield, 2, shield.sale_at - 1) == outcome::not_due);
	const uint64_t delayed = shield.sale_at + 5 * 86400;
	assert(activate(&shield, 2, delayed) == outcome::applied);
	assert(shield.available_at == delayed && shield.expires_at == delayed + 604800);
	assert(purchase(&shield, 3, 43, 150, true, delayed) == outcome::forbidden);
	assert(purchase(&shield, 3, 42, 149, true, delayed) == outcome::insufficient_funds);
	assert(purchase(&shield, 3, 42, 150, false, delayed) == outcome::capacity);
	assert(shield.status == state::available && shield.revision == 3);
	assert(purchase(&shield, 3, 42, 150, true, delayed) == outcome::applied);
	assert(purchase(&shield, 3, 42, 150, true, delayed) == outcome::conflict);
	assert(expire(&shield, 4, maximum) == outcome::conflict);
	assert(shield.uid == 102 && shield.beneficiary == 42 && shield.price_value == 150);

	auto expired = saleable();
	assert(expire(&expired, 3, expired.expires_at - 1) == outcome::not_due);
	assert(purchase(&expired, 3, 42, 150, true, expired.expires_at) == outcome::conflict);
	assert(expire(&expired, 3, expired.expires_at) == outcome::applied);
	assert(expired.status == state::expired &&
	       expired.closed_reason == reason::holding_elapsed);
	assert(expire(&expired, 4, maximum) == outcome::conflict);

	auto paused = saleable();
	const auto original_expiry = paused.expires_at;
	assert(pause(&paused, 3, paused.available_at + 60) == outcome::applied);
	assert(purchase(&paused, 4, 42, 150, true, paused.available_at + 61) == outcome::conflict);
	assert(expire(&paused, 4, maximum) == outcome::not_due);
	assert(resume(&paused, 4, paused.paused_at - 1) == outcome::conflict);
	assert(resume(&paused, 4, paused.paused_at + 86400) == outcome::applied);
	assert(paused.expires_at == original_expiry + 86400);
	assert(expire(&paused, 5, paused.expires_at - 1) == outcome::not_due);
	assert(expire(&paused, 5, paused.expires_at) == outcome::applied);

	// Disabling after an expiry deadline must not resurrect the listing.
	paused = saleable();
	assert(pause(&paused, 3, paused.expires_at + 1) == outcome::applied);
	const auto resume_time = paused.paused_at + 86400;
	assert(resume(&paused, 4, resume_time) == outcome::applied);
	assert(purchase(&paused, 5, 42, 150, true, resume_time) == outcome::conflict);
	assert(expire(&paused, 5, resume_time) == outcome::applied);

	for (auto why : { reason::destroyed, reason::quarantined, reason::excluded,
			  reason::character_deleted, reason::season_reset })
	{
		auto entry = saleable();
		assert(cancel(&entry, 3, why) == outcome::applied);
		assert(entry.closed_reason == why && entry.status == state::cancelled);
		assert(purchase(&entry, 4, 42, maximum, true, entry.available_at) ==
		       outcome::conflict);
	}

	due_queue queue;
	for (uint64_t index = 1; index <= 100000; ++index)
		assert(queue.update(candidate(index, index)));
	assert(queue.size() == 100000);
	assert(queue.due(44199, 64).empty());
	assert(queue.due(maximum, 0).empty());
	const auto batch = queue.due(44200, 64);
	assert(batch.size() == 64 && batch.front() == 1 && batch.back() == 64);
	// Reading work never drops it. Retry sees the same stable listings.
	assert(batch == queue.due(44200, 64));
	auto entry = saleable();
	assert(queue.update(entry));
	assert(queue.size() == 100000 && queue.due(44200, 1).front() == 2);
	assert(pause(&entry, 3, entry.available_at) == outcome::applied);
	assert(queue.update(entry) && queue.size() == 99999);
	assert(resume(&entry, 4, entry.available_at + 1) == outcome::applied);
	assert(queue.update(entry) && queue.size() == 100000);
	assert(purchase(&entry, 5, 42, 150, true, entry.available_at + 1) == outcome::applied);
	assert(queue.update(entry) && queue.size() == 99999);
	auto malformed = candidate(100001, 100001);
	malformed.collect_at++;
	assert(!valid_record(malformed) && !queue.update(malformed) && queue.size() == 99999);
	std::cout
		<< "collector policy: timing, custody conflicts, privacy, prices, pause, terminal "
		   "states and bounded 100000-item scheduling passed\n";
}
