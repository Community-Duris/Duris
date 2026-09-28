#include "economy/shop_trade_accounting.h"

#include <algorithm>
#include <climits>
#include <new>
#include <span>
#include <utility>

namespace
{
using error = economic_accounting_error;

bool accounts_valid(const economic_account_key &wallet, const economic_account_key &bank,
		    const economic_account_key &keeper, uint8_t racewar)
{
	return economic_account_key_valid(wallet) && economic_account_key_valid(bank) &&
	       economic_account_key_valid(keeper) && wallet.kind == economic_account_kind::wallet &&
	       !wallet.context_id && bank.kind == economic_account_kind::bank &&
	       bank.context_id == racewar && keeper.kind == economic_account_kind::treasury &&
	       !keeper.context_id && wallet.lineage.bytes == bank.lineage.bytes &&
	       wallet.lineage.bytes == keeper.lineage.bytes &&
	       wallet.authority_id != bank.authority_id &&
	       wallet.authority_id != keeper.authority_id &&
	       bank.authority_id != keeper.authority_id;
}

bool buying(shop_trade_action action)
{
	return action == shop_trade_action::buy_existing ||
	       action == shop_trade_action::buy_produced;
}

bool producing(shop_trade_action action)
{
	return action == shop_trade_action::buy_produced;
}

bool cleanup(shop_trade_action action)
{
	return action == shop_trade_action::discard_invalid;
}

economic_reason reason_for(shop_trade_action action)
{
	return cleanup(action) ? economic_reason::shop_cleanup :
	       buying(action)  ? economic_reason::shop_buy :
				 economic_reason::shop_sell;
}

economic_source_event source_for(const critical_command &command, const shop_trade_payload &payload)
{
	return { buying(payload.action) ? economic_source_kind::shop_stock :
					  economic_source_kind::item_action,
		 command.operation_id, command.operation_id,
		 buying(payload.action) ? payload.stock_item_uid : payload.selected_item_uid,
		 payload.shop_id };
}

void append_u64(std::vector<uint8_t> *bytes, uint64_t value)
{
	for (size_t byte = 0; byte < 8; ++byte)
		bytes->push_back(static_cast<uint8_t>(value >> (byte * 8)));
}

uint64_t read_u64(std::span<const uint8_t> bytes, size_t offset)
{
	uint64_t value = 0;
	for (size_t byte = 0; byte < 8; ++byte)
		value |= uint64_t(bytes[offset + byte]) << (byte * 8);
	return value;
}

std::vector<uint8_t> facts_for(const economic_account_key &wallet, const economic_account_key &bank,
			       const economic_account_key &keeper)
{
	std::vector<uint8_t> facts;
	facts.reserve(24);
	append_u64(&facts, wallet.authority_id);
	append_u64(&facts, bank.authority_id);
	append_u64(&facts, keeper.authority_id);
	return facts;
}

economic_coin_vector canonical_wallet(int64_t copper)
{
	economic_coin_vector result = {};
	constexpr int64_t units[] = { 1, 10, 100, 1000 };
	for (size_t index = result.size(); index-- > 0;)
	{
		result[index] = copper / units[index];
		copper %= units[index];
	}
	return result;
}

size_t snapshot_index(std::span<const economic_item_snapshot> items, uint64_t uid)
{
	const auto found = std::lower_bound(items.begin(), items.end(), uid,
					    [](const auto &item, uint64_t value)
					    { return item.uid < value; });
	return found == items.end() || found->uid != uid ?
		       items.size() :
		       static_cast<size_t>(found - items.begin());
}

item_owner_identity player_owner(const shop_trade_payload &payload)
{
	return { item_owner_type::player, payload.player_pid, 0 };
}

item_owner_identity shop_owner(const shop_trade_payload &payload)
{
	return { item_owner_type::shopkeeper, item_shopkeeper_owner_id(payload.shop_id), 0 };
}

item_owner_identity before_owner(const shop_trade_payload &payload)
{
	return buying(payload.action) ?
		       producing(payload.action) ? item_owner_identity{} : shop_owner(payload) :
	       cleanup(payload.action) ? shop_owner(payload) :
					 player_owner(payload);
}

item_owner_identity after_owner(const shop_trade_payload &payload)
{
	return buying(payload.action) ? player_owner(payload) :
	       payload.action == shop_trade_action::sell_store ?
					shop_owner(payload) :
					item_owner_identity{ item_owner_type::destruction, 0, 0 };
}

bool shop_payload_valid(const critical_command &command, shop_trade_payload *payload)
{
	return command.payload_version == SHOP_TRADE_PAYLOAD_VERSION &&
	       shop_trade_command_decode_payload(command, payload) && payload->keeper_vnum > 0;
}
} // namespace

