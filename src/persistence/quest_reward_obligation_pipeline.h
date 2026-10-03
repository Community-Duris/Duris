#ifndef QUEST_REWARD_OBLIGATION_PIPELINE_H
#define QUEST_REWARD_OBLIGATION_PIPELINE_H

#include "persistence/critical_command.h"
#include "persistence/quest_reward_obligation_repository.h"

#include <cstddef>
#include <cstdint>

constexpr size_t QUEST_REWARD_ACK_PIPELINE_MAX = 1024;
constexpr size_t QUEST_REWARD_ACK_PIPELINE_PULSE_MAX = 32;

enum class quest_reward_ack_submit_result : uint8_t
{
	queued,
	already_queued,
	invalid,
	unavailable,
	overloaded,
};

struct quest_reward_ack_completion
{
	uint32_t player_pid = 0;
	critical_operation_id offering_operation = {};
	quest_reward_obligation_result result = quest_reward_obligation_result::database_error;
	unsigned int error_code = 0;
};

bool quest_reward_obligation_pipeline_init();
void quest_reward_obligation_pipeline_shutdown();
quest_reward_ack_submit_result
quest_reward_obligation_pipeline_submit(uint32_t player_pid,
					const critical_operation_id &offering_operation);
size_t quest_reward_obligation_pipeline_pulse(quest_reward_ack_completion *completions,
					      size_t capacity);

#endif
