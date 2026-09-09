#ifndef ZONE_STORY_QUEST_CATALOG_H
#define ZONE_STORY_QUEST_CATALOG_H

#include "world/zone_story_quest_tracking.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace zone_story_quest_catalog
{
constexpr uint32_t ZONE_STORY_QUEST_CATALOG_SCHEMA_VERSION = 1;

struct catalog
{
	uint32_t schema_version = ZONE_STORY_QUEST_CATALOG_SCHEMA_VERSION;
	uint32_t content_revision = 0;
	std::vector<zone_story_quest_tracking::quest_definition> definitions;
};

struct diagnostic
{
	int64_t index = -1;
	std::string code;
	std::string message;
};

bool validate(const catalog &catalog, std::vector<diagnostic> *diagnostics);
std::size_t eligible_definition_count(const catalog &catalog, int32_t zone_number,
				      uint32_t content_revision);
} // namespace zone_story_quest_catalog

#endif
