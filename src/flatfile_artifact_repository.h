#ifndef DURIS_FLATFILE_ARTIFACT_REPOSITORY_H
#define DURIS_FLATFILE_ARTIFACT_REPOSITORY_H

#include "flatfile_authority_transaction.h"

#include <cstdint>
#include <string>
#include <vector>

constexpr int32_t FLATFILE_ARTIFACT_NOT_IN_GAME = 1;
constexpr int32_t FLATFILE_ARTIFACT_ON_NPC = 2;
constexpr int32_t FLATFILE_ARTIFACT_ON_PLAYER = 3;
constexpr int32_t FLATFILE_ARTIFACT_ON_GROUND = 4;
constexpr int32_t FLATFILE_ARTIFACT_ON_CORPSE = 5;

struct flatfile_artifact_record
{
	int32_t vnum = 0;
	bool owned = false;
	int32_t location_type = FLATFILE_ARTIFACT_NOT_IN_GAME;
	int32_t location = 0;
	int64_t timer = 0;
	int32_t type = 0;
	int64_t last_update = 0;
	int32_t bind_owner_pid = -1;
	int64_t bind_timer = 0;
	uint64_t revision = 0;
	bool operator==(const flatfile_artifact_record &) const = default;
};

enum class flatfile_artifact_result
{
	ok,
	not_found,
	already_exists,
	unchanged,
	conflict,
	invalid,
	io_error
};

flatfile_artifact_result
flatfile_artifact_establish(const std::string &root,
			    const std::vector<flatfile_artifact_record> &records,
			    std::string *error);
flatfile_artifact_result flatfile_artifact_list(const std::string &root,
						std::vector<flatfile_artifact_record> *records,
						std::string *error);
flatfile_artifact_result flatfile_artifact_prepare_player_release(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	flatfile_authority_operation *operation, std::string *error);

#endif
