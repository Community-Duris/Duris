#include "economy/auction_listing_accounting.h"

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

auction_command_payload payload(int64_t fee)
{
	auction_command_payload value = {};
	value.action = auction_action::list;
	value.actor_pid = 100;
	value.racewar = 1;
	std::memcpy(value.account_name.data(), "listing_test", 12);
	std::memcpy(value.actor_name.data(), "Seller", 6);
	value.expected_wallet_revision = 4;
	value.expected_bank_revision = 7;
	value.start_price = 1000;
	value.buy_price = 5000;
	value.listing_fee = fee;
	value.end_time = 2000000000;
	value.item_count = 1;
	value.items[0] = { 9001, 3, 77 };
	std::memcpy(value.object_blob.data(), "relic", 5);
	value.object_blob_size = 5;
	return value;
}

critical_command command_for(const auction_command_payload &value, uint8_t operation,
			     const economic_account_key &wallet, const economic_account_key &bank,
			     economic_frozen_intent *intent)
{
	critical_command command = {};
	assert(auction_command_build(&command, id(operation), value, critical_source_site::command,
				     critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(auction_listing_accounting_intent(command, id(3), wallet, bank,
						 &command.accounting_intent) ==
	       economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	assert(economic_intent_decode(command.accounting_intent, intent) ==
	       economic_accounting_error::ok);
	return command;
}

auction_listing_accounting_authority authority(const economic_account_key &wallet,
					       const economic_account_key &bank)
{
	auction_listing_accounting_authority value;
	value.epoch = id(3);
	value.wallet = wallet;
	value.bank = bank;
	value.escrow = { wallet.lineage, economic_account_kind::auction_escrow, 13, 0 };
	value.balances_before.wallet.amount = { 0, 0, 0, 10 };
	value.balances_before.bank.amount = { 2, 0, 0, 0 };
	value.balances_before.wallet_revision = 4;
	value.balances_before.bank_revision = 7;
	value.player_owner_revision_before = 8;
	value.items_before.push_back(
		{ 9001,
		  { { item_owner_type::player, 100, 0 }, 9001, 0, 3, item_custody_state::active } });
	return value;
}

auction_command_result result_for(int64_t fee)
{
	auction_command_result result = {};
	result.action = auction_action::list;
	result.event_type = auction_event_type::listed;
	result.auction_id = 400;
	result.status = 1;
	result.seller_pid = 100;
	result.wallet_value_delta = -fee;
	result.wallet.amount = fee ? economic_coin_vector{ 0, 0, 8, 9 } :
				     economic_coin_vector{ 0, 0, 0, 10 };
	result.bank.amount = { 2, 0, 0, 0 };
	result.wallet_revision = 5;
	result.bank_revision = 8;
	result.auction_revision = 1;
	result.player_owner_revision = 9;
	result.auction_owner_revision = 1;
	result.item_count = 1;
	result.item_uids[0] = 9001;
	result.item_revisions[0] = 4;
	return result;
}
} // namespace

int main()
{
	const auto lineage = id(2);
	const economic_account_key wallet = { lineage, economic_account_kind::wallet, 11, 0 };
	const economic_account_key bank = { lineage, economic_account_kind::bank, 12, 1 };
	economic_frozen_intent intent;
	const auto paid_command = command_for(payload(200), 4, wallet, bank, &intent);
	economic_frozen_intent decoded_intent;
	auction_command_payload decoded_payload = {};
	economic_account_key decoded_wallet, decoded_bank;
	assert(auction_listing_accounting_decode(paid_command, &decoded_intent, &decoded_payload,
						 &decoded_wallet,
						 &decoded_bank) == economic_accounting_error::ok);
	assert(decoded_payload.listing_fee == 200 &&
	       economic_account_key_equal(decoded_wallet, wallet) &&
	       economic_account_key_equal(decoded_bank, bank));
	auto tampered = paid_command;
	tampered.accounting_intent.front() ^= 1;
	assert(auction_listing_accounting_decode(tampered, &decoded_intent, &decoded_payload,
						 &decoded_wallet,
						 &decoded_bank) != economic_accounting_error::ok);
	assert(decoded_payload.listing_fee == 200 &&
	       economic_account_key_equal(decoded_wallet, wallet));
	const auto before = authority(wallet, bank);
	const auto paid_result = result_for(200);
	economic_accounting_plan plan;
	assert(auction_listing_accounting_plan(paid_command, intent, before, paid_result, &plan) ==
	       economic_accounting_error::ok);
	assert(plan.accounts.size() == 3 && plan.postings.size() == 2 &&
	       plan.item_events.size() == 1);
	assert(plan.postings[0].copper == -200 && plan.postings[1].copper == 200);
	assert(plan.item_events[0].before.owner.type == item_owner_type::player &&
	       plan.item_events[0].after.owner.type == item_owner_type::auction &&
	       plan.item_events[0].after.owner.id == 400);
	assert(economic_plan_validate_structure(plan) == economic_accounting_error::ok);
	auto damaged = paid_result;
	damaged.item_revisions[0]++;
	assert(auction_listing_accounting_plan(paid_command, intent, before, damaged, &plan) ==
	       economic_accounting_error::corrupt_evidence);

	economic_frozen_intent free_intent;
	const auto free_command = command_for(payload(0), 5, wallet, bank, &free_intent);
	auto noncanonical = authority(wallet, bank);
	noncanonical.balances_before.wallet.amount = { 0, 0, 10, 9 };
	assert(auction_listing_accounting_plan(free_command, free_intent, noncanonical,
					       result_for(0),
					       &plan) == economic_accounting_error::ok);
	assert(plan.accounts.size() == 2 && plan.postings.size() == 1);
	assert(plan.postings[0].copper == 0 &&
	       plan.postings[0].delta == (economic_coin_vector{ 0, 0, -10, 1 }));
	assert(economic_plan_validate_structure(plan) == economic_accounting_error::ok);
	return 0;
}
