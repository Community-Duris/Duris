// Identity-only keeper preparation fixtures. No SQL/locked authority/property AFTER proof.
#include "item/item_transfer_repository.h"
#include "persistence/sql_room_item_payload.h"
#include "player/player_snapshot_codec.h"
#include <cstring>
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>
#ifdef NDEBUG
#error Keeper delta controls require executable assertions
#endif
namespace
{
bool tracking = false;
size_t requests = 0, fail_at = 0, healthy_count = 0, refusal_count = 0, cut_count = 0;
constexpr item_owner_identity room{ item_owner_type::room, 101, 0 };
constexpr item_owner_identity player{ item_owner_type::player, 202, 0 };
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
using delta = item_transfer_custody_delta;
using identity = player_load_item_identity;
identity row(uint64_t uid, uint64_t parent, uint64_t root)
{
	identity value;
	value.item_uid = uid;
	value.root_item_uid = root;
	value.parent_item_uid = parent;
	value.database_id = uid + 1000;
	value.serialized_parent_id = parent ? parent + 1000 : 0;
	value.quantity = 3;
	value.override_mask = 7;
	value.owner = room;
	value.owner_revision = 4;
	value.item_revision = 5;
	value.state = item_custody_state::active;
	return value;
}
economic_item_position position(const identity &value)
{
	return { value.owner,	      value.root_item_uid, value.parent_item_uid,
		 value.item_revision, value.state,	   0 };
}
bool same_snapshot(const economic_item_snapshot &a, const economic_item_snapshot &b)
{
	return a.uid == b.uid && economic_item_position_equal(a.position, b.position);
}
bool same_event(const economic_item_event &a, const economic_item_event &b)
{
	return a.event_index == b.event_index && a.child_index == b.child_index && a.uid == b.uid &&
	       economic_item_position_equal(a.before, b.before) &&
	       economic_item_position_equal(a.after, b.after);
}
void same_delta(const delta &a, const delta &b)
{
	assert(a.before.size() == b.before.size() && a.after.size() == b.after.size() &&
	       a.events.size() == b.events.size());
	for (size_t i = 0; i < a.before.size(); ++i)
		assert(same_snapshot(a.before[i], b.before[i]));
	for (size_t i = 0; i < a.after.size(); ++i)
		assert(same_snapshot(a.after[i], b.after[i]));
	for (size_t i = 0; i < a.events.size(); ++i)
		assert(same_event(a.events[i], b.events[i]));
}
delta sentinel()
{
	delta output;
	auto value = position(row(900, 0, 900));
	output.before = { { 900, value } };
	output.after = { { 900, value } };
	output.events = { { 77, 66, 900, value, value } };
	output.before.reserve(19);
	output.after.reserve(23);
	output.events.reserve(29);
	return output;
}
struct fixture
{
	std::unique_ptr<item_transfer_payload> payload = std::make_unique<item_transfer_payload>();
	sql_room_item_graph before;
	sql_room_item_parent_after_plan plan;
	delta selected;
	explicit fixture(bool put = true)
	{
		before.owner = room;
		before.owner_revision = 10;
		before.identities = { row(100, 0, 100), row(70, 100, 100), row(30, 70, 100),
				      row(300, 0, 300) };
		auto a = row(500, put ? 0 : 30, put ? 500 : 100);
		auto b = row(501, 500, put ? 500 : 100);
		a.owner = b.owner = put ? player : room;
		a.item_revision = 6;
		b.item_revision = 7;
		if (!put)
		{
			before.identities.push_back(a);
			before.identities.push_back(b);
		}
		payload->reason = put ? item_transfer_reason::player_put :
					item_transfer_reason::player_get;
		payload->from_owner = put ? player : room;
		payload->to_owner = put ? room : player;
		payload->expected_from_revision = put ? 8 : 10;
		payload->expected_to_revision = put ? 10 : 8;
		payload->selected_item_uid = 500;
		payload->target_parent_item_uid = put ? 30 : 0;
		payload->target_root_item_uid = put ? 100 : 0;
		payload->expected_target_parent_revision = put ? 5 : 0;
		payload->item_count = 2;
		plan.room_after = before;
		plan.room_after.owner_revision = 11;
		if (!put)
			plan.room_after.identities.resize(4);
		for (const auto &value : { a, b })
		{
			const size_t index = selected.events.size();
			payload->items[index] = { value.item_uid,
						  value.root_item_uid,
						  value.parent_item_uid,
						  value.item_revision,
						  17,
						  value.state };
			auto next = value;
			next.owner = payload->to_owner;
			next.root_item_uid = put ? 100 : 500;
			next.parent_item_uid = value.item_uid == 500 ? (put ? 30 : 0) : 500;
			++next.item_revision;
			next.owner_revision = put ? 11 : 9;
			selected.before.push_back({ value.item_uid, position(value) });
			selected.after.push_back({ next.item_uid, position(next) });
			selected.events.push_back({ static_cast<uint32_t>(index), 0, value.item_uid,
						    position(value), position(next) });
			plan.selected_after_identities.push_back(next);
			if (put)
				plan.room_after.identities.push_back(next);
		}
		for (const size_t index : { 1, 0 }) // UID70 then100; target30 remains unchanged.
		{
			auto old = before.identities[index], next = old;
			++next.item_revision;
			next.owner_revision = 11;
			plan.changed_ancestors.push_back({ old, next, {}, {} });
			plan.room_after.identities[index] = next;
		}
	}
};
struct strong_guard
{
	delta prior;
	const economic_item_snapshot *before, *after;
	const economic_item_event *events;
	size_t before_cap, after_cap, events_cap;
	explicit strong_guard(const delta &out)
		: prior(out)
		, before(out.before.data())
		, after(out.after.data())
		, events(out.events.data())
		, before_cap(out.before.capacity())
		, after_cap(out.after.capacity())
		, events_cap(out.events.capacity())
	{
	}
	void check(const delta &out) const
	{
		same_delta(out, prior);
		assert(out.before.data() == before && out.after.data() == after &&
		       out.events.data() == events);
		assert(out.before.capacity() == before_cap && out.after.capacity() == after_cap &&
		       out.events.capacity() == events_cap);
	}
};
void healthy(fixture &input)
{
	auto output = sentinel();
	tracking = true;
	requests = 0;
	fail_at = 0;
	const bool accepted = item_transfer_repository_room_parent_delta(
		*input.payload, input.before, input.plan, input.selected, &output);
	tracking = false;
	const auto allocations = requests;
	assert(accepted && allocations > 0);
	assert(output.before.size() == 6 && output.after.size() == 6 &&
	       output.events.size() == 2 + input.plan.changed_ancestors.size());
	const uint64_t uids[] = { 30, 70, 100, 300, 500, 501 };
	for (size_t i = 0; i < 6; ++i)
		assert(output.before[i].uid == uids[i] && output.after[i].uid == uids[i]);
	for (size_t i = 0; i < 2; ++i)
		assert(same_event(output.events[i], input.selected.events[i]));
	for (size_t i = 0; i < input.plan.changed_ancestors.size(); ++i)
	{
		const auto &event = output.events[2 + i];
		const auto &participant = input.plan.changed_ancestors[i];
		assert(event.event_index == 2 + i && !event.child_index &&
		       event.uid == participant.before_identity.item_uid &&
		       event.after.revision == event.before.revision + 1 &&
		       economic_item_position_equal(event.before,
						    position(participant.before_identity)) &&
		       economic_item_position_equal(event.after,
						    position(participant.after_identity)));
	}
	for (const auto uid : { 30, 300 })
	{
		const auto before = std::find_if(
			output.before.begin(), output.before.end(),
			[&](const auto &value) { return value.uid == static_cast<uint64_t>(uid); });
		const auto after = std::find_if(
			output.after.begin(), output.after.end(),
			[&](const auto &value) { return value.uid == static_cast<uint64_t>(uid); });
		assert(same_snapshot(*before, *after));
	}
	assert(economic_item_effects_validate(output.before, output.after, output.events, 0) ==
	       economic_accounting_error::ok);
	++healthy_count;
	output = sentinel();
	const strong_guard original(output);
	for (size_t failure = 1; failure <= allocations; ++failure)
	{
		tracking = true;
		requests = 0;
		fail_at = failure;
		errno = 0;
		const bool result = item_transfer_repository_room_parent_delta(
			*input.payload, input.before, input.plan, input.selected, &output);
		tracking = false;
		assert(!result && errno == ENOMEM && requests == failure);
		original.check(output);
		++cut_count;
	}
	fail_at = 0;
}
void refuse(const char *name, auto mutate, bool put = true)
{
	fixture input(put);
	mutate(input);
	auto output = sentinel();
	const strong_guard original(output);
	errno = 0;
	const bool accepted = item_transfer_repository_room_parent_delta(
		*input.payload, input.before, input.plan, input.selected, &output);
	if (accepted)
		std::fprintf(stderr, "UNEXPECTED_ACCEPTANCE %s\n", name);
	assert(!accepted && errno);
	original.check(output);
	++refusal_count;
}
}

