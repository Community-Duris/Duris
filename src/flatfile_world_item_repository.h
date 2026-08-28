#ifndef DURIS_FLATFILE_WORLD_ITEM_REPOSITORY_H
#define DURIS_FLATFILE_WORLD_ITEM_REPOSITORY_H

#include "player_snapshot.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct flatfile_corpse_record
{
	uint32_t owner_pid = 0;
	std::string owner_name;
	uint32_t save_id = 0;
	int32_t room_vnum = 0;
	std::string short_description;
	std::string description;
	std::string keywords;
	int32_t weight = 0;
	std::array<int32_t, 8> values = {};
	uint64_t revision = 0;
	std::vector<player_item_snapshot> items;
};

struct flatfile_saved_world_item_record
{
	std::string item_key;
	int32_t room_vnum = 0;
	uint64_t revision = 0;
	std::vector<player_item_snapshot> items;
};

enum class flatfile_world_item_result
{
	ok,
	not_found,
	already_exists,
	invalid,
	io_error
};

flatfile_world_item_result flatfile_world_item_establish(
	const std::string &root, const std::vector<flatfile_corpse_record> &corpses,
	const std::vector<flatfile_saved_world_item_record> &saved_items, std::string *error);
flatfile_world_item_result
flatfile_world_item_list(const std::string &root, std::vector<flatfile_corpse_record> *corpses,
			 std::vector<flatfile_saved_world_item_record> *saved_items,
			 std::string *error);

#endif
