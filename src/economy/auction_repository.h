#ifndef AUCTION_REPOSITORY_H
#define AUCTION_REPOSITORY_H

#include "economy/auction_command.h"
#include "player/player_snapshot_codec.h"
#include <span>

#include <mysql/mysql.h>
#include "economy/economic_accounting_types.h"

bool auction_repository_execute(MYSQL *connection, const critical_command &command,
				auction_command_result *result, unsigned int *result_code,
				bool *mutation_applied);

// Only schema-2 auction components may call this entry point, inside their
// already-locked inbox transaction. The caller must record the matching EAP1
// root before writing the receipt and committing. Other actions refuse.
bool auction_repository_execute_accounted(MYSQL *connection, const critical_command &command,
					  auction_command_result *result, unsigned int *result_code,
					  bool *mutation_applied);

// Borrowed original schema2/v2 participant only. Caller owns original inbox,
// global custody authority and frozen all-node before validation. No transaction
// lifecycle or publication authority. False requires caller rollback; outputs
// stay unchanged. Native events cover complete ordered trees; receipts roots only.
bool auction_repository_execute_accounted_native(
	MYSQL *, const critical_command &, std::span<const player_item_snapshot> original_selected,
	auction_command_result *, unsigned int *, bool *);

// Borrowed accepted native non-item pre-effect cut. Authenticates original source,
// current whole-player level/save/order/digests and all frozen custody fences.
// Never changes the command or executes native effects; caller retains transaction.
unsigned int auction_repository_validate_accounted_native_cut(MYSQL *,
							      const critical_command &) noexcept;

// Pure ACT2 values decoder, not listing/inbox/source entitlement authentication.
// Complete one-root literal tree and immutable listing-after revision vector.
// Both outputs stay unchanged on refusal. No native materialization or SQL.
bool auction_repository_decode_native_tree_blob(std::span<const uint8_t>,
						std::vector<player_item_snapshot> *,
						std::vector<uint64_t> *) noexcept;

// Borrowed historical ACT2 selected source, authenticated against original EAI,
// EAP/native listing receipts and genuine locked current claim entitlement.
// Fresh schema1/v1 request may obtain source before bind; accepted schema2/v2
// additionally compares exact frozen entitlement and selected digest/fences. No header
// reconstruction, materialization or transaction lifecycle. Strong output.
unsigned int auction_repository_read_original_native_selected(
	MYSQL *, const critical_command &original_claim,
	const critical_operation_id &original_listing_operation,
	std::vector<player_item_snapshot> *selected) noexcept;

// Fresh bid/settlement producer capture under the existing trusted native SQL
// caller's transaction with reconnect disabled. No mutation, admission, inbox,
// identity reservation, or transaction management. Caller supplies its qualified
// lineage/epoch; mappings and all absence facts come from native SQL. On failure
// roll back; command stays unchanged. Schema-2 replay cannot enter this function.
unsigned int auction_repository_prepare_accounting(MYSQL *, const critical_operation_id &lineage,
						   const critical_operation_id &epoch,
						   critical_command *);

// Canonical original typed command only. No current lineage/epoch/mapping read.
// Canonical typed classification for bid/settlement/listing/item pickup/money pickup.
// Classification grants no actor, publication, checkpoint or execution authority.
bool auction_repository_frozen_accounting_valid(const critical_command &) noexcept;

struct auction_accounting_endpoint_readback
{
	economic_account_key bidder_claim{}, previous_claim{}, seller_claim{};
	bool unused_bidder = false;
};
// Read original committed AEC1 creator proof using the complete retained command.
// Resolved keys are observations; the original command/tags/zero IDs stay exact.
// Authenticated unused bidder ENODATA is retained as unused_bidder with an empty
// key, including after a later genuine endpoint. No tagged role returns ENODATA
// without claiming mapped-only receipt proof. Output stays unchanged on failure.
unsigned int auction_repository_readback_endpoints(MYSQL *, const critical_command &,
						   auction_accounting_endpoint_readback *);

#endif
