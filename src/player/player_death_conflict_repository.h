#ifndef PLAYER_DEATH_CONFLICT_REPOSITORY_H
#define PLAYER_DEATH_CONFLICT_REPOSITORY_H

#include "player/player_snapshot.h"
#include "player/player_save_worker.h"
#include <mysql/mysql.h>

// A retained observation is NOT an applied death, an inventory, an ownership
// grant, or permission to release a character. No gameplay caller is enabled
// until the terminal transaction and player-visible recovery route are joined.
enum class player_death_conflict_outcome : uint8_t
{
	retained,
	already_retained,
	read,
	not_found,
	identity_conflict,
	stale_revision,
	commit_unknown,
	failed,
	terminal_committed,
	already_terminal,
};

struct player_death_conflict_result
{
	player_death_conflict_outcome outcome;
	unsigned int error_code;
	player_revision_t source_revision;
};

struct player_death_conflict_case
{
	critical_operation_id operation_id;
	player_revision_t save_revision;
	player_revision_t source_revision;
	uint64_t corpse_item_uid;
};

constexpr size_t PLAYER_DEATH_CONFLICT_LIST_LIMIT = 25;

// Owns one transaction on an exclusively borrowed, idle, autocommit connection
// with automatic reconnect disabled. Captures locked SQL observations alongside
// an immutable version-8 death request. Retries require the SAME request/ID;
// colliding player/revision or corpse identities fail rather than overwrite.
// Source inventory, custody, wallet, player revision and death dispositions are
// never changed. An ambiguous COMMIT must be resolved by replay, not a new ID.
player_death_conflict_result player_death_conflict_retain(MYSQL *connection,
							  const player_snapshot &request) noexcept;

// DB-only save owner. Normal saves retain their existing repository behavior.
// Only a format-8 terminal death rejected for the exact custody/payload mismatch
// may atomically retain evidence and commit its non-item terminal state. A
// previously retained case must replay the same request, never recapture it.
// Accounting must be inactive and wallet conversion must already be complete.
// These entry points do not instantiate or deliver any retained object.
player_save_apply_result player_death_conflict_apply(MYSQL *connection,
						     const player_snapshot &request) noexcept;
player_save_apply_result player_death_conflict_apply_from_pool(const player_snapshot &request,
							       void *context);

// Internal read interfaces, not a player authentication boundary. The adapter
// must derive pid from the authenticated character, never from command text.
// Raw payload is private recovery evidence; do not display it directly. Read
// and list enforce pid in SQL, and leave outputs unchanged on any failure.
player_death_conflict_result player_death_conflict_read(MYSQL *connection, int32_t pid,
							const critical_operation_id &operation_id,
							player_snapshot *output) noexcept;
player_death_conflict_result
player_death_conflict_list(MYSQL *connection, int32_t pid, player_revision_t after_revision,
			   std::vector<player_death_conflict_case> *output) noexcept;

#endif
