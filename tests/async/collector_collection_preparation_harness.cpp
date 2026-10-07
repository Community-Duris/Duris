#include "economy/collector_collection_preparation.h"
#include "economy/collector_collection_image.h"

#include "classes/necromancy.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "item/item_ownership_runtime.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>

extern P_obj object_list;

namespace
{
index_data indexes[8] = {};
room_data rooms[1] = {};
obj_data root = {}, selected = {}, child = {}, sibling = {}, corpse = {};
std::map<uint64_t, item_ownership_runtime_entry> authority;
bool extracted = false;

void link_inside(P_obj object, P_obj parent, P_obj next = nullptr)
{
	object->loc_p = LOC_INSIDE;
	object->loc.inside = parent;
	object->next_content = next;
}

collector::record candidate_record()
{
	collector::rules rules;
	rules.enabled = true;
	collector::record entry;
	assert(collector::enroll(77, "123456789abcdef0123456789abcdef0", 42, selected.obj_uid, 2,
				 1000, rules, &entry) == collector::outcome::applied);
	return entry;
}

void seed_room_tree()
{
	authority.clear();
	extracted = false;
	root = {};
	selected = {};
	child = {};
	sibling = {};
	corpse = {};
	for (int index = 0; index < 8; ++index)
		indexes[index].virtual_number = 500 + index;
	root.obj_uid = 100;
	root.R_num = 0;
	root.type = ITEM_CONTAINER;
	root.weight = 20;
	selected.obj_uid = 101;
	selected.R_num = 1;
	selected.type = ITEM_CONTAINER;
	selected.wear_flags = ITEM_TAKE;
	selected.weight = 7;
	selected.cost = 75;
	selected.condition = 83;
	selected.name = const_cast<char *>("blade collector");
	child.obj_uid = 102;
	child.R_num = 2;
	child.type = ITEM_WEAPON;
	child.wear_flags = ITEM_TAKE;
	child.weight = 2;
	child.name = const_cast<char *>("gem");
	sibling.obj_uid = 103;
	sibling.R_num = 3;
	sibling.type = ITEM_WEAPON;
	sibling.wear_flags = ITEM_TAKE;
	sibling.weight = 3;
	sibling.name = const_cast<char *>("sibling");
	root.loc_p = LOC_ROOM;
	root.loc.room = 0;
	root.contains = &selected;
	link_inside(&selected, &root, &sibling);
	selected.contains = &child;
	link_inside(&child, &selected);
	link_inside(&sibling, &root);
	root.next = &selected;
	selected.prev = &root;
	selected.next = &child;
	child.prev = &selected;
	child.next = &sibling;
	sibling.prev = &child;
	object_list = &root;
	rooms[0].number = 900;
	rooms[0].contents = &root;
	const item_owner_identity owner = { item_owner_type::room, 900, 0 };
	authority.emplace(100, item_ownership_runtime_entry{ 100, 100, 0, owner, 1, 9, 500,
							     item_custody_state::active });
	authority.emplace(101, item_ownership_runtime_entry{ 101, 100, 100, owner, 2, 9, 501,
							     item_custody_state::active });
	authority.emplace(102, item_ownership_runtime_entry{ 102, 100, 101, owner, 3, 9, 502,
							     item_custody_state::active });
	authority.emplace(103, item_ownership_runtime_entry{ 103, 100, 100, owner, 4, 9, 503,
							     item_custody_state::active });
}
}

P_index obj_index = indexes;
P_obj object_list = nullptr;
P_room world = rooms;
extern const int top_of_world = 0;
int top_of_objt = 7;

bool isname(const char *needle, const char *haystack)
{
	return needle && haystack && strstr(haystack, needle);
}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}

bool item_owner_identity_equal(const item_owner_identity &left, const item_owner_identity &right)
{
	return left.type == right.type && left.id == right.id &&
	       left.context_id == right.context_id;
}

uint64_t item_corpse_owner_id(uint32_t player_pid, uint32_t save_id)
{
	return (static_cast<uint64_t>(player_pid) << 32) | save_id;
}

uint64_t item_collector_owner_id(uint64_t listing_id)
{
	return listing_id;
}

bool item_ownership_runtime_lookup(uint64_t item_uid, item_ownership_runtime_entry *entry)
{
	const auto found = authority.find(item_uid);
	if (!entry || found == authority.end())
		return false;
	*entry = found->second;
	return true;
}

bool item_ownership_runtime_owner_revision(const item_owner_identity &owner, uint64_t *revision)
{
	if (!revision || owner.type != item_owner_type::collector)
		return false;
	*revision = 0;
	return true;
}

player_snapshot_capture_result
player_item_snapshot_tree_capture(P_obj object, std::vector<player_item_snapshot> *items, size_t *)
{
	if (!object || !items)
		return player_snapshot_capture_result::invalid_identity;
	player_item_snapshot item = {};
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.object_uid = object->obj_uid;
	item.vnum = OBJ_VNUM(object);
	item.type = object->type;
	item.wear_flags = object->wear_flags;
	item.extra_flags = object->extra_flags;
	item.extra2_flags = object->extra2_flags;
	item.weight = object->weight;
	item.cost = object->cost;
	item.condition = object->condition;
	items->assign(1, item);
	return player_snapshot_capture_result::ok;
}

