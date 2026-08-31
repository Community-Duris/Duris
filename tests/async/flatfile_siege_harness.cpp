#include "persistence/persistence_mode.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{
std::string state_root;
bool rooms_available = true;
unsigned long next_test_uid = 100;

void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

std::vector<uint8_t> read_file(const fs::path &path)
{
	std::ifstream input(path, std::ios::binary);
	require(input.good(), "could not read " + path.string());
	return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

void write_file(const fs::path &path, const std::vector<uint8_t> &bytes)
{
	std::ofstream output(path, std::ios::binary | std::ios::trunc);
	require(output.good(), "could not open " + path.string());
	output.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
	require(output.good(), "could not write " + path.string());
}

fs::path make_state(const fs::path &base, const std::string &name)
{
	const fs::path root = base / name;
	fs::create_directories(root / "metadata");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "metadata", fs::perms::owner_all, fs::perm_options::replace);
	return root;
}
} // namespace

#include "../../src/combat/siege.c"

struct room_data room_storage[3] = {};
P_room world = room_storage;
int top_of_world = 2;

const char *persistence_mode_flatfile_root()
{
	return state_root.c_str();
}

void persistence_assign_item_uid(P_obj object, const char *)
{
	if (object && !object->obj_uid)
		object->obj_uid = next_test_uid++;
}

P_obj read_one_object(char *data)
{
	if (!data || static_cast<uint8_t>(data[0]) != flat_siege_legacy_version)
		return nullptr;
	int32_t count = 0;
	int32_t vnum = 0;
	int16_t craftsmanship = 0;
	int16_t condition = 0;
	uint32_t unique = 0;
	int32_t marker = 0;
	memcpy(&count, data + 1, sizeof(count));
	const uint8_t flags = static_cast<uint8_t>(data[5]);
	memcpy(&vnum, data + 6, sizeof(vnum));
	memcpy(&craftsmanship, data + 10, sizeof(craftsmanship));
	memcpy(&condition, data + 12, sizeof(condition));
	memcpy(&unique, data + 14, sizeof(unique));
	memcpy(&marker, data + 18, sizeof(marker));
	if (count != 1 || flags != O_F_UNIQUE || !flat_siege_legacy_vnum(vnum) ||
	    unique != O_U_VAL0 || static_cast<uint8_t>(data[22]) != O_F_EOL)
		return nullptr;
	P_obj object = new obj_data{};
	object->R_num = vnum;
	object->type = ITEM_CONTAINER;
	object->craftsmanship = craftsmanship;
	object->condition = condition;
	object->value[0] = marker;
	return object;
}

player_snapshot_capture_result
player_item_snapshot_tree_capture(P_obj root, std::vector<player_item_snapshot> *items, size_t *)
{
	if (!root || !items)
		return player_snapshot_capture_result::malformed_source;
	items->clear();
	std::vector<std::pair<P_obj, int32_t>> pending = { { root, PLAYER_SNAPSHOT_NO_PARENT } };
	while (!pending.empty())
	{
		const auto [object, parent] = pending.back();
		pending.pop_back();
		player_item_snapshot item = {};
		item.parent_index = parent;
		item.equipment_slot = -1;
		item.object_uid = object->obj_uid;
		item.vnum = object->R_num;
		item.type = object->type;
		item.condition = object->condition;
		for (size_t index = 0; index < item.values.size(); ++index)
			item.values[index] = object->value[index];
		const int32_t object_index = static_cast<int32_t>(items->size());
		items->push_back(std::move(item));
		std::vector<P_obj> children;
		for (P_obj child = object->contains; child; child = child->next_content)
			children.push_back(child);
		for (auto child = children.rbegin(); child != children.rend(); ++child)
			pending.push_back({ *child, object_index });
	}
	return player_snapshot_capture_result::ok;
}

