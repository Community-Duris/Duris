#include "economy/collector_collection_preparation.h"

#include "classes/necromancy.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "item/item_ownership_runtime.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

// Synthetic native inputs only. No capture, codec, eligibility, identity,
// registry, revision, SQL, materializer, coordinator or publication substitute.
P_obj object_list = nullptr;
static room_data rooms[2]{};
P_room world = rooms;
extern const int top_of_world = 1;
static index_data indexes[6]{};
P_index obj_index = indexes;
int top_of_objt = 5;
static const auto fixture_thread = std::this_thread::get_id();
bool nevent_require_game_thread(const char *)
{
	assert(std::this_thread::get_id() == fixture_thread);
	return true;
}
bool nevent_is_game_thread()
{
	return std::this_thread::get_id() == fixture_thread;
}
void panic_corruption(const char *, const char *, ...)
{
	std::abort();
}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}
void fatal_boot_error(const char *, const char *, ...)
{
	std::abort();
}

using outcome = collector_collection_prepare_outcome;
static constexpr item_owner_identity collector_owner{ item_owner_type::collector, 77, 0 };
static constexpr uint64_t corpse_id = (uint64_t{ 42 } << 32) | 17;

static std::vector<uint8_t> encode(const std::vector<player_item_snapshot> &items)
{
	std::vector<uint8_t> result;
	assert(player_item_snapshot_list_encode(items, &result) ==
	       player_snapshot_codec_result::ok);
	return result;
}

// Independent values oracle: never read an obj_data or a captured snapshot here.
static player_item_snapshot expected_singleton()
{
	player_item_snapshot item{};
	item.parent_index = -1;
	item.object_uid = 200;
	item.generated_key = 17;
	item.vnum = 5001;
	item.type = ITEM_CONTAINER;
	item.string_mask = 15;
	item.name = "collector blade";
	item.short_description = "a marked container";
	item.description = "A marked container rests here.\r\n";
	item.action_description = "original action";
	item.wear_flags = ITEM_TAKE;
	item.anti_flags = 3;
	item.anti2_flags = 4;
	item.extra2_flags = 5;
	item.weight = 7; // Native total 14 minus direct child's aggregate 7.
	item.material = 1;
	item.cost = 97;
	item.condition = 83;
	item.craftsmanship = 3;
	item.bitvectors = { 1, 2, 3, 4, 5 };
	item.values = { 101, 102, 103, 104, 105, 106, 107, 108 };
	item.timers = { 201, 202, 203, 204, 205, 206 };
	item.affects = { { { 1, 31 }, { 2, 32 }, { 3, 33 }, { 4, 34 } } };
	item.dynamic_affects = { { 5, 77, 8 } };
	item.extra_descriptions = { { "inscription", "original detail", false, {} } };
	return item;
}

struct fixture
{
	std::deque<obj_data> objects;
	std::deque<extra_descr_data> extra;
	obj_affect affect{};
	extra_descr_data detail{};
	std::string oversized;
	P_obj root, selected, child, grandchild, sibling, corpse;
	item_owner_identity owner{ item_owner_type::room, 9000, 0 };
	std::vector<item_transfer_entry> expected_rows;
	player_item_snapshot expected = expected_singleton();
	collector::record entry;

