#include "economy/shop_trade_accounting.h"

#include <cassert>
#include <cstring>

namespace
{
using error = economic_accounting_error;

bool buying(shop_trade_action action);

critical_operation_id id(uint8_t first)
{
	critical_operation_id value = {};
	value.bytes[0] = first;
	return value;
}

shop_trade_payload payload_for(shop_trade_action action, bool nested = false)
{
	shop_trade_payload payload = {};
	payload.action = action;
	payload.player_pid = 100;
	payload.shop_id = 300;
	payload.racewar = 1;
	std::memcpy(payload.account_name.data(), "shop_test", 10);
	payload.price = action == shop_trade_action::discard_invalid ? 0 : 200;
	payload.keeper_vnum = 12345;
	payload.expected_keeper_cash = 500;
	payload.expected_wallet_revision = 4;
	payload.expected_bank_revision = 7;
	payload.expected_shop_revision = 9;
	payload.selected_item_uid = 500;
	payload.target_root_item_uid = nested ? 601 : 500;
	payload.target_parent_item_uid = nested ? 600 : 0;
	payload.expected_target_parent_revision = nested ? 3 : 0;
	if (action == shop_trade_action::buy_existing ||
	    action == shop_trade_action::discard_invalid)
	{
		payload.stock_item_uid = 500;
		payload.expected_stock_item_revision = 4;
		payload.stock_vnum = 77;
	}
	else if (action == shop_trade_action::buy_produced)
	{
		payload.stock_item_uid = 400;
		payload.expected_stock_item_revision = 6;
		payload.stock_vnum = 77;
	}
	payload.item_count = 2;
	const bool produced = action == shop_trade_action::buy_produced;
	payload.items[0] = {
		500, 500,
		0,   produced ? ITEM_TRANSFER_ABSENT_REVISION : 4,
		77,  produced ? item_custody_state::absent : item_custody_state::active
	};
	payload.items[1] = {
		501, 500,
		500, produced ? ITEM_TRANSFER_ABSENT_REVISION : 2,
		78,  produced ? item_custody_state::absent : item_custody_state::active
	};
	payload.item_blob[0] = 1;
	payload.item_blob_size = 1;
	return payload;
}

economic_item_snapshot active(uint64_t uid, item_owner_identity owner, uint64_t root,
			      uint64_t parent, uint64_t revision)
{
	return { uid, { owner, root, parent, revision, item_custody_state::active } };
}

shop_trade_accounting_authority authority_for(const shop_trade_payload &payload,
					      const economic_account_key &wallet,
					      const economic_account_key &bank,
					      const economic_account_key &keeper)
{
	shop_trade_accounting_authority authority;
	authority.epoch = id(3);
	authority.wallet_account = wallet;
	authority.bank_account = bank;
	authority.keeper_account = keeper;
	authority.balances_before.wallet.amount = { 0, 0, 0, 10 };
	authority.balances_before.bank.amount = { 2, 0, 0, 0 };
	authority.balances_before.wallet_revision = 4;
	authority.balances_before.bank_revision = 7;
	authority.shop_id = payload.shop_id;
	authority.keeper_vnum = payload.keeper_vnum;
	authority.keeper_cash_before = payload.expected_keeper_cash;
	authority.keeper_roaming = payload.keeper_roaming;
	authority.shop_revision_before = 9;
	authority.player_owner_revision_before = 11;
	authority.counterparty_owner_revision_before = 12;
	const item_owner_identity player = { item_owner_type::player, 100, 0 };
	const item_owner_identity shop = { item_owner_type::shopkeeper,
					   item_shopkeeper_owner_id(payload.shop_id), 0 };
	if (payload.action == shop_trade_action::buy_produced)
	{
		authority.items_before = { active(400, shop, 400, 0, 6), { 500, {} }, { 501, {} } };
		authority.item_vnums_before = { 77, 0, 0 };
		if (payload.target_parent_item_uid)
		{
			authority.items_before.push_back(active(600, player, 601, 601, 3));
			authority.items_before.push_back(active(601, player, 601, 0, 5));
			authority.item_vnums_before.insert(authority.item_vnums_before.end(),
							   { 80, 81 });
		}
	}
	else
	{
		const auto owner = payload.action == shop_trade_action::buy_existing ||
						   payload.action ==
							   shop_trade_action::discard_invalid ?
					   shop :
					   player;
		authority.items_before = { active(500, owner, 500, 0, 4),
					   active(501, owner, 500, 500, 2) };
		authority.item_vnums_before = { 77, 78 };
	}
	return authority;
}

shop_trade_result result_for(const shop_trade_payload &payload, int64_t keeper_after)
{
	shop_trade_result result = {};
	result.action = payload.action;
	result.wallet.amount = payload.action == shop_trade_action::discard_invalid ?
				       economic_coin_vector{ 0, 0, 0, 10 } :
			       buying(payload.action) ? economic_coin_vector{ 0, 0, 8, 9 } :
							economic_coin_vector{ 0, 0, 2, 10 };
	result.bank.amount = { 2, 0, 0, 0 };
	result.wallet_revision = payload.action == shop_trade_action::discard_invalid ? 4 : 5;
	result.bank_revision = payload.action == shop_trade_action::discard_invalid ? 7 : 8;
	result.shop_revision = 10;
	result.keeper_cash = keeper_after;
	result.keeper_cash_recorded = true;
	result.player_owner_revision = 12;
	result.counterparty_owner_revision = 13;
	result.item_count = 2;
	result.item_uids[0] = 500;
	result.item_uids[1] = 501;
	result.item_revisions[0] = payload.action == shop_trade_action::buy_produced ? 1 : 5;
	result.item_revisions[1] = payload.action == shop_trade_action::buy_produced ? 1 : 3;
	return result;
}

bool buying(shop_trade_action action)
{
	return action == shop_trade_action::buy_existing ||
	       action == shop_trade_action::buy_produced;
}

critical_command admitted(const shop_trade_payload &payload, uint8_t operation,
			  const economic_account_key &wallet, const economic_account_key &bank,
			  const economic_account_key &keeper, economic_frozen_intent *intent)
{
	critical_command command = {};
	assert(shop_trade_command_build(&command, id(operation), payload,
					critical_source_site::command,
					critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	assert(shop_trade_accounting_intent(command, id(3), wallet, bank, keeper,
					    &command.accounting_intent) == error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	assert(economic_intent_decode(command.accounting_intent, intent) == error::ok);
	return command;
}

void run_case(shop_trade_action action, bool nested, uint8_t operation,
	      const economic_account_key &wallet, const economic_account_key &bank,
	      const economic_account_key &keeper)
{
	const auto payload = payload_for(action, nested);
	economic_frozen_intent intent;
	const auto command = admitted(payload, operation, wallet, bank, keeper, &intent);
	economic_frozen_intent decoded_intent;
	shop_trade_payload decoded_payload = {};
	economic_account_key decoded_wallet, decoded_bank, decoded_keeper;
	assert(shop_trade_accounting_decode(command, &decoded_intent, &decoded_payload,
					    &decoded_wallet, &decoded_bank,
					    &decoded_keeper) == error::ok);
	assert(decoded_payload.action == action &&
	       economic_account_key_equal(decoded_wallet, wallet) &&
	       economic_account_key_equal(decoded_bank, bank) &&
	       economic_account_key_equal(decoded_keeper, keeper));
	auto authority = authority_for(payload, wallet, bank, keeper);
	const int64_t cash = buying(action)				  ? 700 :
			     action == shop_trade_action::discard_invalid ? 500 :
									    300;
	const auto result = result_for(payload, cash);
	economic_accounting_plan plan;
	assert(shop_trade_accounting_plan(command, intent, authority, result, &plan) == error::ok);
	assert(economic_plan_validate_structure(plan) == error::ok);
	assert(plan.item_events.size() == 2);
	assert(plan.accounts.size() == (action == shop_trade_action::discard_invalid ? 0U : 3U));
	assert(plan.postings.size() == (action == shop_trade_action::discard_invalid ? 0U : 2U));
	assert(plan.item_events[0].after.owner.type ==
	       (buying(action)				? item_owner_type::player :
		action == shop_trade_action::sell_store ? item_owner_type::shopkeeper :
							  item_owner_type::destruction));
	if (nested)
	{
		assert(plan.items_before.size() == 5);
		assert(plan.item_events[0].after.root_uid == 601);
		assert(plan.item_events[0].after.parent_uid == 600);
	}
	auto damaged = result;
	damaged.keeper_cash++;
	assert(shop_trade_accounting_plan(command, intent, authority, damaged, &plan) ==
	       error::corrupt_evidence);
	damaged = result;
	damaged.item_revisions[1]++;
	assert(shop_trade_accounting_plan(command, intent, authority, damaged, &plan) ==
	       error::corrupt_evidence);
	authority.keeper_cash_before++;
	assert(shop_trade_accounting_plan(command, intent, authority, result, &plan) ==
	       error::stale_revision);
	authority = authority_for(payload, wallet, bank, keeper);
	authority.item_vnums_before[0]++;
	assert(shop_trade_accounting_plan(command, intent, authority, result, &plan) ==
	       error::stale_revision);
	if (nested)
	{
		authority = authority_for(payload, wallet, bank, keeper);
		authority.items_before.pop_back();
		authority.item_vnums_before.pop_back();
		assert(shop_trade_accounting_plan(command, intent, authority, result, &plan) ==
		       error::incomplete_coverage);
	}
}
} // namespace

int main()
{
	const auto lineage = id(2);
	const economic_account_key wallet = { lineage, economic_account_kind::wallet, 11, 0 };
	const economic_account_key bank = { lineage, economic_account_kind::bank, 12, 1 };
	const economic_account_key keeper = { lineage, economic_account_kind::treasury, 13, 0 };
	run_case(shop_trade_action::buy_existing, false, 4, wallet, bank, keeper);
	run_case(shop_trade_action::buy_produced, true, 5, wallet, bank, keeper);
	run_case(shop_trade_action::sell_store, false, 6, wallet, bank, keeper);
	run_case(shop_trade_action::sell_destroy, false, 7, wallet, bank, keeper);
	run_case(shop_trade_action::discard_invalid, false, 8, wallet, bank, keeper);

	auto stationary = payload_for(shop_trade_action::sell_store);
	stationary.expected_keeper_cash = 100;
	economic_frozen_intent intent;
	auto command = admitted(stationary, 9, wallet, bank, keeper, &intent);
	auto authority = authority_for(stationary, wallet, bank, keeper);
	auto result = result_for(stationary, 100);
	economic_accounting_plan plan;
	assert(shop_trade_accounting_plan(command, intent, authority, result, &plan) == error::ok);
	assert(plan.accounts.size() == 4 && plan.postings.size() == 2);
	assert(plan.accounts[3].key.kind == economic_account_kind::issuance);
	assert(plan.postings[1].copper == -200);

	auto roaming = stationary;
	roaming.keeper_roaming = 1;
	command = admitted(roaming, 10, wallet, bank, keeper, &intent);
	authority = authority_for(roaming, wallet, bank, keeper);
	assert(shop_trade_accounting_plan(command, intent, authority, result, &plan) ==
	       error::negative_holding);
	roaming.keeper_vnum = 11005;
	command = admitted(roaming, 11, wallet, bank, keeper, &intent);
	authority = authority_for(roaming, wallet, bank, keeper);
	assert(shop_trade_accounting_plan(command, intent, authority, result, &plan) == error::ok);
	assert(plan.accounts[3].key.kind == economic_account_kind::issuance);

	auto free_buy = payload_for(shop_trade_action::buy_existing);
	free_buy.price = 0;
	command = admitted(free_buy, 12, wallet, bank, keeper, &intent);
	authority = authority_for(free_buy, wallet, bank, keeper);
	authority.balances_before.wallet.amount = { 0, 0, 10, 9 };
	result = result_for(free_buy, 500);
	result.wallet.amount = { 0, 0, 0, 10 };
	assert(shop_trade_accounting_plan(command, intent, authority, result, &plan) == error::ok);
	assert(plan.accounts.size() == 3 && plan.postings.size() == 1);
	assert(plan.postings[0].copper == 0);
	assert(plan.postings[0].delta == (economic_coin_vector{ 0, 0, -10, 1 }));

	auto tampered = command;
	tampered.accounting_intent.front() ^= 1;
	economic_frozen_intent decoded_intent;
	shop_trade_payload decoded_payload = {};
	economic_account_key decoded_wallet, decoded_bank, decoded_keeper;
	assert(shop_trade_accounting_decode(tampered, &decoded_intent, &decoded_payload,
					    &decoded_wallet, &decoded_bank,
					    &decoded_keeper) != error::ok);
	assert(!decoded_wallet.authority_id);
	return 0;
}
