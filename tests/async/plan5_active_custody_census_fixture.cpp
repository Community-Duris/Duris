#include "item/item_ownership_runtime.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <limits>
#include <new>
#include <sys/resource.h>

#ifdef NDEBUG
#error "The custody census qualification requires executable assertions"
#endif

namespace
{
bool refuse_allocation = false;
size_t allocations = 0;
size_t cases = 0;
using census = bool (*)(uint64_t, size_t, std::vector<item_ownership_runtime_entry> *);

bool same(const item_ownership_runtime_entry &left, const item_ownership_runtime_entry &right)
{
	return left.item_uid == right.item_uid && left.root_item_uid == right.root_item_uid &&
	       left.parent_item_uid == right.parent_item_uid &&
	       item_owner_identity_equal(left.owner, right.owner) &&
	       left.item_revision == right.item_revision &&
	       left.owner_revision == right.owner_revision && left.vnum == right.vnum &&
	       left.state == right.state;
}

const item_owner_identity player = { item_owner_type::player, 42, 0 };
const item_owner_identity other = { item_owner_type::player, 77, 92 };
const item_owner_identity room = { item_owner_type::room, 1200, 0 };
const item_owner_identity locker = { item_owner_type::locker, 77, 0 };
const item_owner_identity zero = { item_owner_type::player, 700, 0 };
const item_owner_identity unknown = { item_owner_type::player, 9898, 0 };

const item_ownership_runtime_entry original[] = {
	{ 560, 500, 500, player, 8, 8, 101, item_custody_state::destroyed },
	{ 550, 500, 500, player, 16, 8, 102, item_custody_state::active },
	{ 525, 500, 525, locker, 9, 10, 105, item_custody_state::active },
	{ 530, 500, 500, player, 2, 8, 106, item_custody_state::quarantined },
	{ 520, 500, 999999, other, 1, 7, 103, item_custody_state::active },
	{ 510, 500, 520, room, 9, 22, 104, item_custody_state::active },
	{ 500, 500, 0, player, 15, 8, 101, item_custody_state::active },
	{ 800, 800, 0, other, 4, 7, 107, item_custody_state::active },
	{ 901, 900, 900, room, 6, 22, 108, item_custody_state::destroyed },
};

void unchanged()
{
	assert(item_ownership_runtime_size() == std::size(original));
	for (const auto &row : original)
	{
		item_ownership_runtime_entry actual = {};
		assert(item_ownership_runtime_lookup(row.item_uid, &actual) && same(row, actual));
	}
	uint64_t revision = UINT64_MAX;
	assert(item_ownership_runtime_peek_owner_revision(player, &revision) && revision == 19);
	assert(item_ownership_runtime_peek_owner_revision(other, &revision) && revision == 7);
	assert(item_ownership_runtime_peek_owner_revision(room, &revision) && revision == 22);
	assert(item_ownership_runtime_peek_owner_revision(locker, &revision) && revision == 10);
	assert(item_ownership_runtime_peek_owner_revision(zero, &revision) && revision == 0);
	revision = UINT64_MAX;
	assert(!item_ownership_runtime_peek_owner_revision(unknown, &revision) &&
	       revision == UINT64_MAX);
	item_ownership_runtime_entry absent = {};
	assert(!item_ownership_runtime_lookup(777, &absent));
	assert(!item_ownership_runtime_lookup(900, &absent));
}

void observe(census read, uint64_t root, size_t limit, bool success,
	     std::initializer_list<uint64_t> expected, size_t expected_allocations,
	     bool refuse = false)
{
	std::vector<item_ownership_runtime_entry> out = { original[0] };
	allocations = 0;
	refuse_allocation = refuse;
	const bool result = read(root, limit, &out);
	refuse_allocation = false;
	assert(result == success && allocations == expected_allocations);
	assert(out.size() == expected.size());
	size_t index = 0;
	for (uint64_t uid : expected)
	{
		const auto row = std::find_if(std::begin(original), std::end(original),
					      [uid](const auto &value)
					      { return value.item_uid == uid; });
		assert(row != std::end(original) && same(out[index++], *row));
	}
	unchanged();
	++cases;
}

void null_output(census read)
{
	allocations = 0;
	assert(!read(500, 1, nullptr) && allocations == 0);
	unchanged();
	++cases;
}

item_ownership_runtime_entry large_row(uint64_t uid)
{
	constexpr uint64_t capacity = 262144;
	if (uid == capacity)
		return { uid, 999999, 0, other, 3, 7, 201, item_custody_state::active };
	return { uid,
		 1,
		 uid == 1 ? 0U : 1U,
		 player,
		 uid,
		 19,
		 200,
		 uid == 1 ? item_custody_state::active :
			    (uid % 2 ? item_custody_state::destroyed :
				       item_custody_state::quarantined) };
}

void large_unchanged()
{
	assert(item_ownership_runtime_size() == 262144);
	for (uint64_t uid = 1; uid <= 262144; ++uid)
	{
		item_ownership_runtime_entry row = {};
		assert(item_ownership_runtime_lookup(uid, &row) && same(row, large_row(uid)));
	}
	uint64_t revision = UINT64_MAX;
	assert(item_ownership_runtime_peek_owner_revision(player, &revision) && revision == 19);
	assert(item_ownership_runtime_peek_owner_revision(other, &revision) && revision == 7);
	revision = UINT64_MAX;
	assert(!item_ownership_runtime_peek_owner_revision(unknown, &revision) &&
	       revision == UINT64_MAX);
}

void large_observe(census read, size_t limit, bool success, size_t count,
		   size_t expected_allocations, bool refuse = false)
{
	std::vector<item_ownership_runtime_entry> out = { large_row(262144) };
	allocations = 0;
	refuse_allocation = refuse;
	const bool result = read(1, limit, &out);
	refuse_allocation = false;
	assert(result == success && allocations == expected_allocations && out.size() == count);
	for (size_t index = 0; index < count; ++index)
		assert(same(out[index], large_row(index + 1)));
	large_unchanged();
	++cases;
}
}

