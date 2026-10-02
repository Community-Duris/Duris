#ifndef ZONE_STORY_QUEST_CATALOG_H
#define ZONE_STORY_QUEST_CATALOG_H

#include "world/zone_story_quest_tracking.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <map>
#include <vector>

namespace zone_story_quest_catalog
{
constexpr uint32_t ZONE_STORY_QUEST_CATALOG_SCHEMA_VERSION = 1;

struct zone_definition
{
	int32_t zone_number = 0;
	std::string name;
	std::string source_area;
	int32_t first_vnum = 0;
	int32_t last_vnum = 0;
	bool discoverable = true;
};

/* Builder-authored presentation/projection. Native definitions remain the
 * authority for rewards and durable completion identities. */
struct story_step
{
	std::string id;
	std::string text;
	std::string kind;
	std::string hint;
	std::vector<int32_t> item_vnums;
	uint32_t count = 1;
	int32_t slot = -1;
	std::vector<std::string> contracts = {};
};

struct story_definition
{
	std::string id;
	int32_t zone_number = 0;
	std::string title;
	std::string category;
	std::string summary;
	std::vector<std::string> contracts;
	std::vector<story_step> steps;
};

struct story_contact
{
	int32_t mob_vnum = 0;
	std::string name;
	std::string keyword;
	std::string description;
	std::vector<std::string> topics;
};

struct story_mapping
{
	std::string source_area;
	uint32_t revision = 0;
	bool complete = false;
	std::vector<story_definition> stories;
	std::map<std::string, std::string> exclusions;
	std::string introduction = {};
	std::vector<std::string> orientation = {};
	std::vector<story_contact> contacts = {};
};

struct journal_inventory
{
	std::map<int32_t, uint32_t> carried;
	std::map<int32_t, int32_t> equipped;
};

struct catalog
{
	uint32_t schema_version = ZONE_STORY_QUEST_CATALOG_SCHEMA_VERSION;
	uint32_t content_revision = 0;
	std::vector<zone_story_quest_tracking::quest_definition> definitions;
	std::vector<zone_definition> zones = {};
	std::vector<story_mapping> story_mappings = {};
};

struct quest_unit
{
	std::string id;
	int32_t zone_number = 0;
	bool achievement = false;
	bool daily_candidate = false;
	std::vector<std::string> contracts = {};
};

std::vector<quest_unit> quest_units(const catalog &catalog);
bool excluded_contract(const catalog &catalog, std::string_view id);
const story_definition *story_for_contract(const catalog &catalog, std::string_view id);

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
