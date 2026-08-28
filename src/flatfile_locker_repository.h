#ifndef DURIS_FLATFILE_LOCKER_REPOSITORY_H
#define DURIS_FLATFILE_LOCKER_REPOSITORY_H

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

enum class flatfile_locker_result
{
	ok,
	not_found,
	already_exists,
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

#endif
