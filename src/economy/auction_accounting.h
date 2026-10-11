#ifndef DURIS_AUCTION_ACCOUNTING_H
#define DURIS_AUCTION_ACCOUNTING_H

#include "economy/auction_command.h"
#include "economy/economic_accounting_intent.h"

// Inactive typed capability for an auction bid, including outbid and buy-now.
constexpr uint32_t ECONOMIC_WRITER_AUCTION_BID = 9;
constexpr uint64_t ECONOMIC_AUCTION_CLOSING_FEE_SINK_ID = 25;
constexpr uint64_t ECONOMIC_AUCTION_LISTING_FEE_SINK_ID = 26;

struct auction_bid_accounting_listing
{
	uint32_t auction_id = 0;
	uint32_t seller_pid = 0;
	uint32_t winning_bidder_pid = 0;
	uint32_t status = 0;
	uint32_t custody_state = 0;
	int64_t current_price = 0;
	int64_t buy_price = 0;
	uint64_t revision = 0;
	critical_operation_id listing_operation = {};
	critical_operation_id previous_bid_operation = {};
};

struct auction_bid_accounting_accounts
{
	economic_account_key wallet = {};
	economic_account_key bank = {};
	economic_account_key escrow = {};
	economic_account_key bidder_claim = {};
	economic_account_key previous_claim = {};
	economic_account_key seller_claim = {};
	// AEC1 freezes an absent native endpoint by beneficiary, never a guessed mapping.
	uint32_t absent_bidder_pid = 0, absent_previous_pid = 0, absent_seller_pid = 0;
};

struct auction_bid_accounting_claim
{
	int64_t money = 0;
	uint64_t revision = 0;
};

struct auction_bid_accounting_authority
{
	critical_operation_id epoch = {};
	auction_bid_accounting_listing listing = {};
	auction_bid_accounting_accounts accounts = {};
	currency_command_result balances_before = {};
	auction_bid_accounting_claim bidder_claim_before = {};
	auction_bid_accounting_claim previous_claim_before = {};
	auction_bid_accounting_claim seller_claim_before = {};
};

// The listing and account mapping identities are fixed before admission.
economic_accounting_error
auction_bid_accounting_intent(const critical_command &command, const critical_operation_id &epoch,
			      const auction_bid_accounting_listing &listing,
			      const auction_bid_accounting_accounts &accounts,
			      std::vector<uint8_t> *encoded);

// Decode and verify the complete canonical frozen bid. Outputs stay unchanged
// on failure; the repository must still renew the mapped lifetimes.
economic_accounting_error auction_bid_accounting_decode(const critical_command &command,
							economic_frozen_intent *intent,
							auction_command_payload *payload,
							auction_bid_accounting_listing *listing,
							auction_bid_accounting_accounts *accounts);

// Pure comparison against repository-locked state and its native result.
// The caller records this plan in the same transaction as the bid.
economic_accounting_error
auction_bid_accounting_plan(const critical_command &command, const economic_frozen_intent &intent,
			    const auction_bid_accounting_authority &authority,
			    const auction_command_result &result, economic_accounting_plan *plan);

// Genuine complete original auction replay companions. Caller owns inputs,
// prior outputs and sibling retained state in outer_live; callback retains the
// admitted simultaneous peak. Semantic laws and strong outputs remain original.
// No writer/source/execution authority; native qualification remains separate.
economic_accounting_error auction_bid_accounting_intent_bounded(
	const critical_command &command, const critical_operation_id &epoch,
	const auction_bid_accounting_listing &listing,
	const auction_bid_accounting_accounts &accounts, std::vector<uint8_t> *encoded,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept;

economic_accounting_error auction_bid_accounting_decode_bounded(
	const critical_command &command, economic_frozen_intent *intent,
	auction_command_payload *payload, auction_bid_accounting_listing *listing,
	auction_bid_accounting_accounts *accounts, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept;

// Additive original algorithm/fixed-proof counterparts.
economic_accounting_error auction_bid_accounting_intent_fixed_bounded(
	const critical_command &command, const critical_operation_id &epoch,
	const auction_bid_accounting_listing &listing,
	const auction_bid_accounting_accounts &accounts, std::vector<uint8_t> *encoded,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept;
economic_accounting_error auction_bid_accounting_decode_fixed_bounded(
	const critical_command &command, economic_frozen_intent *intent,
	auction_command_payload *payload, auction_bid_accounting_listing *listing,
	auction_bid_accounting_accounts *accounts, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept;

// Exact fixed-companion own entry profiles; full lower join remains pending review.
bool auction_bid_accounting_intent_own_source_frame_bytes(size_t *) noexcept;
bool auction_bid_accounting_intent_initial_inline_bytes(size_t *) noexcept;
constexpr size_t auction_bid_accounting_intent_own_source_query_frame_bytes() noexcept
{
	// own output/result/policy; initial getter has the same sequential graph.
	return sizeof(size_t *) + 2 * sizeof(bool);
}
bool auction_bid_accounting_decode_own_source_frame_bytes(size_t *) noexcept;
bool auction_bid_accounting_decode_initial_inline_bytes(size_t *) noexcept;
constexpr size_t auction_bid_accounting_decode_own_source_query_frame_bytes() noexcept
{
	// Source getter(output,bool), observed/total locals, policy bool,
	// actual current observer getter(output,bool), copy/valid returnN,
	// checked_add(ref,value,bool). Initial query same simpler graph.
	return 3 * sizeof(void *) + 7 * sizeof(size_t) + 4 * sizeof(bool);
}

bool auction_bid_accounting_intent_source_frame_bytes(size_t *) noexcept;
bool auction_bid_accounting_intent_source_supplement_frame_bytes(size_t *) noexcept;
constexpr size_t auction_bid_accounting_intent_source_query_frame_bytes() noexcept
{
	return auction_bid_accounting_intent_own_source_query_frame_bytes() +
	       auction_command_decode_payload_source_query_frame_bytes() +
	       economic_intent_freeze_fixed_source_query_frame_bytes() + 2 * sizeof(void *) +
	       5 * sizeof(size_t) + 4 * sizeof(bool);
}
bool auction_bid_accounting_decode_source_frame_bytes(size_t *) noexcept;
bool auction_bid_accounting_decode_source_supplement_frame_bytes(size_t *) noexcept;
constexpr size_t auction_bid_accounting_decode_source_query_frame_bytes() noexcept
{
	return auction_bid_accounting_decode_own_source_query_frame_bytes() +
	       auction_command_decode_payload_source_query_frame_bytes() +
	       economic_intent_decode_source_query_frame_bytes() +
	       economic_intent_verify_binding_fixed_source_query_frame_bytes() +
	       auction_bid_accounting_intent_source_query_frame_bytes() + 2 * sizeof(void *) +
	       7 * sizeof(size_t) + 6 * sizeof(bool);
}

#endif