namespace
{
// These routes execute the genuine canonical codec, SQL literal projection and
// keeper value delta. Constructed literals/identities confer no locked authority.
using projection = sql_room_item_parent_projection_result;
using plan_type = sql_room_item_parent_after_plan;
size_t composition_routes_count = 0, composition_refusals = 0;
size_t projection_cuts = 0, composition_keeper_cuts = 0;
player_item_snapshot complete_literal(uint64_t uid, int type, int weight, int parent = -1)
{
	player_item_snapshot value;
	value.object_uid = uid;
	value.vnum = 17;
	value.parent_index = parent;
	value.type = type;
	value.weight = weight;
	value.string_mask = STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3;
	value.name = "original encoded literal keys";
	value.short_description = "original encoded literal short";
	value.description = "original encoded literal room";
	value.action_description = "original encoded literal action";
	return value;
}
std::vector<uint8_t> literal_wire(const std::vector<player_item_snapshot> &items)
{
	std::vector<uint8_t> bytes;
	assert(player_item_snapshot_list_encode(items, &bytes) == player_snapshot_codec_result::ok);
	return bytes;
}
void bind_wire(item_transfer_payload &payload, const std::vector<player_item_snapshot> &selected)
{
	const auto bytes = literal_wire(selected);
	assert(bytes.size() <= payload.item_blob.size());
	payload.item_blob_size = bytes.size();
	std::copy(bytes.begin(), bytes.end(), payload.item_blob.begin());
}
bool same_identity(const identity &a, const identity &b)
{
	return a.database_id == b.database_id && a.serialized_parent_id == b.serialized_parent_id &&
	       a.quantity == b.quantity && a.override_mask == b.override_mask &&
	       a.item_uid == b.item_uid && a.root_item_uid == b.root_item_uid &&
	       a.parent_item_uid == b.parent_item_uid &&
	       item_owner_identity_equal(a.owner, b.owner) && a.item_revision == b.item_revision &&
	       a.owner_revision == b.owner_revision && a.state == b.state;
}
void same_identities(const auto &a, const auto &b)
{
	assert(a.size() == b.size());
	for (size_t i = 0; i < a.size(); ++i)
		assert(same_identity(a[i], b[i]));
}
struct projected_guard
{
	plan_type prior;
	std::array<unsigned char, sizeof(plan_type)> representation{};
	explicit projected_guard(const plan_type &out)
		: prior(out)
	{
		std::memcpy(representation.data(), &out, sizeof(out));
	}
	void check(const plan_type &out) const
	{
		// Includes each vector's original data pointer, size and capacity.
		assert(!std::memcmp(representation.data(), &out, sizeof(out)));
		assert(literal_wire(out.room_after.items) == literal_wire(prior.room_after.items));
		assert(literal_wire(out.selected_after) == literal_wire(prior.selected_after));
		same_identities(out.room_after.identities, prior.room_after.identities);
		same_identities(out.selected_after_identities, prior.selected_after_identities);
		assert(out.changed_ancestors.size() == prior.changed_ancestors.size());
		for (size_t i = 0; i < out.changed_ancestors.size(); ++i)
		{
			const auto &a = out.changed_ancestors[i], &b = prior.changed_ancestors[i];
			assert(same_identity(a.before_identity, b.before_identity) &&
			       same_identity(a.after_identity, b.after_identity));
			assert(literal_wire({ a.before_literal }) ==
				       literal_wire({ b.before_literal }) &&
			       literal_wire({ a.after_literal }) ==
				       literal_wire({ b.after_literal }));
		}
	}
};
plan_type actual_projection(const item_transfer_payload &payload, const sql_room_item_graph &before)
{
	plan_type out;
	tracking = true;
	requests = 0;
	fail_at = 0;
	const auto status = sql_room_item_payload_parent_after(payload, before, {}, &out);
	tracking = false;
	const auto allocations = requests;
	assert(status == projection::ok && allocations > 0);
	plan_type sentinel_plan = out;
	const projected_guard original(sentinel_plan);
	for (size_t failure = 1; failure <= allocations; ++failure)
	{
		tracking = true;
		requests = 0;
		fail_at = failure;
		const auto refused =
			sql_room_item_payload_parent_after(payload, before, {}, &sentinel_plan);
		tracking = false;
		// Every observed allocator denial must retain its actual resource class,
		// including codec/extraction children and the owned root collection.
		assert(refused == projection::allocation_unavailable && requests == failure);
		original.check(sentinel_plan);
		++projection_cuts;
	}
	fail_at = 0;
	return out;
}
delta movement_delta(const item_transfer_payload &payload, const plan_type &plan)
{
	assert(payload.item_count == 1 && plan.selected_after_identities.size() == 1);
	const auto &entry = payload.items[0];
	const economic_item_position old{ payload.from_owner,	 entry.root_item_uid,
					  entry.parent_item_uid, entry.expected_item_revision,
					  entry.expected_state,	 0 };
	const auto next = position(plan.selected_after_identities[0]);
	delta selected;
	selected.before.push_back({ entry.item_uid, old });
	selected.after.push_back({ entry.item_uid, next });
	selected.events.push_back({ 0, 0, entry.item_uid, old, next });
	return selected;
}
void composition_route(const item_transfer_payload &payload, const sql_room_item_graph &before,
		       const plan_type &plan, bool put)
{
	const auto selected = movement_delta(payload, plan);
	auto output = sentinel();
	tracking = true;
	requests = 0;
	fail_at = 0;
	const bool accepted = item_transfer_repository_room_parent_delta(payload, before, plan,
									 selected, &output);
	tracking = false;
	const auto allocations = requests;
	assert(accepted && allocations > 0 && output.before.size() == 4 &&
	       output.after.size() == 4 && output.events.size() == 3 &&
	       same_event(output.events[0], selected.events[0]));
	const uint64_t uids[] = { 70, 100, 300, 500 };
	for (size_t i = 0; i < 4; ++i)
		assert(output.before[i].uid == uids[i] && output.after[i].uid == uids[i]);
	assert(output.events[1].uid == 70 && output.events[1].event_index == 1 &&
	       output.events[2].uid == 100 && output.events[2].event_index == 2);
	for (size_t i = 1; i < 3; ++i)
		assert(!output.events[i].child_index &&
		       output.events[i].after.revision == output.events[i].before.revision + 1);
	assert(same_snapshot(output.before[2], output.after[2])); // Unchanged ROOM root300.
	assert(output.after[3].position.root_uid == (put ? 100 : 500) &&
	       output.after[3].position.parent_uid == (put ? 70 : 0) &&
	       output.after[3].position.revision == (put ? 7 : 8));
	assert(economic_item_effects_validate(output.before, output.after, output.events, 0) ==
	       economic_accounting_error::ok);
	++composition_routes_count;
	output = sentinel();
	const strong_guard original(output);
	for (size_t failure = 1; failure <= allocations; ++failure)
	{
		tracking = true;
		requests = 0;
		fail_at = failure;
		errno = 0;
		const bool result = item_transfer_repository_room_parent_delta(
			payload, before, plan, selected, &output);
		tracking = false;
		assert(!result && errno == ENOMEM && requests == failure);
		original.check(output);
		++composition_keeper_cuts;
	}
	fail_at = 0;
	const auto reject_plan = [&](auto mutate)
	{
		auto bad = plan;
		mutate(bad);
		errno = 0;
		assert(!item_transfer_repository_room_parent_delta(payload, before, bad, selected,
								   &output) &&
		       errno);
		original.check(output);
		++composition_refusals;
	};
	reject_plan([](auto &bad) { bad.changed_ancestors[1] = bad.changed_ancestors[0]; });
	reject_plan([](auto &bad) { ++bad.changed_ancestors[0].before_identity.item_revision; });
	reject_plan([](auto &bad) { bad.room_after.owner = player; });
	reject_plan([&](auto &bad) { bad.room_after.owner_revision = before.owner_revision; });
}
void actual_quiver_composition()
{
	const auto missile = [&]()
	{
		auto item = complete_literal(500, ITEM_MISSILE, 5);
		item.values[3] = 3;
		return item;
	}();
	sql_room_item_graph before;
	before.owner = room;
	before.owner_revision = 10;
	before.items = { complete_literal(100, ITEM_CONTAINER, 20),
			 complete_literal(70, ITEM_QUIVER, 10, 0),
			 complete_literal(300, ITEM_OTHER, 3) };
	before.items[1].values[0] = 2;
	before.items[1].values[2] = 3;
	before.identities = { row(100, 0, 100), row(70, 100, 100), row(300, 0, 300) };
	auto put = std::make_unique<item_transfer_payload>();
	put->from_owner = player;
	put->to_owner = room;
	put->reason = item_transfer_reason::player_put;
	put->expected_from_revision = 8;
	put->expected_to_revision = 10;
	put->selected_item_uid = 500;
	put->target_root_item_uid = 100;
	put->target_parent_item_uid = 70;
	put->expected_target_parent_revision = 5;
	put->item_count = 1;
	put->items[0] = { 500, 500, 0, 6, 17, item_custody_state::active };
	bind_wire(*put, { missile });
	const auto inserted = actual_projection(*put, before);
	assert(inserted.room_after.owner_revision == 11 && inserted.room_after.items.size() == 4 &&
	       inserted.room_after.items[0].weight == 25 &&
	       inserted.room_after.items[1].weight == 15 &&
	       inserted.room_after.items[1].values[3] == 1 &&
	       inserted.room_after.items[3].parent_index == 1);
	assert(inserted.changed_ancestors.size() == 2 &&
	       inserted.changed_ancestors[0].before_identity.item_uid == 70 &&
	       inserted.changed_ancestors[1].before_identity.item_uid == 100);
	for (const auto &participant : inserted.changed_ancestors)
		assert(participant.before_identity.owner_revision == 4 &&
		       participant.after_identity.owner_revision == 11 &&
		       participant.before_identity.item_revision == 5 &&
		       participant.after_identity.item_revision == 6);
	assert(inserted.selected_after_identities[0].item_revision == 7 &&
	       inserted.selected_after_identities[0].owner_revision == 11 &&
	       inserted.selected_after_identities[0].root_item_uid == 100 &&
	       inserted.selected_after_identities[0].parent_item_uid == 70);
	assert(literal_wire({ before.items[2] }) ==
		       literal_wire({ inserted.room_after.items[2] }) &&
	       same_identity(before.identities[2], inserted.room_after.identities[2]));
	composition_route(*put, before, inserted, true);
	auto get = std::make_unique<item_transfer_payload>();
	get->from_owner = room;
	get->to_owner = player;
	get->reason = item_transfer_reason::player_get;
	get->expected_from_revision = 11;
	get->expected_to_revision = put->expected_from_revision + 1;
	get->selected_item_uid = 500;
	get->item_count = 1;
	get->items[0] = { 500, 100, 70, 7, 17, item_custody_state::active };
	bind_wire(*get, { missile });
	const auto removed = actual_projection(*get, inserted.room_after);
	assert(removed.room_after.owner_revision == 12 && removed.room_after.items.size() == 3 &&
	       removed.room_after.items[0].weight == 20 &&
	       removed.room_after.items[1].weight == 10 &&
	       removed.room_after.items[1].values[3] == 0 && removed.changed_ancestors.size() == 2);
	for (const auto &participant : removed.changed_ancestors)
		assert(participant.before_identity.owner_revision == 11 &&
		       participant.after_identity.owner_revision == 12 &&
		       participant.before_identity.item_revision == 6 &&
		       participant.after_identity.item_revision == 7);
	assert(removed.selected_after_identities[0].item_revision == 8 &&
	       removed.selected_after_identities[0].owner_revision == 10 &&
	       removed.selected_after_identities[0].root_item_uid == 500 &&
	       !removed.selected_after_identities[0].parent_item_uid);
	assert(literal_wire({ before.items[2] }) == literal_wire({ removed.room_after.items[2] }) &&
	       same_identity(before.identities[2], removed.room_after.identities[2]));
	composition_route(*get, inserted.room_after, removed, false);
	std::printf("ROOM_PARENT_REAL_COMPOSITION {\"routes\":%zu,\"projected_plan_refusals\":%zu,"
		    "\"projection_allocation_cuts\":%zu,\"keeper_allocation_cuts\":%zu,"
		    "\"real_codec_projection_validator\":true,\"pure_helpers_only\":true}\n",
		    composition_routes_count, composition_refusals, projection_cuts,
		    composition_keeper_cuts);
}
}

