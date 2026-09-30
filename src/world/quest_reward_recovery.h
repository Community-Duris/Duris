#ifndef QUEST_REWARD_RECOVERY_H
#define QUEST_REWARD_RECOVERY_H

#include "item/quest_reward_continuation.h"
#include "persistence/critical_command.h"
#include "player/player_revision_state.h"

#include <cstddef>
#include <vector>

struct char_data;
struct player_quest_xp_receipt_snapshot;

// Dispatch a durable quest reward continuation after its player is materialized.
// Unsupported or failed effects leave the obligation pending for a later login.
void quest_reward_recover_pending(char_data *player,
				  const critical_operation_id &offering_operation,
				  const quest_reward_continuation &continuation,
				  uint64_t xp_applied_mask = 0, uint64_t economic_applied_mask = 0,
				  bool economic_history_verified = true);

void quest_reward_recover_xp_entitlement(char_data *player,
					 const critical_operation_id &offering_operation,
					 const quest_reward_continuation &continuation,
					 uint32_t reward_index, uint32_t amount);

// Match the exact operation receipts returned by a successful player save.
void quest_reward_recovery_save_acknowledged(int pid, uint64_t revision,
					     const player_quest_xp_receipt_snapshot *receipts,
					     size_t receipt_count);

// Collect applied, unacknowledged XP and its coupled player components for an
// ordinary or terminal checkpoint. Outputs remain unchanged on failure.
bool quest_reward_recovery_pending_save_receipts(
	int pid, std::vector<player_quest_xp_receipt_snapshot> *receipts,
	player_component_mask_t *components);

// Receipt-bearing rewards wait for the exact save completion; skill-only
// rewards retain the ordinary durable revision fence.
void quest_reward_recovery_pulse(void);

#endif
