#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_player_snapshot_file.h"
#include "flatfile/flatfile_store.h"
#include "player/player_snapshot_codec.h"

#include <filesystem>
#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <tuple>

// Read existing codecs under the authority lock. No recovery replay or write.
int main(int argc, char **argv)
{
	if (argc != 4)
		return 2;
	const std::string root = argv[1];
	const int pid = std::stoi(argv[2]);
	const uint64_t revision = std::stoull(argv[3]);
	flatfile_authority_lock lock;
	std::string error;
	if (!lock.acquire(root, &error) ||
	    std::filesystem::exists(root + "/domains/.critical-authority-transaction"))
		return 3;
	player_snapshot death;
	using namespace flatfile_player_snapshot_file;
	if (flatfile_player_snapshot_read_file(death_directory(root), death_filename(pid, revision),
					       pid, &death,
					       &error) != flatfile_player_load_result::ok ||
	    !death.death || death.revision != revision)
		return 4;
	std::vector<flatfile_item_ownership_record> items;
	if (flatfile_item_repository_recovery_catalog_locked(root, lock, &items, &error) !=
	    flatfile_item_repository_result::ok)
		return 5;
	std::set<uint64_t> uids, roots;
	for (const auto &row : death.death->custody)
	{
		uids.insert(row.item.item_uid);
		roots.insert(row.item.root_item_uid);
	}
	for (size_t i = 1; i < death.death->corpse.size(); ++i)
		uids.insert(death.death->corpse[i].object_uid);
	std::erase_if(items,
		      [&](const auto &item)
		      {
			      return !uids.contains(item.item_uid) &&
				     !(roots.contains(item.root_item_uid) &&
				       item.owner.type == item_owner_type::player &&
				       item.owner.id == static_cast<uint64_t>(pid) &&
				       !item.owner.context_id);
		      });
	using physical_key = std::tuple<unsigned, uint64_t, uint64_t, uint64_t, int32_t>;
	std::map<physical_key, size_t> physical;
	std::set<uint64_t> players;
	bool need_world = false, need_lockers = false;
	for (const auto &item : items)
	{
		if (item.owner.type == item_owner_type::player)
			players.insert(item.owner.id);
		need_world |= item.owner.type == item_owner_type::corpse ||
			      item.owner.type == item_owner_type::room;
		need_lockers |= item.owner.type == item_owner_type::locker;
	}
	// Match physical projections without calling normal list APIs, which may
	// recover and mutate authority. All reads share this one authority lock.
	auto index = [&](const item_owner_identity &owner, const auto &rows)
	{
		for (const auto &row : rows)
			++physical[{ static_cast<unsigned>(owner.type), owner.id, owner.context_id,
				     row.object_uid, row.vnum }];
	};
	for (uint64_t player_pid : players)
	{
		if (!player_pid || player_pid > INT32_MAX)
			return 6;
		player_snapshot player;
		const auto loaded = flatfile_player_snapshot_read(
			root, static_cast<int32_t>(player_pid), &player, &error);
		if (loaded != flatfile_player_load_result::ok &&
		    loaded != flatfile_player_load_result::not_found)
			return 6;
		index({ item_owner_type::player, player_pid, 0 }, player.items);
	}
	if (need_world)
	{
		std::vector<flatfile_corpse_record> corpses;
		std::vector<flatfile_room_item_record> rooms;
		const auto loaded = flatfile_world_item_recovery_list_locked(root, lock, &corpses,
									     &rooms, &error);
		if (loaded != flatfile_world_item_result::ok &&
		    loaded != flatfile_world_item_result::not_found)
			return 6;
		for (const auto &corpse : corpses)
			index({ item_owner_type::corpse,
				(uint64_t{ corpse.owner_pid } << 32) | corpse.save_id, 0 },
			      corpse.items);
		for (const auto &room : rooms)
			index({ item_owner_type::room, static_cast<uint64_t>(room.room_vnum), 0 },
			      room.items);
	}
	if (need_lockers)
	{
		std::vector<flatfile_locker_record> lockers;
		const auto loaded =
			flatfile_locker_recovery_list_locked(root, lock, &lockers, &error);
		if (loaded != flatfile_locker_result::ok &&
		    loaded != flatfile_locker_result::not_found)
			return 6;
		for (const auto &locker : lockers)
			for (const auto &chest : locker.chests)
				index({ item_owner_type::locker, locker.locker_id, chest.chest_id },
				      chest.items);
	}
	std::vector<uint8_t> payload;
	if (player_snapshot_encode(death, &payload) != player_snapshot_codec_result::ok)
		return 7;
	constexpr char digits[] = "0123456789abcdef";
	std::cout << "{\"payload_hex\":\"";
	for (uint8_t byte : payload)
		std::cout << digits[byte >> 4] << digits[byte & 15];
	std::cout << "\",\"owners\":[";
	bool first = true;
	for (const auto &item : items)
	{
		if (!first)
			std::cout << ',';
		first = false;
		const bool materialized =
			!item.coin_payload.empty() ||
			physical[{ static_cast<unsigned>(item.owner.type), item.owner.id,
				   item.owner.context_id, item.item_uid, item.vnum }] == 1;
		std::cout << "{\"item_uid\":" << item.item_uid
			  << ",\"root_item_uid\":" << item.root_item_uid
			  << ",\"parent_item_uid\":" << item.parent_item_uid
			  << ",\"owner_type\":" << static_cast<unsigned>(item.owner.type)
			  << ",\"owner_id\":" << item.owner.id
			  << ",\"owner_context_id\":" << item.owner.context_id
			  << ",\"item_revision\":" << item.item_revision
			  << ",\"vnum\":" << item.vnum
			  << ",\"state\":" << static_cast<unsigned>(item.state)
			  << ",\"materialized\":" << (materialized ? "true" : "false") << '}';
	}
	std::cout << "]}" << std::endl;
}