void obj_from_obj(P_obj object)
{
	assert(object && OBJ_INSIDE(object) && object->loc.inside);
	P_obj parent = object->loc.inside;
	P_obj *cursor = &parent->contains;
	while (*cursor && *cursor != object)
		cursor = &(*cursor)->next_content;
	assert(*cursor == object);
	*cursor = object->next_content;
	object->next_content = nullptr;
	object->loc_p = LOC_NOWHERE;
	object->loc.inside = nullptr;
}

void obj_to_obj(P_obj object, P_obj parent)
{
	assert(object && parent && OBJ_NOWHERE(object));
	object->loc_p = LOC_INSIDE;
	object->loc.inside = parent;
	object->next_content = parent->contains;
	parent->contains = object;
}

void obj_to_room(P_obj object, int room)
{
	assert(object && OBJ_NOWHERE(object) && room == 0);
	object->loc_p = LOC_ROOM;
	object->loc.room = room;
	object->next_content = world[room].contents;
	world[room].contents = object;
}

void extract_obj(P_obj object, int)
{
	assert(object && !object->contains);
	if (OBJ_INSIDE(object))
		obj_from_obj(object);
	else if (OBJ_ROOM(object))
	{
		P_obj *cursor = &world[object->loc.room].contents;
		while (*cursor && *cursor != object)
			cursor = &(*cursor)->next_content;
		assert(*cursor == object);
		*cursor = object->next_content;
		object->next_content = nullptr;
		object->loc_p = LOC_NOWHERE;
	}
	P_obj *cursor = &object_list;
	while (*cursor && *cursor != object)
		cursor = &(*cursor)->next;
	assert(*cursor == object);
	*cursor = object->next;
	object->next = nullptr;
	extracted = true;
}