	fixture(bool corpse_location = false, bool selected_root = false,
		bool preseed_destination = true)
	{
		item_ownership_runtime_reset();
		object_list = nullptr;
		rooms[0] = {};
		rooms[1] = {};
		rooms[0].number = 9000;
		rooms[1].number = 9001;
		for (int i = 0; i < 6; ++i)
		{
			indexes[i] = {};
			indexes[i].virtual_number = 5000 + i;
		}
		selected = object(200, 1, 14);
		selected->g_key = 17;
		selected->str_mask = 15;
		selected->name = const_cast<char *>("collector blade");
		selected->short_description = const_cast<char *>("a marked container");
		selected->description = const_cast<char *>("A marked container rests here.\r\n");
		selected->action_description = const_cast<char *>("original action");
		selected->anti_flags = 3;
		selected->anti2_flags = 4;
		selected->extra2_flags = 5;
		selected->material = 1;
		selected->cost = 97;
		selected->condition = 83;
		selected->craftsmanship = 3;
		selected->bitvector = 1;
		selected->bitvector2 = 2;
		selected->bitvector3 = 3;
		selected->bitvector4 = 4;
		selected->bitvector5 = 5;
		for (int i = 0; i < 8; ++i)
			selected->value[i] = 101 + i;
		for (int i = 0; i < 6; ++i)
			selected->timer[i] = 201 + i;
		for (int i = 0; i < 4; ++i)
		{
			selected->affected[i].location = 1 + i;
			selected->affected[i].modifier = 31 + i;
		}
		affect.type = 5;
		affect.data = 77;
		affect.extra2 = 8;
		selected->affects = &affect;
		detail.keyword = const_cast<char *>("inscription");
		detail.description = const_cast<char *>("original detail");
		selected->ex_description = &detail;
		child = object(90, 2, 7);
		grandchild = object(700, 3, 2);
		sibling = object(50, 4, 3);
		corpse = object(900, 5, 0);
		corpse->type = ITEM_CORPSE;
		corpse->value[CORPSE_FLAGS] = PC_CORPSE;
		corpse->value[CORPSE_PID] = 42;
		corpse->value[CORPSE_SAVEID] = 17;
		selected->contains = child;
		inside(child, selected);
		child->contains = grandchild;
		inside(grandchild, child);
		root = selected_root ? selected : object(500, 0, 25);
		if (selected_root)
		{
			child->next_content = sibling;
			inside(sibling, selected);
			selected->weight = 17;
		}
		else
		{
			root->contains = selected;
			inside(selected, root);
			selected->next_content = sibling;
			inside(sibling, root);
		}
		if (corpse_location)
		{
			owner = { item_owner_type::corpse, corpse_id, 0 };
			corpse->contains = root;
			inside(root, corpse);
		}
		else
		{
			root->loc_p = LOC_ROOM;
			root->loc.room = 0;
			rooms[0].contents = root;
		}
		const uint64_t root_uid = selected_root ? 200 : 500;
		// Explicit UID-sorted complete original root, independent of native walk.
		expected_rows = { { 50, root_uid, root_uid, 4, 5004, item_custody_state::active },
				  { 90, root_uid, 200, 3, 5002, item_custody_state::active },
				  { 200, root_uid, selected_root ? 0u : 500u, 2, 5001,
				    item_custody_state::active } };
		if (!selected_root)
			expected_rows.push_back(
				{ 500, 500, 0, 8, 5000, item_custody_state::active });
		expected_rows.push_back({ 700, root_uid, 90, 5, 5003, item_custody_state::active });
		for (const auto &row : expected_rows)
			assert(item_ownership_runtime_hydrate(
				{ row.item_uid, row.root_item_uid, row.parent_item_uid, owner,
				  row.expected_item_revision, 9, row.vnum, row.expected_state }));
		if (preseed_destination)
			assert(item_ownership_runtime_hydrate_owner(collector_owner, 0));
		collector::rules rules;
		rules.enabled = true;
		assert(collector::enroll(77, "123456789abcdef0123456789abcdef0", 42, 200, 2, 1000,
					 rules, &entry) == collector::outcome::applied);
	}

	~fixture()
	{
		object_list = nullptr;
		rooms[0].contents = nullptr;
		rooms[1].contents = nullptr;
		item_ownership_runtime_reset();
	}

	P_obj object(uint64_t uid, int rnum, int weight = 0)
	{
		objects.emplace_back();
		P_obj value = &objects.back();
		value->obj_uid = uid;
		value->R_num = rnum;
		value->type = ITEM_CONTAINER;
		value->wear_flags = ITEM_TAKE;
		value->weight = weight;
		value->name = const_cast<char *>("plain object");
		value->loc_p = LOC_NOWHERE;
		value->next = object_list;
		if (object_list)
			object_list->prev = value;
		object_list = value;
		return value;
	}

	static void inside(P_obj value, P_obj parent)
	{
		value->loc_p = LOC_INSIDE;
		value->loc.inside = parent;
	}

	item_ownership_runtime_entry lookup(P_obj object)
	{
		item_ownership_runtime_entry value{};
		assert(item_ownership_runtime_lookup(object->obj_uid, &value));
		return value;
	}

	void hydrate(const item_ownership_runtime_entry &value)
	{
		assert(item_ownership_runtime_hydrate(value));
	}

