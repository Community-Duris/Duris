#ifndef ZONE_STORY_QUEST_STATE_REPOSITORY_H
#define ZONE_STORY_QUEST_STATE_REPOSITORY_H

#include <cstdint>
#include <string>
#include <vector>

namespace zone_story_quest_catalog
{
struct catalog;
}

enum class sql_zone_story_quest_state_result
{
	ok,
	not_found,
	invalid,
	io_error,
};

sql_zone_story_quest_state_result
sql_zone_story_quest_state_load(uint32_t expected_catalog_revision, std::string *state,
				std::string *error = nullptr);
sql_zone_story_quest_state_result sql_zone_story_quest_state_save(uint32_t catalog_revision,
								  const std::string &state,
								  std::string *error = nullptr);

// Requires the caller's live transaction and retains its commit/rollback ownership.
sql_zone_story_quest_state_result sql_zone_story_quest_state_remove_player_aliases(
	uint32_t expected_catalog_revision, uint32_t current_season_id,
	const std::vector<uint32_t> &pids, const zone_story_quest_catalog::catalog &catalog,
	std::string *error = nullptr);

#endif
