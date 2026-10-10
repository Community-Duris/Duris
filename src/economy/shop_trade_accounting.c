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

bool shared_accounts_valid(const economic_account_key &wallet, const economic_account_key &bank,
			   uint8_t racewar)
{
	return economic_account_key_valid(wallet) && economic_account_key_valid(bank) &&
	       wallet.kind == economic_account_kind::wallet && !wallet.context_id &&
	       bank.kind == economic_account_kind::bank && bank.context_id == racewar &&
	       wallet.lineage.bytes == bank.lineage.bytes &&
	       wallet.authority_id != bank.authority_id;
}

bool buying(shop_trade_action action)
{
	return action == shop_trade_action::buy_existing ||
	       action == shop_trade_action::buy_produced;
}

economic_account_key shared_counterparty(const critical_operation_id &lineage,
					 shop_trade_action action)
{
	return { lineage,
		 buying(action) || action == shop_trade_action::discard_invalid ?
			 economic_account_kind::sink :
			 economic_account_kind::issuance,
		 buying(action) || action == shop_trade_action::discard_invalid ?
			 ECONOMIC_SHOP_BUY_SINK_ID :
			 ECONOMIC_SHOP_SELL_ISSUANCE_ID,
		 0 };
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
	return (command.payload_version == SHOP_TRADE_PAYLOAD_VERSION ||
		shop_trade_payload_version_is_accounted(command.payload_version)) &&
	       shop_trade_command_decode_payload(command, payload) && payload->keeper_vnum > 0;
}
} // namespace

