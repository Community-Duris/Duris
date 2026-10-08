#ifndef PLAYER_SNAPSHOT_REPOSITORY_H
#define PLAYER_SNAPSHOT_REPOSITORY_H

#include "player/player_save_worker.h"
#include <mysql/mysql.h>
#include <span>

// Include player/player_sql_transaction_cleanup.h when constructing or
// inspecting cleanup evidence. Callers using pointers need no helper definition.
struct player_sql_cleanup;
class player_save_covered_revision;

// Read-only native owner for obsolete ordinary journal-frame coverage. The
// exact held publication reservation remains live throughout observation and
// later retirement. Proof is issued only after same-session row locking and
// confirmed rollback/idle cleanup; this neither applies a snapshot nor ACKs it.
bool player_snapshot_repository_observe_covered_revision(
	int pid, const player_save_execution_guard::held_publication_reservation &reservation,
	player_save_covered_revision *proof) noexcept;

// Borrowed exact full EQ/INV projection inside the caller's original reconnect-
// disabled transaction. Caller holds the global custody-before-physical lock cut
// and has completed its authorized custody mutation. No transaction ownership,
// topology repair, save-revision change, durability or publication authority.
// On any nonzero result the caller must roll back its whole original operation.
unsigned int player_snapshot_repository_project_items_in_transaction(
	MYSQL *, uint32_t pid, uint64_t expected_save_revision,
	std::span<const player_item_snapshot> exact_after) noexcept;

// Caller owns the transaction. Shared by checkpoint and legacy save adapters.
bool player_snapshot_repository_write_pets(MYSQL *connection, const player_snapshot &snapshot);

enum class player_death_terminal_write_outcome : uint8_t
{
	written,
	already_written,
	failed,
};

struct player_death_terminal_write_result
{
	player_death_terminal_write_outcome outcome;
	unsigned int error_code;
};

// Internal transaction participant, NOT a durability acknowledgement. Requires
// the exact retained evidence row and its source revision in the caller's live
// transaction. Never commits, projects items/pets, changes custody, or converts
// money. The conflict owner must commit before acknowledging terminal release.
player_death_terminal_write_result
player_snapshot_repository_write_retained_death(MYSQL *connection, const player_snapshot &request,
						const player_snapshot &retained,
						player_revision_t source_revision);

// Borrows an idle autocommit session with reconnect disabled. Never closes,
// releases or replaces it. A successful save result is not session reuse proof.
// Reuse-sensitive callers must inspect the cleanup overload on every return;
// legacy callers must conservatively retire a handle when cleanup is uncertain.
// The ordinary execution guard ends when borrowed apply returns. A caller
// participating in restored-hold registration must retain an outer execution
// permit through confirmed disposal of retire_required cleanup; this overload
// alone cannot prove that the borrowed transaction/session has been retired.
// The pooled wrapper owns that full interval. Recovery/death-conflict outer
// mutation coverage remains a separate integration requirement.
player_save_apply_result player_snapshot_repository_apply(MYSQL *connection,
							  const player_snapshot &snapshot);
player_save_apply_result player_snapshot_repository_apply(MYSQL *connection,
							  const player_snapshot &snapshot,
							  player_sql_cleanup *cleanup);
struct player_save_recovery_record;
player_save_apply_result
player_snapshot_repository_recovery_apply(MYSQL *, const player_save_recovery_record &);
player_save_apply_result
player_snapshot_repository_recovery_apply(MYSQL *, const player_save_recovery_record &,
					  player_sql_cleanup *cleanup);
player_save_apply_result player_snapshot_repository_apply_from_pool(const player_snapshot &snapshot,
								    void *context);

#endif