	std::vector<uint64_t> registry() const
	{
		std::vector<uint64_t> value{ item_ownership_runtime_size() };
		std::vector<item_owner_identity> owners{ owner, collector_owner };
		for (const auto &object : objects)
		{
			item_ownership_runtime_entry row{};
			const bool found = item_ownership_runtime_lookup(object.obj_uid, &row);
			value.insert(value.end(), { object.obj_uid, found });
			if (!found)
				continue;
			value.insert(value.end(),
				     { row.item_uid, row.root_item_uid, row.parent_item_uid,
				       uint64_t(row.owner.type), row.owner.id, row.owner.context_id,
				       row.item_revision, row.owner_revision, uint64_t(row.vnum),
				       uint64_t(row.state) });
			owners.push_back(row.owner);
		}
		for (const auto &identity : owners)
		{
			uint64_t revision = UINT64_MAX;
			const bool found =
				item_ownership_runtime_peek_owner_revision(identity, &revision);
			value.insert(value.end(), { uint64_t(identity.type), identity.id,
						    identity.context_id, found, revision });
		}
		return value;
	}

	std::vector<uint8_t> graph() const
	{
		// Read object representations only to detect writes to the same instances.
		// Expected capture values never come from these audit bytes.
		static_assert(std::is_trivially_copyable_v<obj_data>);
		std::vector<uint8_t> value;
		auto add = [&](const auto &object)
		{
			const auto *bytes = reinterpret_cast<const uint8_t *>(&object);
			value.insert(value.end(), bytes, bytes + sizeof(object));
		};
		add(object_list);
		for (const auto &object : objects)
			add(object);
		for (const auto &room : rooms)
		{
			add(room.contents);
			add(room.number);
		}
		for (const auto &index : indexes)
			add(index.virtual_number);
		add(affect);
		add(detail);
		for (const auto &description : extra)
			add(description);
		return value;
	}

	std::unique_ptr<collector_command_payload> prepare(bool allow_new_owner = false)
	{
		const auto graph_before = graph(), registry_before = registry_bytes();
		std::unique_ptr<collector_command_payload> result;
		assert(collector_collection_prepare(entry, entry.collect_at, &result) ==
		       outcome::prepared);
		assert(graph() == graph_before);
		if (!allow_new_owner)
			assert(registry_bytes() == registry_before);
		assert(result);
		return result;
	}

	std::vector<uint8_t> registry_bytes() const
	{
		const auto rows = registry();
		const auto *start = reinterpret_cast<const uint8_t *>(rows.data());
		return { start, start + rows.size() * sizeof(uint64_t) };
	}

	void refuse(outcome wanted, uint64_t now = 0)
	{
		auto result = std::make_unique<collector_command_payload>();
		result->listing = 999;
		result->observed_at = 123;
		result->selected_item_uid = 888;
		auto *original = result.get();
		static_assert(std::is_trivially_copyable_v<collector_command_payload>);
		std::vector<uint8_t> original_bytes(sizeof(*original));
		std::memcpy(original_bytes.data(), original, sizeof(*original));
		const auto graph_before = graph(), registry_before = registry_bytes();
		assert(collector_collection_prepare(entry, now ? now : entry.collect_at, &result) ==
		       wanted);
		assert(result.get() == original && result->listing == 999);
		assert(std::memcmp(result.get(), original_bytes.data(), sizeof(*original)) == 0);
		assert(graph() == graph_before && registry_bytes() == registry_before);
	}

	void compare(const collector_command_payload &payload, bool matches)
	{
		P_obj result = corpse;
		const auto graph_before = graph(), registry_before = registry_bytes();
		assert(collector_collection_live_matches(payload, &result) == matches);
		assert(result == (matches ? selected : corpse));
		assert(graph() == graph_before && registry_bytes() == registry_before);
	}