int main()
{
	fixture put, get(false);
	healthy(put);
	healthy(get);
	fixture reordered;
	std::reverse(reordered.selected.before.begin(), reordered.selected.before.end());
	std::reverse(reordered.selected.after.begin(), reordered.selected.after.end());
	healthy(reordered);
	fixture unchanged;
	unchanged.plan.changed_ancestors.clear();
	unchanged.plan.room_after.identities[0] = unchanged.before.identities[0];
	unchanged.plan.room_after.identities[1] = unchanged.before.identities[1];
	healthy(unchanged);
	refuse("oversized selected events",
	       [](auto &x) { x.selected.events.resize(ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS + 1); });
	refuse("selected event count", [](auto &x) { x.selected.events.pop_back(); });
	refuse("selected event UID", [](auto &x) { ++x.selected.events[0].uid; });
	refuse("selected original index", [](auto &x) { x.selected.events[0].event_index = 1; });
	refuse("selected child index", [](auto &x) { x.selected.events[0].child_index = 1; });
	refuse("selected event order",
	       [](auto &x) { std::swap(x.selected.events[0], x.selected.events[1]); });
	refuse("selected event before", [](auto &x) { ++x.selected.events[0].before.revision; });
	refuse("selected event after", [](auto &x) { ++x.selected.events[0].after.revision; });
	refuse("base BEFORE disagrees", [](auto &x) { ++x.selected.before[0].position.revision; });
	refuse("base AFTER disagrees", [](auto &x) { ++x.selected.after[0].position.revision; });
	refuse("missing selected AFTER", [](auto &x) { x.selected.after.pop_back(); });
	refuse("duplicate base witness",
	       [](auto &x) { x.selected.before.push_back(x.selected.before[0]); });
	refuse("duplicate participant",
	       [](auto &x) { x.plan.changed_ancestors[1] = x.plan.changed_ancestors[0]; });
	refuse("participant order", [](auto &x)
	       { std::reverse(x.plan.changed_ancestors.begin(), x.plan.changed_ancestors.end()); });
	refuse("unrelated participant", [](auto &x)
	       { x.plan.changed_ancestors[0].before_identity = x.before.identities[3]; });
	refuse("stale owner clock", [](auto &x) { ++x.before.owner_revision; });
	refuse("wrong ROOM owner", [](auto &x) { x.before.owner = player; });
	refuse("stale old cached clock",
	       [](auto &x) { ++x.plan.changed_ancestors[0].before_identity.owner_revision; });
	refuse("stale next clock",
	       [](auto &x) { --x.plan.changed_ancestors[0].after_identity.owner_revision; });
	refuse("old metadata",
	       [](auto &x) { ++x.plan.changed_ancestors[0].before_identity.quantity; });
	refuse("next database metadata",
	       [](auto &x) { ++x.plan.changed_ancestors[0].after_identity.database_id; });
	refuse("next parent metadata",
	       [](auto &x) { ++x.plan.changed_ancestors[0].after_identity.serialized_parent_id; });
	refuse("next quantity",
	       [](auto &x) { ++x.plan.changed_ancestors[0].after_identity.quantity; });
	refuse("next override metadata",
	       [](auto &x) { ++x.plan.changed_ancestors[0].after_identity.override_mask; });
	refuse("next owner",
	       [](auto &x) { x.plan.changed_ancestors[0].after_identity.owner = player; });
	refuse("next root",
	       [](auto &x) { x.plan.changed_ancestors[0].after_identity.root_item_uid = 300; });
	refuse("next parent",
	       [](auto &x) { x.plan.changed_ancestors[0].after_identity.parent_item_uid = 300; });
	refuse("next state",
	       [](auto &x) {
		       x.plan.changed_ancestors[0].after_identity.state =
			       item_custody_state::destroyed;
	       });
	refuse("next exact increment",
	       [](auto &x) { ++x.plan.changed_ancestors[0].after_identity.item_revision; });
	refuse("missing projected identity", [](auto &x)
	       { x.plan.room_after.identities.erase(x.plan.room_after.identities.begin() + 1); });
	refuse("projected identity metadata",
	       [](auto &x) { ++x.plan.room_after.identities[1].quantity; });
	refuse("maximum global revision", [](auto &x) { x.before.owner_revision = UINT64_MAX; });
	refuse("maximum selected revision",
	       [](auto &x) { x.payload->items[0].expected_item_revision = UINT64_MAX; });
	refuse("maximum participant revision", [](auto &x)
	       { x.plan.changed_ancestors[0].before_identity.item_revision = UINT64_MAX; });
	refuse("unsupported reason",
	       [](auto &x) { x.payload->reason = item_transfer_reason::player_drop; });
	refuse("unsupported route", [](auto &x) { x.payload->to_owner = player; });
	refuse("GET exact event", [](auto &x) { ++x.selected.events[0].after.root_uid; }, false);
	assert(!item_transfer_repository_room_parent_delta(*put.payload, put.before, put.plan,
							   put.selected, nullptr));
	++refusal_count;
	refuse("empty selected request",
	       [](auto &x)
	       {
		       x.payload->item_count = 0;
		       x.selected.events.clear();
	       });
	refuse("overlimit BEFORE witnesses", [](auto &x)
	       { x.selected.before.resize(ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES + 1); });
	refuse("overlimit AFTER witnesses", [](auto &x)
	       { x.selected.after.resize(ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES + 1); });
	refuse("ancestor event capacity",
	       [](auto &x)
	       {
		       x.payload->multi_root = true;
		       x.payload->selected_item_uid = 0;
		       x.payload->item_count = ITEM_TRANSFER_MAX_ITEMS;
		       x.selected = {};
		       for (size_t i = 0; i < ITEM_TRANSFER_MAX_ITEMS; ++i)
		       {
			       const uint64_t uid = 10000 + i;
			       const economic_item_position old{
				       player, uid, 0, 6, item_custody_state::active, 0
			       };
			       const economic_item_position next{
				       room, 100, 30, 7, item_custody_state::active, 0
			       };
			       x.payload->items[i] = { uid, uid, 0,
						       6,   17,	 item_custody_state::active };
			       x.selected.before.push_back({ uid, old });
			       x.selected.after.push_back({ uid, next });
			       x.selected.events.push_back(
				       { static_cast<uint32_t>(i), 0, uid, old, next });
		       }
	       });
	std::printf("ROOM_PARENT_DELTA_PROGRESS healthy=%zu refusals=%zu allocation_cuts=%zu\n",
		    healthy_count, refusal_count, cut_count);
	std::fflush(stdout);
	// The projected ROOM graph must retain the same owner and next fenced clock.
	refuse("projected ROOM clock", [](auto &x) { x.plan.room_after.owner_revision = 10; });
	refuse("projected ROOM owner", [](auto &x) { x.plan.room_after.owner = player; });
	std::printf("ROOM_PARENT_DELTA {\"healthy_cases\":%zu,\"refusal_controls\":%zu,"
		    "\"allocation_cuts\":%zu,\"strong_outputs\":true,\"pure_keeper_only\":true}\n",
		    healthy_count, refusal_count, cut_count);
	actual_quiver_composition();
}
