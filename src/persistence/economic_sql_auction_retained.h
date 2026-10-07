#ifndef DURIS_ECONOMIC_SQL_AUCTION_RETAINED_H
#define DURIS_ECONOMIC_SQL_AUCTION_RETAINED_H
#include "persistence/economic_accounting_repository.h"
#include "player/player_snapshot_codec.h"
// Borrowed trusted transaction; compares immutable original receipt and accounting.
// Never starts/commits a transaction, executes gameplay or captures current balances.
// Includes listing and item/money collection using original EAP/native receipts.
// Later source consumption, mapping retirement and epoch retirement are historical
// facts; current balances/availability never reconstruct this original command.
unsigned int economic_sql_auction_verify_retained(MYSQL *, const critical_command &,
						  uint32_t result_code, critical_failure_stage,
						  uint64_t durable_revision,
						  std::span<const uint8_t> result);
// Borrowed original source observation for accepted native bid/finalize/remove.
// Authenticates original listing receipt/ANF2/ACT2. Caller compares current fences. No claim
// entitlement reconstruction, command mutation, native effects or transaction lifecycle.
struct economic_sql_auction_native_listing_source
{
	uint32_t auction_id = 0;
	std::vector<economic_item_snapshot> items;
	// Empty only for historical v1 root-only receipts; never fabricated literals.
	std::vector<player_item_snapshot> literals;
};
unsigned int economic_sql_auction_read_native_listing_source(
	MYSQL *, const critical_command &, economic_sql_auction_native_listing_source *) noexcept;
// Genuine preparation observation from structural schema1/v2 request and supplied
// lineage/epoch. No fabricated accepted EAI/header; same immutable listing proof.
unsigned int economic_sql_auction_capture_native_listing_source(
	MYSQL *, const critical_command &, const critical_operation_id &lineage,
	const critical_operation_id &epoch, economic_sql_auction_native_listing_source *) noexcept;

// Borrowed proof for an actual stored native v2 bid/settlement creator.
// Reuses immutable canonical root/receipt/source and original listing ANF2/ACT2
// validators. No unseen original envelope, active authority or current balance
// is reconstructed; no transaction lifecycle or gameplay effects.
unsigned int
economic_sql_auction_verify_known_native_creator(MYSQL *, const critical_operation_id &) noexcept;

#endif