	void validate(const collector_command_payload &payload)
	{
		assert(item_owner_identity_valid(owner) &&
		       item_owner_identity_valid(collector_owner));
		assert(item_corpse_owner_id(42, 17) == corpse_id &&
		       item_collector_owner_id(77) == 77);
		assert(payload.action == collector_action::collect && payload.listing == 77 &&
		       payload.expected_listing_revision == 1 && payload.observed_at == 44200 &&
		       !payload.actor_pid && !payload.racewar && !payload.capacity_admitted &&
		       !payload.account_name[0] && !payload.expected_wallet_revision &&
		       !payload.expected_bank_revision && payload.selected_item_uid == 200 &&
		       payload.target_state == item_custody_state::active &&
		       payload.cancel_reason == collector::reason::none);
		assert(item_owner_identity_equal(payload.from_owner, owner));
		assert(item_owner_identity_equal(payload.to_owner, collector_owner));
		assert(payload.expected_from_owner_revision == 9 &&
		       payload.expected_to_owner_revision == 0);
		assert(payload.item_count == expected_rows.size());
		for (size_t i = 0; i < expected_rows.size(); ++i)
		{
			const auto &actual = payload.items[i], &wanted = expected_rows[i];
			assert(actual.item_uid == wanted.item_uid &&
			       actual.root_item_uid == wanted.root_item_uid &&
			       actual.parent_item_uid == wanted.parent_item_uid &&
			       actual.expected_item_revision == wanted.expected_item_revision &&
			       actual.vnum == wanted.vnum &&
			       actual.expected_state == wanted.expected_state);
		}
		const auto bytes = encode({ expected });
		assert(payload.item_blob_size == bytes.size() &&
		       std::equal(bytes.begin(), bytes.end(), payload.item_blob.begin()));
		std::vector<player_item_snapshot> decoded;
		assert(player_item_snapshot_list_decode(payload.item_blob.data(),
							payload.item_blob_size, &decoded) ==
		       player_snapshot_codec_result::ok);
		assert(decoded.size() == 1 && encode(decoded) == bytes);
		// The actual command layer must validate UID uniqueness and full topology.
		critical_operation_id operation{};
		operation.bytes[0] = 1;
		critical_command command;
		assert(collector_command_build(&command, operation, payload,
					       critical_source_site::command,
					       critical_deadline_class::interactive));
		// A synthetic acceptance timestamp exercises the real envelope codec;
		// this is structural validation, not admission to a coordinator/journal.
		command.accepted_at_usec = 123456;
		std::vector<uint8_t> wire;
		assert(critical_command_encode(command, &wire) ==
		       critical_command_codec_result::ok);
		critical_command restored;
		assert(critical_command_decode(wire.data(), wire.size(), &restored) ==
		       critical_command_codec_result::ok);
		assert(critical_command_equal(command, restored));
		auto decoded_payload = std::make_unique<collector_command_payload>();
		assert(collector_command_decode_payload(restored, decoded_payload.get()));
		std::vector<uint8_t> original, roundtrip;
		assert(collector_command_encode_payload(payload, &original) &&
		       collector_command_encode_payload(*decoded_payload, &roundtrip) &&
		       original == roundtrip);
		const critical_entity_key fences[] = { { critical_entity_type::collector, 77 },
						       { critical_entity_type::item, 50 },
						       { critical_entity_type::item, 90 },
						       { critical_entity_type::item, 200 },
						       { critical_entity_type::item, 700 } };
		for (const auto &key : fences)
			assert(std::any_of(command.expected_revisions.begin(),
					   command.expected_revisions.end(),
					   [&](const auto &fence)
					   { return critical_entity_key_equal(fence.key, key); }));
		compare(payload, true);
	}

	void add_siblings(size_t count)
	{
		for (size_t i = 0; i < count; ++i)
		{
			P_obj value = object(10000 + i, 4);
			inside(value, root);
			value->next_content = root->contains;
			root->contains = value;
			hydrate({ value->obj_uid, root->obj_uid, root->obj_uid, owner, 1, 9, 5004,
				  item_custody_state::active });
		}
	}
};

static void command_refuses(const collector_command_payload &payload)
{
	std::vector<uint8_t> bytes{ 11, 22, 33 };
	assert(!collector_command_encode_payload(payload, &bytes));
	assert((bytes == std::vector<uint8_t>{ 11, 22, 33 }));
}