bool player_load_item_graph_materialize_detached(
	const std::vector<player_item_snapshot> &items,
	const std::vector<player_load_item_identity> &identities,
	const item_owner_identity &expected_owner, uint64_t owner_revision, bool hydrate_ownership,
	bool complete_snapshot_state, std::vector<P_obj> *roots,
	player_load_item_materialize_metrics *metrics)
{
	if (!roots || !metrics || items.size() != identities.size() || expected_owner.id == 0 ||
	    owner_revision != 1 || hydrate_ownership || !complete_snapshot_state)
		return false;
	std::vector<P_obj> objects(items.size(), nullptr);
	for (size_t index = 0; index < items.size(); ++index)
	{
		objects[index] = new obj_data{};
		objects[index]->obj_uid = items[index].object_uid;
		objects[index]->R_num = items[index].vnum;
		objects[index]->type = items[index].type;
		objects[index]->condition = items[index].condition;
		for (size_t value = 0; value < items[index].values.size(); ++value)
			objects[index]->value[value] = items[index].values[value];
		if (items[index].parent_index == PLAYER_SNAPSHOT_NO_PARENT)
			roots->push_back(objects[index]);
		else
		{
			P_obj parent = objects[static_cast<size_t>(items[index].parent_index)];
			objects[index]->loc_p = LOC_INSIDE;
			objects[index]->loc.inside = parent;
			P_obj *tail = &parent->contains;
			while (*tail)
				tail = &(*tail)->next_content;
			*tail = objects[index];
		}
	}
	metrics->outcome = player_load_item_materialize_outcome::applied;
	metrics->item_count = items.size();
	return true;
}

int real_room(const int vnum)
{
	if (!rooms_available)
		return NOWHERE;
	for (int room = 0; room <= top_of_world; ++room)
		if (world[room].number == vnum)
			return room;
	return NOWHERE;
}

void obj_to_room(P_obj object, int room)
{
	if (!object || room < 0 || room > top_of_world)
		return;
	object->loc_p = LOC_ROOM;
	object->loc.room = room;
}

void extract_obj(P_obj object, int)
{
	if (!object)
		return;
	P_obj child = object->contains;
	while (child)
	{
		P_obj next = child->next_content;
		extract_obj(child, FALSE);
		child = next;
	}
	delete object;
}

namespace
{
P_obj make_object(int vnum, int room, int condition, int marker)
{
	P_obj object = new obj_data{};
	object->R_num = vnum;
	object->type = ITEM_CONTAINER;
	object->condition = condition;
	object->value[0] = marker;
	object->loc_p = LOC_ROOM;
	object->loc.room = room;
	return object;
}

template <typename T> void append_native(std::vector<uint8_t> *bytes, T value)
{
	const uint8_t *raw = reinterpret_cast<const uint8_t *>(&value);
	bytes->insert(bytes->end(), raw, raw + sizeof(value));
}

std::vector<uint8_t> legacy_siege_file(int room, int vnum, int condition, int marker)
{
	const std::string header = "#" + std::to_string(room) + "\n";
	std::vector<uint8_t> bytes(header.begin(), header.end());
	append_native<uint8_t>(&bytes, flat_siege_legacy_version);
	append_native<int32_t>(&bytes, 1);
	append_native<uint8_t>(&bytes, O_F_UNIQUE);
	append_native<int32_t>(&bytes, vnum);
	append_native<int16_t>(&bytes, 7);
	append_native<int16_t>(&bytes, condition);
	append_native<uint32_t>(&bytes, O_U_VAL0);
	append_native<int32_t>(&bytes, marker);
	append_native<uint8_t>(&bytes, O_F_EOL);
	append_native<uint8_t>(&bytes, '\n');
	return bytes;
}

void discard_live_siege()
{
	while (siege_objects)
	{
		P_siege next = siege_objects->next_siege;
		extract_obj(siege_objects->obj, FALSE);
		delete siege_objects;
		siege_objects = next;
	}
}
} // namespace

