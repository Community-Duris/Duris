#ifndef BOON_REWARD_COMMAND_H
#define BOON_REWARD_COMMAND_H

#include "persistence/critical_command.h"

#include <array>
#include <cstdint>

constexpr uint16_t BOON_REWARD_PAYLOAD_VERSION = 1;
constexpr size_t BOON_REWARD_MAX_RESULTS = 32;
constexpr size_t BOON_REWARD_RESULT_BYTES = 2080;

constexpr uint8_t BOON_RESULT_PROGRESS = 1 << 0;
constexpr uint8_t BOON_RESULT_COMPLETED = 1 << 1;

struct boon_reward_payload
{
	uint32_t pid;
	uint8_t racewar;
	uint16_t level;
	int32_t zone_number;
	uint8_t option;
	double data;
	int32_t victim_vnum;
	int16_t victim_race;
	uint8_t victim_flags;
};

struct boon_reward_entry
{
	uint32_t boon_id;
	uint8_t type;
	uint8_t option;
	uint8_t flags;
	uint8_t repeat;
	double criteria;
	double criteria2;
	double bonus;
	double bonus2;
	double counter;
};

struct boon_reward_result
{
	uint32_t pid;
	uint16_t entry_count;
	std::array<boon_reward_entry, BOON_REWARD_MAX_RESULTS> entries;
};

bool boon_reward_command_build(critical_command *command, critical_operation_id operation_id,
			       const boon_reward_payload &payload);
bool boon_reward_command_decode_payload(const critical_command &command,
					boon_reward_payload *payload);
bool boon_reward_command_encode_result(const boon_reward_result &result,
				       std::array<uint8_t, BOON_REWARD_RESULT_BYTES> *encoded);
bool boon_reward_command_decode_result(const uint8_t *encoded, size_t size,
				       boon_reward_result *result);

#endif
