#include "economy/auction_item_claim_accounting.h"

#include <algorithm>
#include <climits>
#include <new>
#include <utility>

namespace
{
using error = economic_accounting_error;
constexpr size_t claim_fact_header_bytes = 82;
constexpr size_t claim_fact_row_bytes = 27;

template <typename T> T read_number(std::span<const uint8_t> bytes, size_t offset)
{
	T value = 0;
	for (size_t index = 0; index < sizeof(T); ++index)
		value |= static_cast<T>(bytes[offset + index]) << (8 * index);
	return value;
}

void append_u64(std::vector<uint8_t> *bytes, uint64_t value)
{
	for (size_t index = 0; index < 8; ++index)
		bytes->push_back(static_cast<uint8_t>(value >> (index * 8)));
}

void append_u32(std::vector<uint8_t> *bytes, uint32_t value)
{
	for (size_t index = 0; index < 4; ++index)
		bytes->push_back(static_cast<uint8_t>(value >> (index * 8)));
}

void append_u16(std::vector<uint8_t> *bytes, uint16_t value)
{
	bytes->push_back(static_cast<uint8_t>(value));
	bytes->push_back(static_cast<uint8_t>(value >> 8));
}

bool valid_accounts(const economic_account_key &wallet, const economic_account_key &bank,
		    uint8_t racewar)
{
	return economic_account_key_valid(wallet) && economic_account_key_valid(bank) &&
	       wallet.kind == economic_account_kind::wallet && !wallet.context_id &&
	       bank.kind == economic_account_kind::bank && bank.context_id == racewar &&
	       wallet.lineage.bytes == bank.lineage.bytes &&
	       wallet.authority_id != bank.authority_id;
}

bool valid_claim(const auction_command_payload &payload, const auction_item_claim_state &claim)
{
	if (payload.action != auction_action::claim_item || !payload.actor_pid ||
	    !payload.item_count || claim.auction_id != payload.auction_id || !claim.auction_id ||
	    !claim.seller_pid || claim.claimant_pid != payload.actor_pid ||
	    (claim.status != 2 && claim.status != 3) || claim.custody_state != 1 ||
	    !claim.auction_revision || claim.auction_revision == UINT64_MAX ||
	    claim.item_count != payload.item_count ||
	    critical_operation_id_is_zero(claim.listing_operation) ||
	    critical_operation_id_is_zero(claim.claim_source_operation) ||
	    critical_operation_id_equal(claim.listing_operation, claim.claim_source_operation))
		return false;
	const uint32_t staged_claimant = claim.status == 3 || !claim.winner_pid ? claim.seller_pid :
										  claim.winner_pid;
	if (claim.claimant_pid != staged_claimant)
		return false;
	for (size_t index = 0; index < claim.item_count; ++index)
	{
		const auto &row = claim.rows[index];
		const auto &item = payload.items[index];
		if (!row.uid || row.uid != item.item_uid ||
		    row.revision != item.expected_item_revision || row.revision == UINT64_MAX ||
		    row.vnum != item.vnum || row.claim_pid != payload.actor_pid || row.claimed)
			return false;
		for (size_t previous = 0; previous < index; ++previous)
			if (claim.rows[previous].uid == row.uid ||
			    claim.rows[previous].slot == row.slot)
				return false;
	}
	return true;
}

std::vector<uint8_t> frozen_facts(const economic_account_key &wallet,
				  const economic_account_key &bank,
				  const auction_item_claim_state &claim)
{
	std::vector<uint8_t> facts;
	facts.reserve(82 + claim.item_count * 27);
	append_u64(&facts, wallet.authority_id);
	append_u64(&facts, bank.authority_id);
	append_u32(&facts, claim.auction_id);
	append_u32(&facts, claim.seller_pid);
	append_u32(&facts, claim.winner_pid);
	append_u32(&facts, claim.claimant_pid);
	append_u32(&facts, claim.status);
	append_u32(&facts, claim.custody_state);
	append_u64(&facts, claim.auction_revision);
	facts.insert(facts.end(), claim.listing_operation.bytes.begin(),
		     claim.listing_operation.bytes.end());
	facts.insert(facts.end(), claim.claim_source_operation.bytes.begin(),
		     claim.claim_source_operation.bytes.end());
	append_u16(&facts, claim.item_count);
	for (size_t index = 0; index < claim.item_count; ++index)
	{
		const auto &row = claim.rows[index];
		append_u64(&facts, row.uid);
		append_u64(&facts, row.revision);
		append_u16(&facts, row.slot);
		append_u32(&facts, static_cast<uint32_t>(row.vnum));
		append_u32(&facts, row.claim_pid);
		facts.push_back(row.claimed ? 1 : 0);
	}
	return facts;
}

economic_source_event source_for(const auction_item_claim_state &claim)
{
	return { economic_source_kind::auction, claim.claim_source_operation,
		 claim.listing_operation, claim.auction_revision, 0 };
}
} // namespace

