#include "economy/auction_settlement_accounting.h"

#include <cassert>

namespace
{
critical_operation_id id(uint8_t first)
{
	critical_operation_id value = {};
	value.bytes[0] = first;
	return value;
}

auction_settlement_listing listing()
{
	auction_settlement_listing value;
	value.auction_id = 400;
	value.seller_pid = 10;
	value.winner_pid = 20;
	value.status = 1;
	value.custody_state = 1;
	value.quantity = 2;
	value.current_price = 3000;
	value.buy_price = 5000;
	value.revision = 2;
	value.end_time = 1;
	value.listing_operation = id(1);
	value.winning_bid_operation = id(3);
	value.item_count = 2;
	value.items[0] = { 600, 1, 0, 77, 0, false };
	value.items[1] = { 601, 2, 1, 78, 0, false };
	return value;
}

auction_settlement_accounts accounts(bool sale)
{
	auction_settlement_accounts value;
	value.escrow = { id(9), economic_account_kind::auction_escrow, 100, 0 };
	if (sale)
		value.seller_claim = { id(9), economic_account_kind::pending_claim, 101, 0 };
	return value;
}

auction_settlement_authority authority(const auction_settlement_listing &staged,
				       const auction_settlement_accounts &keys, uint32_t claimant,
				       int64_t claim_after)
{
	auction_settlement_authority value;
	value.epoch = id(2);
	value.listing = staged;
	value.accounts = keys;
	value.seller_claim_after = claim_after;
	value.seller_claim_revision_after = claim_after ? 1 : 0;
	value.items_before = {
		{ 600,
		  { { item_owner_type::auction, 400, 0 }, 600, 0, 1, item_custody_state::active } },
		{ 601,
		  { { item_owner_type::auction, 400, 0 }, 601, 0, 2, item_custody_state::active } }
	};
	value.claim_pids_after = { claimant, claimant };
	return value;
}

critical_command command(auction_action action, uint8_t operation, uint32_t fee,
			 const auction_settlement_listing &staged,
			 const auction_settlement_accounts &keys,
			 const critical_operation_id &epoch)
{
	auction_command_payload payload = {};
	payload.action = action;
	payload.auction_id = staged.auction_id;
	payload.closing_fee_basis_points = fee;
	critical_command result = {};
	assert(auction_command_build(&result, id(operation), payload,
				     critical_source_site::zone_event,
				     critical_deadline_class::background));
	result.accepted_at_usec = 1;
	assert(auction_settlement_accounting_intent(result, epoch, staged, keys,
						    &result.accounting_intent) ==
	       economic_accounting_error::ok);
	result.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	assert(critical_command_envelope_valid(result));
	return result;
}

auction_command_result result(auction_action action, const auction_settlement_listing &staged)
{
	auction_command_result value = {};
	value.action = action;
	value.event_type = action == auction_action::remove ? auction_event_type::removed :
			   staged.winner_pid		    ? auction_event_type::sold :
							      auction_event_type::expired;
	value.auction_id = staged.auction_id;
	value.status = action == auction_action::remove ? 3 : 2;
	value.seller_pid = staged.seller_pid;
	value.winner_pid = staged.winner_pid;
	value.final_price = staged.current_price;
	value.auction_revision = staged.revision + 1;
	return value;
}
} // namespace

