#include "economy/auction_accounting.h"

#include <cassert>
#include <climits>
#include <cstring>

namespace
{
critical_operation_id id(uint8_t first)
{
	critical_operation_id value = {};
	value.bytes[0] = first;
	return value;
}

auction_command_payload bid_payload(uint32_t actor, int64_t value, uint64_t wallet_revision)
{
	auction_command_payload payload = {};
	payload.action = auction_action::bid;
	payload.auction_id = 400;
	payload.actor_pid = actor;
	payload.racewar = 1;
	std::memcpy(payload.account_name.data(), "auction_test", 12);
	std::memcpy(payload.actor_name.data(), "Bidder", 7);
	payload.expected_wallet_revision = wallet_revision;
	payload.expected_bank_revision = wallet_revision + 1;
	payload.value = value;
	payload.closing_fee_basis_points = 300;
	return payload;
}

auction_bid_accounting_listing listing(uint32_t winner, int64_t price, uint64_t revision)
{
	auction_bid_accounting_listing state;
	state.auction_id = 400;
	state.seller_pid = 10;
	state.winning_bidder_pid = winner;
	state.status = 1;
	state.custody_state = 1;
	state.current_price = price;
	state.buy_price = 5000;
	state.revision = revision;
	state.listing_operation = id(1);
	state.previous_bid_operation = winner ? id(3) : critical_operation_id{};
	return state;
}

auction_bid_accounting_accounts accounts(uint32_t actor, bool outbid, bool sold)
{
	const auto lineage = id(2);
	auction_bid_accounting_accounts keys;
	keys.wallet = { lineage, economic_account_kind::wallet, actor, 0 };
	keys.bank = { lineage, economic_account_kind::bank, actor + 1000, 1 };
	keys.escrow = { lineage, economic_account_kind::auction_escrow, 400, 0 };
	keys.bidder_claim = { lineage, economic_account_kind::pending_claim, actor + 2000, 0 };
	if (outbid)
		keys.previous_claim = { lineage, economic_account_kind::pending_claim, 20, 0 };
	if (sold)
		keys.seller_claim = { lineage, economic_account_kind::pending_claim, 10, 0 };
	return keys;
}

critical_command accepted(const auction_command_payload &payload, uint8_t operation,
			  const auction_bid_accounting_listing &before,
			  const auction_bid_accounting_accounts &keys,
			  economic_frozen_intent *intent)
{
	critical_command command = {};
	assert(auction_command_build(&command, id(operation), payload,
				     critical_source_site::command,
				     critical_deadline_class::interactive));
	command.accepted_at_usec = operation;
	std::vector<uint8_t> encoded;
	assert(auction_bid_accounting_intent(command, id(9), before, keys, &encoded) ==
	       economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command.accounting_intent = encoded;
	assert(economic_intent_decode(encoded, intent) == economic_accounting_error::ok);
	return command;
}

auction_bid_accounting_authority authority(const auction_bid_accounting_listing &before,
					   const auction_bid_accounting_accounts &keys,
					   uint64_t wallet_revision)
{
	auction_bid_accounting_authority value;
	value.epoch = id(9);
	value.listing = before;
	value.accounts = keys;
	value.balances_before.wallet.amount = { 0, 0, 0, 10 };
	value.balances_before.bank.amount = { 3, 0, 0, 0 };
	value.balances_before.wallet_revision = wallet_revision;
	value.balances_before.bank_revision = wallet_revision + 1;
	value.previous_claim_before = { 200, 7 };
	value.seller_claim_before = { 100, 2 };
	return value;
}

auction_command_result result_for(const auction_command_payload &payload,
				  const auction_bid_accounting_listing &before,
				  uint64_t wallet_revision, int64_t claim_credit_used = 0)
{
	const bool sold = payload.value >= before.buy_price;
	const int64_t bid = sold ? before.buy_price : payload.value;
	const int64_t pay =
		before.winning_bidder_pid == payload.actor_pid ? bid - before.current_price : bid;
	auction_command_result result = {};
	result.action = auction_action::bid;
	result.event_type = sold ? auction_event_type::sold : auction_event_type::bid_placed;
	result.auction_id = before.auction_id;
	result.status = sold ? 2 : 1;
	result.seller_pid = before.seller_pid;
	result.winner_pid = payload.actor_pid;
	result.previous_bidder_pid = before.winning_bidder_pid;
	result.final_price = bid;
	const int64_t wallet_pay = pay - claim_credit_used;
	result.wallet_value_delta = -wallet_pay;
	result.claim_credit_used = claim_credit_used;
	result.wallet.amount = { 0, 0, 0, (10000 - wallet_pay) / 1000 };
	result.bank.amount = { 3, 0, 0, 0 };
	result.wallet_revision = wallet_revision + 1;
	result.bank_revision = wallet_revision + 2;
	result.auction_revision = before.revision + 1;
	return result;
}

int64_t posting_sum(const economic_accounting_plan &plan)
{
	int64_t sum = 0;
	for (const auto &posting : plan.postings)
		sum += posting.copper;
	return sum;
}
} // namespace

int main()
{
	const auto first = listing(0, 2000, 1);
	const auto first_keys = accounts(20, false, false);
	const auto first_payload = bid_payload(20, 3000, 4);
	economic_frozen_intent first_intent;
	const auto first_command = accepted(first_payload, 3, first, first_keys, &first_intent);
	economic_frozen_intent decoded_intent;
	auction_command_payload decoded_payload = {};
	auction_bid_accounting_listing decoded_listing;
	auction_bid_accounting_accounts decoded_accounts;
	assert(auction_bid_accounting_decode(first_command, &decoded_intent, &decoded_payload,
					     &decoded_listing,
					     &decoded_accounts) == economic_accounting_error::ok);
	assert(decoded_listing.auction_id == 400 && decoded_payload.value == 3000 &&
	       economic_account_key_equal(decoded_accounts.escrow, first_keys.escrow));
	auto corrupt_command = first_command;
	corrupt_command.accounting_intent.front() ^= 1;
	assert(auction_bid_accounting_decode(corrupt_command, &decoded_intent, &decoded_payload,
					     &decoded_listing,
					     &decoded_accounts) != economic_accounting_error::ok &&
	       decoded_listing.auction_id == 400);
	auto first_authority = authority(first, first_keys, 4);
	const auto first_result = result_for(first_payload, first, 4);
	economic_accounting_plan plan;
	assert(auction_bid_accounting_plan(first_command, first_intent, first_authority,
					   first_result, &plan) == economic_accounting_error::ok);
	assert(plan.metadata.original_operation_id.bytes == first.listing_operation.bytes);
	assert(plan.metadata.source_event &&
	       plan.metadata.source_event->source.bytes == first.listing_operation.bytes);
	assert(plan.accounts.size() == 2 && plan.postings.size() == 2);
	assert(plan.postings[0].copper == -3000 && plan.postings[1].copper == 3000);
	assert(plan.accounts[1].after == (economic_coin_vector{ 3000, 0, 0, 0 }));
	assert(posting_sum(plan) == 0);
	assert(economic_plan_validate_structure(plan) == economic_accounting_error::ok);

	// Pending auction credit funds the bid before the wallet and is balanced
	// directly against escrow in the same accounting plan.
	auto credit_authority = authority(first, first_keys, 4);
	credit_authority.bidder_claim_before = { 2000, 9 };
	const auto credit_result = result_for(first_payload, first, 4, 2000);
	assert(auction_bid_accounting_plan(first_command, first_intent, credit_authority,
					   credit_result, &plan) == economic_accounting_error::ok);
	assert(plan.accounts.size() == 3 && plan.postings.size() == 3);
	assert(plan.postings[0].copper == -1000 && plan.postings[1].copper == -2000 &&
	       plan.postings[2].copper == 3000);
	assert(economic_plan_validate_structure(plan) == economic_accounting_error::ok);

	auto same = listing(20, 3000, 2);
	const auto same_keys = accounts(20, false, false);
	const auto same_payload = bid_payload(20, 4000, 5);
	economic_frozen_intent same_intent;
	const auto same_command = accepted(same_payload, 4, same, same_keys, &same_intent);
	auto same_authority = authority(same, same_keys, 5);
	assert(auction_bid_accounting_plan(same_command, same_intent, same_authority,
					   result_for(same_payload, same, 5),
					   &plan) == economic_accounting_error::ok);
	assert(plan.postings.size() == 2 && plan.postings[0].copper == -1000);
	assert(plan.accounts[1].before == (economic_coin_vector{ 3000, 0, 0, 0 }));
	assert(plan.accounts[1].after == (economic_coin_vector{ 4000, 0, 0, 0 }));
	const auto self_sale_keys = accounts(20, false, true);
	const auto self_sale_payload = bid_payload(20, 5000, 5);
	economic_frozen_intent self_sale_intent;
	const auto self_sale_command =
		accepted(self_sale_payload, 6, same, self_sale_keys, &self_sale_intent);
	auto self_sale_authority = authority(same, self_sale_keys, 5);
	assert(auction_bid_accounting_plan(self_sale_command, self_sale_intent, self_sale_authority,
					   result_for(self_sale_payload, same, 5),
					   &plan) == economic_accounting_error::ok);
	assert(plan.postings.size() == 5 && plan.postings[0].copper == -2000);
	assert(plan.postings[1].copper == 2000 && plan.postings[2].copper == -5000);
	assert(plan.postings[3].copper == 4850 && plan.postings[4].copper == 150);
	assert(plan.accounts[1].after == economic_coin_vector{});
	assert(posting_sum(plan) == 0);

	const auto outbid_keys = accounts(30, true, true);
	const auto outbid_payload = bid_payload(30, 5000, 8);
	economic_frozen_intent outbid_intent;
	const auto outbid_command = accepted(outbid_payload, 5, same, outbid_keys, &outbid_intent);
	auto outbid_authority = authority(same, outbid_keys, 8);
	const auto outbid_result = result_for(outbid_payload, same, 8);
	assert(auction_bid_accounting_plan(outbid_command, outbid_intent, outbid_authority,
					   outbid_result, &plan) == economic_accounting_error::ok);
	assert(plan.accounts.size() == 5 && plan.postings.size() == 7);
	assert(plan.metadata.original_operation_id.bytes == same.listing_operation.bytes);
	assert(plan.metadata.source_event &&
	       plan.metadata.source_event->source.bytes == same.previous_bid_operation.bytes);
	assert(plan.postings[0].copper == -5000);
	assert(plan.postings[1].copper == 5000);
	assert(plan.postings[2].copper == -3000);
	assert(plan.postings[3].copper == 3000);
	assert(plan.postings[4].copper == -5000);
	assert(plan.postings[5].copper == 4850);
	assert(plan.postings[6].copper == 150);
	assert(posting_sum(plan) == 0);
	assert(economic_plan_validate_structure(plan) == economic_accounting_error::ok);

	const auto digest = plan.metadata.intent_digest;
	auto damaged_result = outbid_result;
	damaged_result.wallet.amount[3]++;
	assert(auction_bid_accounting_plan(outbid_command, outbid_intent, outbid_authority,
					   damaged_result,
					   &plan) == economic_accounting_error::corrupt_evidence);
	assert(plan.metadata.intent_digest == digest);
	auto damaged_authority = outbid_authority;
	damaged_authority.listing.current_price++;
	assert(auction_bid_accounting_plan(outbid_command, outbid_intent, damaged_authority,
					   outbid_result,
					   &plan) == economic_accounting_error::unauthorized);
	damaged_authority = outbid_authority;
	damaged_authority.listing.previous_bid_operation = id(7);
	assert(auction_bid_accounting_plan(outbid_command, outbid_intent, damaged_authority,
					   outbid_result,
					   &plan) == economic_accounting_error::unauthorized);
	damaged_authority = outbid_authority;
	damaged_authority.previous_claim_before.money = UINT_MAX;
	assert(auction_bid_accounting_plan(outbid_command, outbid_intent, damaged_authority,
					   outbid_result,
					   &plan) == economic_accounting_error::overflow);
	damaged_authority = outbid_authority;
	damaged_authority.balances_before.wallet.amount = { 0, 0, 0, 1 };
	assert(auction_bid_accounting_plan(outbid_command, outbid_intent, damaged_authority,
					   outbid_result,
					   &plan) == economic_accounting_error::negative_holding);
	return 0;
}
