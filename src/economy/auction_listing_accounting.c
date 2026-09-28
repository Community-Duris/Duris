#include "economy/auction_listing_accounting.h"

#include "economy/auction_accounting.h"

#include <algorithm>
#include <climits>
#include <new>
#include <span>
#include <utility>

namespace
{
using error = economic_accounting_error;

bool accounts_valid(const economic_account_key &wallet, const economic_account_key &bank,
		    uint8_t racewar)
{
	return economic_account_key_valid(wallet) && economic_account_key_valid(bank) &&
	       wallet.kind == economic_account_kind::wallet && !wallet.context_id &&
	       bank.kind == economic_account_kind::bank && bank.context_id == racewar &&
	       wallet.lineage.bytes == bank.lineage.bytes &&
	       wallet.authority_id != bank.authority_id;
}

bool listing_valid(const auction_command_payload &payload)
{
	if (payload.action != auction_action::list || !payload.actor_pid || payload.auction_id ||
	    !payload.item_count || payload.listing_fee < 0 || payload.listing_fee > UINT_MAX ||
	    payload.start_price < 0 || payload.start_price > UINT_MAX || payload.buy_price < 0 ||
	    payload.buy_price > UINT_MAX ||
	    (payload.buy_price && payload.buy_price < payload.start_price) ||
	    !payload.object_blob_size)
		return false;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &item = payload.items[index];
		if (!item.item_uid || item.expected_item_revision == UINT64_MAX)
			return false;
		for (size_t previous = 0; previous < index; ++previous)
			if (payload.items[previous].item_uid == item.item_uid)
				return false;
	}
	return true;
}

std::vector<uint8_t> facts_for(const economic_account_key &wallet, const economic_account_key &bank)
{
	std::vector<uint8_t> facts;
	facts.reserve(16);
	for (uint64_t value : { wallet.authority_id, bank.authority_id })
		for (size_t byte = 0; byte < 8; ++byte)
			facts.push_back(static_cast<uint8_t>(value >> (byte * 8)));
	return facts;
}

economic_source_event source_for(const critical_command &command)
{
	return { economic_source_kind::auction, command.operation_id, command.operation_id, 0, 0 };
}

economic_coin_vector copper(int64_t amount)
{
	return { amount, 0, 0, 0 };
}

economic_coin_vector canonical_wallet(int64_t amount)
{
	economic_coin_vector coins = {};
	constexpr int64_t units[] = { 1, 10, 100, 1000 };
	for (size_t index = coins.size(); index-- > 0;)
	{
		coins[index] = amount / units[index];
		amount %= units[index];
	}
	return coins;
}
} // namespace

