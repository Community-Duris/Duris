#ifndef DURIS_FLATFILE_BOON_REPOSITORY_H
#define DURIS_FLATFILE_BOON_REPOSITORY_H

#include "boon_reward_command.h"
#include "critical_command_coordinator.h"

#include <cstdint>
#include <string>
#include <vector>

struct flatfile_boon_definition
{
	uint32_t id = 0;
	int64_t start_time = 0;
	int64_t duration = 0;
	uint8_t racewar = 0;
	uint8_t type = 0;
	uint8_t option = 0;
	double criteria = 0;
	double criteria2 = 0;
	double bonus = 0;
	double bonus2 = 0;
	bool random = false;
	bool active = false;
	uint32_t target_pid = 0;
	bool repeat = false;
	std::string author;
};

struct flatfile_boon_player_projection
{
	int64_t points = 0;
	int64_t stats = 0;
};

enum class flatfile_boon_result
{
	ok,
	not_found,
	invalid,
	io_error,
	already_exists,
};

flatfile_boon_result
flatfile_boon_establish(const std::string &root,
			const std::vector<flatfile_boon_definition> &definitions,
			std::string *error);
flatfile_boon_result flatfile_boon_load_player(const std::string &root, uint32_t pid,
					       flatfile_boon_player_projection *player,
					       std::string *error);
critical_apply_result flatfile_boon_repository_apply(const std::string &root,
						     const critical_command &command);

#endif
