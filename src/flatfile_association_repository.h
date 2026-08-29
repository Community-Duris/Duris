#ifndef DURIS_FLATFILE_ASSOCIATION_REPOSITORY_H
#define DURIS_FLATFILE_ASSOCIATION_REPOSITORY_H

#include "flatfile_authority_transaction.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

constexpr size_t FLATFILE_ASSOCIATION_RANK_COUNT = 8;

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
flatfile_association_result flatfile_association_prepare_player_remove(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	const std::string &expected_name, flatfile_authority_operation *operation,
	std::string *error);

#endif