economic_accounting_error auction_listing_accounting_intent(const critical_command &command,
							    const critical_operation_id &epoch,
							    const economic_account_key &wallet,
							    const economic_account_key &bank,
							    std::vector<uint8_t> *encoded)
{
	if (!encoded || command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    critical_operation_id_is_zero(epoch))
		return error::invalid_version;
	auction_command_payload payload = {};
	if (!auction_command_decode_payload(command, &payload))
		return error::corrupt_evidence;
	if (!listing_valid(payload) || !accounts_valid(wallet, bank, payload.racewar))
		return error::invalid_identity;
	try
	{
		economic_admission_facts facts;
		facts.metadata.lineage = wallet.lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = payload.actor_pid;
		facts.metadata.writer_id = ECONOMIC_WRITER_AUCTION_LISTING;
		facts.metadata.reason = economic_reason::auction_listing;
		facts.metadata.source_event = source_for(command);
		facts.facts = facts_for(wallet, bank);
		return economic_intent_freeze(command, facts, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error auction_listing_accounting_decode(const critical_command &command,
							    economic_frozen_intent *intent,
							    auction_command_payload *payload,
							    economic_account_key *wallet,
							    economic_account_key *bank)
{
	if (!intent || !payload || !wallet || !bank ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return error::invalid_version;
	try
	{
		auction_command_payload parsed_payload = {};
		if (!auction_command_decode_payload(command, &parsed_payload) ||
		    parsed_payload.action != auction_action::list)
			return error::invalid_identity;
		economic_frozen_intent parsed_intent;
		if (economic_intent_decode(command.accounting_intent, &parsed_intent) !=
			    error::ok ||
		    economic_intent_verify_binding(command, parsed_intent) != error::ok)
			return error::corrupt_evidence;
		const auto facts = std::span<const uint8_t>(parsed_intent.admission.facts);
		if (facts.size() != 16)
			return error::invalid_identity;
		const auto number = [&](size_t offset)
		{
			uint64_t value = 0;
			for (size_t byte = 0; byte < 8; ++byte)
				value |= static_cast<uint64_t>(facts[offset + byte]) << (byte * 8);
			return value;
		};
		const auto &lineage = parsed_intent.admission.metadata.lineage;
		const economic_account_key parsed_wallet = { lineage, economic_account_kind::wallet,
							     number(0), 0 };
		const economic_account_key parsed_bank = { lineage, economic_account_kind::bank,
							   number(8), parsed_payload.racewar };
		critical_command projected = command;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		const auto frozen = auction_listing_accounting_intent(
			projected, parsed_intent.admission.metadata.epoch, parsed_wallet,
			parsed_bank, &expected);
		if (frozen != error::ok || expected != command.accounting_intent)
			return error::unauthorized;
		*intent = std::move(parsed_intent);
		*payload = parsed_payload;
		*wallet = parsed_wallet;
		*bank = parsed_bank;
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error auction_listing_accounting_plan(
	const critical_command &command, const economic_frozen_intent &intent,
	const auction_listing_accounting_authority &authority, const auction_command_result &result,
	economic_accounting_plan *plan)
{
	if (!plan)
		return error::corrupt_evidence;
	try
	{
		auto status = economic_intent_verify_binding(command, intent);
		if (status != error::ok)
			return status;
		auction_command_payload payload = {};
		if (!auction_command_decode_payload(command, &payload))
			return error::corrupt_evidence;
		if (!listing_valid(payload) ||
		    !accounts_valid(authority.wallet, authority.bank, payload.racewar) ||
		    !economic_account_key_valid(authority.escrow) ||
		    authority.escrow.kind != economic_account_kind::auction_escrow ||
		    authority.escrow.context_id ||
		    authority.escrow.lineage.bytes != authority.wallet.lineage.bytes ||
		    authority.escrow.authority_id == authority.wallet.authority_id ||
		    authority.escrow.authority_id == authority.bank.authority_id)
			return error::invalid_identity;
		const auto &meta = intent.admission.metadata;
		const auto source = source_for(command);
		if (meta.writer_id != ECONOMIC_WRITER_AUCTION_LISTING ||
		    meta.reason != economic_reason::auction_listing ||
		    meta.actor_kind != economic_actor_kind::domain ||
		    meta.actor_id != payload.actor_pid || !meta.source_event ||
		    meta.source_event->kind != source.kind ||
		    meta.source_event->source.bytes != source.source.bytes ||
		    meta.source_event->generation.bytes != source.generation.bytes ||
		    meta.source_event->sequence != source.sequence ||
		    meta.source_event->slot != source.slot ||
		    meta.lineage.bytes != authority.wallet.lineage.bytes ||
		    meta.epoch.bytes != authority.epoch.bytes ||
		    !critical_operation_id_is_zero(meta.original_operation_id) ||
		    intent.admission.facts != facts_for(authority.wallet, authority.bank))
			return error::unauthorized;
		if (authority.items_before.size() != payload.item_count ||
		    authority.balances_before.wallet_revision != payload.expected_wallet_revision ||
		    authority.balances_before.bank_revision != payload.expected_bank_revision ||
		    authority.balances_before.wallet_revision == UINT64_MAX ||
		    authority.balances_before.bank_revision == UINT64_MAX ||
		    authority.player_owner_revision_before == UINT64_MAX)
			return error::stale_revision;
		int64_t wallet_before = 0;
		status = economic_coin_value(authority.balances_before.wallet.amount,
					     &wallet_before);
		if (status != error::ok)
			return status;
		if (wallet_before < payload.listing_fee)
			return error::negative_holding;
		if (result.action != auction_action::list ||
		    result.event_type != auction_event_type::listed || !result.auction_id ||
		    result.status != 1 || result.seller_pid != payload.actor_pid ||
		    result.winner_pid || result.previous_bidder_pid || result.final_price ||
		    result.wallet_value_delta != -payload.listing_fee ||
		    result.wallet.amount != canonical_wallet(wallet_before - payload.listing_fee) ||
		    result.bank.amount != authority.balances_before.bank.amount ||
		    result.wallet_revision != authority.balances_before.wallet_revision + 1 ||
		    result.bank_revision != authority.balances_before.bank_revision + 1 ||
		    result.auction_revision != 1 ||
		    result.player_owner_revision != authority.player_owner_revision_before + 1 ||
		    result.auction_owner_revision != 1 || result.item_count != payload.item_count)
			return error::corrupt_evidence;
		economic_accounting_plan candidate;
		status = economic_intent_plan_metadata(command, intent, &candidate.metadata);
		if (status != error::ok)
			return status;
		candidate.accounts.push_back(
			{ authority.wallet, authority.balances_before.wallet.amount,
			  result.wallet.amount, authority.balances_before.wallet_revision,
			  result.wallet_revision });
		candidate.accounts.push_back({ authority.escrow, {}, {}, 0, 1 });
		economic_coin_vector delta = {};
		status = economic_coin_delta(candidate.accounts[0].before,
					     candidate.accounts[0].after, &delta);
		if (status != error::ok)
			return status;
		if (std::any_of(delta.begin(), delta.end(), [](int64_t part) { return part != 0; }))
			candidate.postings.push_back({ 0, 0, 0, delta, -payload.listing_fee });
		if (payload.listing_fee)
		{
			candidate.accounts.push_back(
				{ { authority.wallet.lineage, economic_account_kind::sink,
				    ECONOMIC_AUCTION_LISTING_FEE_SINK_ID, 0 },
				  {},
				  {},
				  0,
				  0 });
			candidate.postings.push_back(
				{ 1, 2, 0, copper(payload.listing_fee), payload.listing_fee });
		}
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const auto &item = authority.items_before[index];
			const auto &position = item.position;
			if (item.uid != payload.items[index].item_uid ||
			    position.owner.type != item_owner_type::player ||
			    position.owner.id != payload.actor_pid || position.owner.context_id ||
			    position.root_uid != item.uid || position.parent_uid ||
			    position.revision != payload.items[index].expected_item_revision ||
			    position.state != item_custody_state::active)
				return error::stale_revision;
			if (result.item_uids[index] != item.uid ||
			    result.item_revisions[index] != position.revision + 1)
				return error::corrupt_evidence;
			economic_item_position after = position;
			after.owner = { item_owner_type::auction, result.auction_id, 0 };
			after.revision++;
			candidate.items_before.push_back(item);
			candidate.items_after.push_back({ item.uid, after });
			candidate.item_events.push_back(
				{ static_cast<uint32_t>(index), 0, item.uid, position, after });
		}
		for (size_t index = payload.item_count; index < AUCTION_COMMAND_MAX_ITEMS; ++index)
			if (result.item_uids[index] || result.item_revisions[index])
				return error::corrupt_evidence;
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
