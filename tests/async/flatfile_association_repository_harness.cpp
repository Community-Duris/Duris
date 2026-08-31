#include "flatfile/flatfile_association_repository.h"

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

static flatfile_association_record guild()
{
	flatfile_association_record value = {};
	value.association_id = 7;
	value.name = "The Test Guild";
	value.racewar = 1;
	value.bits = 2;
	value.prestige = 100;
	value.construction = 200;
	value.platinum = 3;
	value.gold = 4;
	value.silver = 5;
	value.copper = 6;
	value.frags = 1500;
	value.top_frags = 1200;
	value.top_fragger = "Player";
	value.ranks[0] = "Member";
	value.ranks[7] = "Leader";
	value.revision = 9;
	value.members = { { 2, "Other", 3, 40, 0, 300, 2 }, { 1, "Player", 7, 50, 2, 1200, 4 } };
	return value;
}

static flatfile_guildhall_record guildhall()
{
	flatfile_guildhall_record value = {};
	value.guildhall_id = 12;
	value.association_id = 7;
	value.type = 1;
	value.outside_vnum = 588184;
	value.racewar = 2;
	flatfile_guildhall_room_record entrance = {};
	entrance.room_id = 42;
	entrance.vnum = 48020;
	entrance.name = "Test Entrance&n";
	entrance.type = 1;
	entrance.values[0] = 2;
	entrance.exits.fill(-1);
	entrance.exits[0] = 48021;
	flatfile_guildhall_room_record heartstone = {};
	heartstone.room_id = 41;
	heartstone.vnum = 48021;
	heartstone.type = 2;
	heartstone.values[1] = 3;
	heartstone.exits.fill(-1);
	heartstone.exits[2] = 48020;
	value.rooms = { entrance, heartstone };
	return value;
}

