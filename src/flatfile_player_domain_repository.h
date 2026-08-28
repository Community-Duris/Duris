#ifndef DURIS_FLATFILE_PLAYER_DOMAIN_REPOSITORY_H
#define DURIS_FLATFILE_PLAYER_DOMAIN_REPOSITORY_H

#include "player_load_repository.h"
#include "critical_command_coordinator.h"

#include <cstdint>
#include <string>
#include <vector>

struct flatfile_player_domain_record
{
	int32_t pid = 0;
	std::string account_name;
	int8_t racewar = 0;
	player_load_domain_state domains = {};
	std::vector<int64_t> recent_pvp_deaths;
	std::vector<int32_t> completed_epic_zones;
};

enum class flatfile_player_domain_result
{
	ok,
	not_found,
	conflict,
	invalid,
	io_error
};

flatfile_player_domain_result
flatfile_player_domain_establish(const std::string &root,
				 const flatfile_player_domain_record &record, std::string *error);
flatfile_player_domain_result flatfile_player_domain_establish_initial_player(
	const std::string &root, const flatfile_player_domain_record &record, std::string *error);
flatfile_player_domain_result flatfile_player_domain_load(const std::string &root, int32_t pid,
							  const std::string &account_name,
							  int8_t racewar,
							  flatfile_player_domain_record *record,
							  std::string *error);
critical_apply_result flatfile_player_domain_apply(const std::string &root,
						   const critical_command &command);

#endif