extern "C" void *__real__Znwm(size_t size);
extern "C" void *__wrap__Znwm(size_t size)
{
	++allocations;
	if (refuse_allocation)
		throw std::bad_alloc();
	return __real__Znwm(size);
}

// Census qualification has no native quest publication capability. Any call
// into that unrelated retained entry point must fail instead of granting it.
bool nevent_is_game_thread()
{
	std::fputs("UNEXPECTED native quest publication in custody census\n", stderr);
	std::abort();
}

int main()
{
	const auto active = item_ownership_runtime_snapshot_active_root;
	const auto history = item_ownership_runtime_snapshot_root;
	item_ownership_runtime_reset();
	for (const auto &row : original)
		assert(item_ownership_runtime_hydrate(row));
	// The cache owns a newer owner revision than these retained item rows.
	// Observation must preserve the exact rows rather than repairing them.
	assert(item_ownership_runtime_hydrate_owner(player, 19));
	assert(item_ownership_runtime_hydrate_owner(zero, 0));
	unchanged();

	observe(active, 500, 5, true, { 500, 510, 520, 525, 550 }, 1);
	observe(active, 500, 4, false, {}, 0);
	observe(history, 500, 7, true, { 500, 510, 520, 525, 530, 550, 560 }, 1);
	observe(history, 500, 6, false, {}, 0);
	observe(active, 500, std::numeric_limits<size_t>::max(), true, { 500, 510, 520, 525, 550 },
		1);
	observe(history, 500, std::numeric_limits<size_t>::max(), true,
		{ 500, 510, 520, 525, 530, 550, 560 }, 1);
	observe(active, 0, 1, false, {}, 0);
	observe(active, 500, 0, false, {}, 0);
	observe(history, 0, 1, false, {}, 0);
	observe(history, 500, 0, false, {}, 0);
	null_output(active);
	null_output(history);
	observe(active, 777, 1, true, {}, 0);
	observe(history, 777, 1, true, {}, 0);
	observe(active, 900, 1, true, {}, 0);
	observe(history, 900, 1, true, { 901 }, 1);
	observe(active, 500, 5, false, {}, 1, true);
	observe(history, 500, 7, false, {}, 1, true);
	observe(active, 777, 1, true, {}, 0, true);
	observe(history, 777, 1, true, {}, 0, true);
	observe(active, 500, 4, false, {}, 0, true);
	observe(history, 500, 6, false, {}, 0, true);

	item_ownership_runtime_reset();
	for (uint64_t uid = 1; uid <= 262144; ++uid)
		assert(item_ownership_runtime_hydrate(large_row(uid)));
	large_unchanged();
	const auto started = std::chrono::steady_clock::now();
	large_observe(active, 1, true, 1, 1);
	const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
				    std::chrono::steady_clock::now() - started)
				    .count();
	large_observe(history, 1, false, 0, 0);
	large_observe(history, 262143, true, 262143, 1);
	large_observe(history, 262142, false, 0, 0);
	large_observe(active, 1, false, 0, 1, true);
	large_observe(history, 262143, false, 0, 1, true);
	large_observe(history, 262142, false, 0, 0, true);
	struct rusage usage = {};
	assert(getrusage(RUSAGE_SELF, &usage) == 0);
	std::printf("{\"cases\":%zu,\"cache_rows\":262144,\"selected_active\":1,"
		    "\"selected_history\":262142,\"unrelated_active\":1,"
		    "\"allocation_refusals\":4,\"read_only\":true,"
		    "\"sample_with_immutability_check_usec\":%lld,\"peak_rss_kib\":%ld,"
		    "\"world_authority_qualified\":false,\"release_host_qualified\":false}\n",
		    cases, static_cast<long long>(micros), usage.ru_maxrss);
	item_ownership_runtime_reset();
	return 0;
}
