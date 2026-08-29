#ifndef DURIS_CORPSE_LIFECYCLE_COMMAND_H
#define DURIS_CORPSE_LIFECYCLE_COMMAND_H

#include "critical_command.h"

#include <array>
#include <cstdint>
#include <string>

constexpr uint16_t CORPSE_LIFECYCLE_PAYLOAD_VERSION = 5;
constexpr uint16_t CORPSE_LIFECYCLE_PREVIOUS_PAYLOAD_VERSION = 4;
constexpr uint16_t CORPSE_LIFECYCLE_INTERMEDIATE_PAYLOAD_VERSION = 3;
constexpr uint16_t CORPSE_LIFECYCLE_RELEASE_PAYLOAD_VERSION = 2;
constexpr uint16_t CORPSE_LIFECYCLE_LEGACY_PAYLOAD_VERSION = 1;
constexpr size_t CORPSE_LIFECYCLE_OWNER_NAME_MAX_BYTES = 255;
constexpr size_t CORPSE_LIFECYCLE_SHORT_DESCRIPTION_MAX_BYTES = 512;
constexpr size_t CORPSE_LIFECYCLE_DESCRIPTION_MAX_BYTES = 64 * 1024;
constexpr size_t CORPSE_LIFECYCLE_KEYWORDS_MAX_BYTES = 512;
constexpr size_t CORPSE_LIFECYCLE_LEGACY_RESULT_BYTES = 32;
constexpr size_t CORPSE_LIFECYCLE_PREVIOUS_RESULT_BYTES = 64;
constexpr size_t CORPSE_LIFECYCLE_RESULT_BYTES = 96;

enum class corpse_lifecycle_action : uint8_t
{
	upsert = 1,
	remove = 2,
	release = 3,
	destroy = 4,
	resurrect = 5,
	raise_follower = 6,
	release_nested = 7,
};

struct corpse_lifecycle_payload
{
	corpse_lifecycle_action action = corpse_lifecycle_action::upsert;
	uint32_t owner_pid = 0;
	uint32_t save_id = 0;
	uint64_t expected_corpse_revision = 0;
	uint64_t expected_room_revision = 0;
	uint32_t destination_player_pid = 0;
	int32_t old_room_vnum = 0;
	uint64_t expected_player_revision = 0;
	uint64_t expected_wallet_revision = 0;
	uint64_t target_root_item_uid = 0;
	uint64_t target_parent_item_uid = 0;
	uint64_t expected_target_parent_revision = 0;
	int32_t room_vnum = 0;
	int32_t weight = 0;
	std::array<int32_t, 8> values = {};
	std::array<int32_t, 4> money = {};
	std::string owner_name;
	std::string short_description;
	std::string description;
	std::string keywords;
};

struct corpse_lifecycle_result
{
	uint32_t owner_pid = 0;
	uint32_t save_id = 0;
	corpse_lifecycle_action action = corpse_lifecycle_action::upsert;
	uint64_t corpse_revision = 0;
	uint64_t catalog_revision = 0;
	uint64_t corpse_owner_revision = 0;
	uint64_t room_owner_revision = 0;
	uint64_t player_owner_revision = 0;
	uint64_t wallet_revision = 0;
	uint64_t max_item_revision = 0;
	uint32_t item_count = 0;
	std::array<int32_t, 4> wallet = {};
};

bool corpse_lifecycle_command_encode_payload(const corpse_lifecycle_payload &payload,
					     std::vector<uint8_t> *encoded);
bool corpse_lifecycle_command_decode_payload(const critical_command &command,
					     corpse_lifecycle_payload *payload);
bool corpse_lifecycle_command_encode_result(
	const corpse_lifecycle_result &result,
	std::array<uint8_t, CORPSE_LIFECYCLE_RESULT_BYTES> *encoded);
bool corpse_lifecycle_command_decode_result(const uint8_t *encoded, size_t encoded_size,
					    corpse_lifecycle_result *result);
bool corpse_lifecycle_command_build(critical_command *command, critical_operation_id operation_id,
				    const corpse_lifecycle_payload &payload,
				    critical_source_site source_site,
				    critical_deadline_class deadline_class);

#endif
