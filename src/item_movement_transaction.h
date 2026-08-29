#ifndef ITEM_MOVEMENT_TRANSACTION_H
#define ITEM_MOVEMENT_TRANSACTION_H

#include "critical_command_coordinator.h"
#include "item_transfer_command.h"
#include "structs.h"

#include <cstddef>
#include <cstdint>

constexpr size_t ITEM_MOVEMENT_PENDING_MAX = 1024;
constexpr size_t ITEM_MOVEMENT_CONTEXT_MAX_BYTES = 128;

using item_movement_completion_fn = void (*)(P_char actor, bool committed,
					     const item_transfer_result &result,
					     unsigned int error_code, const uint8_t *context,
					     size_t context_size);

struct item_movement_health
{
	uint64_t pending;
	uint64_t retained_offline;
	uint64_t submitted;
	uint64_t committed;
	uint64_t rejected;
	uint64_t submission_failures;
	uint64_t stale_publications;
};

bool item_movement_transaction_submit(P_char actor, P_obj root, P_obj target_container,
				      const item_owner_identity &from_owner,
				      const item_owner_identity &to_owner,
				      item_transfer_reason reason, int64_t reason_id,
				      item_movement_completion_fn completion, const void *context,
				      size_t context_size, P_obj corpse_context = NULL);
void item_movement_transaction_handle_completions(const critical_completion *completions,
						  size_t count);
void item_movement_transaction_player_ready(P_char actor);
bool item_movement_transaction_player_busy(P_char actor);
item_movement_health item_movement_transaction_health_copy(void);
void item_movement_transaction_reset_for_tests(void);

#endif
