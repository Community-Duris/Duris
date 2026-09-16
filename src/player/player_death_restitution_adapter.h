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

void player_death_restitution_runtime_handle_completions(const critical_completion *completions,
							 size_t count);
void player_death_restitution_runtime_shutdown(void);

// Login admission is target-only.  It does not inspect or quiesce unrelated
// descriptors, save queues, or database sessions.
bool player_death_restitution_runtime_login_admit(int pid);

player_death_restitution_runtime_live_health
player_death_restitution_runtime_live_health_copy(void);

#endif
