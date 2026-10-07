#ifndef ITEM_TRANSFER_REPOSITORY_H
#define ITEM_TRANSFER_REPOSITORY_H

#include "item/item_transfer_command.h"
#include "item/economic_accounting_item_reference.h"
#include "economy/economic_accounting_types.h"

#include <mysql/mysql.h>
#include "player/player_snapshot.h"
#include <span>

// Only an enclosing, admitted accounting owner may supply this context. A
// schema-v1 inbox ID alone is not admission. The enclosing owner inserts and
// verifies references after its root operation; nullptr selects the legacy writer.
struct item_transfer_accounting_context
{
	critical_operation_id root_operation_id = {};
	uint16_t child_index = 0;
	uint16_t line_index_base = 0;
};

// Locked custody evidence captured by a successful item transfer. The source
// and target roots are included as witnesses so the accounting plan can prove
// that moved subtrees remain well-formed across the ownership change.
struct item_transfer_custody_delta
{
	std::vector<economic_item_snapshot> before;
	std::vector<economic_item_snapshot> after;
	std::vector<economic_item_event> events;
};

// Typed admitted HRT execution only: called after participating owner locks
// and revision checks, before selected custody/physical reads or mutation.
// The callback returns an errno value; nonzero follows existing SQL rollback.
// Values/opaque context alone grant no admission/publication/ACK authority.
using item_transfer_source_custody_hook = unsigned int (*)(MYSQL *, const critical_command &,
							   uint64_t locked_from_revision,
							   uint64_t locked_to_revision,
							   void *original_owner_context) noexcept;

bool item_transfer_repository_execute(
	MYSQL *connection, const critical_command &command, item_transfer_result *result,
	unsigned int *result_code, bool *mutation_applied,
	item_transfer_failure_stage *failure_stage = nullptr,
	const item_transfer_accounting_context *accounting_context = nullptr,
	item_transfer_custody_delta *custody_delta = nullptr,
	item_transfer_source_custody_hook source_custody_hook = nullptr,
	void *original_owner_context = nullptr);
// Compound commands may use one inbox operation for consecutive transfer
// segments. The offset keeps their item-ledger event indexes disjoint.
bool item_transfer_repository_execute_at_offset(
	MYSQL *connection, const critical_command &command, uint16_t event_index_base,
	item_transfer_result *result, unsigned int *result_code, bool *mutation_applied,
	item_transfer_failure_stage *failure_stage = nullptr,
	const item_transfer_accounting_context *accounting_context = nullptr,
	item_transfer_custody_delta *custody_delta = nullptr,
	item_transfer_source_custody_hook source_custody_hook = nullptr,
	void *original_owner_context = nullptr);
// Called only inside the enclosing coin transaction; never commits independently.
bool item_transfer_repository_execute_coin(
	MYSQL *connection, const critical_command &command, const std::array<int32_t, 4> &before,
	item_transfer_result *result, unsigned int *result_code, bool *mutation_applied,
	item_transfer_failure_stage *failure_stage = nullptr,
	const item_transfer_accounting_context *accounting_context = nullptr);
// Transaction-scoped owner primitives used by compound authority commands.
// Callers must acquire every participating owner in canonical identity order.
bool item_transfer_repository_ensure_owner(MYSQL *connection, const item_owner_identity &owner);
bool item_transfer_repository_lock_owner(MYSQL *connection, const item_owner_identity &owner,
					 uint64_t *revision);
bool item_transfer_repository_advance_owner(MYSQL *connection, const item_owner_identity &owner,
					    uint64_t prior_revision);
bool item_transfer_repository_destroy_owners(MYSQL *connection, const item_owner_identity *owners,
					     size_t owner_count);
// Retire selected container/item roots while preserving their contents with the
// current owner.  The caller owns the enclosing SQL transaction and its physical
// projection changes; this function advances authoritative custody and ledger rows.
bool item_transfer_repository_revoke_roots_preserving_children(MYSQL *connection,
							       const uint64_t *item_uids,
							       size_t item_count);

