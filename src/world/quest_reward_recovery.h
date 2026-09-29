#ifndef QUEST_REWARD_RECOVERY_H
#define QUEST_REWARD_RECOVERY_H

#include "item/quest_reward_continuation.h"
#include "persistence/critical_command.h"

struct char_data;

// Dispatch a durable quest reward continuation after its player is materialized.
// Unsupported or failed effects leave the obligation pending for a later login.
void quest_reward_recover_pending(char_data *player,
				 const critical_operation_id &offering_operation,
				 const quest_reward_continuation &continuation,
				 uint64_t xp_applied_mask = 0);

void quest_reward_recover_xp_entitlement(char_data *player,
					 const critical_operation_id &offering_operation,
					 const quest_reward_continuation &continuation,
					 uint32_t reward_index, uint32_t amount);

// Release quest reward acknowledgements only after requested player snapshots
// have crossed the save pipeline's durable revision fence.
void quest_reward_recovery_pulse(void);

#endif
