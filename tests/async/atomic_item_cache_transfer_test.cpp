#include "item/item_ownership_runtime.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <new>
#include <utility>

namespace
{
bool tracking = false;
size_t requests = 0, fail_at = 0;
}
void *operator new(size_t bytes)
{
	if (tracking && ++requests == fail_at)
		throw std::bad_alloc();
	if (void *p = std::malloc(bytes ? bytes : 1))
		return p;
	throw std::bad_alloc();
}
void *operator new[](size_t bytes)
{
	return ::operator new(bytes);
}
void operator delete(void *p) noexcept
{
	std::free(p);
}
void operator delete[](void *p) noexcept
{
	std::free(p);
}
void operator delete(void *p, size_t) noexcept
{
	std::free(p);
}
void operator delete[](void *p, size_t) noexcept
{
	std::free(p);
}

namespace
{
const item_owner_identity source{ item_owner_type::player, 812, 0 };
const item_owner_identity room{ item_owner_type::room, 1300, 0 };
const item_owner_identity destroyed{ item_owner_type::destruction, 0, 0 };
struct fixture
{
	item_transfer_payload payload{};
	item_transfer_result result{};
	std::vector<item_ownership_runtime_entry> before;
	std::vector<std::pair<item_owner_identity, uint64_t>> other_owners;
	item_owner_identity destination{};
	bool destination_present = false;
	uint64_t source_clock = 10, destination_clock = 0;
};
bool same(const item_ownership_runtime_entry &a, const item_ownership_runtime_entry &b)
{
	return a.item_uid == b.item_uid && a.root_item_uid == b.root_item_uid &&
	       a.parent_item_uid == b.parent_item_uid &&
	       item_owner_identity_equal(a.owner, b.owner) && a.item_revision == b.item_revision &&
	       a.owner_revision == b.owner_revision && a.vnum == b.vnum && a.state == b.state;
}
fixture setup(bool same_owner, bool nested, bool destruction, bool existing_destination,
	      size_t extra_owners = 0)
{
	item_ownership_runtime_reset();
	fixture f;
	f.destination = destruction ? destroyed : (same_owner ? source : room);
	f.before = { { 100, 100, 0, source, 5, 7, 1400, item_custody_state::active },
		     { 101, nested ? 100U : 101U, nested ? 100U : 0U, source, 7, 10, 1401,
		       item_custody_state::active },
		     { 102, 102, 0, source, 3, 4, 1402, item_custody_state::active } };
	assert(item_ownership_runtime_hydrate(f.before[2]));
	assert(item_ownership_runtime_hydrate(f.before[0]));
	assert(item_ownership_runtime_hydrate(f.before[1]));
	assert(item_ownership_runtime_hydrate_owner(source, 10));
	if (same_owner)
	{
		f.destination_present = true;
		f.destination_clock = 10;
	}
	else if (existing_destination)
	{
		assert(item_ownership_runtime_hydrate_owner(f.destination, 3));
		f.destination_present = true;
		f.destination_clock = 3;
	}
	f.payload.from_owner = source;
	f.payload.to_owner = f.destination;
	f.payload.reason = destruction ? item_transfer_reason::destruction :
					 item_transfer_reason::player_drop;
	f.payload.expected_from_revision = 10;
	f.payload.expected_to_revision = f.destination_clock;
	f.payload.selected_item_uid = 100;
	f.payload.target_root_item_uid = 100;
	f.payload.multi_root = !nested;
	f.payload.item_count = 2;
	f.payload.items[0] = { 100, 100, 0, 5, 1400, item_custody_state::active };
	f.payload.items[1] = { 101,  nested ? 100U : 101U,	nested ? 100U : 0U, 7,
			       1401, item_custody_state::active };
	f.result = { 100, 2, 11, same_owner ? 11U : f.destination_clock + 1, 8, 0 };
	for (size_t i = 0; i < extra_owners; ++i)
	{
		const item_owner_identity empty_owner{ item_owner_type::locker, 900000 + i, 0 };
		const uint64_t revision = 50 + i;
		assert(item_ownership_runtime_hydrate_owner(empty_owner, revision));
		f.other_owners.push_back({ empty_owner, revision });
	}
	return f;
}
bool before_exact(const fixture &f)
{
	if (item_ownership_runtime_size() != f.before.size())
		return false;
	for (const auto &expected : f.before)
	{
		item_ownership_runtime_entry actual{};
		if (!item_ownership_runtime_lookup(expected.item_uid, &actual) ||
		    !same(actual, expected))
			return false;
	}
	for (const auto &[owner, expected] : f.other_owners)
	{
		uint64_t actual = 0;
		if (!item_ownership_runtime_peek_owner_revision(owner, &actual) ||
		    actual != expected)
			return false;
	}
	uint64_t clock = 999;
	if (!item_ownership_runtime_peek_owner_revision(source, &clock) || clock != f.source_clock)
		return false;
	clock = 999;
	const bool present = item_ownership_runtime_peek_owner_revision(f.destination, &clock);
	return present == f.destination_present &&
	       (present ? clock == f.destination_clock : clock == 999);
}
void after_exact(const fixture &f)
{
	for (size_t i = 0; i < f.before.size(); ++i)
	{
		auto expected = f.before[i];
		if (i < 2)
		{
			uint64_t root = 0, parent = 0;
			assert(item_transfer_target_topology(f.payload, expected.item_uid, &root,
							     &parent));
			expected.root_item_uid = root;
			expected.parent_item_uid = parent;
			expected.owner = f.destination;
			++expected.item_revision;
			expected.owner_revision = f.result.to_owner_revision;
			expected.state = f.destination.type == item_owner_type::destruction ?
						 item_custody_state::destroyed :
						 item_custody_state::active;
		}
		item_ownership_runtime_entry actual{};
		assert(item_ownership_runtime_lookup(expected.item_uid, &actual) &&
		       same(actual, expected));
	}
	uint64_t clock = 0;
	assert(item_ownership_runtime_peek_owner_revision(source, &clock) &&
	       clock == f.result.from_owner_revision);
	assert(item_ownership_runtime_peek_owner_revision(f.destination, &clock) &&
	       clock == f.result.to_owner_revision);
	for (const auto &[owner, expected] : f.other_owners)
	{
		uint64_t actual = 0;
		assert(item_ownership_runtime_peek_owner_revision(owner, &actual) &&
		       actual == expected);
	}
	// Retrying the original BEFORE receipt cannot publish a second revision.
	assert(!item_ownership_runtime_apply(f.payload, f.result));
	item_ownership_runtime_entry once{};
	assert(item_ownership_runtime_lookup(100, &once) && once.item_revision == 6);
}
void allocation_cuts(bool same_owner, bool nested, bool destruction, bool existing,
		     size_t extra_owners = 0)
{
	size_t success_cut = 0;
	for (size_t cut = 1; cut < 32; ++cut)
	{
		auto f = setup(same_owner, nested, destruction, existing, extra_owners);
		requests = 0;
		fail_at = cut;
		tracking = true;
		bool accepted = false, escaped = false;
		try
		{
			accepted = item_ownership_runtime_apply(f.payload, f.result);
		}
		catch (...)
		{
			escaped = true;
		}
		tracking = false;
		if (escaped || (!accepted && !before_exact(f)))
		{
			std::fprintf(
				stderr,
				"non-atomic cache at allocation cut %zu escaped=%d accepted=%d\n",
				cut, escaped, accepted);
			std::exit(2);
		}
		if (accepted)
		{
			assert(requests < cut);
			after_exact(f);
			success_cut = cut;
			break;
		}
		assert(requests == cut);
	}
	assert(success_cut);
	std::printf("allocation cuts=%zu same=%d nested=%d destroy=%d existing=%d\n",
		    success_cut - 1, same_owner, nested, destruction, existing);
}
void refusal_cases()
{
	for (unsigned variant = 0; variant < 12; ++variant)
	{
		auto f = setup(false, true, false, false);
		switch (variant)
		{
		case 0:
			f.payload.items[1].expected_item_revision = 6;
			break;
		case 1:
			f.payload.items[1] = f.payload.items[0];
			break;
		case 2:
			f.payload.items[0].root_item_uid = 999;
			break;
		case 3:
			f.payload.items[0].parent_item_uid = 999;
			break;
		case 4:
			f.payload.items[0].vnum = 999;
			break;
		case 5:
			f.payload.items[0].expected_state = item_custody_state::destroyed;
			break;
		case 6:
			f.payload.expected_from_revision = 9;
			break;
		case 7:
			f.payload.expected_to_revision = 1;
			break;
		case 8:
			f.result.max_item_revision = 7;
			break;
		case 9:
			f.result.item_count = 1;
			break;
		case 10:
			f.result.to_owner_revision = 0;
			break;
		case 11:
			f.payload.from_owner = { item_owner_type::player, 999, 0 };
			break;
		}
		bool accepted = false, escaped = false;
		try
		{
			accepted = item_ownership_runtime_apply(f.payload, f.result);
		}
		catch (...)
		{
			escaped = true;
		}
		assert(!accepted && !escaped && before_exact(f));
	}
	// Overflow at a genuine hydrated item revision refuses before owner insertion.
	auto f = setup(false, false, false, false);
	f.before[0].item_revision = UINT64_MAX;
	f.before[0].owner_revision = 10;
	assert(item_ownership_runtime_hydrate(f.before[0]));
	f.payload.items[0].expected_item_revision = UINT64_MAX;
	assert(!item_ownership_runtime_apply(f.payload, f.result) && before_exact(f));
}
void restore_destroyed_case()
{
	item_ownership_runtime_reset();
	const item_ownership_runtime_entry before{ 300, 300, 0,	   destroyed,
						   4,	2,   1460, item_custody_state::destroyed };
	assert(item_ownership_runtime_hydrate(before));
	item_transfer_payload payload{};
	payload.from_owner = destroyed;
	payload.to_owner = source;
	payload.reason = item_transfer_reason::operator_repair;
	payload.expected_from_revision = 2;
	payload.selected_item_uid = 300;
	payload.target_root_item_uid = 300;
	payload.item_count = 1;
	payload.items[0] = { 300, 300, 0, 4, 1460, item_custody_state::destroyed };
	const item_transfer_result result{ 300, 1, 3, 1, 5, 0 };
	assert(item_ownership_runtime_apply(payload, result));
	item_ownership_runtime_entry after{};
	assert(item_ownership_runtime_lookup(300, &after) &&
	       after.state == item_custody_state::active &&
	       item_owner_identity_equal(after.owner, source) && after.item_revision == 5 &&
	       after.owner_revision == 1);
	assert(!item_ownership_runtime_apply(payload, result));
}
void nested_destination_case()
{
	auto f = setup(false, true, false, true);
	const item_ownership_runtime_entry parent{ 200, 200, 0,	   room,
						   9,	3,   1450, item_custody_state::active };
	assert(item_ownership_runtime_hydrate_batch(&parent, 1));
	f.before.push_back(parent);
	f.payload.reason = item_transfer_reason::player_put;
	f.payload.target_root_item_uid = 200;
	f.payload.target_parent_item_uid = 200;
	f.payload.expected_target_parent_revision = 9;
	assert(item_ownership_runtime_apply(f.payload, f.result));
	item_ownership_runtime_entry actual{};
	assert(item_ownership_runtime_lookup(100, &actual) && actual.root_item_uid == 200 &&
	       actual.parent_item_uid == 200);
	assert(item_ownership_runtime_lookup(101, &actual) && actual.root_item_uid == 200 &&
	       actual.parent_item_uid == 100);
	assert(item_ownership_runtime_lookup(200, &actual) && same(actual, parent));
}
}
int main()
{
	allocation_cuts(false, false, false, false);
	allocation_cuts(false, true, false, false);
	allocation_cuts(false, true, false, true);
	allocation_cuts(true, true, false, false);
	allocation_cuts(false, true, true, false);
	// Real zero-item owner hydration grows the actual original table. Sweep
	// across its genuine bucket boundaries, testing every node/bucket request
	// without importing a policy class or touching private table storage.
	for (size_t owners = 1; owners <= 64; ++owners)
		allocation_cuts(false, true, false, false, owners);
	refusal_cases();
	nested_destination_case();
	restore_destroyed_case();
	item_ownership_runtime_reset();
	std::puts(
		"[PASS] canonical ordinary cache transfer keeps exact BEFORE at every allocation cut");
}
