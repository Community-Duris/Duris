// Pure BEFORE ancestor identity membership; no SQL/native/property AFTER mutation.
#include "item/item_transfer_room_parents.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>

#ifdef NDEBUG
#error Ancestor identity controls require executable assertions
#endif
namespace
{
bool tracking = false;
size_t requests = 0, fail_at = 0, healthy_cases = 0, allocation_cuts = 0, refusals = 0;
using row = player_load_item_identity;
using error = item_transfer_room_parent_error;
constexpr item_owner_identity room{ item_owner_type::room, 101, 0 };
constexpr item_owner_identity player{ item_owner_type::player, 202, 0 };
constexpr uint64_t clock_revision = 10;
}
void *operator new(size_t bytes)
{
	if (tracking && ++requests == fail_at)
		throw std::bad_alloc();
	if (auto *value = std::malloc(bytes ? bytes : 1))
		return value;
	throw std::bad_alloc();
}
void *operator new[](size_t bytes)
{
	return ::operator new(bytes);
}
void operator delete(void *value) noexcept
{
	std::free(value);
}
void operator delete[](void *value) noexcept
{
	std::free(value);
}
void operator delete(void *value, size_t) noexcept
{
	std::free(value);
}
void operator delete[](void *value, size_t) noexcept
{
	std::free(value);
}

namespace
{
row identity(uint64_t uid, uint64_t parent, uint64_t root)
{
	row value;
	value.database_id = uid + 1000;
	value.serialized_parent_id = parent ? parent + 1000 : 0;
	value.quantity = 3;
	value.override_mask = 7;
	value.item_uid = uid;
	value.root_item_uid = root;
	value.parent_item_uid = parent;
	value.owner = room;
	value.item_revision = uid + 1;
	value.owner_revision = 4; // Older cached row clock, fenced by global clock10.
	value.state = item_custody_state::active;
	return value;
}
bool same(const row &left, const row &right)
{
	return left.database_id == right.database_id &&
	       left.serialized_parent_id == right.serialized_parent_id &&
	       left.quantity == right.quantity && left.override_mask == right.override_mask &&
	       left.item_uid == right.item_uid && left.root_item_uid == right.root_item_uid &&
	       left.parent_item_uid == right.parent_item_uid &&
	       item_owner_identity_equal(left.owner, right.owner) &&
	       left.item_revision == right.item_revision &&
	       left.owner_revision == right.owner_revision && left.state == right.state;
}
void same_rows(std::span<const row> left, std::span<const row> right)
{
	assert(left.size() == right.size());
	for (size_t i = 0; i < left.size(); ++i)
		assert(same(left[i], right[i]));
}
std::vector<row> prior_output()
{
	std::vector<row> result{ identity(900, 0, 900), identity(901, 900, 900) };
	result.reserve(37);
	return result;
}
void unchanged(const std::vector<row> &output, const std::vector<row> &prior, const row *data,
	       size_t capacity)
{
	same_rows(output, prior);
	assert(output.data() == data && output.capacity() == capacity);
}
std::unique_ptr<item_transfer_payload> put()
{
	auto payload = std::make_unique<item_transfer_payload>();
	payload->reason = item_transfer_reason::player_put;
	payload->from_owner = player;
	payload->to_owner = room;
	payload->expected_from_revision = 8;
	payload->expected_to_revision = clock_revision;
	payload->target_parent_item_uid = 30;
	payload->target_root_item_uid = 100;
	payload->expected_target_parent_revision = 31;
	payload->item_count = 1;
	payload->items[0] = { 500, 500, 0, 2, 17, item_custody_state::active };
	return payload;
}
std::unique_ptr<item_transfer_payload> get(const std::vector<row> &before,
					   std::initializer_list<uint64_t> selected)
{
	auto payload = std::make_unique<item_transfer_payload>();
	payload->reason = item_transfer_reason::player_get;
	payload->from_owner = room;
	payload->to_owner = player;
	payload->expected_from_revision = clock_revision;
	payload->expected_to_revision = 8;
	payload->multi_root = selected.size() > 1;
	for (const auto uid : selected)
	{
		const auto found = std::find_if(before.begin(), before.end(), [&](const auto &value)
						{ return value.item_uid == uid; });
		assert(found != before.end());
		payload->items[payload->item_count++] = { found->item_uid,
							  found->root_item_uid,
							  found->parent_item_uid,
							  found->item_revision,
							  17,
							  found->state };
	}
	return payload;
}
void healthy(const item_transfer_payload &payload, const std::vector<row> &before,
	     const std::vector<uint64_t> &expected)
{
	const auto original_before = before;
	auto output = prior_output();
	tracking = true;
	requests = 0;
	fail_at = 0;
	const auto result = item_transfer_room_parent_candidates(payload, room, clock_revision,
								 before, &output);
	tracking = false;
	const auto allocations = requests;
	assert(result == error::ok && allocations > 0 && output.size() == expected.size());
	for (size_t i = 0; i < expected.size(); ++i)
	{
		assert(output[i].item_uid == expected[i]);
		const auto original = std::find_if(before.begin(), before.end(),
						   [&](const auto &value)
						   { return value.item_uid == expected[i]; });
		assert(original != before.end() && same(output[i], *original));
	}
	same_rows(before, original_before);
	++healthy_cases;
	output = prior_output();
	const auto prior = output;
	const auto *data = output.data();
	const auto capacity = output.capacity();
	for (size_t failure = 1; failure <= allocations; ++failure)
	{
		tracking = true;
		requests = 0;
		fail_at = failure;
		const auto refused = item_transfer_room_parent_candidates(
			payload, room, clock_revision, before, &output);
		tracking = false;
		assert(refused == error::allocation_failure && requests == failure);
		unchanged(output, prior, data, capacity);
		same_rows(before, original_before);
		++allocation_cuts;
	}
	fail_at = 0;
}
void reject(error expected, const item_transfer_payload &payload, const std::vector<row> &before,
	    const item_owner_identity &owner = room, uint64_t clock = clock_revision)
{
	auto output = prior_output();
	const auto prior = output;
	const auto original_before = before;
	const auto *data = output.data();
	const auto capacity = output.capacity();
	assert(item_transfer_room_parent_candidates(payload, owner, clock, before, &output) ==
	       expected);
	unchanged(output, prior, data, capacity);
	same_rows(before, original_before);
	++refusals;
}
}

