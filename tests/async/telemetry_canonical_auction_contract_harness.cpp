#include "economy/auction_money_claim_accounting.h"
#include "economy/auction_settlement_accounting.h"

#include <cassert>
#include <cstdio>
#include <cstring>

namespace
{
critical_operation_id id(uint8_t first)
{
	critical_operation_id value = {};
	value.bytes[0] = first;
	return value;
}

void print_hex(std::span<const uint8_t> bytes)
{
	for (uint8_t byte : bytes)
		std::printf("%02x", byte);
}

void emit(const critical_command &command, const economic_accounting_plan &plan,
	  const auction_command_result &result)
{
	std::vector<uint8_t> encoded;
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> receipt = {};
	assert(economic_plan_encode(plan, &encoded) == economic_accounting_error::ok);
	assert(auction_command_encode_result(result, &receipt));
	std::printf("TELEMETRY_AUCTION_PLAN ");
	print_hex(command.accounting_intent);
	std::printf(" ");
	print_hex(encoded);
	std::printf(" ");
	print_hex(receipt);
	std::printf("\n");
}

int64_t sale(uint8_t operation, uint32_t auction_id, int64_t price, uint32_t fee_bps,
	     int64_t pending_before, uint64_t pending_revision, const economic_account_key &pending)
{
	auction_settlement_listing listing;
	listing.auction_id = auction_id;
	listing.seller_pid = 100;
	listing.winner_pid = 200;
	listing.status = listing.custody_state = listing.quantity = 1;
	listing.current_price = price;
	listing.revision = 2;
	listing.end_time = 1;
	listing.listing_operation = id(operation + 30);
	listing.winning_bid_operation = id(operation + 40);
	listing.item_count = 1;
	const uint64_t uid = 600 + auction_id;
	listing.items[0] = { uid, 1, 0, 77, 0, false };
	auction_settlement_accounts accounts;
	accounts.escrow = { pending.lineage, economic_account_kind::auction_escrow,
			    1000 + auction_id, 0 };
	accounts.seller_claim = pending;
	auction_command_payload payload = {};
	payload.action = auction_action::finalize;
	payload.auction_id = auction_id;
	payload.closing_fee_basis_points = fee_bps;
	critical_command command = {};
	assert(auction_command_build(&command, id(operation), payload,
				     critical_source_site::zone_event,
				     critical_deadline_class::background));
	command.accepted_at_usec = 1;
	assert(auction_settlement_accounting_intent(command, id(3), listing, accounts,
						    &command.accounting_intent) ==
	       economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	economic_frozen_intent intent;
	assert(economic_intent_decode(command.accounting_intent, &intent) ==
	       economic_accounting_error::ok);
	const int64_t proceeds = price - price * fee_bps / 10000;
	auction_settlement_authority authority;
	authority.epoch = id(3);
	authority.listing = listing;
	authority.accounts = accounts;
	authority.seller_claim_before = pending_before;
	authority.seller_claim_after = pending_before + proceeds;
	authority.seller_claim_revision_before = pending_revision;
	authority.seller_claim_revision_after = pending_revision + 1;
	authority.items_before = { { uid,
				     { { item_owner_type::auction, auction_id, 0 },
				       uid,
				       0,
				       1,
				       item_custody_state::active } } };
	authority.claim_pids_after = { listing.winner_pid };
	auction_command_result result = {};
	result.action = auction_action::finalize;
	result.event_type = auction_event_type::sold;
	result.auction_id = auction_id;
	result.status = 2;
	result.seller_pid = listing.seller_pid;
	result.winner_pid = listing.winner_pid;
	result.final_price = price;
	result.auction_revision = listing.revision + 1;
	economic_accounting_plan plan;
	assert(auction_settlement_accounting_plan(command, intent, authority, result, &plan) ==
	       economic_accounting_error::ok);
	emit(command, plan, result);
	return proceeds;
}
} // namespace

int main()
{
	// Native pure owner plans and portable receipts, not database commits.
	const economic_account_key wallet = { id(2), economic_account_kind::wallet, 11, 0 };
	const economic_account_key bank = { id(2), economic_account_kind::bank, 12, 1 };
	const economic_account_key pending = { id(2), economic_account_kind::pending_claim, 13, 0 };
	const int64_t first = sale(10, 400, 300, 300, 0, 0, pending);
	const int64_t second = sale(11, 401, 200, 0, first, 1, pending);
	auction_money_claim_state state;
	state.beneficiary_pid = 100;
	state.money = first + second;
	state.revision = 2;
	state.sources = { { id(10), 2, 100, pending.authority_id, static_cast<uint64_t>(first) },
			  { id(11), 2, 100, pending.authority_id, static_cast<uint64_t>(second) } };
	auction_command_payload payload = {};
	payload.action = auction_action::claim_money;
	payload.actor_pid = state.beneficiary_pid;
	payload.racewar = 1;
	std::memcpy(payload.account_name.data(), "claim_test", 10);
	std::memcpy(payload.actor_name.data(), "Claimant", 8);
	payload.expected_wallet_revision = 8;
	payload.expected_bank_revision = 2;
	critical_command command = {};
	assert(auction_command_build(&command, id(20), payload, critical_source_site::command,
				     critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(auction_money_claim_accounting_intent(command, id(3), wallet, bank, pending, state,
						     &command.accounting_intent) ==
	       economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	economic_frozen_intent intent;
	assert(economic_intent_decode(command.accounting_intent, &intent) ==
	       economic_accounting_error::ok);
	auction_money_claim_authority authority;
	authority.epoch = id(3);
	authority.wallet = wallet;
	authority.bank = bank;
	authority.claim_account = pending;
	authority.claim = state;
	authority.balances_before.wallet.amount = { 0, 0, 0, 10 };
	authority.balances_before.bank.amount = { 5, 0, 0, 0 };
	authority.balances_before.wallet_revision = 8;
	authority.balances_before.bank_revision = 2;
	auction_command_result result = {};
	result.action = auction_action::claim_money;
	result.event_type = auction_event_type::money_claimed;
	result.wallet_value_delta = state.money;
	result.wallet.amount = { 1, 9, 4, 10 };
	result.bank.amount = authority.balances_before.bank.amount;
	result.wallet_revision = 9;
	result.bank_revision = 3;
	result.auction_revision = 9;
	economic_accounting_plan plan;
	assert(state.money == 491);
	assert(auction_money_claim_accounting_plan(command, intent, authority, result, &plan) ==
	       economic_accounting_error::ok);
	emit(command, plan, result);
	return 0;
}