economic_accounting_error
shop_trade_accounting_intent(const critical_command &command, const critical_operation_id &epoch,
			     const economic_account_key &wallet, const economic_account_key &bank,
			     const economic_account_key &keeper, std::vector<uint8_t> *encoded)
{
	if (!encoded || command.payload_version != SHOP_TRADE_PAYLOAD_VERSION ||
	    command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
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

economic_accounting_error shop_trade_shared_accounting_intent(const critical_command &command,
							      const critical_operation_id &epoch,
							      const economic_account_key &wallet,
							      const economic_account_key &bank,
							      std::vector<uint8_t> *encoded)
{
	if (!encoded || !shop_trade_payload_version_is_accounted(command.payload_version) ||
	    command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    critical_operation_id_is_zero(epoch))
		return error::invalid_version;
	shop_trade_payload payload = {};
	if (!shop_payload_valid(command, &payload))
		return error::corrupt_evidence;
	if (!shared_accounts_valid(wallet, bank, payload.racewar))
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
		facts.facts.reserve(16);
		append_u64(&facts.facts, wallet.authority_id);
		append_u64(&facts.facts, bank.authority_id);
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
		const bool shared = facts.size() == 16;
		if ((!shared && facts.size() != 24) ||
		    shared != (shop_trade_payload_version_is_accounted(command.payload_version)))
			return error::invalid_identity;
		const auto &lineage = parsed_intent.admission.metadata.lineage;
		const economic_account_key parsed_wallet = { lineage, economic_account_kind::wallet,
							     read_u64(facts, 0), 0 };
		const economic_account_key parsed_bank = { lineage, economic_account_kind::bank,
							   read_u64(facts, 8),
							   parsed_payload.racewar };
		const economic_account_key parsed_keeper =
			shared ? shared_counterparty(lineage, parsed_payload.action) :
				 economic_account_key{ lineage, economic_account_kind::treasury,
						       read_u64(facts, 16), 0 };
		critical_command projected = command;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		const auto status =
			shared ? shop_trade_shared_accounting_intent(
					 projected, parsed_intent.admission.metadata.epoch,
					 parsed_wallet, parsed_bank, &expected) :
				 shop_trade_accounting_intent(
					 projected, parsed_intent.admission.metadata.epoch,
					 parsed_wallet, parsed_bank, parsed_keeper, &expected);
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
		const bool shared = intent.admission.facts.size() == 16;
		if (shared ? (!shared_accounts_valid(authority.wallet_account,
						     authority.bank_account, payload.racewar) ||
			      !economic_account_key_equal(
				      authority.keeper_account,
				      shared_counterparty(authority.wallet_account.lineage,
							  payload.action))) :
			     !accounts_valid(authority.wallet_account, authority.bank_account,
					     authority.keeper_account, payload.racewar))
			return error::invalid_identity;
		critical_command projected = command;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		status = shared ? shop_trade_shared_accounting_intent(projected, authority.epoch,
								      authority.wallet_account,
								      authority.bank_account,
								      &expected) :
				  shop_trade_accounting_intent(projected, authority.epoch,
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
				  shared ? economic_coin_vector{} :
					   economic_coin_vector{ authority.keeper_cash_before, 0, 0,
								 0 },
				  shared ? economic_coin_vector{} :
					   economic_coin_vector{ keeper_after, 0, 0, 0 },
				  shared ? 0 : authority.shop_revision_before,
				  shared ? 0 : result.shop_revision }
			};
			// A free purchase has no monetary counterparty. Retain native wallet/bank
			// revisions and any copper-neutral denomination normalization only.
			if (shared && !payload.price)
				candidate.accounts.pop_back();
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
			if (!shared && keeper_after != authority.keeper_cash_before)
				candidate.postings.push_back(
					{ static_cast<uint32_t>(candidate.postings.size()),
					  2,
					  0,
					  { keeper_after - authority.keeper_cash_before, 0, 0, 0 },
					  keeper_after - authority.keeper_cash_before });
			if (shared)
			{
				economic_coin_vector opposite = {};
				for (size_t denomination = 0; denomination < delta.size();
				     ++denomination)
				{
					if (delta[denomination] == INT64_MIN)
						return error::overflow;
					opposite[denomination] = -delta[denomination];
				}
				if (payload.price)
					candidate.postings.push_back(
						{ static_cast<uint32_t>(candidate.postings.size()),
						  2, 0, opposite,
						  buying(payload.action) ? payload.price :
									   -payload.price });
			}
			else if (issued)
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

#include <type_traits>
namespace
{
bool shop_accounting_size_add(size_t &total, size_t amount) noexcept
{
	if (amount > SIZE_MAX - total)
		return false;
	total += amount;
	return true;
}
bool shop_accounting_payload_heap(const shop_trade_payload &p, size_t &total) noexcept
{
	for (const auto *binding :
	     { &p.recovery_manifest.player_before, &p.recovery_manifest.player_after,
	       &p.recovery_manifest.keeper_before, &p.recovery_manifest.keeper_after,
	       &p.recovery_manifest.live_target_before, &p.recovery_manifest.live_target_after })
		if (binding->ordered_item_uids.capacity() > SIZE_MAX / sizeof(uint64_t) ||
		    !shop_accounting_size_add(total, binding->ordered_item_uids.capacity() *
							     sizeof(uint64_t)))
			return false;
	return true;
}
struct shop_accounting_decode_work
{
	shop_trade_payload parsed{}, revalidated{};
	economic_frozen_intent intent;
	critical_command projection;
	economic_admission_facts admission;
	std::vector<uint8_t> expected;
	economic_account_key wallet{}, bank{}, keeper{};
};
struct shop_accounting_decode_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer;
	const shop_accounting_decode_work *work = nullptr;
	bool denied = false;
	static bool forward(size_t amount, void *opaque) noexcept
	{
		auto &self = *static_cast<shop_accounting_decode_budget *>(opaque);
		if (self.denied || !self.reserve || !self.reserve(amount, self.context))
		{
			self.denied = true;
			return false;
		}
		return true;
	}
	bool prefix(size_t &out, size_t additional = 0) noexcept
	{
		// Public parameters/results; payload/intent/facts locals live in work.
		// Original facts span/shared flag/lineage/status, read_u64 arguments,
		// loop/result, account/source/reason helper arguments and return values;
		// current/prefix/heap observer parameters, binding range/iterators.
		constexpr size_t frames =
			8 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(std::span<const uint8_t>) +
			sizeof(bool) + sizeof(error) + 2 * sizeof(void *) + 3 * sizeof(size_t) +
			sizeof(uint64_t) + 2 * sizeof(economic_account_key) +
			sizeof(economic_source_event) + sizeof(economic_reason) +
			sizeof(shop_trade_action) + 14 * sizeof(void *) + 10 * sizeof(size_t) +
			8 * sizeof(bool) + 6 * sizeof(const shop_trade_recovery_forest_binding *) +
			sizeof(std::initializer_list<const shop_trade_recovery_forest_binding *>) +
			2 * sizeof(const shop_trade_recovery_forest_binding **);
		size_t value = outer, heap = 0;
		if (!shop_accounting_size_add(value, sizeof(*this)) ||
		    !shop_accounting_size_add(value, sizeof(shop_accounting_decode_work)) ||
		    !shop_accounting_size_add(value, frames) ||
		    !shop_accounting_size_add(value, critical_command_copy_frame_bytes()) ||
		    !shop_accounting_size_add(value, critical_command_valid_frame_bytes()))
		{
			denied = true;
			return false;
		}
		if (work &&
		    (!shop_accounting_payload_heap(work->parsed, value) ||
		     !shop_accounting_payload_heap(work->revalidated, value) ||
		     !critical_command_current_heap_bytes(work->projection, &heap) ||
		     !shop_accounting_size_add(value, heap) ||
		     !shop_accounting_size_add(value, work->intent.admission.facts.capacity()) ||
		     !shop_accounting_size_add(value, work->admission.facts.capacity()) ||
		     !shop_accounting_size_add(value, work->expected.capacity())))
		{
			denied = true;
			return false;
		}
		if (!shop_accounting_size_add(value, additional))
		{
			denied = true;
			return false;
		}
		out = value;
		return true;
	}
	bool peak(size_t extra = 0) noexcept
	{
		size_t value = 0;
		if (!prefix(value, extra))
		{
			denied = true;
			return false;
		}
		return forward(value, this);
	}
};
} // namespace

economic_accounting_error shop_trade_accounting_decode_bounded(
	const critical_command &command, economic_frozen_intent *intent,
	shop_trade_payload *payload, economic_account_key *wallet, economic_account_key *bank,
	economic_account_key *keeper, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer) noexcept
{
	if (!intent || !payload || !wallet || !bank || !keeper || !reserve ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
		return error::invalid_version;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return error::capacity;
#else
	shop_accounting_decode_budget budget{ reserve, context, outer };
	if (!budget.peak())
		return error::capacity;
	if (!critical_command_envelope_valid(command))
		return error::invalid_version;
	shop_accounting_decode_work work;
	budget.work = &work;
	size_t nested = 0, request = 0;
	static_assert(std::is_nothrow_move_assignable_v<economic_frozen_intent>);
	static_assert(std::is_nothrow_move_assignable_v<shop_trade_payload>);
	try
	{
		if ((command.payload_version != SHOP_TRADE_PAYLOAD_VERSION &&
		     !shop_trade_payload_version_is_accounted(command.payload_version)) ||
		    !budget.prefix(nested) ||
		    !shop_trade_command_decode_payload_bounded(
			    command, &work.parsed, shop_accounting_decode_budget::forward, &budget,
			    nested) ||
		    work.parsed.keeper_vnum <= 0)
			return budget.denied ? error::capacity : error::invalid_identity;
		if (!budget.prefix(nested) ||
		    economic_intent_decode_bounded(command.accounting_intent, &work.intent,
						   shop_accounting_decode_budget::forward, &budget,
						   nested) != error::ok ||
		    !budget.prefix(nested) ||
		    economic_intent_verify_binding_bounded(command, work.intent,
							   shop_accounting_decode_budget::forward,
							   &budget, nested) != error::ok)
			return budget.denied ? error::capacity : error::corrupt_evidence;
		const auto facts = std::span<const uint8_t>(work.intent.admission.facts);
		const bool shared = facts.size() == 16;
		if ((!shared && facts.size() != 24) ||
		    shared != shop_trade_payload_version_is_accounted(command.payload_version))
			return error::invalid_identity;
		const auto &lineage = work.intent.admission.metadata.lineage;
		work.wallet = { lineage, economic_account_kind::wallet, read_u64(facts, 0), 0 };
		work.bank = { lineage, economic_account_kind::bank, read_u64(facts, 8),
			      work.parsed.racewar };
		work.keeper = shared ? shared_counterparty(lineage, work.parsed.action) :
				       economic_account_key{ lineage,
							     economic_account_kind::treasury,
							     read_u64(facts, 16), 0 };
		if (!budget.peak() ||
		    !critical_command_fresh_copy_request_bytes(command, &request) ||
		    !budget.peak(request))
			return error::capacity;
		work.projection = command;
		work.projection.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		work.projection.accounting_intent.clear();
		work.projection.publication_required = false;
		// Reproduce the original projected-intent builder's own full decoder,
		// not just a metadata comparison on the first decoded payload.
		if (critical_operation_id_is_zero(work.intent.admission.metadata.epoch) ||
		    (!shared && work.projection.payload_version != SHOP_TRADE_PAYLOAD_VERSION) ||
		    (shared &&
		     !shop_trade_payload_version_is_accounted(work.projection.payload_version)) ||
		    !budget.prefix(nested) ||
		    !shop_trade_command_decode_payload_bounded(
			    work.projection, &work.revalidated,
			    shop_accounting_decode_budget::forward, &budget, nested) ||
		    work.revalidated.keeper_vnum <= 0 ||
		    (shared ? !shared_accounts_valid(work.wallet, work.bank,
						     work.revalidated.racewar) :
			      !accounts_valid(work.wallet, work.bank, work.keeper,
					      work.revalidated.racewar)))
			return budget.denied ? error::capacity : error::unauthorized;
		work.admission.metadata.lineage = work.wallet.lineage;
		work.admission.metadata.epoch = work.intent.admission.metadata.epoch;
		work.admission.metadata.actor_kind = economic_actor_kind::domain;
		work.admission.metadata.actor_id = work.revalidated.player_pid;
		work.admission.metadata.writer_id = ECONOMIC_WRITER_SHOP_TRADE;
		work.admission.metadata.reason = reason_for(work.revalidated.action);
		if (!cleanup(work.revalidated.action))
			work.admission.metadata.source_event =
				source_for(work.projection, work.revalidated);
		if (!budget.peak((shared ? 16 : 24) + 5 * sizeof(void *) + 4 * sizeof(size_t) +
				 sizeof(uint64_t)))
			return error::capacity;
		work.admission.facts.reserve(shared ? 16 : 24);
		append_u64(&work.admission.facts, work.wallet.authority_id);
		append_u64(&work.admission.facts, work.bank.authority_id);
		if (!shared)
			append_u64(&work.admission.facts, work.keeper.authority_id);
		if (!budget.prefix(nested))
			return error::capacity;
		const auto status = economic_intent_freeze_fixed_bounded(
			work.projection, work.admission, &work.expected,
			shop_accounting_decode_budget::forward, &budget, nested);
		if (status != error::ok || work.expected != command.accounting_intent)
			return budget.denied ? error::capacity : error::unauthorized;
		// All five transfers are nonallocating. Admit the actual vector move
		// assignment/destruction carriers before the first output changes.
		if (!budget.peak(10 * sizeof(void *) + 6 * sizeof(std::vector<uint64_t>) +
				 3 * sizeof(std::vector<uint8_t>) +
				 9 * sizeof(std::allocator<uint8_t>)))
			return error::capacity;
		*intent = std::move(work.intent);
		*payload = std::move(work.parsed);
		*wallet = work.wallet;
		*bank = work.bank;
		*keeper = work.keeper;
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
#endif
}

bool shop_trade_accounting_intent_current_heap_bytes(const economic_frozen_intent &value,
						     size_t *output) noexcept
{
	if (!output)
		return false;
	*output = value.admission.facts.capacity();
	return true;
}
bool shop_trade_accounting_payload_current_heap_bytes(const shop_trade_payload &value,
						      size_t *output) noexcept
{
	if (!output)
		return false;
	size_t bytes = 0;
	if (!shop_accounting_payload_heap(value, bytes))
		return false;
	*output = bytes;
	return true;
}
bool shop_trade_accounting_account_current_heap_bytes(const economic_account_key &,
						      size_t *output) noexcept
{
	if (!output)
		return false;
	*output = 0;
	return true;
}
size_t shop_trade_accounting_decoded_heap_observer_frame_bytes() noexcept
{
	// Public reference/output/result; payload helper reference/total/range,
	// actual six binding pointer array/initializer-list and range iterators,
	// checked-add total/amount/result and capacity access reference/result.
	return 6 * sizeof(void *) + 4 * sizeof(size_t) + 4 * sizeof(bool) +
	       6 * sizeof(const shop_trade_recovery_forest_binding *) +
	       sizeof(std::initializer_list<const shop_trade_recovery_forest_binding *>) +
	       2 * sizeof(const shop_trade_recovery_forest_binding **);
}
