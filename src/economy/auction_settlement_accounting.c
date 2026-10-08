#include "economy/auction_settlement_accounting.h"

#include "economy/auction_accounting.h"

#include <algorithm>
#include <climits>
#include <new>
#include <span>
#include <utility>

namespace
{
using error = economic_accounting_error;

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

bool empty(const economic_account_key &key)
{
	return critical_operation_id_is_zero(key.lineage) && key.kind == economic_account_kind{} &&
	       !key.authority_id && !key.context_id;
}

bool account(const economic_account_key &key, economic_account_kind kind,
	     const critical_operation_id &lineage, uint64_t context)
{
	return economic_account_key_valid(key) && key.kind == kind &&
	       key.lineage.bytes == lineage.bytes && key.context_id == context;
}

bool valid(const auction_command_payload &payload, const auction_settlement_listing &listing,
	   const auction_settlement_accounts &accounts, bool resolved = false)
{
	if ((payload.action != auction_action::finalize &&
	     payload.action != auction_action::remove) ||
	    !payload.auction_id || listing.auction_id != payload.auction_id ||
	    !listing.seller_pid || listing.status != 1 || listing.custody_state != 1 ||
	    !listing.quantity || listing.quantity > AUCTION_COMMAND_MAX_ITEMS ||
	    listing.item_count != listing.quantity || !listing.revision ||
	    listing.revision == UINT64_MAX || !listing.end_time || listing.current_price < 0 ||
	    listing.current_price > UINT_MAX || listing.buy_price < 0 ||
	    critical_operation_id_is_zero(listing.listing_operation) ||
	    payload.closing_fee_basis_points > 10000)
		return false;
	if (listing.winner_pid ?
		    !listing.current_price ||
			    critical_operation_id_is_zero(listing.winning_bid_operation) ||
			    critical_operation_id_equal(listing.listing_operation,
							listing.winning_bid_operation) :
		    !critical_operation_id_is_zero(listing.winning_bid_operation))
		return false;
	const bool sale = payload.action == auction_action::finalize && listing.winner_pid;
	const auto &lineage = accounts.escrow.lineage;
	if (!account(accounts.escrow, economic_account_kind::auction_escrow, lineage, 0) ||
	    (sale ? (accounts.absent_seller_pid ?
			     (accounts.absent_seller_pid != listing.seller_pid ||
			      (resolved ?
				       !account(accounts.seller_claim,
						economic_account_kind::pending_claim, lineage, 0) :
				       !empty(accounts.seller_claim))) :
			     !account(accounts.seller_claim, economic_account_kind::pending_claim,
				      lineage, 0)) :
		    (!empty(accounts.seller_claim) || accounts.absent_seller_pid)) ||
	    (payload.actor_pid ?
		     !account(accounts.actor_wallet, economic_account_kind::wallet, lineage, 0) ||
			     !account(accounts.actor_bank, economic_account_kind::bank, lineage,
				      payload.racewar) :
		     !empty(accounts.actor_wallet) || !empty(accounts.actor_bank)))
		return false;
	const uint64_t ids[] = { accounts.escrow.authority_id, accounts.seller_claim.authority_id,
				 accounts.actor_wallet.authority_id,
				 accounts.actor_bank.authority_id };
	for (size_t index = 0; index < 4; ++index)
		for (size_t earlier = 0; earlier < index; ++earlier)
			if (ids[index] && ids[index] == ids[earlier])
				return false;
	for (size_t index = 0; index < listing.item_count; ++index)
	{
		const auto &item = listing.items[index];
		if (!item.uid || !item.revision || item.revision == UINT64_MAX || item.vnum < 0 ||
		    item.claim_pid || item.claimed || item.slot != index)
			return false;
		for (size_t prior = 0; prior < index; ++prior)
			if (listing.items[prior].uid == item.uid)
				return false;
	}
	return true;
}

std::vector<uint8_t> facts(const auction_settlement_listing &listing,
			   const auction_settlement_accounts &accounts)
{
	std::vector<uint8_t> result;
	result.reserve(122 + listing.item_count * 27);
	for (uint64_t mapping :
	     { accounts.escrow.authority_id,
	       accounts.absent_seller_pid ? uint64_t{ 0 } : accounts.seller_claim.authority_id,
	       accounts.actor_wallet.authority_id, accounts.actor_bank.authority_id })
		append_u64(&result, mapping);
	for (uint32_t value : { listing.auction_id, listing.seller_pid, listing.winner_pid,
				listing.status, listing.custody_state, listing.quantity })
		append_u32(&result, value);
	for (uint64_t value :
	     { static_cast<uint64_t>(listing.current_price),
	       static_cast<uint64_t>(listing.buy_price), listing.revision, listing.end_time })
		append_u64(&result, value);
	result.insert(result.end(), listing.listing_operation.bytes.begin(),
		      listing.listing_operation.bytes.end());
	result.insert(result.end(), listing.winning_bid_operation.bytes.begin(),
		      listing.winning_bid_operation.bytes.end());
	append_u16(&result, listing.item_count);
	for (size_t index = 0; index < listing.item_count; ++index)
	{
		const auto &item = listing.items[index];
		append_u64(&result, item.uid);
		append_u64(&result, item.revision);
		append_u16(&result, item.slot);
		append_u32(&result, static_cast<uint32_t>(item.vnum));
		append_u32(&result, item.claim_pid);
		result.push_back(item.claimed ? 1 : 0);
	}
	if (accounts.absent_seller_pid)
	{
		result.insert(result.end(), { 'A', 'E', 'C', '1' });
		append_u32(&result, accounts.absent_seller_pid);
	}
	return result;
}

economic_source_event source(const auction_settlement_listing &listing,
			     const auction_command_payload &payload)
{
	return { economic_source_kind::auction,
		 listing.winner_pid ? listing.winning_bid_operation : listing.listing_operation,
		 listing.listing_operation, listing.revision,
		 payload.action == auction_action::remove ? 1U : 0U };
}

economic_coin_vector copper(int64_t amount)
{
	return { amount, 0, 0, 0 };
}

void posting(economic_accounting_plan *plan, uint16_t account, int64_t amount)
{
	plan->postings.push_back({ static_cast<uint32_t>(plan->postings.size()), account, 0,
				   copper(amount), amount });
}
} // namespace