int main(int argc, char **argv)
{
	require(argc == 2, "temporary directory argument required");
	const fs::path base = fs::absolute(argv[1]);
	fs::create_directories(base / "Players");
	fs::current_path(base);
	world[1].number = 1001;
	world[2].number = 1002;

	const fs::path root = make_state(base, "state");
	state_root = root.string();
	require(flat_siege_load(nullptr), "missing siege authority was not treated as empty");
	require(!siege_objects, "missing siege authority populated live state");

	P_obj root_object = make_object(461, 1, 73, 111);
	P_obj child = make_object(900, NOWHERE, 42, 222);
	child->loc_p = LOC_INSIDE;
	child->loc.inside = root_object;
	root_object->contains = child;
	siege_objects = new siege{ root_object, nullptr };
	std::string error;
	require(flat_siege_save(&error), "could not save siege state: " + error);
	const fs::path authority = root / "metadata/siege";
	require(fs::is_regular_file(authority), "siege authority was not published");
	const fs::perms permissions = fs::status(authority).permissions();
	require((permissions & fs::perms::group_all) == fs::perms::none &&
			(permissions & fs::perms::others_all) == fs::perms::none,
		"siege authority permissions were not private");
	const std::vector<uint8_t> valid = read_file(authority);

	discard_live_siege();
	require(flat_siege_load(&error), "saved siege state did not load: " + error);
	require(siege_objects && !siege_objects->next_siege && siege_objects->obj->loc.room == 1 &&
			siege_objects->obj->condition == 73 && siege_objects->obj->value[0] == 111,
		"siege root state did not round trip");
	require(siege_objects->obj->contains && siege_objects->obj->contains->condition == 42 &&
			siege_objects->obj->contains->value[0] == 222,
		"siege contents did not round trip");

	discard_live_siege();
	std::vector<uint8_t> corrupt = valid;
	corrupt[corrupt.size() / 2] ^= 0x40;
	write_file(authority, corrupt);
	require(!flat_siege_load(&error), "corrupt siege authority was accepted");
	require(!siege_objects, "failed siege load changed live state");
	siege_objects = new siege{ make_object(462, 2, 55, 333), nullptr };
	require(!flat_siege_save(&error), "siege save overwrote corrupt authority");
	require(read_file(authority) == corrupt, "corrupt siege authority was modified");

	discard_live_siege();
	write_file(authority, valid);
	rooms_available = false;
	require(!flat_siege_load(&error), "unknown siege room was accepted");
	require(!siege_objects, "unknown-room load changed live state");
	rooms_available = true;

	fs::remove(authority);
	const fs::path legacy_path = base / "Players/siege";
	write_file(legacy_path, legacy_siege_file(1, 464, 66, 10));
	require(flat_siege_load(&error), "legacy siege state did not import: " + error);
	require(fs::is_regular_file(authority), "legacy import did not publish typed authority");
	require(siege_objects && !siege_objects->next_siege && siege_objects->obj->loc.room == 1 &&
			siege_objects->obj->R_num == 464 && siege_objects->obj->condition == 66 &&
			siege_objects->obj->value[0] == 10,
		"legacy siege object did not import exactly");

	discard_live_siege();
	write_file(legacy_path, { '#', '1', '\n' });
	require(flat_siege_load(&error), "typed siege authority did not take import precedence");
	require(siege_objects && siege_objects->obj->value[0] == 10,
		"typed authority did not preserve imported siege state");

	discard_live_siege();
	fs::remove(authority);
	require(!flat_siege_load(&error), "truncated legacy siege state was accepted");
	require(!fs::exists(authority), "failed legacy import published typed authority");
	require(!siege_objects, "failed legacy import changed live state");

	fs::remove(legacy_path);
	const fs::path symlink_target = base / "legacy-siege-target";
	write_file(symlink_target, legacy_siege_file(1, 461, 50, 12));
	fs::create_symlink(symlink_target, legacy_path);
	require(!flat_siege_load(&error), "legacy siege symlink was accepted");
	require(!fs::exists(authority), "legacy symlink import published typed authority");
	require(!siege_objects, "legacy symlink import changed live state");

	std::cout << "flat-file siege passed\n";
	return 0;
}
