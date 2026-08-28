#ifndef DURIS_FLATFILE_LOCKER_REPOSITORY_H
#define DURIS_FLATFILE_LOCKER_REPOSITORY_H

#include "flatfile_authority_transaction.h"
#include "item_transfer_command.h"
#include "player_snapshot.h"

#include <cstdint>
#include <string>
#include <vector>

struct flatfile_locker_chest_record
{
	uint32_t chest_id = 0;
	std::string chest_name;
	std::string password_hash;
	bool is_public = false;
	std::string sort_config;
	uint64_t revision = 0;
	std::vector<player_item_snapshot> items;
};

struct flatfile_locker_record
{
	uint32_t locker_id = 0;
	std::string locker_name;
	int32_t owner_pid = 0;
	int32_t owner_assoc_id = 0;
	int8_t racewar = 0;
	int8_t race = 0;
	uint64_t revision = 0;
	std::vector<flatfile_locker_chest_record> chests;
};

struct flatfile_locker_access_record
{
	std::string owner_name;
	std::string visitor_name;
	uint64_t revision = 0;
};

struct flatfile_locker_custody_item
{
	uint64_t item_uid = 0;
	int32_t vnum = 0;
};

struct flatfile_locker_custody_owner
{
	item_owner_identity owner = { item_owner_type::unknown, 0, 0 };
	std::vector<flatfile_locker_custody_item> items;
};

struct flatfile_locker_player_removal
{
	flatfile_authority_operation operation;
	std::vector<flatfile_locker_custody_owner> custody;
};

enum class flatfile_locker_result
{
	ok,
	not_found,
	already_exists,
	unchanged,
	conflict,
	invalid,
	io_error
};

flatfile_locker_result flatfile_locker_establish(
	const std::string &root, const std::vector<flatfile_locker_record> &lockers,
	const std::vector<flatfile_locker_access_record> &access, std::string *error);
flatfile_locker_result flatfile_locker_list(const std::string &root,
					    std::vector<flatfile_locker_record> *lockers,
					    std::vector<flatfile_locker_access_record> *access,
					    std::string *error);
flatfile_locker_result
flatfile_locker_prepare_player_remove(const std::string &root, const flatfile_authority_lock &lock,
				      uint32_t pid, const std::string &player_name,
				      flatfile_locker_player_removal *removal, std::string *error);

#endif