economic_accounting_error auction_settlement_accounting_intent(
	const critical_command &command, const critical_operation_id &epoch,
	const auction_settlement_listing &listing, const auction_settlement_accounts &accounts,
	std::vector<uint8_t> *encoded)
{
	if (!encoded || command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    critical_operation_id_is_zero(epoch))
		return error::invalid_version;
	auction_command_payload payload = {};
	if (!auction_command_decode_payload(command, &payload))
		return error::corrupt_evidence;
	if (!valid(payload, listing, accounts) ||
	    critical_operation_id_equal(command.operation_id, listing.listing_operation) ||
	    critical_operation_id_equal(command.operation_id, listing.winning_bid_operation))
		return error::invalid_identity;
	try
	{
		economic_admission_facts admission;
		admission.metadata.lineage = accounts.escrow.lineage;
		admission.metadata.epoch = epoch;
		admission.metadata.original_operation_id = listing.listing_operation;
		admission.metadata.actor_kind = economic_actor_kind::domain;
		admission.metadata.actor_id = payload.actor_pid ? payload.actor_pid :
								  listing.auction_id;
		admission.metadata.writer_id = ECONOMIC_WRITER_AUCTION_SETTLEMENT;
		admission.metadata.reason = payload.action == auction_action::remove ?
						    economic_reason::auction_cancel :
						    economic_reason::auction_settle;
		admission.metadata.source_event = source(listing, payload);
		admission.facts = facts(listing, accounts);
		return economic_intent_freeze(command, admission, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error auction_settlement_accounting_decode(
	const critical_command &command, economic_frozen_intent *intent,
	auction_command_payload *payload, auction_settlement_listing *listing,
	auction_settlement_accounts *accounts)
{
	if (!intent || !payload || !listing || !accounts ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return error::invalid_version;
	try
	{
		auction_command_payload parsed_payload = {};
		if (!auction_command_decode_payload(command, &parsed_payload) ||
		    (parsed_payload.action != auction_action::finalize &&
		     parsed_payload.action != auction_action::remove))
			return error::invalid_identity;
		economic_frozen_intent parsed_intent;
		if (economic_intent_decode(command.accounting_intent, &parsed_intent) !=
			    error::ok ||
		    economic_intent_verify_binding(command, parsed_intent) != error::ok)
			return error::corrupt_evidence;
		const auto facts = std::span<const uint8_t>(parsed_intent.admission.facts);
		if (facts.size() < 122)
			return error::invalid_identity;
		const auto number = [&](size_t offset, size_t width)
		{
			uint64_t value = 0;
			for (size_t byte = 0; byte < width; ++byte)
				value |= static_cast<uint64_t>(facts[offset + byte]) << (byte * 8);
			return value;
		};
		const auto count = static_cast<uint16_t>(number(120, 2));
		if (!count || count > AUCTION_COMMAND_MAX_ITEMS ||
		    (facts.size() != 122 + static_cast<size_t>(count) * 27 &&
		     facts.size() != 130 + static_cast<size_t>(count) * 27))
			return error::invalid_identity;
		const auto &lineage = parsed_intent.admission.metadata.lineage;
		auction_settlement_accounts parsed_accounts;
		const size_t original_size = 122 + static_cast<size_t>(count) * 27;
		if (facts.size() == original_size + 8)
		{
			if (!std::equal(facts.begin() + original_size,
					facts.begin() + original_size + 4,
					std::array<uint8_t, 4>{ 'A', 'E', 'C', '1' }.begin()))
				return error::invalid_version;
			parsed_accounts.absent_seller_pid =
				static_cast<uint32_t>(number(original_size + 4, 4));
			if (!parsed_accounts.absent_seller_pid)
				return error::invalid_identity;
		}
		parsed_accounts.escrow = { lineage, economic_account_kind::auction_escrow,
					   number(0, 8), 0 };
		if (const auto id = number(8, 8))
			parsed_accounts.seller_claim = { lineage,
							 economic_account_kind::pending_claim, id,
							 0 };
		if (const auto id = number(16, 8))
			parsed_accounts.actor_wallet = { lineage, economic_account_kind::wallet, id,
							 0 };
		if (const auto id = number(24, 8))
			parsed_accounts.actor_bank = { lineage, economic_account_kind::bank, id,
						       parsed_payload.racewar };
		auction_settlement_listing parsed_listing;
		parsed_listing.auction_id = static_cast<uint32_t>(number(32, 4));
		parsed_listing.seller_pid = static_cast<uint32_t>(number(36, 4));
		parsed_listing.winner_pid = static_cast<uint32_t>(number(40, 4));
		parsed_listing.status = static_cast<uint32_t>(number(44, 4));
		parsed_listing.custody_state = static_cast<uint32_t>(number(48, 4));
		parsed_listing.quantity = static_cast<uint32_t>(number(52, 4));
		parsed_listing.current_price = static_cast<int64_t>(number(56, 8));
		parsed_listing.buy_price = static_cast<int64_t>(number(64, 8));
		parsed_listing.revision = number(72, 8);
		parsed_listing.end_time = number(80, 8);
		std::copy_n(facts.begin() + 88, 16, parsed_listing.listing_operation.bytes.begin());
		std::copy_n(facts.begin() + 104, 16,
			    parsed_listing.winning_bid_operation.bytes.begin());
		parsed_listing.item_count = count;
		for (size_t index = 0; index < count; ++index)
		{
			const size_t offset = 122 + index * 27;
			auto &item = parsed_listing.items[index];
			item.uid = number(offset, 8);
			item.revision = number(offset + 8, 8);
			item.slot = static_cast<uint16_t>(number(offset + 16, 2));
			item.vnum = static_cast<int32_t>(number(offset + 18, 4));
			item.claim_pid = static_cast<uint32_t>(number(offset + 22, 4));
			if (facts[offset + 26] > 1)
				return error::invalid_identity;
			item.claimed = facts[offset + 26] == 1;
		}
		critical_command projected = command;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		if (auction_settlement_accounting_intent(
			    projected, parsed_intent.admission.metadata.epoch, parsed_listing,
			    parsed_accounts, &expected) != error::ok ||
		    expected != command.accounting_intent)
			return error::unauthorized;
		*intent = std::move(parsed_intent);
		*payload = parsed_payload;
		*listing = parsed_listing;
		*accounts = parsed_accounts;
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error auction_settlement_accounting_plan(
	const critical_command &command, const economic_frozen_intent &intent,
	const auction_settlement_authority &authority, const auction_command_result &result,
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
		const auto &listing = authority.listing;
		const auto &accounts = authority.accounts;
		if (!valid(payload, listing, accounts, true))
			return error::invalid_identity;
		if (accounts.absent_seller_pid &&
		    (authority.seller_claim_before || authority.seller_claim_revision_before))
			return error::corrupt_evidence;
		const auto &meta = intent.admission.metadata;
		const auto expected_source = source(listing, payload);
		if (meta.writer_id != ECONOMIC_WRITER_AUCTION_SETTLEMENT ||
		    meta.reason != (payload.action == auction_action::remove ?
					    economic_reason::auction_cancel :
					    economic_reason::auction_settle) ||
		    meta.actor_kind != economic_actor_kind::domain ||
		    meta.actor_id != (payload.actor_pid ? payload.actor_pid : listing.auction_id) ||
		    !meta.source_event || meta.source_event->kind != expected_source.kind ||
		    meta.source_event->source.bytes != expected_source.source.bytes ||
		    meta.source_event->generation.bytes != expected_source.generation.bytes ||
		    meta.source_event->sequence != expected_source.sequence ||
		    meta.source_event->slot != expected_source.slot ||
		    meta.lineage.bytes != accounts.escrow.lineage.bytes ||
		    meta.epoch.bytes != authority.epoch.bytes ||
		    meta.original_operation_id.bytes != listing.listing_operation.bytes ||
		    intent.admission.facts != facts(listing, accounts))
			return error::unauthorized;
		const bool sale = payload.action == auction_action::finalize && listing.winner_pid;
		const int64_t held = listing.winner_pid ? listing.current_price : 0;
		const int64_t fee =
			sale ? static_cast<int64_t>(static_cast<__int128_t>(held) *
						    payload.closing_fee_basis_points / 10000) :
			       0;
		const int64_t proceeds = sale ? held - fee : 0;
		if (authority.seller_claim_before < 0 ||
		    (sale &&
		     (authority.seller_claim_before > static_cast<int64_t>(UINT_MAX) - proceeds ||
		      authority.seller_claim_revision_before == UINT64_MAX)))
			return error::overflow;
		if (sale &&
		    (authority.seller_claim_after != authority.seller_claim_before + proceeds ||
		     authority.seller_claim_revision_after !=
			     authority.seller_claim_revision_before + 1))
			return error::corrupt_evidence;
		if (payload.actor_pid && (authority.actor_balances_before.wallet_revision !=
						  payload.expected_wallet_revision ||
					  authority.actor_balances_before.bank_revision !=
						  payload.expected_bank_revision))
			return error::stale_revision;
		if (result.action != payload.action ||
		    result.event_type != (payload.action == auction_action::remove ?
						  auction_event_type::removed :
					  listing.winner_pid ? auction_event_type::sold :
							       auction_event_type::expired) ||
		    result.auction_id != listing.auction_id ||
		    result.status != (payload.action == auction_action::remove ? 3U : 2U) ||
		    result.seller_pid != listing.seller_pid ||
		    result.winner_pid != listing.winner_pid || result.previous_bidder_pid ||
		    result.final_price != listing.current_price || result.wallet_value_delta ||
		    result.wallet.amount != authority.actor_balances_before.wallet.amount ||
		    result.bank.amount != authority.actor_balances_before.bank.amount ||
		    result.wallet_revision != authority.actor_balances_before.wallet_revision ||
		    result.bank_revision != authority.actor_balances_before.bank_revision ||
		    result.auction_revision != listing.revision + 1 || result.item_count ||
		    result.player_owner_revision || result.auction_owner_revision ||
		    std::any_of(result.item_uids.begin(), result.item_uids.end(),
				[](uint64_t uid) { return uid != 0; }) ||
		    std::any_of(result.item_revisions.begin(), result.item_revisions.end(),
				[](uint64_t revision) { return revision != 0; }))
			return error::corrupt_evidence;
		const uint32_t claimant = sale ? listing.winner_pid : listing.seller_pid;
		if (authority.items_before.size() != listing.item_count ||
		    authority.claim_pids_after.size() != listing.item_count)
			return error::incomplete_coverage;
		for (size_t index = 0; index < listing.item_count; ++index)
		{
			const auto &item = authority.items_before[index];
			const auto &position = item.position;
			if (item.uid != listing.items[index].uid ||
			    position.owner.type != item_owner_type::auction ||
			    position.owner.id != listing.auction_id || position.owner.context_id ||
			    position.root_uid != item.uid || position.parent_uid ||
			    position.revision != listing.items[index].revision ||
			    position.state != item_custody_state::active ||
			    authority.claim_pids_after[index] != claimant)
				return error::stale_revision;
		}
		economic_accounting_plan candidate;
		status = economic_intent_plan_metadata(command, intent, &candidate.metadata);
		if (status != error::ok)
			return status;
		candidate.accounts.push_back({ accounts.escrow, copper(held),
					       copper(sale ? 0 : held), listing.revision,
					       listing.revision + 1 });
		if (sale)
		{
			candidate.accounts.push_back(
				{ accounts.seller_claim, copper(authority.seller_claim_before),
				  copper(authority.seller_claim_before + proceeds),
				  authority.seller_claim_revision_before,
				  authority.seller_claim_revision_before + 1 });
			posting(&candidate, 0, -held);
			if (proceeds)
				posting(&candidate, 1, proceeds);
			if (fee)
			{
				candidate.accounts.push_back(
					{ { accounts.escrow.lineage, economic_account_kind::sink,
					    ECONOMIC_AUCTION_CLOSING_FEE_SINK_ID, 0 },
					  {},
					  {},
					  0,
					  0 });
				posting(&candidate,
					static_cast<uint16_t>(candidate.accounts.size() - 1), fee);
			}
		}
		candidate.items_before = authority.items_before;
		candidate.items_after = authority.items_before;
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
