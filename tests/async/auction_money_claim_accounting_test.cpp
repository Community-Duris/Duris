#include "economy/auction_money_claim_accounting.h"

#include <cassert>
#include <cstring>

namespace
{
critical_operation_id id(uint8_t first)
{
	critical_operation_id value = {};
	value.bytes[0] = first;
	return value;
}

critical_command command_for(const auction_money_claim_state &claim,
			     const economic_account_key &wallet, const economic_account_key &bank,
			     const economic_account_key &pending, economic_frozen_intent *intent)
{
	auction_command_payload payload = {};
	payload.action = auction_action::claim_money;
	payload.actor_pid = claim.beneficiary_pid;
	payload.racewar = 1;
	std::memcpy(payload.account_name.data(), "claim_test", 10);
	std::memcpy(payload.actor_name.data(), "Claimant", 8);
	payload.expected_wallet_revision = 8;
	payload.expected_bank_revision = 2;
	critical_command command = {};
	assert(auction_command_build(&command, id(20), payload, critical_source_site::command,
				     critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(auction_money_claim_accounting_intent(command, id(3), wallet, bank, pending, claim,
						     &command.accounting_intent) ==
	       economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	assert(economic_intent_decode(command.accounting_intent, intent) ==
	       economic_accounting_error::ok);
	return command;
}
} // namespace

int main()
{
	const auto lineage = id(2);
	const economic_account_key wallet = { lineage, economic_account_kind::wallet, 11, 0 };
	const economic_account_key bank = { lineage, economic_account_kind::bank, 12, 1 };
	const economic_account_key pending = { lineage, economic_account_kind::pending_claim, 13,
					       0 };
	auction_money_claim_state claim;
	claim.beneficiary_pid = 100;
	claim.money = 500;
	claim.revision = 4;
	claim.sources = { { id(10), 1, 100, 13, 300 }, { id(11), 2, 100, 13, 200 } };
	economic_frozen_intent intent;
	const auto command = command_for(claim, wallet, bank, pending, &intent);
	economic_frozen_intent decoded_intent;
	auction_command_payload decoded_payload = {};
	economic_account_key decoded_wallet, decoded_bank, decoded_claim;
	assert(auction_money_claim_accounting_decode(
		       command, &decoded_intent, &decoded_payload, &decoded_wallet, &decoded_bank,
		       &decoded_claim) == economic_accounting_error::ok);
	assert(decoded_payload.actor_pid == 100 &&
	       economic_account_key_equal(decoded_wallet, wallet) &&
	       economic_account_key_equal(decoded_bank, bank) &&
	       economic_account_key_equal(decoded_claim, pending));
	auto malformed = command;
	malformed.accounting_intent.front() ^= 1;
	assert(auction_money_claim_accounting_decode(
		       malformed, &decoded_intent, &decoded_payload, &decoded_wallet, &decoded_bank,
		       &decoded_claim) != economic_accounting_error::ok &&
	       economic_account_key_equal(decoded_claim, pending));
	auction_money_claim_authority before;
	before.epoch = id(3);
	before.wallet = wallet;
	before.bank = bank;
	before.claim_account = pending;
	before.claim = claim;
	before.balances_before.wallet.amount = { 0, 0, 0, 10 };
	before.balances_before.bank.amount = { 5, 0, 0, 0 };
	before.balances_before.wallet_revision = 8;
	before.balances_before.bank_revision = 2;
	auction_command_result result = {};
	result.action = auction_action::claim_money;
	result.event_type = auction_event_type::money_claimed;
	result.wallet_value_delta = 500;
	result.wallet.amount = { 0, 0, 5, 10 };
	result.bank.amount = { 5, 0, 0, 0 };
	result.wallet_revision = 9;
	result.bank_revision = 3;
	result.auction_revision = 9;
	economic_accounting_plan plan;
	assert(auction_money_claim_accounting_plan(command, intent, before, result, &plan) ==
	       economic_accounting_error::ok);
	assert(plan.accounts.size() == 2 && plan.postings.size() == 2);
	assert(plan.postings[0].copper == 500 && plan.postings[1].copper == -500);
	assert(plan.accounts[1].before[0] == 500 && plan.accounts[1].after[0] == 0);
	assert(economic_plan_validate_structure(plan) == economic_accounting_error::ok);

	auto changed = before;
	changed.claim.sources[1].amount = 201;
	assert(auction_money_claim_accounting_plan(command, intent, changed, result, &plan) !=
	       economic_accounting_error::ok);
	changed = before;
	changed.claim.money = 600; // 100 unaccounted legacy copper.
	assert(auction_money_claim_accounting_plan(command, intent, changed, result, &plan) !=
	       economic_accounting_error::ok);
	changed = before;
	changed.claim.sources[0].operation = id(12);
	assert(auction_money_claim_accounting_plan(command, intent, changed, result, &plan) !=
	       economic_accounting_error::ok);
	auto corrupted = result;
	corrupted.wallet_value_delta = 499;
	assert(auction_money_claim_accounting_plan(command, intent, before, corrupted, &plan) ==
	       economic_accounting_error::corrupt_evidence);
	return 0;
}
