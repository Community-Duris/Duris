#ifndef PLAYER_DEATH_RESTITUTION_RUNTIME_H
#define PLAYER_DEATH_RESTITUTION_RUNTIME_H

#include "persistence/critical_command_coordinator.h"
#include "persistence/player_death_restitution_command.h"

#include <cstdint>

// These callbacks are the target-only seam for the live game-core adapter.
// They must not stop unrelated runtime threads or database sessions.
using player_death_restitution_recipient_is_offline_fn = bool (*)(uint32_t recipient_pid,
								  void *context);
using player_death_restitution_pending_save_empty_fn = bool (*)(uint32_t recipient_pid,
								uint64_t expected_save_revision,
								void *context);
using player_death_restitution_acquire_save_login_fence_fn =
	bool (*)(uint32_t recipient_pid, uint64_t expected_save_revision, void *context);
using player_death_restitution_release_save_login_fence_fn =
	void (*)(uint32_t recipient_pid, uint64_t expected_save_revision, void *context);

struct player_death_restitution_runtime_callbacks
{
	player_death_restitution_recipient_is_offline_fn recipient_is_offline;
	player_death_restitution_pending_save_empty_fn pending_save_is_empty;
	player_death_restitution_acquire_save_login_fence_fn acquire_target_save_login_fence;
	player_death_restitution_release_save_login_fence_fn release_target_save_login_fence;
};

enum class player_death_restitution_runtime_result : uint8_t
{
	accepted = 1,
	attached,
	invalid_plan,
	unauthorized,
	recipient_online,
	pending_save,
	fence_unavailable,
	coordinator_unavailable,
	overloaded,
	journal_uncertain,
	journal_failure,
	identity_conflict,
};

struct player_death_restitution_runtime_submission
{
	critical_operation_id operation_id;
	uint32_t recipient_pid;
	uint64_t expected_save_revision;
	bool target_save_login_fence_held;
};

// Read-only boundary check.  This must be connected to the public login/save
// barrier before any live claim is made.
player_death_restitution_runtime_result player_death_restitution_runtime_preflight(
	const player_death_restitution_plan &plan,
	const player_death_restitution_runtime_callbacks &callbacks, void *context);

// Submit an already canonical, approved critical command.  The command payload
// is decoded and revalidated before the target fence is acquired; no command
// text or database operation bypasses this boundary.
player_death_restitution_runtime_result player_death_restitution_runtime_submit_command(
	const critical_command &command,
	const player_death_restitution_runtime_callbacks &callbacks, void *context,
	player_death_restitution_runtime_submission *submission);

// Restore the target fence for a command read from the durable critical-command
// journal.  This validates the actual replayed payload and acquires the fence
// without submitting a duplicate operation.
player_death_restitution_runtime_result player_death_restitution_runtime_restore_replayed_command(
	const critical_command &command,
	const player_death_restitution_runtime_callbacks &callbacks, void *context,
	player_death_restitution_runtime_submission *submission);

// Acquire only the recipient save/login fence, then submit one critical
// command.  The fence remains held until complete() or abort() is called.
player_death_restitution_runtime_result
player_death_restitution_runtime_submit(const player_death_restitution_plan &plan,
					const player_death_restitution_runtime_callbacks &callbacks,
					void *context,
					player_death_restitution_runtime_submission *submission);

void player_death_restitution_runtime_abort(
	player_death_restitution_runtime_submission *submission,
	const player_death_restitution_runtime_callbacks &callbacks, void *context);

bool player_death_restitution_runtime_complete(
	player_death_restitution_runtime_submission *submission,
	const critical_completion &completion,
	const player_death_restitution_runtime_callbacks &callbacks, void *context);

#endif