int main()
{
	const auto staged = listing();
	const auto keys = accounts(true);
	auto before = authority(staged, keys, staged.winner_pid, 2910);
	const auto settle = command(auction_action::finalize, 4, 300, staged, keys, before.epoch);
	economic_frozen_intent decoded_intent;
	auction_command_payload decoded_payload = {};
	auction_settlement_listing decoded_listing;
	auction_settlement_accounts decoded_accounts;
	assert(auction_settlement_accounting_decode(settle, &decoded_intent, &decoded_payload,
						    &decoded_listing, &decoded_accounts) ==
	       economic_accounting_error::ok);
	assert(decoded_listing.items[1].uid == 601 && decoded_listing.winner_pid == 20 &&
	       decoded_payload.action == auction_action::finalize &&
	       economic_account_key_equal(decoded_accounts.escrow, keys.escrow));
	auto corrupt_settle = settle;
	corrupt_settle.accounting_intent.front() ^= 1;
	assert(auction_settlement_accounting_decode(
		       corrupt_settle, &decoded_intent, &decoded_payload, &decoded_listing,
		       &decoded_accounts) != economic_accounting_error::ok &&
	       decoded_listing.items[1].uid == 601);
	economic_frozen_intent intent;
	assert(economic_intent_decode(settle.accounting_intent, &intent) ==
	       economic_accounting_error::ok);
	economic_accounting_plan plan;
	assert(auction_settlement_accounting_plan(settle, intent, before,
						  result(auction_action::finalize, staged),
						  &plan) == economic_accounting_error::ok);
	assert(plan.accounts.size() == 3 && plan.postings.size() == 3);
	assert(plan.postings[0].copper == -3000 && plan.postings[1].copper == 2910 &&
	       plan.postings[2].copper == 90);
	assert(plan.item_events.empty() && plan.items_before.size() == 2 &&
	       plan.items_after.size() == 2);
	assert(plan.metadata.original_operation_id.bytes == staged.listing_operation.bytes);
	assert(plan.metadata.source_event &&
	       plan.metadata.source_event->source.bytes == staged.winning_bid_operation.bytes);
	before.seller_claim_after++;
	assert(auction_settlement_accounting_plan(
		       settle, intent, before, result(auction_action::finalize, staged), &plan) ==
	       economic_accounting_error::corrupt_evidence);
	before = authority(staged, accounts(false), staged.seller_pid, 0);
	const auto remove =
		command(auction_action::remove, 5, 0, staged, before.accounts, before.epoch);
	assert(economic_intent_decode(remove.accounting_intent, &intent) ==
	       economic_accounting_error::ok);
	assert(auction_settlement_accounting_plan(remove, intent, before,
						  result(auction_action::remove, staged),
						  &plan) == economic_accounting_error::ok);
	assert(plan.accounts.size() == 1 && plan.postings.empty() &&
	       plan.accounts[0].before == plan.accounts[0].after &&
	       plan.accounts[0].before[0] == 3000 &&
	       plan.metadata.reason == economic_reason::auction_cancel);
	auto no_bid_removal = staged;
	no_bid_removal.winner_pid = 0;
	no_bid_removal.winning_bid_operation = {};
	before = authority(no_bid_removal, accounts(false), no_bid_removal.seller_pid, 0);
	const auto remove_no_bid = command(auction_action::remove, 7, 0, no_bid_removal,
					   before.accounts, before.epoch);
	assert(economic_intent_decode(remove_no_bid.accounting_intent, &intent) ==
	       economic_accounting_error::ok);
	assert(auction_settlement_accounting_plan(remove_no_bid, intent, before,
						  result(auction_action::remove, no_bid_removal),
						  &plan) == economic_accounting_error::ok);
	assert(plan.accounts.size() == 1 && plan.accounts[0].before == economic_coin_vector{} &&
	       plan.accounts[0].after == economic_coin_vector{} && plan.postings.empty());
	auto expired = staged;
	expired.winner_pid = 0;
	expired.winning_bid_operation = {};
	before = authority(expired, accounts(false), expired.seller_pid, 0);
	const auto finalize_expired =
		command(auction_action::finalize, 6, 300, expired, before.accounts, before.epoch);
	assert(economic_intent_decode(finalize_expired.accounting_intent, &intent) ==
	       economic_accounting_error::ok);
	assert(auction_settlement_accounting_plan(finalize_expired, intent, before,
						  result(auction_action::finalize, expired),
						  &plan) == economic_accounting_error::ok);
	assert(plan.accounts.size() == 1 && plan.accounts[0].before[0] == 0 &&
	       plan.postings.empty());
	return 0;
}
