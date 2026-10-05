#ifndef DURIS_ECONOMIC_SQL_SHOP_TRADE_TRANSACTION_H
#define DURIS_ECONOMIC_SQL_SHOP_TRADE_TRANSACTION_H

#include "economy/shop_trade_accounting.h"
#include "persistence/economic_accounting_repository.h"
#include "persistence/critical_command_completion.h"
#include "persistence/shop_item_runtime_payload.h"

struct economic_sql_shop_trade_context
{
	economic_sql_authority_snapshot authority;
	shop_trade_accounting_authority before;
	uint32_t bank_id = 0;
	uint32_t keeper_id = 0;
	unsigned long session_id = 0;
	// Original v8 UID/gap-lock scope, including absent future production.
	// Values only; held original transaction supplies the actual lock lifetime.
	std::vector<uint64_t> recovery_custody_uids;
};

// Standalone inactive shop capability. The caller owns an open inbox
// transaction. v5 locks three ordinary money lifetimes; v6 locks two (wallet/
// bank) and keeps the decoded virtual keeper key without a native mapping.
// Lock native keeper cash and wallet rows, ownership and physical inventory
// rows for the item tree, and owner revisions.
// v8 first locks all four owner identities and the complete sorted manifest/
// current custody cut, then authenticates full native BEFORE values. AFTER
// verification runs after mutation inside this same original parent transaction.
// The locks and this context expire at the caller's commit or rollback. The
// gameplay shop configuration still needs a durable witness before activation.
unsigned int economic_sql_shop_trade_lock(MYSQL *connection, const critical_command &command,
					  economic_sql_shop_trade_context *context);

// Apply one admitted trade and record its canonical accounting root inside the
// caller's open inbox transaction. The caller writes the result receipt and
// outbox before commit; this function never commits.
unsigned int
economic_sql_shop_trade_execute_and_record(MYSQL *connection, const critical_command &command,
					   const economic_sql_shop_trade_context &context,
					   shop_trade_result *result, unsigned int *result_code,
					   bool *mutation_applied);

// Verify the retained canonical root, postings, item references, and source
// claim on operation-ID replay. The caller verifies the inbox receipt/outbox.
unsigned int economic_sql_shop_trade_verify_retained(MYSQL *connection,
						     const critical_command &command,
						     unsigned int result_code,
						     const uint8_t *result_payload,
						     size_t result_size);

#ifndef __NO_MYSQL__
// Values only. Neither this image nor its revisions grant publication or ACK.
struct economic_sql_shop_trade_publication
{
	economic_sql_authority_snapshot authority;
	unsigned long session_id = 0;
	uint32_t bank_id = 0, keeper_id = 0;
	uint64_t player_save_revision = 0;
	uint32_t player_level = 0;
	currency_vector wallet{}, bank{};
	uint64_t wallet_revision = 0, bank_revision = 0;
	int64_t keeper_cash = 0;
	uint64_t shop_revision = 0;
	bool keeper_roaming = false, payload_checkpoint_recorded = false, rejected = false;
	uint64_t payload_checkpoint_revision = 0;
	// Result's primary/counterparty identities (discard/production differ).
	uint64_t player_owner_revision = 0, counterparty_owner_revision = 0;
	uint64_t wallet_owner_revision = 0, keeper_owner_revision = 0;
	// Sorted whole ordinary player + keeper custody and original references.
	std::vector<economic_item_snapshot> custody;
	std::vector<int32_t> custody_vnums;
	shop_item_runtime_image keeper_items;
	// Independent complete player EQ/INV proof against the holder-supplied
	// expected current forest; selected-tree proof below remains separate.
	shop_item_runtime_image whole_player_items;
	// Selected and target ancestors only, NOT a whole-player inventory image.
	shop_item_runtime_image player_items;
};
#else
struct economic_sql_shop_trade_publication;
#endif

// Read-only CURRENT v6 projection in the caller's reconnect-disabled original
// transaction. Locks authority -> player/status/bank -> keeper -> sorted owner
// revisions -> sorted custody -> native physical/payloads. It never calls the
// precommit source lock/apply, creates rows, commits or grants ACK. Caller must
// subsequently verify the exact historical inbox/root/outbox and keep the cut
// through native publication and confirmed cleanup. The original holder supplies
// the exact whole current player forest: frozen AFTER state on success, unchanged
// original on rejection. It is comparison input, not independent authority.
// Ordinary inline coin custody stays separate; every physical player row counts.
// Failure preserves output.
unsigned int economic_sql_shop_trade_lock_publication(
	MYSQL *, const critical_command &, const critical_completion &,
	std::span<const player_item_snapshot> expected_current_player_items,
	economic_sql_shop_trade_publication *) noexcept;

// Cold v8 counterpart: exact retained manifest UIDs supply order, while native
// custody/physical rows/sidecars supply values. Both full forests authenticate
// the canonical receipt's BEFORE (rejection) or AFTER (success) phase. No player
// token or fabricated expected body is accepted. Read-only original transaction
// only; caller must still verify the historical inbox/root/outbox and own the
// exact retained native recovery/publication lifetime. This does not manufacture
// that owner, mutate a world body, reconstruct live NORENT witnesses or grant ACK.
// No-manifest/older commands refuse; legacy expected-body overload remains.
unsigned int
economic_sql_shop_trade_lock_publication(MYSQL *, const critical_command &,
					 const critical_completion &,
					 economic_sql_shop_trade_publication *) noexcept;

#endif
