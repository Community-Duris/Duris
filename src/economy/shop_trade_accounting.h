#ifndef DURIS_SHOP_TRADE_ACCOUNTING_H
#define DURIS_SHOP_TRADE_ACCOUNTING_H

#include "economy/economic_accounting_intent.h"
#include "economy/shop_trade_command.h"

// Typed capability for the native shop trade transaction; gameplay admission
// and native publication are separate owners. The legacy treasury form remains
// available only for exact retained/inactive compatibility.
constexpr uint32_t ECONOMIC_WRITER_SHOP_TRADE = 14;
constexpr uint64_t ECONOMIC_SHOP_UNFUNDED_SALE_ISSUANCE_ID = 25;
// Existing version-1 shop policies already permit these virtual counterparty kinds.
// Their identities are fixed by policy, independent of keeper/shop lifetimes.
constexpr uint64_t ECONOMIC_SHOP_BUY_SINK_ID = 21;
constexpr uint64_t ECONOMIC_SHOP_SELL_ISSUANCE_ID = 22;

struct shop_trade_accounting_authority
{
	critical_operation_id epoch = {};
	economic_account_key wallet_account = {};
	economic_account_key bank_account = {};
	// Legacy treasury or the exact shared sink/issuance derived from this action.
	economic_account_key keeper_account = {};
	currency_command_result balances_before = {};
	uint32_t shop_id = 0;
	int32_t keeper_vnum = 0;
	int64_t keeper_cash_before = 0;
	bool keeper_roaming = false;
	uint64_t shop_revision_before = 0;
	uint64_t player_owner_revision_before = 0;
	uint64_t counterparty_owner_revision_before = 0;
	// The complete selected tree, plus the stocked exemplar and target ancestors
	// when a produced item is placed inside an existing player container.
	std::vector<economic_item_snapshot> items_before;
	// Parallel native VNUM evidence. Absent produced UIDs have VNUM zero.
	std::vector<int32_t> item_vnums_before;
};

// Freeze durable wallet, bank and keeper-treasury mapping IDs before admission.
economic_accounting_error
shop_trade_accounting_intent(const critical_command &command, const critical_operation_id &epoch,
			     const economic_account_key &wallet, const economic_account_key &bank,
			     const economic_account_key &keeper, std::vector<uint8_t> *encoded);

// v6 only: freeze native wallet/bank lifetimes. Shop ID and keeper VNUM remain in
// the immutable command; no keeper treasury mapping or holding is admitted.
// Buy payments use the fixed shop sink; sale proceeds use fixed issuance.
economic_accounting_error shop_trade_shared_accounting_intent(const critical_command &command,
							      const critical_operation_id &epoch,
							      const economic_account_key &wallet,
							      const economic_account_key &bank,
							      std::vector<uint8_t> *encoded);

// Decode only an exact, self-consistent schema-2 shop intent. Outputs are
// unchanged on refusal.
economic_accounting_error
shop_trade_accounting_decode(const critical_command &command, economic_frozen_intent *intent,
			     shop_trade_payload *payload, economic_account_key *wallet,
			     economic_account_key *bank, economic_account_key *keeper);

// Compare the admitted operation, locked native preimages and native result.
// The repository must record this plan with its shop, wallet, item, receipt and
// outbox changes in the same transaction; this pure function does not write.
economic_accounting_error
shop_trade_accounting_plan(const critical_command &command, const economic_frozen_intent &intent,
			   const shop_trade_accounting_authority &authority,
			   const shop_trade_result &result, economic_accounting_plan *plan);

// Full original SHOP schema/metadata/facts/binding and projected-intent proof.
// Outer owns actual input, prior outputs and sibling storage. Private candidates
// are censused at every prospective cut. Outputs stay unchanged on refusal.
economic_accounting_error
shop_trade_accounting_decode_bounded(const critical_command &, economic_frozen_intent *,
				     shop_trade_payload *, economic_account_key *,
				     economic_account_key *, economic_account_key *,
				     bool (*reserve_scratch_peak)(size_t, void *) noexcept,
				     void *context, size_t outer_live_scratch) noexcept;
// Allocation-free CURRENT returned-value heap observers. Account-key fields
// are entirely inline; these counts exclude the caller-owned inline objects.
bool shop_trade_accounting_intent_current_heap_bytes(const economic_frozen_intent &,
						     size_t *) noexcept;
bool shop_trade_accounting_payload_current_heap_bytes(const shop_trade_payload &,
						      size_t *) noexcept;
bool shop_trade_accounting_account_current_heap_bytes(const economic_account_key &,
						      size_t *) noexcept;
size_t shop_trade_accounting_decoded_heap_observer_frame_bytes() noexcept;

#endif
