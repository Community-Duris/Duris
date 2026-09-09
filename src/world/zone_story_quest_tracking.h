#ifndef ZONE_STORY_QUEST_TRACKING_H
#define ZONE_STORY_QUEST_TRACKING_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace zone_story_quest_tracking
{
constexpr uint32_t ZONE_STORY_QUEST_TRACKING_SCHEMA_VERSION = 1;
constexpr std::string_view ZONE_STORY_QUEST_SOURCE_SYSTEM = "zone_story";

constexpr uint32_t ZONE_STORY_CREDIT_NONE = 0;
constexpr uint32_t ZONE_STORY_CREDIT_PERSONAL = 1U << 0;
constexpr uint32_t ZONE_STORY_CREDIT_SOLO = 1U << 1;
constexpr uint32_t ZONE_STORY_CREDIT_GROUP_PARTICIPANT = 1U << 2;
constexpr uint32_t ZONE_STORY_CREDIT_LEADERSHIP = 1U << 3;

struct quest_definition
{
	std::string definition_id;
	std::string source_system;
	int32_t zone_number = 0;
	std::string source_area;
	int32_t giver_vnum = 0;
	std::string completion_key;
	bool active = false;
	bool eligible_for_zone_completion = false;
	bool repeatable = false;
	uint32_t content_revision = 0;
};

struct completion_transaction
{
	uint32_t schema_version = 0;
	std::string transaction_id;
	std::string quest_definition_id;
	int32_t zone_number = 0;
	uint32_t direct_completer_pid = 0;
	std::vector<uint32_t> credited_pids;
	int32_t room_vnum = 0;
	int64_t completed_at = 0;
	uint32_t season_id = 0;
	uint32_t content_revision = 0;
};

bool validate_definition(const quest_definition &definition, std::string *error = nullptr);
bool validate_transaction(const completion_transaction &transaction, std::string *error = nullptr);

uint32_t credit_mask_for_pid(const completion_transaction &transaction, uint32_t pid);
bool is_solo_transaction(const completion_transaction &transaction);
bool is_leadership_transaction(const completion_transaction &transaction);

std::string serialize_transaction(const completion_transaction &transaction,
				  std::string *error = nullptr);
bool deserialize_transaction(std::string_view encoded, completion_transaction *transaction,
			     std::string *error = nullptr);
} // namespace zone_story_quest_tracking

#endif