int main()
{
	// Owned-state rules execute with the production codec and no live objects.
	player_item_snapshot image = {};
	image.object_uid = 901;
	image.vnum = 501;
	image.type = ITEM_CONTAINER;
	image.parent_index = 7;
	image.equipment_slot = 3;
	image.weight = 17;
	image.cost = 123;
	image.condition = 83;
	image.string_mask = STRUNG_KEYS | STRUNG_DESC2;
	image.name = "collector custom container";
	image.short_description = "a preserved custom container";
	image.values[0] = 444;
	image.timers[0] = 19;
	image.extra2_flags = ITEM2_ACCOUNT_BOUND;
	image.dynamic_affects.push_back({ 1, 2, 3 });
	const std::array<int64_t, 2> weights = { 5, 7 };
	std::vector<uint8_t> first, second;
	assert(collector_collection_prepare_image(image, weights, &first));
	assert(collector_collection_prepare_image(image, weights, &second) && first == second);
	assert(image.weight == 17 && image.parent_index == 7 && image.equipment_slot == 3);
	std::vector<player_item_snapshot> roundtrip;
	assert(player_item_snapshot_list_decode(first.data(), first.size(), &roundtrip) ==
	       player_snapshot_codec_result::ok);
	assert(roundtrip.size() == 1 && roundtrip[0].weight == 5 &&
	       roundtrip[0].object_uid == 901 && roundtrip[0].vnum == 501 &&
	       roundtrip[0].parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
	       roundtrip[0].equipment_slot == 0 && roundtrip[0].cost == 123 &&
	       roundtrip[0].condition == 83 && roundtrip[0].name == image.name &&
	       roundtrip[0].short_description == image.short_description &&
	       roundtrip[0].values == image.values && roundtrip[0].timers == image.timers &&
	       roundtrip[0].extra2_flags == image.extra2_flags &&
	       roundtrip[0].dynamic_affects.size() == 1 &&
	       roundtrip[0].dynamic_affects[0].data == 2);
	const std::array<int64_t, 1> negative = { -1 }, excessive = { 18 };
	const std::array<int64_t, 2> overflow = { INT64_MAX, 1 };
	assert(!collector_collection_prepare_image(image, negative, &second));
	assert(!collector_collection_prepare_image(image, excessive, &second));
	assert(!collector_collection_prepare_image(image, overflow, &second));
	assert(second == first);
	assert(!collector_collection_prepare_image(image, weights, nullptr));
	image.weight = -1;
	assert(!collector_collection_prepare_image(image, {}, &second));
	image.weight = INT32_MAX;
	assert(collector_collection_prepare_image(image, {}, &second));
	image.weight = 12;
	assert(collector_collection_prepare_image(image, weights, &second));
	assert(player_item_snapshot_list_decode(second.data(), second.size(), &roundtrip) ==
		       player_snapshot_codec_result::ok &&
	       roundtrip[0].weight == 0);
	image.name.assign(PLAYER_SNAPSHOT_MAX_STRING_BYTES + 1, 'x');
	assert(!collector_collection_prepare_image(image, {}, &second));
	image.name = "collector";
	image.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	image.equipment_slot = 0;
	image.extra_descriptions.assign(
		17, { std::string(4096, 'k'), std::string(4096, 'd'), false, {} });
	assert(player_item_snapshot_list_encode({ image }, &second) ==
		       player_snapshot_codec_result::ok &&
	       second.size() > COLLECTOR_COMMAND_ITEM_BLOB_MAX_BYTES);
	assert(!collector_collection_prepare_image(image, {}, &second));

	seed_room_tree();
	const collector::record entry = candidate_record();
	std::unique_ptr<collector_command_payload> payload;
	assert(collector_collection_prepare(entry, entry.collect_at, &payload) ==
	       collector_collection_prepare_outcome::prepared);
	assert(payload && payload->action == collector_action::collect &&
	       payload->listing == entry.listing &&
	       payload->selected_item_uid == selected.obj_uid &&
	       payload->expected_from_owner_revision == 9 &&
	       payload->expected_to_owner_revision == 0 && payload->item_count == 4 &&
	       payload->items[0].item_uid == 100 && payload->items[1].item_uid == 101 &&
	       payload->items[2].item_uid == 102 && payload->items[3].item_uid == 103 &&
	       payload->item_blob_size > 0);
	std::vector<player_item_snapshot> decoded;
	assert(player_item_snapshot_list_decode(payload->item_blob.data(), payload->item_blob_size,
						&decoded) == player_snapshot_codec_result::ok);
	assert(decoded.size() == 1 && decoded[0].weight == 5 && decoded[0].object_uid == 101 &&
	       decoded[0].vnum == 501 && decoded[0].cost == 75 && decoded[0].condition == 83 &&
	       decoded[0].parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
	       decoded[0].equipment_slot == 0);
	std::unique_ptr<collector_command_payload> repeated;
	assert(collector_collection_prepare(entry, entry.collect_at, &repeated) ==
	       collector_collection_prepare_outcome::prepared);
	assert(repeated->item_blob_size == payload->item_blob_size &&
	       repeated->item_blob == payload->item_blob &&
	       repeated->item_count == payload->item_count && selected.contains == &child &&
	       selected.weight == 7 && !extracted);
	child.weight = -1;
	assert(collector_collection_prepare(entry, entry.collect_at, &repeated) ==
	       collector_collection_prepare_outcome::invalid_topology);
	child.weight = 8;
	assert(collector_collection_prepare(entry, entry.collect_at, &repeated) ==
	       collector_collection_prepare_outcome::invalid_topology);
	child.weight = 2;
	P_obj live = nullptr;
	assert(collector_collection_live_matches(*payload, &live) && live == &selected);
	selected.cost++;
	assert(!collector_collection_live_matches(*payload, &live));
	selected.cost--;
	authority[102].parent_item_uid = 100;
	assert(!collector_collection_live_matches(*payload, &live));
	authority[102].parent_item_uid = 101;
	assert(collector_collection_live_matches(*payload, &live));
	assert(collector_collection_detach_live(live));
	assert(extracted && root.contains == &child && child.loc.inside == &root &&
	       child.next_content == &sibling && sibling.loc.inside == &root &&
	       !selected.contains && OBJ_NOWHERE(&selected));

	seed_room_tree();
	auto claimed = candidate_record();
	authority[101].owner = { item_owner_type::player, 42, 0 };
	assert(collector_collection_prepare(claimed, claimed.collect_at, &payload) ==
	       collector_collection_prepare_outcome::claimed);
	authority[101].owner = { item_owner_type::room, 900, 0 };
	selected.extra_flags = ITEM_NOSELL;
	assert(collector_collection_prepare(claimed, claimed.collect_at, &payload) ==
	       collector_collection_prepare_outcome::excluded);
	selected.extra_flags = 0;
	object_list = &root;
	root.next = nullptr;
	assert(collector_collection_prepare(claimed, claimed.collect_at, &payload) ==
	       collector_collection_prepare_outcome::missing_item);

	seed_room_tree();
	const item_owner_identity corpse_owner = { item_owner_type::corpse,
						   item_corpse_owner_id(42, 12), 0 };
	for (auto &[uid, row] : authority)
	{
		(void)uid;
		row.owner = corpse_owner;
	}
	corpse.obj_uid = 200;
	corpse.R_num = 4;
	corpse.type = ITEM_CORPSE;
	corpse.value[CORPSE_FLAGS] = PC_CORPSE;
	corpse.value[CORPSE_PID] = 42;
	corpse.value[CORPSE_SAVEID] = 12;
	corpse.contains = &root;
	root.loc_p = LOC_INSIDE;
	root.loc.inside = &corpse;
	root.next_content = nullptr;
	rooms[0].contents = &corpse;
	corpse.loc_p = LOC_ROOM;
	corpse.loc.room = 0;
	sibling.next = &corpse;
	corpse.prev = &sibling;
	assert(collector_collection_prepare(candidate_record(), entry.collect_at, &payload) ==
	       collector_collection_prepare_outcome::prepared);
}
