#ifndef PLAYER_DEATH_RESTITUTION_ADAPTER_H
#define PLAYER_DEATH_RESTITUTION_ADAPTER_H

#include "player/player_death_restitution_runtime.h"

#include <cstddef>
#include <cstdint>

struct player_death_restitution_runtime_live_health
{
	size_t pending_operations;
	size_t fenced_targets;
};

enum class player_death_restitution_runtime_operation_phase : uint8_t
{
	unknown = 0,
	awaiting_durability,
	admitted,
	durable_execution,
	journal_uncertain,
	durable_receipt_unverified,
	terminal_failure,
};

// This is deliberately a public, privacy-preserving status projection.  It
// carries no player/item identifiers or payload bytes.  A durable receipt is
// not an exact target readback, so the runtime never reports `verified` here.
struct player_death_restitution_runtime_operation_status
{
	critical_operation_id operation_id;
	player_death_restitution_runtime_operation_phase phase;
	critical_command_durability durability;
	critical_apply_outcome completion_outcome;
	bool completion_available;
	bool durable_receipt_recorded;
	bool target_save_login_fence_held;
	bool exact_verification_required;
	bool retry_safe;
};

// Game-thread boundary for the native per-player restitution command.  The
// adapter owns the callback context and retains accepted/ambiguous submissions
// until critical-command completion is published.
player_death_restitution_runtime_result player_death_restitution_runtime_submit_live(
	const player_death_restitution_plan &plan,
	player_death_restitution_runtime_submission *submission_out = nullptr);

// Staff-only boundary for an already approved canonical critical command.  The
// actor and level are checked before the exact command is handed to the runtime.
player_death_restitution_runtime_result player_death_restitution_runtime_submit_live_approved(
	const critical_command &approved_command, const char *actor, int actor_level,
	player_death_restitution_runtime_submission *submission_out = nullptr);

// Read-only operation lookup for a reconnecting staff actor.  The actor must
// exactly match the protected plan actor; unknown and mismatched identities
// both return false so this boundary cannot be used to enumerate operations.
bool player_death_restitution_runtime_operation_status_copy(
	const char *actor, int actor_level, const critical_operation_id &operation_id,
	player_death_restitution_runtime_operation_status *status_out);

void player_death_restitution_runtime_handle_completions(const critical_completion *completions,
							 size_t count);
// Coordinator replay hook.  The coordinator calls this with each actual
// journal command before it starts workers or accepts logins.
bool player_death_restitution_runtime_restore_replayed_command(const critical_command &command,
							       void *context);
// Initialization-failure cleanup only; accepted operations are not aborted by
// normal shutdown because an ambiguous operation must remain fenced/replayable.
void player_death_restitution_runtime_abort_all(void);
void player_death_restitution_runtime_shutdown(void);

// Login admission is target-only.  It does not inspect or quiesce unrelated
// descriptors, save queues, or database sessions.
bool player_death_restitution_runtime_login_admit(int pid);

player_death_restitution_runtime_live_health
player_death_restitution_runtime_live_health_copy(void);

#endif
