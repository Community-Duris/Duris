#ifndef DURIS_FLATFILE_ZONE_STORY_QUEST_STATE_H
#define DURIS_FLATFILE_ZONE_STORY_QUEST_STATE_H

#include <cstdint>
#include <string>

enum class flatfile_zone_story_quest_result
{
	ok,
	not_found,
	invalid,
	corrupt,
	io_error,
};

flatfile_zone_story_quest_result flatfile_zone_story_quest_state_load(
	const char *root, uint32_t expected_catalog_revision, std::string *state,
	std::string *error = nullptr);
flatfile_zone_story_quest_result flatfile_zone_story_quest_state_save(
	const char *root, uint32_t catalog_revision, const std::string &state,
	std::string *error = nullptr);

#endif
