#include "persistence/sql_room_item_payload.h"
#include "player/player_snapshot_codec.h"

#include <cassert>
#include <cstdio>
#include <memory>

using projection = sql_room_item_parent_projection_result;
static constexpr item_owner_identity room = { item_owner_type::room, 120, 0 };
static constexpr item_owner_identity player = { item_owner_type::player, 7, 0 };

static player_item_snapshot literal(uint64_t uid, int type, int weight, int parent = -1)
{
	player_item_snapshot item;
	item.object_uid = uid;
	item.vnum = 1200;
	item.parent_index = parent;
	item.type = type;
	item.weight = weight;
	item.string_mask = STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3;
	item.name = "exact keys";
	item.short_description = "exact short";
	item.description = "exact room";
	item.action_description = "exact action";
	return item;
}

static sql_room_item_graph graph(std::vector<player_item_snapshot> items)
{
	sql_room_item_graph result;
	result.owner = room;
	result.owner_revision = 4;
	result.items = std::move(items);
	for (const auto &item : result.items)
	{
		player_load_item_identity row;
		row.item_uid = item.object_uid;
		row.item_revision = 10;
		row.owner_revision = 3;
		row.owner = room;
		row.state = item_custody_state::active;
		row.root_item_uid = item.parent_index < 0 ?
					    item.object_uid :
					    result.identities[item.parent_index].root_item_uid;
		row.parent_item_uid =
			item.parent_index < 0 ? 0 : result.items[item.parent_index].object_uid;
		result.identities.push_back(row);
	}
	return result;
}

static std::unique_ptr<item_transfer_payload>
command(const sql_room_item_graph &before, bool put,
	const std::vector<player_item_snapshot> &selected, uint64_t parent_uid = 1)
{
	auto payload = std::make_unique<item_transfer_payload>();
	payload->reason = put ? item_transfer_reason::player_put : item_transfer_reason::player_get;
	payload->from_owner = put ? player : room;
	payload->to_owner = put ? room : player;
	payload->expected_from_revision = put ? 8 : before.owner_revision;
	payload->expected_to_revision = put ? before.owner_revision : 8;
	payload->selected_item_uid = selected.size() == 1 ? selected.front().object_uid : 0;
	payload->multi_root = selected.size() != 1;
	payload->target_root_item_uid = put ? before.identities.front().root_item_uid : 0;
	payload->target_parent_item_uid = put ? parent_uid : 0;
	payload->expected_target_parent_revision = put ? 10 : 0;
	payload->item_count = selected.size();
	std::vector<uint8_t> wire;
	assert(player_item_snapshot_list_encode(selected, &wire) ==
	       player_snapshot_codec_result::ok);
	payload->item_blob_size = wire.size();
	std::copy(wire.begin(), wire.end(), payload->item_blob.begin());
	for (size_t i = 0; i < selected.size(); ++i)
	{
		const auto &item = selected[i];
		auto &entry = payload->items[i];
		entry.item_uid = item.object_uid;
		entry.vnum = item.vnum;
		entry.expected_item_revision = 10;
		entry.expected_state = item_custody_state::active;
		entry.root_item_uid = item.object_uid;
		entry.parent_item_uid = 0;
		if (!put)
			for (const auto &row : before.identities)
				if (row.item_uid == item.object_uid)
				{
					entry.root_item_uid = row.root_item_uid;
					entry.parent_item_uid = row.parent_item_uid;
				}
	}
	std::sort(payload->items.begin(), payload->items.begin() + payload->item_count,
		  [](const auto &a, const auto &b) { return a.item_uid < b.item_uid; });
	return payload;
}

static void refusal(const item_transfer_payload &payload, const sql_room_item_graph &before,
		    projection expected,
		    const std::vector<sql_room_item_parent_weight_fact> &facts = {})
{
	sql_room_item_parent_after_plan sentinel;
	sentinel.room_after.owner_revision = 999;
	sentinel.selected_after = { literal(999, ITEM_CONTAINER, 99) };
	assert(sql_room_item_payload_parent_after(payload, before, facts, &sentinel) == expected);
	assert(sentinel.room_after.owner_revision == 999 && sentinel.selected_after.size() == 1 &&
	       sentinel.selected_after[0].object_uid == 999 && sentinel.changed_ancestors.empty());
}