economic_accounting_error
shop_trade_accounting_intent(const critical_command &command, const critical_operation_id &epoch,
			     const economic_account_key &wallet, const economic_account_key &bank,
			     const economic_account_key &keeper, std::vector<uint8_t> *encoded)
{
	if (!encoded || command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    critical_operation_id_is_zero(epoch))
		return error::invalid_version;
	shop_trade_payload payload = {};
	if (!shop_payload_valid(command, &payload))
		return error::corrupt_evidence;
	if (!accounts_valid(wallet, bank, keeper, payload.racewar))
		return error::invalid_identity;
	try
	{
		economic_admission_facts facts;
		facts.metadata.lineage = wallet.lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = payload.player_pid;
		facts.metadata.writer_id = ECONOMIC_WRITER_SHOP_TRADE;
		facts.metadata.reason = reason_for(payload.action);
		if (!cleanup(payload.action))
			facts.metadata.source_event = source_for(command, payload);
		facts.facts = facts_for(wallet, bank, keeper);
		return economic_intent_freeze(command, facts, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error
shop_trade_accounting_decode(const critical_command &command, economic_frozen_intent *intent,
			     shop_trade_payload *payload, economic_account_key *wallet,
			     economic_account_key *bank, economic_account_key *keeper)
{
	if (!intent || !payload || !wallet || !bank || !keeper ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return error::invalid_version;
	try
	{
		shop_trade_payload parsed_payload = {};
		if (!shop_payload_valid(command, &parsed_payload))
			return error::invalid_identity;
		economic_frozen_intent parsed_intent;
		if (economic_intent_decode(command.accounting_intent, &parsed_intent) !=
			    error::ok ||
		    economic_intent_verify_binding(command, parsed_intent) != error::ok)
			return error::corrupt_evidence;
		const auto facts = std::span<const uint8_t>(parsed_intent.admission.facts);
		if (facts.size() != 24)
			return error::invalid_identity;
		const auto &lineage = parsed_intent.admission.metadata.lineage;
		const economic_account_key parsed_wallet = { lineage, economic_account_kind::wallet,
							     read_u64(facts, 0), 0 };
		const economic_account_key parsed_bank = { lineage, economic_account_kind::bank,
							   read_u64(facts, 8),
							   parsed_payload.racewar };
		const economic_account_key parsed_keeper = { lineage,
							     economic_account_kind::treasury,
							     read_u64(facts, 16), 0 };
		critical_command projected = command;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		const auto status = shop_trade_accounting_intent(
			projected, parsed_intent.admission.metadata.epoch, parsed_wallet,
			parsed_bank, parsed_keeper, &expected);
		if (status != error::ok || expected != command.accounting_intent)
			return error::unauthorized;
		*intent = std::move(parsed_intent);
		*payload = parsed_payload;
		*wallet = parsed_wallet;
		*bank = parsed_bank;
		*keeper = parsed_keeper;
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error
shop_trade_accounting_plan(const critical_command &command, const economic_frozen_intent &intent,
			   const shop_trade_accounting_authority &authority,
			   const shop_trade_result &result, economic_accounting_plan *plan)
{
	if (!plan)
		return error::corrupt_evidence;
	try
	{
		auto status = economic_intent_verify_binding(command, intent);
		if (status != error::ok)
			return status;
		shop_trade_payload payload = {};
		if (!shop_payload_valid(command, &payload))
			return error::corrupt_evidence;
		if (!accounts_valid(authority.wallet_account, authority.bank_account,
				    authority.keeper_account, payload.racewar))
			return error::invalid_identity;
		critical_command projected = command;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		status = shop_trade_accounting_intent(projected, authority.epoch,
						      authority.wallet_account,
						      authority.bank_account,
						      authority.keeper_account, &expected);
		if (status != error::ok || expected != command.accounting_intent)
			return error::unauthorized;
		if (authority.shop_id != payload.shop_id ||
		    authority.keeper_vnum != payload.keeper_vnum ||
		    authority.keeper_cash_before != payload.expected_keeper_cash ||
		    authority.keeper_roaming != static_cast<bool>(payload.keeper_roaming) ||
		    authority.shop_revision_before != payload.expected_shop_revision ||
		    authority.balances_before.wallet_revision != payload.expected_wallet_revision ||
		    authority.balances_before.bank_revision != payload.expected_bank_revision)
			return error::stale_revision;
		if (authority.shop_revision_before == UINT64_MAX ||
		    authority.player_owner_revision_before == UINT64_MAX ||
		    authority.counterparty_owner_revision_before == UINT64_MAX)
			return error::overflow;
		if (authority.items_before.size() != authority.item_vnums_before.size() ||
		    authority.items_before.size() < payload.item_count ||
		    authority.items_before.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES)
			return error::capacity;
		int64_t wallet_before = 0;
		status = economic_coin_value(authority.balances_before.wallet.amount,
					     &wallet_before);
		if (status != error::ok)
			return status;
		int64_t bank_before = 0;
		status = economic_coin_value(authority.balances_before.bank.amount, &bank_before);
		if (status != error::ok)
			return status;
		const bool charged = !cleanup(payload.action);
		int64_t wallet_after = wallet_before;
		int64_t keeper_after = authority.keeper_cash_before;
		bool issued = false;
		if (charged)
		{
			if (authority.balances_before.wallet_revision == UINT64_MAX ||
			    authority.balances_before.bank_revision == UINT64_MAX)
				return error::overflow;
			if (buying(payload.action))
			{
				if (wallet_before < payload.price)
					return error::negative_holding;
				if (keeper_after > INT_MAX - payload.price)
					return error::overflow;
				wallet_after -= payload.price;
				keeper_after += payload.price;
			}
			else
			{
				if (wallet_before > INT64_MAX - payload.price)
					return error::overflow;
				wallet_after += payload.price;
				if (keeper_after >= payload.price)
					keeper_after -= payload.price;
				else if (authority.keeper_roaming && authority.keeper_vnum != 11005)
					return error::negative_holding;
				else
					issued = true;
			}
		}
		if (result.action != payload.action || !result.keeper_cash_recorded ||
		    result.keeper_cash != keeper_after ||
		    result.shop_revision != authority.shop_revision_before + 1 ||
		    result.wallet.amount != (charged ? canonical_wallet(wallet_after) :
						       authority.balances_before.wallet.amount) ||
		    result.bank.amount != authority.balances_before.bank.amount ||
		    result.wallet_revision != authority.balances_before.wallet_revision +
						      static_cast<uint64_t>(charged) ||
		    result.bank_revision != authority.balances_before.bank_revision +
						    static_cast<uint64_t>(charged) ||
		    result.player_owner_revision != authority.player_owner_revision_before + 1 ||
		    result.counterparty_owner_revision !=
			    authority.counterparty_owner_revision_before + 1 ||
		    result.item_count != payload.item_count)
			return error::corrupt_evidence;
		economic_accounting_plan candidate;
		status = economic_intent_plan_metadata(command, intent, &candidate.metadata);
		if (status != error::ok)
			return status;
		if (charged)
		{
			candidate.accounts = {
				{ authority.wallet_account, authority.balances_before.wallet.amount,
				  result.wallet.amount, authority.balances_before.wallet_revision,
				  result.wallet_revision },
				{ authority.bank_account, authority.balances_before.bank.amount,
				  result.bank.amount, authority.balances_before.bank_revision,
				  result.bank_revision },
				{ authority.keeper_account,
				  { authority.keeper_cash_before, 0, 0, 0 },
				  { keeper_after, 0, 0, 0 },
				  authority.shop_revision_before,
				  result.shop_revision }
			};
			economic_coin_vector delta = {};
			status = economic_coin_delta(candidate.accounts[0].before,
						     candidate.accounts[0].after, &delta);
			if (status != error::ok)
				return status;
			if (candidate.accounts[0].before != candidate.accounts[0].after)
				candidate.postings.push_back(
					{ static_cast<uint32_t>(candidate.postings.size()), 0, 0,
					  delta,
					  buying(payload.action) ? -payload.price :
								   payload.price });
			if (keeper_after != authority.keeper_cash_before)
				candidate.postings.push_back(
					{ static_cast<uint32_t>(candidate.postings.size()),
					  2,
					  0,
					  { keeper_after - authority.keeper_cash_before, 0, 0, 0 },
					  keeper_after - authority.keeper_cash_before });
			if (issued)
			{
				candidate.accounts.push_back(
					{ { authority.wallet_account.lineage,
					    economic_account_kind::issuance,
					    ECONOMIC_SHOP_UNFUNDED_SALE_ISSUANCE_ID, 0 },
					  {},
					  {},
					  0,
					  0 });
				candidate.postings.push_back(
					{ static_cast<uint32_t>(candidate.postings.size()),
					  3,
					  0,
					  { -payload.price, 0, 0, 0 },
					  -payload.price });
			}
		}
		candidate.items_before = authority.items_before;
		candidate.items_after = authority.items_before;
		std::vector<bool> witnessed(authority.items_before.size(), false);
		const auto from = before_owner(payload);
		const auto to = after_owner(payload);
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const auto &entry = payload.items[index];
			const size_t position_index =
				snapshot_index(authority.items_before, entry.item_uid);
			if (position_index == authority.items_before.size() ||
			    witnessed[position_index])
				return error::incomplete_coverage;
			witnessed[position_index] = true;
			const auto &before = authority.items_before[position_index].position;
			if (producing(payload.action))
			{
				if (before.state != item_custody_state::absent ||
				    authority.item_vnums_before[position_index])
					return error::stale_revision;
			}
			else if (before.state != item_custody_state::active ||
				 !item_owner_identity_equal(before.owner, from) ||
				 before.root_uid != entry.root_item_uid ||
				 before.parent_uid != entry.parent_item_uid ||
				 before.revision != entry.expected_item_revision ||
				 before.equipment_slot ||
				 authority.item_vnums_before[position_index] != entry.vnum ||
				 before.revision == UINT64_MAX)
				return error::stale_revision;
			economic_item_position after = before;
			after.owner = to;
			after.root_uid = producing(payload.action) ? payload.target_root_item_uid :
								     entry.root_item_uid;
			after.parent_uid = entry.item_uid == payload.selected_item_uid &&
							   payload.target_parent_item_uid ?
						   payload.target_parent_item_uid :
						   entry.parent_item_uid;
			after.revision = producing(payload.action) ? 1 : before.revision + 1;
			after.state = to.type == item_owner_type::destruction ?
					      item_custody_state::destroyed :
					      item_custody_state::active;
			after.equipment_slot = 0;
			if (result.item_uids[index] != entry.item_uid ||
			    result.item_revisions[index] != after.revision)
				return error::corrupt_evidence;
			candidate.items_after[position_index].position = after;
			candidate.item_events.push_back(
				{ static_cast<uint32_t>(index), 0, entry.item_uid, before, after });
		}
		for (size_t index = payload.item_count; index < SHOP_TRADE_MAX_ITEMS; ++index)
			if (result.item_uids[index] || result.item_revisions[index])
				return error::corrupt_evidence;
		if (producing(payload.action))
		{
			const size_t stock =
				snapshot_index(authority.items_before, payload.stock_item_uid);
			if (stock == authority.items_before.size() || witnessed[stock])
				return error::incomplete_coverage;
			witnessed[stock] = true;
			const auto &position = authority.items_before[stock].position;
			if (position.state != item_custody_state::active ||
			    !item_owner_identity_equal(position.owner, shop_owner(payload)) ||
			    position.root_uid != payload.stock_item_uid || position.parent_uid ||
			    position.revision != payload.expected_stock_item_revision ||
			    authority.item_vnums_before[stock] != payload.stock_vnum)
				return error::stale_revision;
			uint64_t ancestor_uid = payload.target_parent_item_uid;
			while (ancestor_uid)
			{
				const size_t ancestor =
					snapshot_index(authority.items_before, ancestor_uid);
				if (ancestor == authority.items_before.size() ||
				    witnessed[ancestor])
					return error::incomplete_coverage;
				witnessed[ancestor] = true;
				const auto &parent = authority.items_before[ancestor].position;
				if (parent.state != item_custody_state::active ||
				    !item_owner_identity_equal(parent.owner,
							       player_owner(payload)) ||
				    parent.root_uid != payload.target_root_item_uid ||
				    (ancestor_uid == payload.target_parent_item_uid &&
				     parent.revision != payload.expected_target_parent_revision))
					return error::stale_revision;
				ancestor_uid = parent.parent_uid;
			}
		}
		if (std::find(witnessed.begin(), witnessed.end(), false) != witnessed.end())
			return error::incomplete_coverage;
		status = economic_plan_normalize(&candidate);
		if (status != error::ok)
			return status;
		*plan = std::move(candidate);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}
