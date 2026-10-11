#ifndef DURIS_AUCTION_SETTLEMENT_ACCOUNTING_H
#define DURIS_AUCTION_SETTLEMENT_ACCOUNTING_H

#include "economy/auction_command.h"
#include "economy/economic_accounting_intent.h"

constexpr uint32_t ECONOMIC_WRITER_AUCTION_SETTLEMENT = 11;

struct auction_settlement_item
{
	uint64_t uid = 0;
	uint64_t revision = 0;
	uint16_t slot = 0;
	int32_t vnum = 0;
	uint32_t claim_pid = 0;
	bool claimed = false;
};

struct auction_settlement_listing
{
	uint32_t auction_id = 0;
	uint32_t seller_pid = 0;
	uint32_t winner_pid = 0;
	uint32_t status = 0;
	uint32_t custody_state = 0;
	uint32_t quantity = 0;
	int64_t current_price = 0;
	int64_t buy_price = 0;
	uint64_t revision = 0;
	uint64_t end_time = 0;
	critical_operation_id listing_operation = {};
	critical_operation_id winning_bid_operation = {};
	uint16_t item_count = 0;
	std::array<auction_settlement_item, AUCTION_COMMAND_MAX_ITEMS> items = {};
};

struct auction_settlement_accounts
{
	economic_account_key escrow = {};
	economic_account_key seller_claim = {};
	economic_account_key actor_wallet = {};
	economic_account_key actor_bank = {};
	uint32_t absent_seller_pid = 0;
};

struct auction_settlement_authority
{
	critical_operation_id epoch = {};
	auction_settlement_listing listing = {};
	auction_settlement_accounts accounts = {};
	currency_command_result actor_balances_before = {};
	int64_t seller_claim_before = 0;
	uint64_t seller_claim_revision_before = 0;
	int64_t seller_claim_after = 0;
	uint64_t seller_claim_revision_after = 0;
	std::vector<economic_item_snapshot> items_before;
	std::vector<uint32_t> claim_pids_after;
};

// Closure freezes the exact auction, escrow lifetime, staged item UIDs and
// winning bid source before running the native finalize/remove operation.
economic_accounting_error auction_settlement_accounting_intent(
	const critical_command &command, const critical_operation_id &epoch,
	const auction_settlement_listing &listing, const auction_settlement_accounts &accounts,
	std::vector<uint8_t> *encoded);

// Decode and verify the complete canonical frozen settlement. Outputs stay
// unchanged on failure; the repository must still renew the mapped lifetimes.
economic_accounting_error auction_settlement_accounting_decode(
	const critical_command &command, economic_frozen_intent *intent,
	auction_command_payload *payload, auction_settlement_listing *listing,
	auction_settlement_accounts *accounts);

// A timed sale spends existing escrow into seller claim and fee sink. Removal
// advances the escrow witness without reimbursing the prior winner. Item
// custody is unchanged; claim rights are checked against native after-state.
economic_accounting_error auction_settlement_accounting_plan(
	const critical_command &command, const economic_frozen_intent &intent,
	const auction_settlement_authority &authority, const auction_command_result &result,
	economic_accounting_plan *plan);

// Genuine complete original auction replay companions. Caller owns inputs,
// prior outputs and sibling retained state in outer_live; callback retains the
// admitted simultaneous peak. Semantic laws and strong outputs remain original.
// No writer/source/execution authority; native qualification remains separate.
economic_accounting_error auction_settlement_accounting_intent_bounded(
	const critical_command &command, const critical_operation_id &epoch,
	const auction_settlement_listing &listing, const auction_settlement_accounts &accounts,
	std::vector<uint8_t> *encoded, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept;

economic_accounting_error auction_settlement_accounting_decode_bounded(
	const critical_command &command, economic_frozen_intent *intent,
	auction_command_payload *payload, auction_settlement_listing *listing,
	auction_settlement_accounts *accounts, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept;

// Additive original algorithm/fixed-proof counterparts.
economic_accounting_error auction_settlement_accounting_intent_fixed_bounded(
	const critical_command &command, const critical_operation_id &epoch,
	const auction_settlement_listing &listing, const auction_settlement_accounts &accounts,
	std::vector<uint8_t> *encoded, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept;
economic_accounting_error auction_settlement_accounting_decode_fixed_bounded(
	const critical_command &command, economic_frozen_intent *intent,
	auction_command_payload *payload, auction_settlement_listing *listing,
	auction_settlement_accounts *accounts, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept;

// Own Source and actual prospective INLINE for the paired fixed intent.
bool auction_settlement_accounting_intent_own_source_frame_bytes(size_t *) noexcept;
bool auction_settlement_accounting_intent_initial_inline_bytes(size_t *) noexcept;
constexpr size_t auction_settlement_accounting_intent_own_source_query_frame_bytes() noexcept
{
	return sizeof(void *) + 2 * sizeof(bool);
}
bool auction_settlement_accounting_intent_source_frame_bytes(size_t *) noexcept;
bool auction_settlement_accounting_intent_source_supplement_frame_bytes(size_t *) noexcept;
constexpr size_t auction_settlement_accounting_intent_source_query_frame_bytes() noexcept
{
	return auction_settlement_accounting_intent_own_source_query_frame_bytes() +
	       auction_command_decode_payload_source_query_frame_bytes() +
	       economic_intent_freeze_fixed_source_query_frame_bytes() + 2 * sizeof(void *) +
	       5 * sizeof(size_t) + 4 * sizeof(bool);
}

// Own Source and actual prospective INLINE for the paired fixed decode.
bool auction_settlement_accounting_decode_own_source_frame_bytes(size_t *) noexcept;
bool auction_settlement_accounting_decode_initial_inline_bytes(size_t *) noexcept;
constexpr size_t auction_settlement_accounting_decode_own_source_query_frame_bytes() noexcept
{
	return 3 * sizeof(void *) + 7 * sizeof(size_t) + 4 * sizeof(bool);
}
bool auction_settlement_accounting_decode_source_frame_bytes(size_t *) noexcept;
bool auction_settlement_accounting_decode_source_supplement_frame_bytes(size_t *) noexcept;
constexpr size_t auction_settlement_accounting_decode_source_query_frame_bytes() noexcept
{
	return auction_settlement_accounting_decode_own_source_query_frame_bytes() +
	       auction_command_decode_payload_source_query_frame_bytes() +
	       economic_intent_decode_source_query_frame_bytes() +
	       economic_intent_verify_binding_fixed_source_query_frame_bytes() +
	       auction_settlement_accounting_intent_source_query_frame_bytes() +
	       2 * sizeof(void *) + 7 * sizeof(size_t) + 6 * sizeof(bool);
}

#endif
