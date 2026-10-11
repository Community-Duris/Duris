#ifndef DURIS_ITEM_MOVEMENT_STATE_INTERNAL_H
#define DURIS_ITEM_MOVEMENT_STATE_INTERNAL_H

// Internal shared representation only. The common movement registry remains
// its sole retention owner; this header grants no execution or ACK capability.
#include "item/item_movement_transaction.h"
#include "item/ordinary_drop_recovery.h"
#include "player/player_save_pipeline.h"

#include <array>
#include <memory>

namespace item_movement_detail
{
enum class publication_state : uint8_t
{
	none,
	ready,
	retrying,
	owner_waiting,
	blocked,
	ack_pending,
};

struct pending_movement
{
	uint32_t actor_pid;
	uint64_t actor_runtime_id;
	item_transfer_payload payload;
	item_owner_identity requested_to_owner;
	uint64_t requested_target_parent_uid;
	item_transfer_reason requested_reason;
	int64_t requested_reason_id;
	uint64_t requested_corpse_uid;
	bool adopting;
	bool adoption_only;
	item_movement_completion_fn completion;
	item_movement_publication_fn publication;
	std::array<uint8_t, ITEM_MOVEMENT_CONTEXT_MAX_BYTES> context;
	size_t context_size;
	bool completion_ready;
	bool publication_failed;
	unsigned int publication_attempts;
	publication_state publication_status;
	bool creation_batch;
	bool registry_applied;
	bool recovered_publication;
	// Preserve completed effects independently of retry/disposition status.
	bool craft_publication_started = false;
	bool ordinary_receipt_sealed = false;
	bool ordinary_publication_ready = false;
	bool restored_sql_drop = false;
	std::shared_ptr<const critical_command> live_drop_command = {};
	player_literal_inventory_token live_drop_token = {};
	ordinary_drop_live_publication_state live_drop_state = {};
	bool live_drop_acknowledged = false;
	bool live_drop_admission_refused = false;
	bool publication_inflight = false;
	std::shared_ptr<const critical_command> held_command = {};
	player_held_retirement_checkpoint_token held_token = {};
	bool craft_publication_ready = false;
	bool collector_invalidated;
	critical_completion completed;
	bool disposition_blocked = false;
	bool disposition_blocked_this_batch = false;
	bool publication_attempted_this_batch = false;
};
} // namespace item_movement_detail

#endif
