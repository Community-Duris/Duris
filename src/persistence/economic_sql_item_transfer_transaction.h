#ifndef DURIS_ECONOMIC_SQL_ITEM_TRANSFER_TRANSACTION_H
#define DURIS_ECONOMIC_SQL_ITEM_TRANSFER_TRANSACTION_H

#include "economy/item_transfer_accounting.h"
#include "item/item_transfer_repository.h"
#include "persistence/economic_accounting_repository.h"
#include "item/item_ownership_runtime.h"
#include "world/quest_mobile_native.h"

#include <mysql/mysql.h>

#include <cstddef>
#include <cstdint>

struct economic_sql_item_transfer_context
{
	economic_sql_authority_snapshot authority;
	unsigned long session_id = 0;
};

// Lock and validate the active lineage epoch before any item rows are changed.
// The caller's transaction owns this shared lock through commit or rollback.
unsigned int economic_sql_item_transfer_lock(MYSQL *connection, const critical_command &command,
					     economic_sql_item_transfer_context *context);

// Record a typed item-transfer root and its exact ownership-event references
// inside the caller's existing SQL transaction. This function never commits.
unsigned int economic_sql_item_transfer_record(MYSQL *connection, const critical_command &command,
					       const item_transfer_custody_delta *custody_delta,
					       unsigned int result_code, bool mutation_applied,
					       const economic_sql_item_transfer_context &context);

// Verify the exact root, canonical plan, references, and retained result on an
// operation-ID replay. This does not consult the current epoch or item state.
unsigned int economic_sql_item_transfer_verify_retained(MYSQL *connection,
							const critical_command &command,
							unsigned int result_code,
							const uint8_t *result_payload,
							size_t result_size);

struct economic_sql_native_quest_context
{
	economic_sql_authority_snapshot authority;
	unsigned long session_id = 0;
	uint64_t acknowledged_save_revision = 0;
	std::vector<player_item_snapshot> original_player_before, original_player_after;
	std::vector<uint8_t> original_command;
};

// Explicit original native quest v12 route only. Structural binding is not
// admission/source/held-body authority. The atomic caller proves those facts,
// obtains the native participant's actual original held player body, and owns
// evidence/inbox/outbox/reward obligations and one commit/rollback. Generic
// item admission and the old APIs above stay closed to native commands.
unsigned int economic_sql_native_quest_lock(MYSQL *, const critical_command &,
					    economic_sql_native_quest_context *);
unsigned int economic_sql_native_quest_record(MYSQL *, const critical_command &,
					      const item_transfer_custody_delta *,
					      unsigned int result_code, bool mutation_applied,
					      const economic_sql_native_quest_context &);
unsigned int economic_sql_native_quest_verify_retained(MYSQL *, const critical_command &,
						       unsigned int result_code,
						       const uint8_t *result_payload,
						       size_t result_size);

struct critical_completion;
// Values from one original locked publication session, never an ACK capability.
// native is the authenticated CURRENT cash-v2 image. No historical transition
// operation or unknown cash is manufactured from a current value.
struct economic_sql_native_quest_publication
{
	unsigned long session_id = 0;
	quest_mobile_native_image native;
	std::vector<item_ownership_runtime_entry> custody;
	uint64_t from_owner_revision = 0;
	uint64_t to_owner_revision = 0;
	// Actual locked final-giver owner, including an empty player forest.
	uint64_t player_owner_revision = 0;
	uint64_t acknowledged_save_revision = 0;
};

// Caller owns reconnect-disabled IN_TRANS and must separately authenticate the
// exact retained inbox/root/source/reward/outbox/result on this same session.
// Lock native lifetime before player save fence, owner revisions and complete
// ascending current native/player/selected UID custody. Compare full current
// native image and player runtime image to original immutable body values;
// success selects AFTER, authentic rejection selects BEFORE. Caller-supplied
// values alone are never proof. Return current authenticated cash/reference,
// custody and revisions only after final original-session check; output stays
// unchanged on error. Never starts/ends/reconnects or writes SQL/world/ACK.
// Missing original bodies, cash-v2 authority or exact stock/reference refuses;
// never-admitted and absent-row birth are not admitted by this interface.
unsigned int economic_sql_native_quest_lock_publication(
	MYSQL *, const critical_command &, const critical_completion &,
	std::span<const player_item_snapshot> original_native_before,
	std::span<const player_item_snapshot> original_player_before,
	std::span<const player_item_snapshot> original_player_after,
	uint64_t acknowledged_save_revision, economic_sql_native_quest_publication *) noexcept;

struct critical_native_recovery_envelope;
// Phase2 continuation readback only. Retained values select the original locked
// projection; the actual repository receipt is then authenticated on that same
// session before any output. No completion delivery, new hold, execution or ACK
// is manufactured. Caller still owes full world proof and confirmed rollback.
unsigned int economic_sql_native_quest_lock_recovered_continuation(
	MYSQL *, const critical_native_recovery_envelope &,
	std::span<const player_item_snapshot> original_native_before,
	std::span<const player_item_snapshot> original_player_before,
	std::span<const player_item_snapshot> original_player_after,
	uint64_t acknowledged_save_revision, economic_sql_native_quest_publication *) noexcept;

#endif