economic_accounting_error auction_item_claim_accounting_intent(
	const critical_command &command, const critical_operation_id &epoch,
	const economic_account_key &wallet, const economic_account_key &bank,
	const auction_item_claim_state &claim, std::vector<uint8_t> *encoded)
{
	if (!encoded || command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    critical_operation_id_is_zero(epoch))
		return error::invalid_version;
	auction_command_payload payload = {};
	if (!auction_command_decode_payload(command, &payload))
		return error::corrupt_evidence;
	if (!valid_accounts(wallet, bank, payload.racewar) || !valid_claim(payload, claim) ||
	    critical_operation_id_equal(command.operation_id, claim.listing_operation) ||
	    critical_operation_id_equal(command.operation_id, claim.claim_source_operation))
		return error::invalid_identity;
	try
	{
		economic_admission_facts facts;
		facts.metadata.lineage = wallet.lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.original_operation_id = claim.listing_operation;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = payload.actor_pid;
		facts.metadata.writer_id = ECONOMIC_WRITER_AUCTION_ITEM_CLAIM;
		facts.metadata.reason = economic_reason::auction_claim;
		facts.metadata.source_event = source_for(claim);
		facts.facts = frozen_facts(wallet, bank, claim);
		return economic_intent_freeze(command, facts, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error auction_item_claim_accounting_decode(const critical_command &command,
							       economic_frozen_intent *intent,
							       auction_command_payload *payload,
							       auction_item_claim_state *claim,
							       economic_account_key *wallet,
							       economic_account_key *bank)
{
	if (!intent || !payload || !claim || !wallet || !bank ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return error::invalid_version;
	try
	{
		auction_command_payload parsed_payload = {};
		if (!auction_command_decode_payload(command, &parsed_payload) ||
		    parsed_payload.action != auction_action::claim_item)
			return error::invalid_identity;
		economic_frozen_intent parsed_intent;
		if (economic_intent_decode(command.accounting_intent, &parsed_intent) !=
			    error::ok ||
		    economic_intent_verify_binding(command, parsed_intent) != error::ok)
			return error::corrupt_evidence;
		const auto facts = std::span<const uint8_t>(parsed_intent.admission.facts);
		if (facts.size() < claim_fact_header_bytes)
			return error::invalid_identity;
		const uint16_t count = read_number<uint16_t>(facts, 80);
		if (!count || count > AUCTION_COMMAND_MAX_ITEMS ||
		    facts.size() != claim_fact_header_bytes + count * claim_fact_row_bytes)
			return error::invalid_identity;
		const auto &lineage = parsed_intent.admission.metadata.lineage;
		const economic_account_key parsed_wallet = { lineage, economic_account_kind::wallet,
							     read_number<uint64_t>(facts, 0), 0 };
		const economic_account_key parsed_bank = { lineage, economic_account_kind::bank,
							   read_number<uint64_t>(facts, 8),
							   parsed_payload.racewar };
		auction_item_claim_state parsed_claim;
		parsed_claim.auction_id = read_number<uint32_t>(facts, 16);
		parsed_claim.seller_pid = read_number<uint32_t>(facts, 20);
		parsed_claim.winner_pid = read_number<uint32_t>(facts, 24);
		parsed_claim.claimant_pid = read_number<uint32_t>(facts, 28);
		parsed_claim.status = read_number<uint32_t>(facts, 32);
		parsed_claim.custody_state = read_number<uint32_t>(facts, 36);
		parsed_claim.auction_revision = read_number<uint64_t>(facts, 40);
		std::copy_n(facts.begin() + 48, 16, parsed_claim.listing_operation.bytes.begin());
		std::copy_n(facts.begin() + 64, 16,
			    parsed_claim.claim_source_operation.bytes.begin());
		parsed_claim.item_count = count;
		for (size_t index = 0; index < count; ++index)
		{
			const size_t offset =
				claim_fact_header_bytes + index * claim_fact_row_bytes;
			auto &entry = parsed_claim.rows[index];
			entry.uid = read_number<uint64_t>(facts, offset);
			entry.revision = read_number<uint64_t>(facts, offset + 8);
			entry.slot = read_number<uint16_t>(facts, offset + 16);
			entry.vnum =
				static_cast<int32_t>(read_number<uint32_t>(facts, offset + 18));
			entry.claim_pid = read_number<uint32_t>(facts, offset + 22);
			if (facts[offset + 26] > 1)
				return error::invalid_identity;
			entry.claimed = facts[offset + 26] == 1;
		}
		critical_command projected = command;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		const auto frozen = auction_item_claim_accounting_intent(
			projected, parsed_intent.admission.metadata.epoch, parsed_wallet,
			parsed_bank, parsed_claim, &expected);
		if (frozen != error::ok || expected != command.accounting_intent)
			return error::unauthorized;
		*intent = std::move(parsed_intent);
		*payload = parsed_payload;
		*claim = parsed_claim;
		*wallet = parsed_wallet;
		*bank = parsed_bank;
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error auction_item_claim_accounting_plan(
	const critical_command &command, const economic_frozen_intent &intent,
	const auction_item_claim_accounting_authority &authority,
	const auction_command_result &result, economic_accounting_plan *plan)
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
		const auto &claim = authority.claim;
		if (!valid_accounts(authority.wallet_account, authority.bank_account,
				    payload.racewar) ||
		    !valid_claim(payload, claim))
			return error::invalid_identity;
		const auto &meta = intent.admission.metadata;
		const auto source = source_for(claim);
		if (meta.writer_id != ECONOMIC_WRITER_AUCTION_ITEM_CLAIM ||
		    meta.reason != economic_reason::auction_claim ||
		    meta.actor_kind != economic_actor_kind::domain ||
		    meta.actor_id != payload.actor_pid || !meta.source_event ||
		    meta.source_event->kind != source.kind ||
		    meta.source_event->source.bytes != source.source.bytes ||
		    meta.source_event->generation.bytes != source.generation.bytes ||
		    meta.source_event->sequence != source.sequence ||
		    meta.source_event->slot != source.slot ||
		    meta.lineage.bytes != authority.wallet_account.lineage.bytes ||
		    meta.epoch.bytes != authority.epoch.bytes ||
		    meta.original_operation_id.bytes != claim.listing_operation.bytes ||
		    intent.admission.facts !=
			    frozen_facts(authority.wallet_account, authority.bank_account, claim))
			return error::unauthorized;
		if (authority.items_before.size() != claim.item_count ||
		    authority.player_owner_revision_before == UINT64_MAX ||
		    authority.auction_owner_revision_before == UINT64_MAX ||
		    authority.balances_before.wallet_revision != payload.expected_wallet_revision ||
		    authority.balances_before.bank_revision != payload.expected_bank_revision)
			return error::stale_revision;
		if (result.action != auction_action::claim_item ||
		    result.event_type != auction_event_type::item_claimed ||
		    result.auction_id != claim.auction_id || result.status != claim.status ||
		    result.seller_pid != claim.seller_pid ||
		    result.winner_pid != payload.actor_pid || result.previous_bidder_pid ||
		    result.final_price || result.wallet_value_delta ||
		    result.wallet.amount != authority.balances_before.wallet.amount ||
		    result.bank.amount != authority.balances_before.bank.amount ||
		    result.wallet_revision != authority.balances_before.wallet_revision ||
		    result.bank_revision != authority.balances_before.bank_revision ||
		    result.auction_revision != claim.auction_revision + 1 ||
		    result.player_owner_revision != authority.player_owner_revision_before + 1 ||
		    result.auction_owner_revision != authority.auction_owner_revision_before + 1 ||
		    result.item_count != claim.item_count)
			return error::corrupt_evidence;
		economic_accounting_plan candidate;
		status = economic_intent_plan_metadata(command, intent, &candidate.metadata);
		if (status != error::ok)
			return status;
		for (size_t index = 0; index < claim.item_count; ++index)
		{
			const auto &item = authority.items_before[index];
			const auto &position = item.position;
			if (item.uid != payload.items[index].item_uid ||
			    position.owner.type != item_owner_type::auction ||
			    position.owner.id != claim.auction_id || position.owner.context_id ||
			    position.root_uid != item.uid || position.parent_uid ||
			    position.revision != payload.items[index].expected_item_revision ||
			    position.state != item_custody_state::active)
				return error::stale_revision;
			if (result.item_uids[index] != item.uid ||
			    result.item_revisions[index] != position.revision + 1)
				return error::corrupt_evidence;
			economic_item_position after = position;
			after.owner = { item_owner_type::player, payload.actor_pid, 0 };
			after.revision++;
			candidate.items_before.push_back(item);
			candidate.items_after.push_back({ item.uid, after });
			candidate.item_events.push_back(
				{ static_cast<uint32_t>(index), 0, item.uid, position, after });
		}
		for (size_t index = claim.item_count; index < AUCTION_COMMAND_MAX_ITEMS; ++index)
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
