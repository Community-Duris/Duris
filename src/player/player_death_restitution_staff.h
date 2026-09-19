#ifndef PLAYER_DEATH_RESTITUTION_STAFF_H
#define PLAYER_DEATH_RESTITUTION_STAFF_H

#include "player/player_death_restitution_adapter.h"

#include <cstddef>
#include <cstdint>

// The canonical critical-command codec already enforces the payload bound;
// the staff handoff keeps the same bound while chunking around the in-game
// command reader's per-line limit.
constexpr size_t PLAYER_DEATH_RESTITUTION_STAFF_MAX_COMMAND_BYTES =
	CRITICAL_COMMAND_MAX_ENCODED_BYTES;
// The game command reader accepts at most 1023 characters per input line,
// including the 18-character "restitution chunk " prefix.  Round the remaining
// payload budget down to an even number of hex characters (whole bytes).
constexpr size_t PLAYER_DEATH_RESTITUTION_STAFF_MAX_CHUNK_HEX = 1004;
constexpr size_t PLAYER_DEATH_RESTITUTION_STAFF_MAX_CHUNKS =
	(CRITICAL_COMMAND_MAX_ENCODED_BYTES * 2 + PLAYER_DEATH_RESTITUTION_STAFF_MAX_CHUNK_HEX -
	 1) /
	PLAYER_DEATH_RESTITUTION_STAFF_MAX_CHUNK_HEX;

enum class player_death_restitution_staff_staging_state : uint8_t
{
	inactive = 0,
	active,
};

struct player_death_restitution_staff_staging_status
{
	player_death_restitution_staff_staging_state state;
	size_t accepted_chunks;
	size_t accepted_hex_bytes;
	size_t max_chunks;
	size_t max_hex_bytes;
};

// Decode and submit one canonical, operator-approved critical command.  The
// actor must be the plan actor and meet the staff level gate; no SQL or shell
// path exists at this boundary.
player_death_restitution_runtime_result player_death_restitution_staff_submit_hex(
	const char *actor, int actor_level, const char *canonical_hex, size_t canonical_hex_size,
	player_death_restitution_runtime_submission *submission_out = nullptr);

// Chunked game-command handoff for plans larger than one input line.  These
// functions run on the game thread; the bounded staging slots are discarded at
// commit/abort and never authorize a partial payload.
player_death_restitution_runtime_result player_death_restitution_staff_begin(const char *actor,
									     int actor_level);
player_death_restitution_runtime_result
player_death_restitution_staff_append_hex(const char *actor, int actor_level, const char *hex_chunk,
					  size_t hex_chunk_size);
player_death_restitution_runtime_result player_death_restitution_staff_commit(
	const char *actor, int actor_level,
	player_death_restitution_runtime_submission *submission_out = nullptr);
player_death_restitution_runtime_result player_death_restitution_staff_abort(const char *actor,
									     int actor_level);

// Read-only progress projection for the actor-bound in-memory staging slot.
// It contains only transport counts and bounds, never payload bytes.
bool player_death_restitution_staff_get_staging_status(
	const char *actor, int actor_level,
	player_death_restitution_staff_staging_status *status_out);

#endif
