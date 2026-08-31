#ifndef DURIS_FLATFILE_ASSOCIATION_REPOSITORY_H
#define DURIS_FLATFILE_ASSOCIATION_REPOSITORY_H

#include "flatfile/flatfile_authority_transaction.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

constexpr size_t FLATFILE_ASSOCIATION_RANK_COUNT = 8;
constexpr size_t FLATFILE_GUILDHALL_VALUE_COUNT = 8;
constexpr size_t FLATFILE_GUILDHALL_EXIT_COUNT = 10;

struct flatfile_association_member_record
{
	uint32_t pid = 0;
	std::string name;
	uint32_t bits = 0;
	uint32_t debt = 0;
	uint16_t online_status = 0;
	int64_t contributed_frags = 0;
	uint64_t revision = 0;
};

struct flatfile_association_record
{
	uint32_t association_id = 0;
	std::string name;
	uint32_t racewar = 0;
	uint32_t bits = 0;
	uint64_t prestige = 0;
	uint64_t construction = 0;
	uint32_t platinum = 0;
	uint32_t gold = 0;
	uint32_t silver = 0;
	uint32_t copper = 0;
	int64_t frags = 0;
	int64_t top_frags = 0;
	std::string top_fragger;
	std::array<std::string, FLATFILE_ASSOCIATION_RANK_COUNT> ranks;
	uint64_t revision = 0;
	std::vector<flatfile_association_member_record> members;
};

struct flatfile_alliance_record
{
	uint32_t forging_association_id = 0;
	uint32_t joining_association_id = 0;
	int32_t tribute_owed = 0;
};

struct flatfile_guildhall_room_record
{
	int32_t room_id = 0;
	int32_t vnum = 0;
	std::string name;
	int32_t type = 0;
	std::array<uint32_t, FLATFILE_GUILDHALL_VALUE_COUNT> values = {};
	std::array<int32_t, FLATFILE_GUILDHALL_EXIT_COUNT> exits = {};
};

struct flatfile_guildhall_record
{
	int32_t guildhall_id = 0;
	uint32_t association_id = 0;
	int32_t type = 0;
	int32_t outside_vnum = 0;
	int32_t racewar = 0;
	std::vector<flatfile_guildhall_room_record> rooms;
};

struct flatfile_outpost_record
{
	int32_t outpost_id = 0;
	uint32_t owner_association_id = 0;
	int32_t level = 1;
	int32_t walls = 0;
	int32_t archers = 0;
	int32_t resources = 0;
	int32_t applied_resources = 100000;
	int32_t hitpoints = 0;
	int32_t territory = 0;
	int32_t portal_room = 0;
	int32_t golems = 0;
	int32_t meurtriere = 0;
	int32_t scouts = 0;
};

enum class flatfile_association_result
{
	ok,
	not_found,
	already_exists,
	unchanged,
	conflict,
	invalid,
	io_error
};

flatfile_association_result
flatfile_association_establish(const std::string &root,
			       const std::vector<flatfile_association_record> &associations,
			       std::string *error);
flatfile_association_result
flatfile_association_list(const std::string &root,
			  std::vector<flatfile_association_record> *associations,
			  std::string *error);
flatfile_association_result
flatfile_association_save(const std::string &root, const flatfile_association_record &association,
			  std::string *error);
flatfile_association_result flatfile_association_erase(const std::string &root,
						       uint32_t association_id, std::string *error);
flatfile_association_result flatfile_association_ledger_append(const std::string &root,
							       uint32_t association_id,
							       bool system_entry,
							       const std::string &message,
							       std::string *error);
flatfile_association_result flatfile_association_ledger_list(const std::string &root,
							     uint32_t association_id,
							     bool system_entries,
							     std::vector<std::string> *messages,
							     std::string *error);
flatfile_association_result flatfile_alliance_list(const std::string &root,
						   std::vector<flatfile_alliance_record> *alliances,
						   std::string *error);
flatfile_association_result
flatfile_alliance_replace(const std::string &root,
			  const std::vector<flatfile_alliance_record> &alliances,
			  std::string *error);
flatfile_association_result
flatfile_guildhall_list(const std::string &root, std::vector<flatfile_guildhall_record> *guildhalls,
			std::string *error);
flatfile_association_result flatfile_guildhall_save(const std::string &root,
						    const flatfile_guildhall_record &guildhall,
						    std::string *error);
flatfile_association_result flatfile_guildhall_erase(const std::string &root, int32_t guildhall_id,
						     std::string *error);
flatfile_association_result flatfile_guildhall_room_erase(const std::string &root,
							  int32_t guildhall_id, int32_t room_id,
							  std::string *error);
flatfile_association_result
flatfile_outpost_establish(const std::string &root,
			   const std::vector<flatfile_outpost_record> &outposts,
			   std::string *error);
flatfile_association_result flatfile_outpost_list(const std::string &root,
						  std::vector<flatfile_outpost_record> *outposts,
						  std::string *error);
flatfile_association_result flatfile_outpost_save(const std::string &root,
						  const flatfile_outpost_record &outpost,
						  std::string *error);
flatfile_association_result flatfile_association_prepare_player_remove(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	const std::string &expected_name, flatfile_authority_operation *operation,
	std::string *error);

#endif
