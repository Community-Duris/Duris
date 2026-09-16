#ifndef PLAYER_DEATH_RESTITUTION_STAFF_H
#define PLAYER_DEATH_RESTITUTION_STAFF_H

#include "player/player_death_restitution_adapter.h"

#include <cstddef>

// The canonical critical-command codec already enforces the payload bound;
// the staff handoff keeps the same bound while chunking around the in-game
// command reader's per-line limit.
constexpr size_t PLAYER_DEATH_RESTITUTION_STAFF_MAX_COMMAND_BYTES =
    CRITICAL_COMMAND_MAX_ENCODED_BYTES;
constexpr size_t PLAYER_DEATH_RESTITUTION_STAFF_MAX_CHUNK_HEX = 1023;

// Decode and submit one canonical, operator-approved critical command.  The
// actor must be the plan actor and meet the staff level gate; no SQL or shell
// path exists at this boundary.
player_death_restitution_runtime_result player_death_restitution_staff_submit_hex(
    const char *actor, int actor_level, const char *canonical_hex, size_t canonical_hex_size,
    player_death_restitution_runtime_submission *submission_out = nullptr);

// Chunked game-command handoff for plans larger than one input line.  These
// functions run on the game thread; the bounded staging slots are discarded at
// commit/abort and never authorize a partial payload.
player_death_restitution_runtime_result player_death_restitution_staff_begin(
    const char *actor, int actor_level);
player_death_restitution_runtime_result player_death_restitution_staff_append_hex(
    const char *actor, int actor_level, const char *hex_chunk, size_t hex_chunk_size);
player_death_restitution_runtime_result player_death_restitution_staff_commit(
    const char *actor, int actor_level,
    player_death_restitution_runtime_submission *submission_out = nullptr);
player_death_restitution_runtime_result player_death_restitution_staff_abort(
    const char *actor, int actor_level);

#endif
