#ifndef EPIC_COMMAND_H
#define EPIC_COMMAND_H

#include "persistence/critical_command.h"

#include <cstdint>

constexpr uint16_t EPIC_COMMAND_PAYLOAD_VERSION = 1;
constexpr size_t EPIC_COMMAND_PAYLOAD_BYTES = 32;
constexpr size_t EPIC_RESULT_PAYLOAD_BYTES = 24;

enum class epic_reason_type : uint16_t
{
	unknown = 0,
	zone_award,
	pvp_award,
	ship_pvp_award,
	elite_mob_award,
	quest_award,
	random_zone_award,
	nexus_award,
	boon_award,
	bottle_award,
	achievement_award,
	random_mob_award,
	store_purchase,
	tradeskill_reset,
	specialization_purchase,
	epic_skill_purchase,
	epic_skill_refund,
	level_purchase,
	ascend_descend,
	ship_purchase,
	admin_adjustment,
};

enum epic_command_flag : uint16_t
{
	EPIC_COMMAND_REQUIRE_FUNDS = UINT16_C(1) << 0,
};

struct epic_command_payload
{
	uint32_t pid;
	int64_t delta;
	epic_reason_type reason;
	uint16_t flags;
	int64_t reason_id;
};

struct epic_command_result
{
	int64_t balance;
	uint64_t revision;
	int64_t delta;
};

bool epic_command_encode_payload(const epic_command_payload &payload,
				 std::vector<uint8_t> *encoded);
bool epic_command_decode_payload(const critical_command &command, epic_command_payload *payload);
bool epic_command_encode_result(const epic_command_result &result,
				std::array<uint8_t, EPIC_RESULT_PAYLOAD_BYTES> *encoded);
bool epic_command_decode_result(const uint8_t *encoded, size_t size, epic_command_result *result);
bool epic_command_build(critical_command *command, critical_operation_id operation_id,
			const epic_command_payload &payload, uint64_t expected_revision,
			critical_source_site source_site, critical_deadline_class deadline_class);

#endif
