#ifndef DURIS_FLATFILE_SHIP_REPOSITORY_H
#define DURIS_FLATFILE_SHIP_REPOSITORY_H

#include "flatfile/flatfile_authority_transaction.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct flatfile_ship_slot_record
{
	uint8_t slot_index = 0;
	int32_t slot_type = 0;
	int32_t item_index = 0;
	int32_t position = 0;
	int32_t timer = 0;
	std::array<int32_t, 5> values = {};
};

struct flatfile_ship_crew_record
{
	int32_t crew_index = 0;
	int32_t sail_skill_milli = 0;
	int32_t guns_skill_milli = 0;
	int32_t repair_skill_milli = 0;
	int32_t sail_chief = 0;
	int32_t guns_chief = 0;
	int32_t repair_chief = 0;
};

struct flatfile_ship_record
{
	uint32_t ship_id = 0;
	uint32_t owner_pid = 0;
	std::string owner_name;
	std::string ship_name;
	uint8_t ship_class = 0;
	int32_t frags = 0;
	int32_t anchor_room = 0;
	int32_t time_played = 0;
	int32_t mainsail = 0;
	int8_t race = 0;
	int32_t money = 0;
	uint64_t flags = 0;
	std::array<int32_t, 4> armor = {};
	std::array<int32_t, 4> internal = {};
	flatfile_ship_crew_record crew;
	std::vector<flatfile_ship_slot_record> slots;
	uint64_t revision = 0;
};

enum class flatfile_ship_result
{
	ok,
	not_found,
	already_exists,
	unchanged,
	conflict,
	invalid,
	io_error
};

using flatfile_ship_owner_pid_resolver = bool (*)(const char *owner_name, uint32_t *pid,
						  std::string *error);

flatfile_ship_result flatfile_ship_establish(const std::string &root,
					     const std::vector<flatfile_ship_record> &ships,
					     std::string *error);
flatfile_ship_result flatfile_ship_list(const std::string &root,
					std::vector<flatfile_ship_record> *ships,
					std::string *error);
flatfile_ship_result flatfile_ship_upsert(const std::string &root, flatfile_ship_record *ship,
					  std::string *error);
flatfile_ship_result flatfile_ship_remove(const std::string &root, uint32_t ship_id,
					  const std::string &expected_owner, std::string *error);
flatfile_ship_result flatfile_ship_import_legacy(const std::string &root,
						 const std::string &legacy_directory,
						 flatfile_ship_owner_pid_resolver resolve_owner,
						 std::string *error);
flatfile_ship_result
flatfile_ship_prepare_player_remove(const std::string &root, const flatfile_authority_lock &lock,
				    uint32_t pid, const std::string &expected_name,
				    flatfile_authority_operation *operation, std::string *error);

#endif