static std::vector<flatfile_outpost_record> outposts()
{
	std::vector<flatfile_outpost_record> records(3);
	for (int index = 0; index < 3; ++index)
		records[index].outpost_id = index;
	return records;
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = fs::path(argv[1]) / "association";
	prepare_root(root);
	std::string error;
	std::vector<flatfile_association_record> records;
	require(flatfile_association_list(root.string(), &records, &error) ==
			flatfile_association_result::not_found,
		"missing association authority did not fail closed");
	const auto source = guild();
	require(flatfile_association_establish(root.string(), { source }, &error) ==
			flatfile_association_result::ok,
		"association establishment failed: " + error);
	require(flatfile_association_establish(root.string(), { source }, &error) ==
			flatfile_association_result::already_exists,
		"association establishment retry was not idempotent");
	require(flatfile_association_save(root.string(), source, &error) ==
			flatfile_association_result::unchanged,
		"unchanged association save advanced authority");
	require(flatfile_association_list(root.string(), &records, &error) ==
				flatfile_association_result::ok &&
			records.size() == 1 && records[0].members.size() == 2 &&
			records[0].members[0].pid == 1 && records[0].members[0].name == "player" &&
			records[0].members[0].contributed_frags == 1200 &&
			records[0].members[1].pid == 2 && records[0].top_fragger == "player" &&
			records[0].ranks[7] == "Leader" && records[0].platinum == 3,
		"association catalog was not canonical or did not round trip");
	auto conflicting = source;
	conflicting.frags++;
	require(flatfile_association_establish(root.string(), { conflicting }, &error) ==
			flatfile_association_result::invalid,
		"conflicting association establishment was accepted");

	const fs::path invalid_root = fs::path(argv[1]) / "invalid";
	prepare_root(invalid_root);
	auto duplicate = source;
	duplicate.association_id = 8;
	duplicate.members = { source.members[0] };
	require(flatfile_association_establish(invalid_root.string(), { source, duplicate },
					       &error) == flatfile_association_result::invalid,
		"duplicate cross-association member PID was accepted");
	auto bad_top = source;
	bad_top.top_fragger.clear();
	require(flatfile_association_establish(invalid_root.string(), { bad_top }, &error) ==
			flatfile_association_result::invalid,
		"inconsistent top-fragger aggregate was accepted");

	auto updated = source;
	updated.prestige = 101;
	updated.members[1].debt = 51;
	require(flatfile_association_save(root.string(), updated, &error) ==
			flatfile_association_result::ok,
		"association update failed: " + error);
	require(flatfile_association_list(root.string(), &records, &error) ==
				flatfile_association_result::ok &&
			records.size() == 1 && records[0].prestige == 101 &&
			records[0].revision == 10 && records[0].members[0].debt == 51 &&
			records[0].members[0].revision == 5 && records[0].members[1].revision == 2,
		"association update did not preserve and advance revisions exactly");
	auto duplicate_save = source;
	duplicate_save.association_id = 8;
	duplicate_save.name = "Duplicate Member Guild";
	require(flatfile_association_save(root.string(), duplicate_save, &error) ==
			flatfile_association_result::invalid,
		"association save accepted a member PID owned by another guild");
	auto second = source;
	second.association_id = 8;
	second.name = "Second Guild";
	second.frags = 0;
	second.top_frags = 0;
	second.top_fragger.clear();
	second.members.clear();
	require(flatfile_association_save(root.string(), second, &error) ==
			flatfile_association_result::ok,
		"new association save failed: " + error);
	require(flatfile_association_erase(root.string(), 8, &error) ==
			flatfile_association_result::ok,
		"association erase failed: " + error);
	require(flatfile_association_erase(root.string(), 8, &error) ==
			flatfile_association_result::unchanged,
		"association erase retry was not idempotent");

	std::vector<std::string> messages;
	require(flatfile_association_ledger_list(root.string(), 7, false, &messages, &error) ==
			flatfile_association_result::not_found,
		"missing association ledger did not report not found");
	require(flatfile_association_ledger_append(root.string(), 7, false, "Player first",
						   &error) == flatfile_association_result::ok &&
			flatfile_association_ledger_append(root.string(), 7, true,
							   "System withdrew coins", &error) ==
				flatfile_association_result::ok &&
			flatfile_association_ledger_append(root.string(), 7, false, "Player second",
							   &error) ==
				flatfile_association_result::ok,
		"association ledger append failed: " + error);
	require(flatfile_association_ledger_list(root.string(), 7, false, &messages, &error) ==
				flatfile_association_result::ok &&
			messages == std::vector<std::string>{ "Player second", "Player first" },
		"player association ledger did not list newest first");
	require(flatfile_association_ledger_list(root.string(), 7, true, &messages, &error) ==
				flatfile_association_result::ok &&
			messages == std::vector<std::string>{ "System withdrew coins" },
		"system association ledger was not filtered independently");
	for (int index = 0; index <= 100; ++index)
		require(flatfile_association_ledger_append(
				root.string(), 7, false, "Player " + std::to_string(index),
				&error) == flatfile_association_result::ok,
			"association ledger retention append failed: " + error);
	require(flatfile_association_ledger_list(root.string(), 7, false, &messages, &error) ==
				flatfile_association_result::ok &&
			messages.size() == 100 && messages.front() == "Player 100" &&
			messages.back() == "Player 1",
		"association ledger did not retain the newest 100 player entries");
	require(flatfile_association_ledger_list(root.string(), 7, true, &messages, &error) ==
				flatfile_association_result::ok &&
			messages == std::vector<std::string>{ "System withdrew coins" },
		"player ledger retention removed system history");
	const fs::path ledger = root / "domains/association_ledger_7";
	const auto ledger_permissions = fs::status(ledger).permissions();
	require((ledger_permissions & (fs::perms::group_all | fs::perms::others_all)) ==
			fs::perms::none,
		"association ledger permissions are not private");
	require(flatfile_association_ledger_append(root.string(), 7, false, "bad\nentry", &error) ==
			flatfile_association_result::invalid,
		"association ledger accepted a control character");

	std::vector<flatfile_alliance_record> alliance_records;
	require(flatfile_alliance_list(root.string(), &alliance_records, &error) ==
			flatfile_association_result::not_found,
		"missing alliance authority did not report not found");
	const std::vector<flatfile_alliance_record> alliance_source = { { 9, 10, 20 },
									{ 7, 8, -5 } };
	require(flatfile_alliance_replace(root.string(), alliance_source, &error) ==
			flatfile_association_result::ok,
		"alliance authority creation failed: " + error);
	require(flatfile_alliance_list(root.string(), &alliance_records, &error) ==
				flatfile_association_result::ok &&
			alliance_records.size() == 2 &&
			alliance_records[0].forging_association_id == 7 &&
			alliance_records[0].joining_association_id == 8 &&
			alliance_records[0].tribute_owed == -5 &&
			alliance_records[1].forging_association_id == 9,
		"alliance authority was not canonical or did not round trip");
	require(flatfile_alliance_replace(root.string(), alliance_source, &error) ==
			flatfile_association_result::unchanged,
		"unchanged alliance rewrite advanced authority");
	require(flatfile_alliance_replace(root.string(), { { 7, 8, 0 }, { 7, 9, 0 } }, &error) ==
			flatfile_association_result::invalid,
		"alliance authority accepted a guild in two alliances");
	require(flatfile_alliance_replace(root.string(), { { 7, 7, 0 } }, &error) ==
			flatfile_association_result::invalid,
		"alliance authority accepted a self-alliance");
	require(flatfile_alliance_replace(root.string(), {}, &error) ==
				flatfile_association_result::ok &&
			flatfile_alliance_list(root.string(), &alliance_records, &error) ==
				flatfile_association_result::ok &&
			alliance_records.empty(),
		"alliance authority did not persist an empty replacement");
	require(flatfile_alliance_replace(root.string(), alliance_source, &error) ==
			flatfile_association_result::ok,
		"alliance authority could not be restored for corruption test");

	std::vector<flatfile_guildhall_record> guildhall_records;
	require(flatfile_guildhall_list(root.string(), &guildhall_records, &error) ==
			flatfile_association_result::not_found,
		"missing guildhall authority did not report not found");
	const auto guildhall_source = guildhall();
	require(flatfile_guildhall_save(root.string(), guildhall_source, &error) ==
			flatfile_association_result::ok,
		"guildhall authority creation failed: " + error);
	require(flatfile_guildhall_list(root.string(), &guildhall_records, &error) ==
				flatfile_association_result::ok &&
			guildhall_records.size() == 1 && guildhall_records[0].guildhall_id == 12 &&
			guildhall_records[0].association_id == 7 &&
			guildhall_records[0].outside_vnum == 588184 &&
			guildhall_records[0].rooms.size() == 2 &&
			guildhall_records[0].rooms[0].room_id == 41 &&
			guildhall_records[0].rooms[0].values[1] == 3 &&
			guildhall_records[0].rooms[1].name == "Test Entrance&n" &&
			guildhall_records[0].rooms[1].exits[0] == 48021,
		"guildhall authority was not canonical or did not round trip");
	require(flatfile_guildhall_save(root.string(), guildhall_source, &error) ==
			flatfile_association_result::unchanged,
		"unchanged guildhall save advanced authority");
	const fs::path guildhall_file = root / "domains/association_guildhalls";
	const auto guildhall_permissions = fs::status(guildhall_file).permissions();
	require((guildhall_permissions & (fs::perms::group_all | fs::perms::others_all)) ==
			fs::perms::none,
		"guildhall authority permissions are not private");
	auto changed_guildhall = guildhall_source;
	changed_guildhall.outside_vnum++;
	changed_guildhall.rooms[0].name = "Renamed Entrance&n";
	require(flatfile_guildhall_save(root.string(), changed_guildhall, &error) ==
			flatfile_association_result::ok,
		"guildhall authority update failed: " + error);
	auto duplicate_room = guildhall_source;
	duplicate_room.rooms[1].room_id = duplicate_room.rooms[0].room_id;
	require(flatfile_guildhall_save(root.string(), duplicate_room, &error) ==
			flatfile_association_result::invalid,
		"guildhall authority accepted duplicate room IDs");
	auto duplicate_vnum = guildhall_source;
	duplicate_vnum.rooms[1].vnum = duplicate_vnum.rooms[0].vnum;
	require(flatfile_guildhall_save(root.string(), duplicate_vnum, &error) ==
			flatfile_association_result::invalid,
		"guildhall authority accepted duplicate room vnums");
	auto second_guildhall = guildhall_source;
	second_guildhall.guildhall_id = 13;
	second_guildhall.association_id = 8;
	second_guildhall.rooms.resize(1);
	require(flatfile_guildhall_save(root.string(), second_guildhall, &error) ==
			flatfile_association_result::invalid,
		"guildhall authority accepted a room ID and vnum owned by another hall");
	auto invalid_name = guildhall_source;
	invalid_name.rooms[0].name = "forged\nroom";
	require(flatfile_guildhall_save(root.string(), invalid_name, &error) ==
			flatfile_association_result::invalid,
		"guildhall authority accepted a control character in a room name");
	auto too_many_rooms = guildhall_source;
	too_many_rooms.rooms.clear();
	for (int index = 0; index < 51; ++index)
	{
		auto room = guildhall_source.rooms[0];
		room.room_id = 100 + index;
		room.vnum = 48100 + index;
		too_many_rooms.rooms.push_back(room);
	}
	require(flatfile_guildhall_save(root.string(), too_many_rooms, &error) ==
			flatfile_association_result::invalid,
		"guildhall authority accepted more than 50 rooms");
	require(flatfile_guildhall_room_erase(root.string(), 12, 42, &error) ==
				flatfile_association_result::ok &&
			flatfile_guildhall_room_erase(root.string(), 12, 42, &error) ==
				flatfile_association_result::unchanged &&
			flatfile_guildhall_list(root.string(), &guildhall_records, &error) ==
				flatfile_association_result::ok &&
			guildhall_records[0].rooms.size() == 1,
		"guildhall room erase was not durable and idempotent");
	require(flatfile_guildhall_save(root.string(), guildhall_source, &error) ==
				flatfile_association_result::ok &&
			flatfile_guildhall_erase(root.string(), 12, &error) ==
				flatfile_association_result::ok &&
			flatfile_guildhall_erase(root.string(), 12, &error) ==
				flatfile_association_result::unchanged &&
			flatfile_guildhall_list(root.string(), &guildhall_records, &error) ==
				flatfile_association_result::ok &&
			guildhall_records.empty(),
		"guildhall erase was not complete and idempotent");
	require(flatfile_guildhall_save(root.string(), guildhall_source, &error) ==
			flatfile_association_result::ok,
		"guildhall authority could not be restored for corruption test");

	std::vector<flatfile_outpost_record> outpost_records;
	require(flatfile_outpost_list(root.string(), &outpost_records, &error) ==
			flatfile_association_result::not_found,
		"missing outpost authority did not report not found");
	auto outpost_source = outposts();
	require(flatfile_outpost_establish(root.string(), { outpost_source[0], outpost_source[1] },
					   &error) == flatfile_association_result::invalid,
		"outpost authority accepted an incomplete fixed record set");
	require(flatfile_outpost_establish(
			root.string(), { outpost_source[2], outpost_source[0], outpost_source[1] },
			&error) == flatfile_association_result::ok,
		"outpost authority creation failed: " + error);
	require(flatfile_outpost_establish(root.string(), outpost_source, &error) ==
			flatfile_association_result::already_exists,
		"outpost authority establishment retry was not idempotent");
	require(flatfile_outpost_list(root.string(), &outpost_records, &error) ==
				flatfile_association_result::ok &&
			outpost_records.size() == 3 && outpost_records[0].outpost_id == 0 &&
			outpost_records[0].level == 1 &&
			outpost_records[0].applied_resources == 100000 &&
			outpost_records[2].outpost_id == 2,
		"outpost authority was not canonical or did not retain schema defaults");
	const fs::path outpost_file = root / "domains/association_outposts";
	const auto outpost_permissions = fs::status(outpost_file).permissions();
	require((outpost_permissions & (fs::perms::group_all | fs::perms::others_all)) ==
			fs::perms::none,
		"outpost authority permissions are not private");
	auto changed_outpost = outpost_source[1];
	changed_outpost.owner_association_id = 7;
	changed_outpost.level = 8;
	changed_outpost.walls = 1;
	changed_outpost.archers = 1;
	changed_outpost.resources = 11;
	changed_outpost.applied_resources = 12;
	changed_outpost.hitpoints = 250000;
	changed_outpost.territory = 13;
	changed_outpost.portal_room = 1;
	changed_outpost.golems = 4;
	changed_outpost.meurtriere = 1;
	changed_outpost.scouts = 14;
	require(flatfile_outpost_save(root.string(), changed_outpost, &error) ==
				flatfile_association_result::ok &&
			flatfile_outpost_save(root.string(), changed_outpost, &error) ==
				flatfile_association_result::unchanged &&
			flatfile_outpost_list(root.string(), &outpost_records, &error) ==
				flatfile_association_result::ok &&
			outpost_records[1].owner_association_id == 7 &&
			outpost_records[1].level == 8 && outpost_records[1].walls == 1 &&
			outpost_records[1].archers == 1 && outpost_records[1].resources == 11 &&
			outpost_records[1].applied_resources == 12 &&
			outpost_records[1].hitpoints == 250000 &&
			outpost_records[1].territory == 13 && outpost_records[1].portal_room == 1 &&
			outpost_records[1].golems == 4 && outpost_records[1].meurtriere == 1 &&
			outpost_records[1].scouts == 14,
		"outpost mutation did not preserve every table field or idempotence");
	auto invalid_outpost = changed_outpost;
	invalid_outpost.archers = 2;
	require(flatfile_outpost_save(root.string(), invalid_outpost, &error) ==
			flatfile_association_result::invalid,
		"outpost authority accepted an invalid boolean field");
	invalid_outpost = changed_outpost;
	invalid_outpost.hitpoints = -1;
	require(flatfile_outpost_save(root.string(), invalid_outpost, &error) ==
			flatfile_association_result::invalid,
		"outpost authority accepted negative hitpoints");

	flatfile_authority_operation operation;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire association authority");
		require(flatfile_association_prepare_player_remove(root.string(), lock, 1, "pLaYeR",
								   &operation, &error) ==
					flatfile_association_result::ok &&
				operation.filename == "association_catalog",
			"association member removal was not prepared: " + error);
	}
	require(flatfile_association_list(root.string(), &records, &error) ==
				flatfile_association_result::ok &&
			records[0].members.size() == 2,
		"prepared association removal published before commit");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not reacquire association authority");
		require(flatfile_authority_transaction_commit_operations(root.string(), lock,
									 { operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"association removal transaction failed: " + error);
	}
	require(flatfile_association_list(root.string(), &records, &error) ==
				flatfile_association_result::ok &&
			records[0].members.size() == 1 && records[0].members[0].pid == 2 &&
			records[0].frags == 300 && records[0].top_frags == 0 &&
			records[0].top_fragger.empty() && records[0].revision == 11,
		"association removal did not update membership and frag aggregates");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire association authority for retry");
		require(flatfile_association_prepare_player_remove(root.string(), lock, 1, "player",
								   &operation, &error) ==
				flatfile_association_result::unchanged,
			"association removal retry was not idempotent");
	}

	{
		std::fstream file(ledger, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open association ledger for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x33;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_association_ledger_list(root.string(), 7, false, &messages, &error) ==
			flatfile_association_result::invalid,
		"corrupt association ledger was exposed");
	require(flatfile_association_ledger_append(root.string(), 7, false, "overwrite", &error) ==
			flatfile_association_result::invalid,
		"association ledger append overwrote corrupt history");
	const fs::path alliance_file = root / "domains/association_alliances";
	{
		std::fstream file(alliance_file, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open alliance authority for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x71;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_alliance_list(root.string(), &alliance_records, &error) ==
			flatfile_association_result::invalid,
		"corrupt alliance authority was exposed");
	require(flatfile_alliance_replace(root.string(), alliance_source, &error) ==
			flatfile_association_result::invalid,
		"alliance replacement overwrote corrupt authority");
	{
		std::fstream file(guildhall_file, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open guildhall authority for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x27;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_guildhall_list(root.string(), &guildhall_records, &error) ==
			flatfile_association_result::invalid,
		"corrupt guildhall authority was exposed");
	require(flatfile_guildhall_save(root.string(), guildhall_source, &error) ==
			flatfile_association_result::invalid,
		"guildhall save overwrote corrupt authority");
	require(flatfile_guildhall_erase(root.string(), 12, &error) ==
			flatfile_association_result::invalid,
		"guildhall erase overwrote corrupt authority");
	{
		std::fstream file(outpost_file, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open outpost authority for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x19;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_outpost_list(root.string(), &outpost_records, &error) ==
			flatfile_association_result::invalid,
		"corrupt outpost authority was exposed");
	require(flatfile_outpost_save(root.string(), changed_outpost, &error) ==
			flatfile_association_result::invalid,
		"outpost save overwrote corrupt authority");
	require(flatfile_outpost_establish(root.string(), outpost_source, &error) ==
			flatfile_association_result::invalid,
		"outpost establishment overwrote corrupt authority");

	const fs::path catalog = root / "domains/association_catalog";
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open association catalog for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_association_list(root.string(), &records, &error) ==
			flatfile_association_result::invalid,
		"corrupt association authority was exposed");
	require(flatfile_association_save(root.string(), source, &error) ==
			flatfile_association_result::invalid,
		"association save overwrote corrupt authority");
	require(flatfile_association_erase(root.string(), 7, &error) ==
			flatfile_association_result::invalid,
		"association erase overwrote corrupt authority");
	std::cout << "flat-file association repository passed\n";
	return 0;
}
