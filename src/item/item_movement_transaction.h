#ifndef ITEM_MOVEMENT_TRANSACTION_H
#define ITEM_MOVEMENT_TRANSACTION_H

#include "persistence/critical_command_coordinator.h"
#include "item/item_transfer_command.h"
#include "core/structs.h"

#include <cstddef>
#include <cstdint>

constexpr size_t ITEM_MOVEMENT_PENDING_MAX = 1024;
constexpr size_t ITEM_MOVEMENT_CONTEXT_MAX_BYTES = 128;

using item_movement_completion_fn = void (*)(P_char actor, bool committed,
					     const item_transfer_result &result,
					     unsigned int error_code, const uint8_t *context,
					     size_t context_size);

// A submission can be refused for reasons that are operationally very different: a
// transient conflict the player should simply retry, versus ledger state that disagrees
// with live topology and will never resolve on its own. Callers map this onto both the
// player-facing text and the structured diagnostic, so the two classes stay separable.
enum class item_movement_reject
{
	none,
	invalid_request,
	queue_saturated,
	pending_conflict,
	owner_mismatch,
	missing_owner_revision,
	topology_mismatch,
	snapshot_failure,
	allocation_failure,
	command_build_failure,
	coordinator_rejected,
};

const char *item_movement_reject_name(item_movement_reject reason);
bool item_movement_reject_is_transient(item_movement_reject reason);

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
				      size_t context_size, P_obj corpse_context = NULL,
				      item_movement_reject *reject = NULL);
bool item_movement_transaction_submit_batch(
	P_char actor, P_obj const *roots, size_t root_count, P_obj target_container,
	const item_owner_identity &from_owner, const item_owner_identity &to_owner,
	item_transfer_reason reason, int64_t reason_id, item_movement_completion_fn completion,
	const void *context, size_t context_size, P_obj corpse_context = NULL,
	item_movement_reject *reject = NULL);
bool item_creation_grant_submit_to_player(P_char actor, P_obj object, P_char recipient,
					  P_obj target_container = NULL);
bool item_creation_grant_submit_to_player_before_entry(P_char actor, P_obj object,
						       P_char recipient);
bool item_creation_grant_submit_to_room(P_char actor, P_obj object, int room);
bool item_creation_grant_mark_blocking(P_char actor);
bool item_creation_grant_blocks_commands(P_char actor);
void item_movement_transaction_handle_completions(const critical_completion *completions,
						  size_t count);
void item_movement_transaction_player_ready(P_char actor);
bool item_movement_transaction_player_busy(P_char actor);
item_movement_health item_movement_transaction_health_copy(void);
void item_movement_transaction_reset_for_tests(void);

#endif
