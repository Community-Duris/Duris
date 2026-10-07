#ifndef DURIS_ECONOMIC_SQL_ITEM_TRANSFER_TRANSACTION_H
#define DURIS_ECONOMIC_SQL_ITEM_TRANSFER_TRANSACTION_H

#include "economy/item_transfer_accounting.h"
#include "item/item_transfer_repository.h"
#include "persistence/economic_accounting_repository.h"
#include "item/item_ownership_runtime.h"
#include "world/quest_mobile_native.h"
#include "persistence/economic_sql_native_mobile_birth_transaction.h"
#include "item/held_retirement_recovery.h"

#include <mysql/mysql.h>

#include <cstddef>
#include <cstdint>
#include <optional>

struct economic_sql_item_transfer_context
{
	economic_sql_authority_snapshot authority;
	unsigned long session_id = 0;
	// Actual original pipeline values copied before the first SQL read. These
	// values and readiness are worker evidence, never publication/ACK authority.
	std::optional<held_retirement_recovery> held_retirement;
	std::vector<uint8_t> original_held_command;
	bool held_source_custody_locked = false;
};

// Lock and validate the active lineage epoch before any item rows are changed.
// The caller's transaction owns this shared lock through commit or rollback.
unsigned int economic_sql_item_transfer_lock(MYSQL *connection, const critical_command &command,
					     economic_sql_item_transfer_context *context);

// Execution seam: caller already ensured/locked the participating owner rows
// in canonical order and checked these revisions, but has not read selected
// custody or physical rows. Never creates owners or introduces later locks.
unsigned int economic_sql_held_retirement_lock_source_custody(
	MYSQL *, const critical_command &, uint64_t locked_from_revision,
	uint64_t locked_to_revision, economic_sql_item_transfer_context *) noexcept;

// Existing executor callback adapter. Opaque value is the exact original
// economic_sql_item_transfer_context owned by the enclosing admitted SQL root.
unsigned int economic_sql_held_retirement_source_custody_hook(
	MYSQL *, const critical_command &, uint64_t locked_from_revision,
	uint64_t locked_to_revision, void *original_owner_context) noexcept;

// Read-only original save fence, before participating owner/custody locks.
// Values must come from the actual guarded original pipeline or retained HRT1.
// The caller separately authenticates original receipt/root/source/current
// physical forest and owns one reconnect-disabled transaction and rollback.
unsigned int
economic_sql_held_retirement_lock_saved_revision(MYSQL *, const critical_command &,
						 const held_retirement_recovery &,
						 uint64_t *observed_save_revision) noexcept;
// After complete custody locks, check selected UID in the eight original
// physical domains. AFTER requires absence everywhere; BEFORE exactly one
// player row. This is no literal/sidecar/census/publication/ACK capability.
unsigned int economic_sql_held_retirement_lock_selected_domains(MYSQL *, const critical_command &,
								const held_retirement_recovery &,
								bool original_after) noexcept;

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

// Dedicated zero-item v16 money root participants. The caller retains the
// original reconnect-disabled transaction, typed inbox/outbox/result, commit,
// physical publication and guarded ACK. These functions never commit/retry.
struct economic_sql_native_money_context
{
	economic_sql_native_quest_context original;
	economic_account_key player_wallet, native_wallet;
};
unsigned int economic_sql_native_money_lock(MYSQL *, const critical_command &,
					    economic_sql_native_money_context *) noexcept;
unsigned int economic_sql_native_money_record(MYSQL *, const critical_command &,
					      unsigned int result_code, bool mutation_applied,
					      const economic_sql_native_money_context &) noexcept;
unsigned int
economic_sql_native_money_verify_retained(MYSQL *, const critical_command &,
					  unsigned int result_code,
					  std::span<const uint8_t> result_payload) noexcept;

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

// Shared repository owner authenticates original inbox/root/outbox before
// current participant locks. Success item_transfer outbox remains event_index0,
// destination4/event_type1/payload_version1; failure has no outbox. No ACK.
unsigned int
economic_sql_native_money_verify_receipt_in_transaction(MYSQL *, const critical_command &,
							const critical_completion &) noexcept;
// Primary-owned historical proof, before any current fee participant locks.
// Requires the actual original acceptance command plus its exact typed48
// successful receipt. This is not receipt-only authority or a command lookup.
// Same reconnect-disabled IN_TRANS session; no writes, commit, replay or ACK.
unsigned int economic_sql_native_fee_verify_acceptance_in_transaction(
	MYSQL *, const critical_command &actual_fee_action,
	const critical_command &original_acceptance,
	std::span<const uint8_t> original_typed48_result) noexcept;

// Actual action command comes from the original retained child carrier. Caller
// also supplies literal stored continuation; a hash-only inbox is not a carrier.
unsigned int economic_sql_native_fee_verify_obligation_in_transaction(
	MYSQL *, const critical_command &actual_fee_action,
	std::span<const uint8_t> literal_continuation) noexcept;

// Root-owned fee receipt proof must precede current participant locks. Exact
// NFR1 result/source/acceptance/root/outbox, no receipt-only authority or ACK.
unsigned int
economic_sql_native_fee_verify_receipt_in_transaction(MYSQL *, const critical_command &,
						      const critical_completion &) noexcept;
struct economic_sql_native_fee_publication
{
	economic_sql_native_quest_publication original;
	native_mobile_wallet_origin origin;
};
unsigned int economic_sql_native_fee_lock_publication(
	MYSQL *, const critical_command &, const critical_completion &,
	std::span<const player_item_snapshot> original_native_before,
	std::span<const player_item_snapshot> original_player_before,
	std::span<const player_item_snapshot> original_player_after,
	uint64_t acknowledged_save_revision, economic_sql_native_fee_publication *) noexcept;

struct economic_sql_native_money_publication
{
	economic_sql_native_quest_publication original;
	currency_vector player_cash{};
	uint64_t player_wallet_revision = 0;
	native_mobile_wallet_origin origin;
};
unsigned int economic_sql_native_money_lock_publication(
	MYSQL *, const critical_command &, const critical_completion &,
	std::span<const player_item_snapshot> original_native_before,
	std::span<const player_item_snapshot> original_player_before,
	std::span<const player_item_snapshot> original_player_after,
	uint64_t acknowledged_save_revision, economic_sql_native_money_publication *) noexcept;

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

// Dedicated NQF2 original financial participant. Caller proves the genuine
// retained acceptance/child carriers before current participant locks and owns
// the one reconnect-disabled original transaction, native fee execution,
// inbox/reward/outbox/result and commit. Generic admission remains closed.
struct economic_sql_native_fee_context
{
	economic_sql_native_quest_context original;
	economic_account_key native_wallet;
};
unsigned int economic_sql_native_fee_lock(MYSQL *, const critical_command &,
					  economic_sql_native_fee_context *) noexcept;
unsigned int economic_sql_native_fee_record(MYSQL *, const critical_command &,
					    unsigned int result_code, bool mutation_applied,
					    const economic_sql_native_fee_context &) noexcept;
unsigned int economic_sql_native_fee_verify_retained(MYSQL *, const critical_command &,
						     unsigned int result_code,
						     std::span<const uint8_t>) noexcept;

#endif