int main()
{
	auto quiver = literal(1, ITEM_QUIVER, 10);
	quiver.values[0] = 1;
	quiver.values[2] = 3;
	auto missile = literal(2, ITEM_MISSILE, 5);
	missile.values[3] = 3;
	auto before = graph({ quiver });
	auto put = command(before, true, { missile });
	sql_room_item_parent_after_plan after;
	assert(sql_room_item_payload_parent_after(*put, before, {}, &after) == projection::ok);
	assert(after.room_after.owner_revision == 5 && after.room_after.items.size() == 2);
	assert(after.room_after.items[0].values[3] == 1 && after.room_after.items[0].weight == 15);
	assert(after.room_after.items[1].parent_index == 0 && after.changed_ancestors.size() == 1);
	assert(after.changed_ancestors[0].before_identity.item_revision == 10 &&
	       after.changed_ancestors[0].after_identity.item_revision == 11);
	assert(after.selected_after_identities[0].root_item_uid == 1 &&
	       after.selected_after_identities[0].parent_item_uid == 1);
	auto get = command(after.room_after, false, { missile });
	get->items[0].expected_item_revision = 11;
	sql_room_item_parent_after_plan emptied;
	assert(sql_room_item_payload_parent_after(*get, after.room_after, {}, &emptied) ==
	       projection::ok);
	assert(emptied.room_after.items.size() == 1 && emptied.room_after.items[0].weight == 10 &&
	       emptied.room_after.items[0].values[3] == 0);
	auto refill = command(emptied.room_after, true, { missile });
	refill->expected_target_parent_revision = 12;
	assert(sql_room_item_payload_parent_after(*refill, emptied.room_after, {}, &after) ==
	       projection::ok);

	before.items[0].values[1] = CONT_CLOSED;
	refusal(*put, before, projection::invalid);
	before.items[0] = quiver;
	before.items[0].values[3] = 1;
	refusal(*put, before, projection::invalid);
	before.items[0] = quiver;
	auto wrong_ammo = missile;
	wrong_ammo.values[3] = 4;
	auto wrong = command(before, true, { wrong_ammo });
	refusal(*wrong, before, projection::invalid);
	put->expected_target_parent_revision = 9;
	refusal(*put, before, projection::stale);
	put->expected_target_parent_revision = 10;
	put->target_root_item_uid = 999;
	refusal(*put, before, projection::invalid);
	put->target_root_item_uid = 1;
	before.items[0].weight = INT32_MAX;
	refusal(*put, before, projection::overflow);
	before.items[0] = quiver;
	before.identities[0].owner.id = 999;
	refusal(*put, before, projection::invalid);

	// Both external ancestors gain the literal weight, each exactly once.
	before = graph({ literal(1, ITEM_CONTAINER, 20), literal(2, ITEM_CONTAINER, 10, 0) });
	auto child = literal(3, ITEM_MISSILE, 5);
	put = command(before, true, { child }, 2);
	assert(sql_room_item_payload_parent_after(*put, before, {}, &after) == projection::ok);
	assert(after.changed_ancestors.size() == 2 && after.room_after.items[0].weight == 25 &&
	       after.room_after.items[1].weight == 15);
	before.items[1].weight = -10;
	assert(sql_room_item_payload_parent_after(*put, before, {}, &after) == projection::ok);
	assert(after.changed_ancestors.size() == 1 &&
	       after.changed_ancestors[0].before_identity.item_uid == 2 &&
	       after.room_after.items[0].weight == 20 &&
	       after.room_after.identities[0].item_revision == 10);

	// A reducing parent requires actual native shell/all-direct-contents facts.
	auto reducing = literal(1, ITEM_CONTAINER, 11);
	reducing.values[4] = 50;
	before = graph({ reducing });
	put = command(before, true, { child });
	refusal(*put, before, projection::weight_fact_required);
	std::vector<sql_room_item_parent_weight_fact> facts = { { 1, 10, 10, 2 } };
	assert(sql_room_item_payload_parent_after(*put, before, facts, &after) == projection::ok);
	assert(after.room_after.items[0].weight == 14);
	facts[0].item_revision = 9;
	refusal(*put, before, projection::stale, facts);
	facts[0].item_revision = 10;
	facts.push_back({ 99, 10, 10, 0 });
	refusal(*put, before, projection::invalid, facts);

	// Two roots share the original fact; its owned contents sum advances each time.
	auto a = literal(2, ITEM_MISSILE, 3, 0);
	auto b = literal(3, ITEM_MISSILE, 5, 0);
	reducing.weight = 14;
	before = graph({ reducing, a, b });
	a.parent_index = b.parent_index = -1;
	get = command(before, false, { a, b });
	facts = { { 1, 10, 10, 8 } };
	assert(sql_room_item_payload_parent_after(*get, before, facts, &after) == projection::ok);
	assert(after.room_after.items.size() == 1 && after.room_after.items[0].weight == 10 &&
	       after.changed_ancestors.size() == 1 &&
	       after.changed_ancestors[0].after_identity.item_revision == 11);
	get->items[1].expected_item_revision = 9;
	refusal(*get, before, projection::stale, facts);
	get->items[1].expected_item_revision = 10;
	before.items[1].short_description = "changed actual literal";
	refusal(*get, before, projection::stale, facts);

	// A lower parent's weight change also advances a later direct parent's fact.
	auto outer = literal(1, ITEM_CONTAINER, 18);
	outer.values[4] = 50;
	auto inner = literal(2, ITEM_CONTAINER, 11, 0);
	inner.values[4] = 50;
	a = literal(3, ITEM_MISSILE, 2, 1);
	b = literal(4, ITEM_MISSILE, 5, 0);
	before = graph({ outer, inner, a, b });
	a.parent_index = b.parent_index = -1;
	get = command(before, false, { a, b });
	facts = { { 1, 10, 10, 16 }, { 2, 10, 10, 2 } };
	assert(sql_room_item_payload_parent_after(*get, before, facts, &after) == projection::ok);
	assert(after.room_after.items[0].weight == 15 && after.room_after.items[1].weight == 10 &&
	       after.changed_ancestors.size() == 2);
	// The original extractor orders selected roots by UID. A reversed complete
	// literal refuses; its canonical order drives sequential native weight rules.
	outer.weight = 20;
	inner.weight = 14;
	a = literal(4, ITEM_MISSILE, 6, 1);
	b = literal(3, ITEM_MISSILE, 5, 0);
	before = graph({ outer, inner, a, b });
	a.parent_index = b.parent_index = -1;
	get = command(before, false, { a, b });
	facts = { { 1, 10, 10, 19 }, { 2, 10, 10, 8 } };
	refusal(*get, before, projection::stale, facts);
	get = command(before, false, { b, a });
	assert(sql_room_item_payload_parent_after(*get, before, facts, &after) == projection::ok);
	assert(after.room_after.items[0].weight == 14 && after.room_after.items[1].weight == 11);
	get->items[1].item_uid = get->items[0].item_uid;
	refusal(*get, before, projection::invalid, facts);
	puts("PASS owned ROOM parent AFTER: quiver cycle, ancestor weights, clamp, facts, multi-root and strong refusals");
}