int main()
{
	// Deliberately shuffled UID order and a separate, untouched ROOM root.
	const std::vector<row> graph{ identity(20, 30, 100), identity(300, 0, 300),
				      identity(100, 0, 100), identity(50, 70, 100),
				      identity(30, 70, 100), identity(70, 100, 100) };
	auto insertion = put();
	healthy(*insertion, graph, { 30, 70, 100 });
	auto subtree = get(graph, { 30, 20 });
	healthy(*subtree, graph, { 70, 100 });
	auto multiple = get(graph, { 30, 20, 50 });
	healthy(*multiple, graph, { 70, 100 });
	auto direct = get(graph, { 100, 70, 50, 30, 20 });
	healthy(*direct, graph, {});
	auto reordered = graph;
	std::reverse(reordered.begin(), reordered.end());
	std::reverse(multiple->items.begin(), multiple->items.begin() + multiple->item_count);
	healthy(*multiple, reordered, { 70, 100 });
	auto newer = graph;
	for (auto &value : newer)
		value.owner_revision = clock_revision;
	healthy(*insertion, newer, { 30, 70, 100 });
	std::vector<row> deepest;
	std::vector<uint64_t> all_ancestors;
	for (size_t i = 0; i < PLAYER_SNAPSHOT_MAX_DEPTH; ++i)
	{
		deepest.push_back(identity(400 + i, i ? 399 + i : 0, 400));
		all_ancestors.push_back(400 + i);
	}
	auto depth_put = put();
	depth_put->target_parent_item_uid = deepest.back().item_uid;
	depth_put->target_root_item_uid = 400;
	depth_put->expected_target_parent_revision = deepest.back().item_revision;
	healthy(*depth_put, deepest, all_ancestors);

	const auto change_put = [&](error expected, auto mutate)
	{
		auto candidate = put();
		mutate(*candidate);
		reject(expected, *candidate, graph);
	};
	change_put(error::unsupported,
		   [](auto &p) { p.reason = item_transfer_reason::player_drop; });
	change_put(error::unsupported, [](auto &p) { p.from_owner = room; });
	change_put(error::invalid, [](auto &p) { p.item_count = 0; });
	change_put(error::invalid, [](auto &p) { p.item_count = ITEM_TRANSFER_MAX_ITEMS + 1; });
	change_put(error::invalid, [](auto &p) { p.target_parent_item_uid = 0; });
	change_put(error::invalid, [](auto &p) { p.target_root_item_uid = 0; });
	change_put(error::stale, [](auto &p) { ++p.expected_to_revision; });
	change_put(error::stale, [](auto &p) { ++p.expected_target_parent_revision; });
	change_put(error::stale, [](auto &p) { p.target_parent_item_uid = 999; });
	change_put(error::malformed, [](auto &p) { p.target_root_item_uid = 300; });
	change_put(error::malformed, [](auto &p) { p.items[0].item_uid = 100; });
	change_put(error::malformed, [](auto &p) { p.items[0].item_uid = 0; });
	change_put(error::malformed,
		   [](auto &p)
		   {
			   p.item_count = 2;
			   p.items[1] = p.items[0];
		   });
	reject(error::invalid, *insertion, graph, player);
	reject(error::invalid, *insertion, graph, { item_owner_type::room, 0, 0 });
	reject(error::stale, *insertion, graph, { item_owner_type::room, 999, 0 });
	reject(error::stale, *insertion, graph, room, clock_revision + 1);
	reject(error::stale, *insertion, {});
	reject(error::invalid, *insertion, std::vector<row>(PLAYER_LOAD_ITEM_MAX + 1));
	assert(item_transfer_room_parent_candidates(*insertion, room, clock_revision, graph,
						    nullptr) == error::invalid);
	++refusals;
	const auto change_graph = [&](auto mutate)
	{
		auto candidate = graph;
		mutate(candidate);
		reject(error::malformed, *insertion, candidate);
	};
	change_graph([](auto &v) { v[0].item_uid = 0; });
	change_graph([](auto &v) { v[0].root_item_uid = 0; });
	change_graph([](auto &v) { v[0].item_revision = 0; });
	change_graph([](auto &v) { v[0].owner = player; });
	change_graph([](auto &v) { v[0].owner_revision = clock_revision + 1; });
	change_graph([](auto &v) { v[0].parent_item_uid = 999; });
	change_graph([](auto &v) { v[0].root_item_uid = 300; });
	change_graph([](auto &v) { v.push_back(v[0]); });
	change_graph([](auto &v) { v.erase(v.begin() + 2); });
	for (const auto state : { item_custody_state::absent, item_custody_state::destroyed,
				  item_custody_state::quarantined })
		change_graph([&](auto &v) { v[0].state = state; });
	change_graph(
		[](auto &v)
		{
			v.push_back(identity(90, 95, 90));
			v.push_back(identity(95, 90, 90));
		});
	change_graph([](auto &v) { v.push_back(identity(90, 90, 90)); });
	deepest.push_back(identity(400 + PLAYER_SNAPSHOT_MAX_DEPTH, deepest.back().item_uid, 400));
	reject(error::malformed, *depth_put, deepest);
	const auto change_get = [&](error expected, auto mutate)
	{
		auto candidate = get(graph, { 30, 20 });
		mutate(*candidate);
		reject(expected, *candidate, graph);
	};
	change_get(error::unsupported, [](auto &p) { p.to_owner = room; });
	change_get(error::stale, [](auto &p) { ++p.expected_from_revision; });
	change_get(error::stale, [](auto &p) { p.items[0].item_uid = 999; });
	change_get(error::stale, [](auto &p) { ++p.items[0].expected_item_revision; });
	change_get(error::stale, [](auto &p) { p.items[0].parent_item_uid = 100; });
	change_get(error::stale, [](auto &p) { p.items[0].root_item_uid = 300; });
	change_get(error::stale,
		   [](auto &p) { p.items[0].expected_state = item_custody_state::destroyed; });
	change_get(error::malformed, [](auto &p) { p.items[1] = p.items[0]; });
	reject(error::stale, *subtree, {});
	auto selected_cycle = graph;
	selected_cycle[4].parent_item_uid = 20; // Selected20->30->20 cannot be skipped.
	auto cycle_get = get(selected_cycle, { 30, 20 });
	reject(error::malformed, *cycle_get, selected_cycle);

	std::printf(
		"ITEM_TRANSFER_ROOM_PARENTS {\"healthy_cases\":%zu,\"negative_controls\":%zu,"
		"\"allocation_cuts\":%zu,\"strong_outputs\":true,\"before_identities_only\":true}\n",
		healthy_cases, refusals, allocation_cuts);
}