int main(int argc, char **argv)
{
	assert(argc == 2);
	const std::string name = argv[1];
	const bool corpse = name == "corpse_nested" || name == "corpse_root" ||
			    name == "corpse_pid" || name == "corpse_saveid" ||
			    name == "corpse_flag" || name == "corpse_reciprocal";
	fixture f(corpse, name == "room_root" || name == "corpse_root", name != "new_owner");
	if (name == "new_owner")
	{
		uint64_t revision = 999;
		assert(!item_ownership_runtime_peek_owner_revision(collector_owner, &revision));
		assert(revision == 999);
		const size_t count = item_ownership_runtime_size();
		auto payload = f.prepare(true);
		assert(item_ownership_runtime_peek_owner_revision(collector_owner, &revision) &&
		       revision == 0 && item_ownership_runtime_size() == count);
		f.validate(*payload);
		const auto before = f.registry();
		f.validate(*f.prepare());
		assert(f.registry() == before);
	}
	else if (name == "room_nested" || name == "room_root" || name == "corpse_nested" ||
		 name == "corpse_root")
	{
		auto first = f.prepare();
		f.validate(*first);
		auto second = f.prepare();
		f.validate(*second);
		std::vector<uint8_t> a, b;
		assert(collector_command_encode_payload(*first, &a) &&
		       collector_command_encode_payload(*second, &b) && a == b);
	}
	else if (name == "ordinary_mask")
	{
		f.selected->str_mask = STRUNG_KEYS;
		f.expected.string_mask = STRUNG_KEYS;
		f.expected.short_description.clear();
		f.expected.description.clear();
		f.expected.action_description.clear();
		auto payload = f.prepare();
		f.validate(*payload);
		f.selected->description = const_cast<char *>("changed prototype reference");
		f.compare(*payload, true); // Ordinary capture omits this unmasked field.
		f.selected->name = const_cast<char *>("changed literal keys");
		f.compare(*payload, false);
	}
	else if (name == "newer_item")
	{
		auto row = f.lookup(f.selected);
		row.item_revision = 7;
		f.hydrate(row);
		f.expected_rows[2].expected_item_revision = 7;
		f.validate(*f.prepare()); // Environmental revision can exceed enrollment.
	}
	else if (name == "root_limit" || name == "root_limit_plus_one")
	{
		f.add_siblings(COLLECTOR_COMMAND_MAX_ITEMS - 5 + (name == "root_limit_plus_one"));
		if (name == "root_limit_plus_one")
			f.refuse(outcome::limit_exceeded);
		else
		{
			auto payload = f.prepare();
			assert(payload->item_count == COLLECTOR_COMMAND_MAX_ITEMS);
			f.compare(*payload, true);
			std::vector<uint8_t> bytes;
			assert(collector_command_encode_payload(*payload, &bytes));
		}
	}
	else if (name == "blob_budget" || name == "string_budget" || name == "depth_budget")
	{
		if (name == "string_budget")
		{
			f.oversized.assign(PLAYER_SNAPSHOT_MAX_STRING_BYTES + 1, 'x');
			f.selected->description = f.oversized.data();
		}
		else if (name == "blob_budget")
		{
			f.oversized.assign(4000, 'x');
			for (int i = 0; i < 34; ++i)
			{
				f.extra.emplace_back();
				auto *value = &f.extra.back();
				value->keyword = const_cast<char *>("budget");
				value->description = f.oversized.data();
				value->next = f.selected->ex_description;
				f.selected->ex_description = value;
			}
		}
		else
		{
			P_obj parent = f.grandchild;
			for (size_t i = 0; i < PLAYER_SNAPSHOT_MAX_DEPTH; ++i)
			{
				auto value = f.object(10000 + i, 4);
				fixture::inside(value, parent);
				parent->contains = value;
				f.hydrate({ value->obj_uid, 500, parent->obj_uid, f.owner, 1, 9,
					    5004, item_custody_state::active });
				parent = value;
			}
		}
		// Real snapshot/codec succeeds above the Collector blob cap, then adapter refuses.
		if (name == "blob_budget")
		{
			std::vector<player_item_snapshot> captured;
			assert(player_item_snapshot_tree_capture(f.selected, &captured, nullptr) ==
			       player_snapshot_capture_result::ok);
			assert(encode({ captured.front() }).size() >
			       COLLECTOR_COMMAND_ITEM_BLOB_MAX_BYTES);
		}
		f.refuse(outcome::invalid_topology);
	}
	else if (name == "negative_child" || name == "negative_selected" || name == "weight_sum" ||
		 name == "zero_weight" || name == "max_weight")
	{
		if (name == "negative_child")
			f.child->weight = -1;
		else if (name == "negative_selected")
			f.selected->weight = -1;
		else if (name == "weight_sum")
			f.child->weight = INT_MAX;
		else
		{
			f.child->weight = 0;
			f.selected->weight = name == "zero_weight" ? 0 : INT_MAX;
			f.expected.weight = name == "zero_weight" ? 0 : INT_MAX;
			f.validate(*f.prepare());
			goto done;
		}
		f.refuse(outcome::invalid_topology);
	}
	else if (name == "native_duplicate_nonselected")
	{
		auto duplicate = f.object(50, 4, 3);
		fixture::inside(duplicate, f.root);
		duplicate->next_content = f.root->contains;
		f.root->contains = duplicate;
		auto payload = f.prepare();
		assert(payload->item_count == 6 && payload->items[0].item_uid == 50 &&
		       payload->items[1].item_uid == 50);
		f.compare(*payload, true); // Pointer traversal does not enforce all UID uniqueness.
		command_refuses(*payload); // Genuine command layer supplies that refusal.
	}
	else if (name.starts_with("command_"))
	{
		auto payload = f.prepare();
		if (name == "command_duplicate")
			payload->items[0] = payload->items[1];
		else if (name == "command_parent")
			payload->items[0].parent_item_uid = 999;
		else if (name == "command_cycle")
		{
			payload->items[0].parent_item_uid = 90;
			payload->items[1].parent_item_uid = 50;
		}
		else if (name == "command_owner_max")
			payload->expected_from_owner_revision = UINT64_MAX;
		else
			assert(false);
		command_refuses(*payload);
	}
	else if (name.starts_with("pre_"))
	{
		outcome wanted = outcome::excluded;
		if (name == "pre_take")
			f.selected->wear_flags = 0;
		else if (name == "pre_money")
			f.selected->type = ITEM_MONEY;
		else if (name == "pre_corpse")
			f.selected->type = ITEM_CORPSE;
		else if (name == "pre_artifact")
			f.selected->extra_flags = ITEM_ARTIFACT;
		else if (name == "pre_transient")
			f.selected->extra_flags = ITEM_TRANSIENT;
		else if (name == "pre_norent")
			f.selected->extra_flags = ITEM_NORENT;
		else if (name == "pre_nosell")
			f.selected->extra_flags = ITEM_NOSELL;
		else if (name == "pre_bound")
			f.selected->extra2_flags = ITEM2_ACCOUNT_BOUND;
		else if (name == "pre_unique")
			f.selected->name = const_cast<char *>("unique blade");
		else if (name == "pre_rnum")
			f.selected->R_num = -1;
		else if (name == "pre_duplicate_selected")
		{
			f.object(200, 1);
			wanted = outcome::missing_item;
		}
		else if (name == "pre_powerunique")
		{
			f.selected->name = const_cast<char *>("unique powerunique blade");
			f.expected.name = "unique powerunique blade";
			f.validate(*f.prepare());
			goto done;
		}
		else
		{
			auto row = f.lookup(f.selected);
			if (name == "pre_claimed_player" || name == "pre_claimed_shop" ||
			    name == "pre_claimed_pet")
			{
				row.owner = { name == "pre_claimed_player" ?
						      item_owner_type::player :
					      name == "pre_claimed_shop" ?
						      item_owner_type::shopkeeper :
						      item_owner_type::pet,
					      42, name == "pre_claimed_pet" ? 17u : 0u };
				wanted = outcome::claimed;
			}
			else if (name == "pre_destroyed")
			{
				row.state = item_custody_state::destroyed;
				wanted = outcome::destroyed;
			}
			else if (name == "pre_quarantined")
			{
				row.state = item_custody_state::quarantined;
				wanted = outcome::stale_custody;
			}
			else if (name == "pre_revision_max")
			{
				row.item_revision = UINT64_MAX;
				wanted = outcome::stale_custody;
			}
			else if (name == "pre_revision_old")
			{
				f.entry.item_revision = 3;
				wanted = outcome::stale_custody;
			}
			else if (name == "pre_missing_registry")
			{
				item_ownership_runtime_forget(200);
				wanted = outcome::stale_custody;
				f.refuse(wanted);
				goto done;
			}
			else
				assert(false);
			f.hydrate(row);
		}
		f.refuse(wanted);
	}
	else
	{
		auto payload = f.prepare();
		if (name == "sibling_revision" || name == "ancestor_revision" ||
		    name == "descendant_revision" || name == "row_owner_revision" ||
		    name == "row_owner" || name == "row_root" || name == "row_parent" ||
		    name == "row_state" || name == "row_vnum" || name == "row_revision_zero")
		{
			P_obj object = name == "ancestor_revision"   ? f.root :
				       name == "descendant_revision" ? f.grandchild :
								       f.sibling;
			auto row = f.lookup(object);
			if (name == "row_owner_revision")
				row.owner_revision = 10;
			else if (name == "row_owner")
				row.owner = { item_owner_type::player, 42, 0 };
			else if (name == "row_root")
				row.root_item_uid = 999;
			else if (name == "row_parent")
				row.parent_item_uid = 90;
			else if (name == "row_state")
				row.state = item_custody_state::destroyed;
			else if (name == "row_vnum")
				row.vnum = 9999;
			else if (name == "row_revision_zero")
				row.item_revision = 0;
			else
				++row.item_revision;
			if (name == "row_revision_zero")
				item_ownership_runtime_forget(object->obj_uid);
			f.hydrate(row);
		}
		else if (name == "owner_clock_only")
		{
			assert(item_ownership_runtime_hydrate_owner(f.owner, 10));
			f.compare(*payload,
				  true); // Item rows, not the owner map, are observed here.
			goto done;
		}
		else if (name == "extra_child")
		{
			auto object = f.object(1000, 4);
			fixture::inside(object, f.root);
			object->next_content = f.root->contains;
			f.root->contains = object;
			f.hydrate({ 1000, 500, 500, f.owner, 1, 9, 5004,
				    item_custody_state::active });
		}
		else if (name == "missing_child")
			f.child->contains = nullptr;
		else if (name == "reciprocal")
			f.child->loc.inside = f.sibling;
		else if (name == "ancestor_reciprocal")
			f.root->contains = f.sibling;
		else if (name == "sibling_cycle")
			f.sibling->next_content = f.selected;
		else if (name == "global_cycle")
			f.objects.front().next = object_list;
		else if (name == "duplicate_selected")
			f.object(200, 1);
		else if (name == "room_vnum")
			rooms[0].number = 9001;
		else if (name == "room_reciprocal")
			rooms[0].contents = nullptr;
		else if (name == "corpse_pid")
			f.corpse->value[CORPSE_PID] = 43;
		else if (name == "corpse_saveid")
			f.corpse->value[CORPSE_SAVEID] = 18;
		else if (name == "corpse_flag")
			f.corpse->value[CORPSE_FLAGS] = 0;
		else if (name == "corpse_reciprocal")
			f.corpse->contains = nullptr;
		else if (name == "literal_cost")
			++f.selected->cost;
		else if (name == "literal_condition")
			++f.selected->condition;
		else if (name == "literal_fixed_affect")
			++f.selected->affected[1].modifier;
		else if (name == "literal_dynamic_affect")
			++f.affect.data;
		else if (name == "literal_extension")
			f.detail.description = const_cast<char *>("changed detail");
		else if (name == "literal_string")
			f.selected->short_description = const_cast<char *>("changed literal short");
		else if (name == "literal_weight")
			++f.child->weight;
		else if (name == "literal_generated_key")
			++f.selected->g_key;
		else if (name == "literal_value")
			++f.selected->value[3];
		else if (name == "literal_timer")
			++f.selected->timer[2];
		else if (name == "literal_flags")
			f.selected->extra_flags = ITEM_NOSELL;
		else if (name == "literal_mask")
			f.selected->str_mask = STRUNG_KEYS;
		else if (name == "affect_cycle")
			f.affect.next = &f.affect;
		else if (name == "description_cycle")
			f.detail.next = &f.detail;
		else if (name == "live_eligibility_omitted")
		{
			f.selected->str_mask = 0;
			payload = f.prepare();
			f.selected->name = const_cast<char *>("unique blade");
			assert(!collector_collection_item_eligible(f.selected));
			f.compare(*payload, true); // No eligibility rerun; unmasked name omitted.
			f.refuse(outcome::excluded); // Fresh preparation still checks eligibility.
			goto done;
		}
		else if (name == "not_due")
		{
			f.refuse(outcome::not_due, f.entry.collect_at - 1);
			goto done;
		}
		else
			assert(false && "unknown case");
		f.compare(*payload, false);
	}
done:
	std::printf(
		"PASS %s genuine_capture_codec_custody_command=1 native_durable_publication=0\n",
		name.c_str());
}
