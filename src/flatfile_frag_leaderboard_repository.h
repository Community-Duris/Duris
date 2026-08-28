#ifndef DURIS_FLATFILE_FRAG_LEADERBOARD_REPOSITORY_H
#define DURIS_FLATFILE_FRAG_LEADERBOARD_REPOSITORY_H

#include "flatfile_authority_transaction.h"

#include <cstdint>
#include <string>
#include <vector>

struct flatfile_frag_leaderboard_record
{
	uint32_t pid = 0;
	std::string account_name;
	std::string character_name;
	int64_t total_frags = 0;
	int32_t racewar = 0;
	std::string race_name;
	std::string class_name;
	int32_t level = 0;
	int64_t deleted_at = 0;
	int64_t last_updated = 0;
	uint64_t revision = 0;
	bool operator==(const flatfile_frag_leaderboard_record &) const = default;
};

enum class flatfile_frag_leaderboard_result
{
	ok,
	not_found,
	already_exists,
	unchanged,
	invalid,
	io_error
};

flatfile_frag_leaderboard_result
flatfile_frag_leaderboard_establish(const std::string &root,
				    const std::vector<flatfile_frag_leaderboard_record> &records,
				    std::string *error);
flatfile_frag_leaderboard_result
flatfile_frag_leaderboard_list(const std::string &root,
			       std::vector<flatfile_frag_leaderboard_record> *records,
			       std::string *error);
flatfile_frag_leaderboard_result
flatfile_frag_leaderboard_upsert(const std::string &root,
				 const flatfile_frag_leaderboard_record &record,
				 std::string *error);
flatfile_frag_leaderboard_result flatfile_frag_leaderboard_prepare_tombstone(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	int64_t deleted_at, flatfile_authority_operation *operation, std::string *error);

#endif