struct quest_mobile_native_image;
// Explicit participant only; never selected by generic/legacy execution.
// The root authenticates original admission/source and owns the caller's
// reconnect-disabled transaction, identity/exclusion locks, reward obligation,
// evidence, commit and publication. Lock the native image before owner/custody.
// On true with result_code!=0 no mutation occurred. Any false after
// mutation_applied=true requires rollback/retirement of the ORIGINAL transaction.
// original_player_items is the complete held EQ/INV preimage for acceptance;
// consumption supplies an empty span. No birth/adoption or owner creation.
bool item_transfer_repository_execute_native_mobile(
	MYSQL *, const critical_command &,
	std::span<const player_item_snapshot> original_player_items, item_transfer_result *,
	unsigned int *result_code, bool *mutation_applied, item_transfer_custody_delta *,
	quest_mobile_native_image *after);

// Separate explicit v16 zero-item participant, inside the SAME original root.
// The parent locks/authenticates both economic mapping lifetimes and admission,
// records the two finite-wallet effects/postings, inbox/outbox and result together.
// This participant proves the original historical native origin, current native
// image, player save/wallet and complete unchanged forests before any DML. It
// changes only the real player wallet and native cash/image, never custody.
// Any false (including after a DML attempt) REQUIRES rollback of the original
// transaction; unchanged outputs never imply no write was attempted. All output
// values stay unchanged on false. True supplies authenticated applied AFTER only.
bool item_transfer_repository_execute_native_mobile_money(
	MYSQL *, const critical_command &,
	std::span<const player_item_snapshot> original_player_items,
	item_native_mobile_money_result *, unsigned int *result_code, bool *mutation_applied,
	quest_mobile_native_image *after) noexcept;

// Separate authentic v14 NQF2 fee participant. Original caller first proves
// actual action/acceptance carriers and source, locks native economic mapping,
// and owns financial sink44 postings/root/result/outbox in this transaction.
// Only native cash/mobile revision mutates. Giver wallet/stock/custody/full
// forests stay unchanged. Any false requires original-session rollback; strong
// outputs do not establish that no DML was attempted. No commit/retry/ACK.
bool item_transfer_repository_execute_native_mobile_fee(
	MYSQL *, const critical_command &,
	std::span<const player_item_snapshot> original_player_items,
	item_native_mobile_fee_result *, unsigned int *result_code, bool *mutation_applied,
	quest_mobile_native_image *after) noexcept;

// Read-only complete globally sorted publication cut. Caller already owns the
// original native lifetime, final-giver save fence and sorted revision locks.
// Explicit immutable UIDs ignore current owner/state/coin policy; active foreign
// root/parent claims and malformed contexts join the same bounded cut. No write,
// owner adoption, inbox lock, transaction or ACK authority; failure preserves output.
struct item_native_quest_publication_custody
{
	economic_item_snapshot snapshot;
	int32_t vnum = 0;
};
bool item_transfer_repository_lock_native_publication_custody(
	MYSQL *, const item_transfer_payload &,
	std::span<const player_item_snapshot> original_native_before,
	std::span<const player_item_snapshot> original_player_before,
	std::span<const player_item_snapshot> original_player_after,
	std::vector<item_native_quest_publication_custody> *) noexcept;

struct held_retirement_recovery;
// HRT-only complete custody observation. Caller must authenticate the exact
// original root/receipt/source and guarded pipeline capture, lock the selected
// native lifetime/save fences and every participating owner in canonical order,
// then call this BEFORE selected custody/physical locks or any mutation.
// Original ordinary v9/10 kind8 command + complete guarded HRT1 BEFORE/AFTER
// forest binds the single ITEM_PICK/HOLD source. Selected tombstone/other owner
// and active foreign root/parent links join the same ascending global UID cut.
// No public generic shape bypass, source write, transaction, adoption or ACK
// capability; output is unchanged on failure. Caller owns rollback and current
// receipt/literal/hold qualification after inspecting this read-only cut.
// Malformed player contexts remain visible for exact-forest refusal. This value
// has no coin-sidecar field; the root must prove actual physical sidecar shape.
bool item_transfer_repository_lock_held_retirement_publication_custody(
	MYSQL *, const critical_command &original, const held_retirement_recovery &captured,
	std::vector<item_native_quest_publication_custody> *) noexcept;

#endif
