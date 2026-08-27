#ifndef COMBAT_OUTCOME_COMMAND_H
#define COMBAT_OUTCOME_COMMAND_H

#include "critical_command.h"
#include "currency_command.h"

#include <array>
#include <cstdint>

constexpr uint16_t COMBAT_OUTCOME_PAYLOAD_VERSION = 1;
constexpr size_t COMBAT_OUTCOME_MAX_PARTICIPANTS = 15;
constexpr size_t COMBAT_OUTCOME_ROOM_NAME_MAX_BYTES = 96;
constexpr size_t COMBAT_OUTCOME_DESCRIPTION_MAX_BYTES = 128;
constexpr size_t COMBAT_OUTCOME_RESULT_BYTES = 1536;

enum class combat_participant_role : uint8_t
{
	unknown = 0,
	killer,
	killer_group,
	victim,
	victim_group,
};

enum combat_participant_flag : uint8_t
{
	COMBAT_PARTICIPANT_IN_ROOM = UINT8_C(1) << 0,
	COMBAT_PARTICIPANT_LEADER = UINT8_C(1) << 1,
	COMBAT_PARTICIPANT_SPILL_BLOOD = UINT8_C(1) << 2,
};

struct combat_outcome_participant
{
	uint32_t pid;
	combat_participant_role role;
	uint8_t flags;
	uint16_t level;
	uint8_t racewar;
	int64_t frag_delta;
	int64_t epic_delta;
	int64_t wallet_delta_copper;
	uint64_t expected_frag_revision;
	uint64_t expected_epic_revision;
	uint64_t expected_wallet_revision;
	uint64_t expected_bank_revision;
	std::array<char, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1> account_name;
	std::array<char, COMBAT_OUTCOME_DESCRIPTION_MAX_BYTES + 1> description;
};

struct combat_outcome_payload
{
	uint32_t victim_pid;
	int32_t room_vnum;
	int64_t gameplay_read_occurred_at;
	uint64_t gameplay_read_token;
	std::array<char, COMBAT_OUTCOME_ROOM_NAME_MAX_BYTES + 1> room_name;
	uint16_t participant_count;
	std::array<combat_outcome_participant, COMBAT_OUTCOME_MAX_PARTICIPANTS> participants;
};

struct combat_outcome_participant_result
{
	uint32_t pid;
	int64_t frags;
	int64_t epics;
	int64_t wallet_value;
	currency_vector bank;
	uint64_t frag_revision;
	uint64_t epic_revision;
	uint64_t wallet_revision;
	uint64_t bank_revision;
};

struct combat_outcome_result
{
	uint64_t event_id;
	uint16_t participant_count;
	std::array<combat_outcome_participant_result, COMBAT_OUTCOME_MAX_PARTICIPANTS> participants;
};

bool combat_outcome_command_encode_payload(const combat_outcome_payload &payload,
					   std::vector<uint8_t> *encoded);
bool combat_outcome_command_decode_payload(const critical_command &command,
					   combat_outcome_payload *payload);
bool combat_outcome_command_encode_result(const combat_outcome_result &result,
					  std::array<uint8_t, COMBAT_OUTCOME_RESULT_BYTES> *encoded);
bool combat_outcome_command_decode_result(const uint8_t *encoded, size_t size,
					  combat_outcome_result *result);
bool combat_outcome_command_build(critical_command *command, critical_operation_id operation_id,
				  const combat_outcome_payload &payload);

#endif
