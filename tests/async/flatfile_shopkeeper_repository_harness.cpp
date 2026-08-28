#include "flatfile_shopkeeper_repository.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

static void prepare_root(const fs::path &root)
{
	fs::create_directories(root / "domains");
	fs::create_directories(root / "players");
	fs::create_directories(root / "identities/names");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "domains", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "players", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities/names", fs::perms::owner_all, fs::perm_options::replace);
}

static player_item_snapshot item(uint64_t uid, int32_t parent, int16_t slot, int32_t vnum)
{
	player_item_snapshot value = {};
	value.parent_index = parent;
	value.equipment_slot = slot;
	value.object_uid = uid;
	value.generated_key = uid + 1000;
	value.vnum = vnum;
	value.name = "shop item";
	value.short_description = "a shop item";
	value.dynamic_affects.push_back({ 3, 4, 5 });
	value.extra_descriptions.push_back({ "mark", "a maker's mark", false, {} });
	return value;
}

static flatfile_shopkeeper_record shop(uint32_t id, uint64_t uid)
{
	flatfile_shopkeeper_record value = {};
	value.shop_id = id;
	value.mob_vnum = 1000 + id;
	value.room_vnum = 2000 + id;
	value.saved_at = 3000 + id;
	value.revision = 2;
	value.affects = { { 9, 10, 11, 12, { 1, 2, 3, 4, 5 } },
			  { 1, 2, 3, 4, { 6, 7, 8, 9, 10 } } };
	value.items = { item(uid, PLAYER_SNAPSHOT_NO_PARENT, 3, 400), item(uid + 1, 0, -1, 401),
			item(uid + 2, PLAYER_SNAPSHOT_NO_PARENT, -1, 402) };
	return value;
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = fs::path(argv[1]) / "shops";
	prepare_root(root);
	std::string error;
	std::vector<flatfile_shopkeeper_record> records;
	require(flatfile_shopkeeper_list(root.string(), &records, &error) ==
			flatfile_shopkeeper_result::not_found,
		"missing shopkeeper authority did not fail closed");
	const auto first = shop(2, 100);
	const auto second = shop(1, 200);
	require(flatfile_shopkeeper_establish(root.string(), { first, second }, &error) ==
			flatfile_shopkeeper_result::ok,
		"shopkeeper establishment failed: " + error);
	require(flatfile_shopkeeper_establish(root.string(), { second, first }, &error) ==
			flatfile_shopkeeper_result::already_exists,
		"canonical shopkeeper establishment retry was not idempotent");
	require(flatfile_shopkeeper_list(root.string(), &records, &error) ==
				flatfile_shopkeeper_result::ok &&
			records.size() == 2 && records[0].shop_id == 1 &&
			records[1].affects[0].type == 1 &&
			records[1].affects[1].bitvectors[4] == 5 && records[1].items.size() == 3 &&
			records[1].items[0].equipment_slot == 3 &&
			records[1].items[1].parent_index == 0 &&
			records[1].items[1].dynamic_affects[0].extra2 == 5,
		"shopkeeper catalog was not canonical or did not round trip state");
	auto conflicting = first;
	conflicting.room_vnum++;
	require(flatfile_shopkeeper_establish(root.string(), { conflicting, second }, &error) ==
			flatfile_shopkeeper_result::invalid,
		"conflicting shopkeeper establishment was accepted");

	const fs::path invalid_root = fs::path(argv[1]) / "invalid";
	prepare_root(invalid_root);
	auto duplicate_uid = second;
	duplicate_uid.items[0].object_uid = 100;
	require(flatfile_shopkeeper_establish(invalid_root.string(), { first, duplicate_uid },
					      &error) == flatfile_shopkeeper_result::invalid,
		"duplicate item UID across shopkeepers was accepted");
	auto nested_equipment = first;
	nested_equipment.items[1].equipment_slot = 4;
	require(flatfile_shopkeeper_establish(invalid_root.string(), { nested_equipment },
					      &error) == flatfile_shopkeeper_result::invalid,
		"nested shopkeeper equipment was accepted");
	auto duplicate_slot = first;
	duplicate_slot.items[2].equipment_slot = 3;
	require(flatfile_shopkeeper_establish(invalid_root.string(), { duplicate_slot }, &error) ==
			flatfile_shopkeeper_result::invalid,
		"duplicate shopkeeper equipment slot was accepted");

	const fs::path catalog = root / "domains/shopkeeper_catalog";
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open shopkeeper catalog for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_shopkeeper_list(root.string(), &records, &error) ==
			flatfile_shopkeeper_result::invalid,
		"corrupt shopkeeper authority was exposed");
	std::cout << "flat-file shopkeeper repository passed\n";
	return 0;
}
