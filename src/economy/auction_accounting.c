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
	for (size_t index = 0; index < sizeof(value); ++index)
		bytes->push_back(static_cast<uint8_t>(value >> (index * 8)));
}

void append_u32(std::vector<uint8_t> *bytes, uint32_t value)
{
	for (size_t index = 0; index < sizeof(value); ++index)
		bytes->push_back(static_cast<uint8_t>(value >> (index * 8)));
}

bool valid_listing(const auction_bid_accounting_listing &listing,
		   const auction_command_payload &payload)
{
	return listing.auction_id && listing.auction_id == payload.auction_id &&
	       listing.seller_pid && listing.seller_pid != payload.actor_pid &&
	       listing.status == 1 && listing.custody_state == 1 && listing.revision &&
	       listing.revision != UINT64_MAX && listing.current_price >= 0 &&
	       listing.buy_price >= 0 && listing.current_price <= UINT_MAX &&
	       !critical_operation_id_is_zero(listing.listing_operation) &&
	       (listing.winning_bidder_pid ?
			!critical_operation_id_is_zero(listing.previous_bid_operation) &&
				!critical_operation_id_equal(listing.listing_operation,
							     listing.previous_bid_operation) &&
				listing.current_price > 0 :
			critical_operation_id_is_zero(listing.previous_bid_operation));
}

bool account(const economic_account_key &key, economic_account_kind kind,
	     const critical_operation_id &lineage)
{
	return economic_account_key_valid(key) && key.kind == kind && !key.context_id &&
	       key.lineage.bytes == lineage.bytes;
}

bool empty_account(const economic_account_key &key)
{
	return critical_operation_id_is_zero(key.lineage) && key.kind == economic_account_kind{} &&
	       !key.authority_id && !key.context_id;
}

bool valid_accounts(const auction_bid_accounting_accounts &accounts,
		    const auction_bid_accounting_listing &listing,
		    const auction_command_payload &payload, bool sold, bool resolved = false)
{
	const auto &lineage = accounts.wallet.lineage;
	const bool outbid = listing.winning_bidder_pid &&
			    listing.winning_bidder_pid != payload.actor_pid;
	const auto endpoint = [&](const economic_account_key &key, uint32_t absent, uint32_t pid,
				  bool required, bool unused)
	{
		if (!required)
			return !absent && empty_account(key);
		if (!absent)
			return account(key, economic_account_kind::pending_claim, lineage);
		return absent == pid &&
		       (resolved && !unused ?
				account(key, economic_account_kind::pending_claim, lineage) :
				empty_account(key));
	};
	if (!account(accounts.wallet, economic_account_kind::wallet, lineage) ||
	    !economic_account_key_valid(accounts.bank) ||
	    accounts.bank.kind != economic_account_kind::bank ||
	    accounts.bank.context_id != payload.racewar ||
	    accounts.bank.lineage.bytes != lineage.bytes ||
	    !account(accounts.escrow, economic_account_kind::auction_escrow, lineage) ||
	    !endpoint(accounts.bidder_claim, accounts.absent_bidder_pid, payload.actor_pid, true,
		      true) ||
	    !endpoint(accounts.previous_claim, accounts.absent_previous_pid,
		      listing.winning_bidder_pid, outbid, false) ||
	    !endpoint(accounts.seller_claim, accounts.absent_seller_pid, listing.seller_pid, sold,
		      false))
		return false;
	const uint64_t ids[] = {
		accounts.wallet.authority_id,	      accounts.bank.authority_id,
		accounts.escrow.authority_id,	      accounts.bidder_claim.authority_id,
		accounts.previous_claim.authority_id, accounts.seller_claim.authority_id
	};
	for (size_t index = 0; index < 6; ++index)
		for (size_t prior = 0; prior < index; ++prior)
			if (ids[index] && ids[index] == ids[prior])
				return false;
	return true;
}

bool bid_value(const auction_command_payload &payload,
	       const auction_bid_accounting_listing &listing, int64_t *bid, bool *sold)
{
	if (payload.action != auction_action::bid || !payload.actor_pid || payload.value <= 0 ||
	    payload.closing_fee_basis_points > 10000)
		return false;
	*bid = listing.buy_price && payload.value >= listing.buy_price ? listing.buy_price :
									 payload.value;
	*sold = listing.buy_price > 0 && *bid >= listing.buy_price;
	return *bid > 0 && *bid <= UINT_MAX &&
	       (listing.winning_bidder_pid ? *bid > listing.current_price :
					     *bid >= listing.current_price);
}

std::vector<uint8_t> frozen_facts(const auction_bid_accounting_listing &listing,
				  const auction_bid_accounting_accounts &accounts)
{
	std::vector<uint8_t> facts;
	facts.reserve(140);
	for (uint64_t mapping :
	     { accounts.wallet.authority_id, accounts.bank.authority_id,
	       accounts.escrow.authority_id,
	       accounts.absent_bidder_pid ? uint64_t{ 0 } : accounts.bidder_claim.authority_id,
	       accounts.absent_previous_pid ? uint64_t{ 0 } : accounts.previous_claim.authority_id,
	       accounts.absent_seller_pid ? uint64_t{ 0 } : accounts.seller_claim.authority_id })
		append_u64(&facts, mapping);
	append_u32(&facts, listing.auction_id);
	append_u32(&facts, listing.seller_pid);
	append_u32(&facts, listing.winning_bidder_pid);
	append_u32(&facts, listing.status);
	append_u32(&facts, listing.custody_state);
	append_u64(&facts, static_cast<uint64_t>(listing.current_price));
	append_u64(&facts, static_cast<uint64_t>(listing.buy_price));
	append_u64(&facts, listing.revision);
	facts.insert(facts.end(), listing.listing_operation.bytes.begin(),
		     listing.listing_operation.bytes.end());
	facts.insert(facts.end(), listing.previous_bid_operation.bytes.begin(),
		     listing.previous_bid_operation.bytes.end());
	if (accounts.absent_bidder_pid || accounts.absent_previous_pid ||
	    accounts.absent_seller_pid)
	{
		facts.insert(facts.end(), { 'A', 'E', 'C', '1' });
		append_u32(&facts, accounts.absent_bidder_pid);
		append_u32(&facts, accounts.absent_previous_pid);
		append_u32(&facts, accounts.absent_seller_pid);
	}
	return facts;
}

economic_source_event source_for(const auction_bid_accounting_listing &listing)
{
	return { economic_source_kind::auction,
		 listing.winning_bidder_pid ? listing.previous_bid_operation :
					      listing.listing_operation,
		 listing.listing_operation, listing.revision, 0 };
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

void posting(economic_accounting_plan *plan, uint16_t account_index,
	     const economic_coin_vector &delta, int64_t value)
{
	plan->postings.push_back(
		{ static_cast<uint32_t>(plan->postings.size()), account_index, 0, delta, value });
}
} // namespace

economic_accounting_error
auction_bid_accounting_intent(const critical_command &command, const critical_operation_id &epoch,
			      const auction_bid_accounting_listing &listing,
			      const auction_bid_accounting_accounts &accounts,
			      std::vector<uint8_t> *encoded)
{
	if (!encoded || command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    critical_operation_id_is_zero(epoch))
		return error::invalid_version;
	auction_command_payload payload = {};
	if (!auction_command_decode_payload(command, &payload))
		return error::corrupt_evidence;
	int64_t bid = 0;
	bool sold = false;
	if (!valid_listing(listing, payload) || !bid_value(payload, listing, &bid, &sold) ||
	    !valid_accounts(accounts, listing, payload, sold) ||
	    critical_operation_id_equal(command.operation_id, listing.listing_operation) ||
	    critical_operation_id_equal(command.operation_id, listing.previous_bid_operation))
		return error::invalid_identity;
	try
	{
		economic_admission_facts facts;
		facts.metadata.lineage = accounts.wallet.lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.original_operation_id = listing.listing_operation;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = payload.actor_pid;
		facts.metadata.writer_id = ECONOMIC_WRITER_AUCTION_BID;
		facts.metadata.reason = economic_reason::auction_bid;
		facts.metadata.source_event = source_for(listing);
		facts.facts = frozen_facts(listing, accounts);
		return economic_intent_freeze(command, facts, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error auction_bid_accounting_decode(const critical_command &command,
							economic_frozen_intent *intent,
							auction_command_payload *payload,
							auction_bid_accounting_listing *listing,
							auction_bid_accounting_accounts *accounts)
{
	if (!intent || !payload || !listing || !accounts ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return error::invalid_version;
	try
	{
		auction_command_payload parsed_payload = {};
		if (!auction_command_decode_payload(command, &parsed_payload) ||
		    parsed_payload.action != auction_action::bid)
			return error::invalid_identity;
		economic_frozen_intent parsed_intent;
		if (economic_intent_decode(command.accounting_intent, &parsed_intent) !=
			    error::ok ||
		    economic_intent_verify_binding(command, parsed_intent) != error::ok)
			return error::corrupt_evidence;
		const auto facts = std::span<const uint8_t>(parsed_intent.admission.facts);
		if (facts.size() != 124 && facts.size() != 140)
			return error::invalid_identity;
		const auto number = [&](size_t offset, size_t width)
		{
			uint64_t value = 0;
			for (size_t byte = 0; byte < width; ++byte)
				value |= static_cast<uint64_t>(facts[offset + byte]) << (byte * 8);
			return value;
		};
		const auto &lineage = parsed_intent.admission.metadata.lineage;
		auction_bid_accounting_accounts parsed_accounts;
		if (facts.size() == 140)
		{
			if (!std::equal(facts.begin() + 124, facts.begin() + 128,
					std::array<uint8_t, 4>{ 'A', 'E', 'C', '1' }.begin()))
				return error::invalid_version;
			parsed_accounts.absent_bidder_pid = static_cast<uint32_t>(number(128, 4));
			parsed_accounts.absent_previous_pid = static_cast<uint32_t>(number(132, 4));
			parsed_accounts.absent_seller_pid = static_cast<uint32_t>(number(136, 4));
			if (!parsed_accounts.absent_bidder_pid &&
			    !parsed_accounts.absent_previous_pid &&
			    !parsed_accounts.absent_seller_pid)
				return error::invalid_identity;
		}
		parsed_accounts.wallet = { lineage, economic_account_kind::wallet, number(0, 8),
					   0 };
		parsed_accounts.bank = { lineage, economic_account_kind::bank, number(8, 8),
					 parsed_payload.racewar };
		parsed_accounts.escrow = { lineage, economic_account_kind::auction_escrow,
					   number(16, 8), 0 };
		if (number(24, 8) || !parsed_accounts.absent_bidder_pid)
			parsed_accounts.bidder_claim = { lineage,
							 economic_account_kind::pending_claim,
							 number(24, 8), 0 };
		if (const auto id = number(32, 8))
			parsed_accounts.previous_claim = { lineage,
							   economic_account_kind::pending_claim, id,
							   0 };
		if (const auto id = number(40, 8))
			parsed_accounts.seller_claim = { lineage,
							 economic_account_kind::pending_claim, id,
							 0 };
		auction_bid_accounting_listing parsed_listing;
		parsed_listing.auction_id = static_cast<uint32_t>(number(48, 4));
		parsed_listing.seller_pid = static_cast<uint32_t>(number(52, 4));
		parsed_listing.winning_bidder_pid = static_cast<uint32_t>(number(56, 4));
		parsed_listing.status = static_cast<uint32_t>(number(60, 4));
		parsed_listing.custody_state = static_cast<uint32_t>(number(64, 4));
		parsed_listing.current_price = static_cast<int64_t>(number(68, 8));
		parsed_listing.buy_price = static_cast<int64_t>(number(76, 8));
		parsed_listing.revision = number(84, 8);
		std::copy_n(facts.begin() + 92, 16, parsed_listing.listing_operation.bytes.begin());
		std::copy_n(facts.begin() + 108, 16,
			    parsed_listing.previous_bid_operation.bytes.begin());
		critical_command projected = command;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		if (auction_bid_accounting_intent(projected, parsed_intent.admission.metadata.epoch,
						  parsed_listing, parsed_accounts,
						  &expected) != error::ok ||
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

economic_accounting_error
auction_bid_accounting_plan(const critical_command &command, const economic_frozen_intent &intent,
			    const auction_bid_accounting_authority &authority,
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
		const auto &listing = authority.listing;
		const auto &accounts = authority.accounts;
		int64_t bid = 0;
		bool sold = false;
		if (!valid_listing(listing, payload) || !bid_value(payload, listing, &bid, &sold) ||
		    !valid_accounts(accounts, listing, payload, sold, true))
			return error::invalid_identity;
		if ((accounts.absent_bidder_pid && (authority.bidder_claim_before.money ||
						    authority.bidder_claim_before.revision)) ||
		    (accounts.absent_previous_pid && (authority.previous_claim_before.money ||
						      authority.previous_claim_before.revision)) ||
		    (accounts.absent_seller_pid && (authority.seller_claim_before.money ||
						    authority.seller_claim_before.revision)))
			return error::corrupt_evidence;
		const auto &meta = intent.admission.metadata;
		const auto source = source_for(listing);
		if (meta.writer_id != ECONOMIC_WRITER_AUCTION_BID ||
		    meta.reason != economic_reason::auction_bid ||
		    meta.actor_kind != economic_actor_kind::domain ||
		    meta.actor_id != payload.actor_pid || !meta.source_event ||
		    meta.source_event->kind != source.kind ||
		    meta.source_event->source.bytes != source.source.bytes ||
		    meta.source_event->generation.bytes != source.generation.bytes ||
		    meta.source_event->sequence != source.sequence ||
		    meta.source_event->slot != source.slot ||
		    meta.lineage.bytes != accounts.wallet.lineage.bytes ||
		    meta.epoch.bytes != authority.epoch.bytes ||
		    meta.original_operation_id.bytes != listing.listing_operation.bytes ||
		    intent.admission.facts != frozen_facts(listing, accounts))
			return error::unauthorized;
		const bool outbid = listing.winning_bidder_pid &&
				    listing.winning_bidder_pid != payload.actor_pid;
		const int64_t to_pay = listing.winning_bidder_pid == payload.actor_pid ?
					       bid - listing.current_price :
					       bid;
		const int64_t fee =
			sold ? static_cast<int64_t>(static_cast<__int128_t>(bid) *
						    payload.closing_fee_basis_points / 10000) :
			       0;
		const int64_t proceeds = sold ? bid - fee : 0;
		if (authority.bidder_claim_before.money < 0 ||
		    authority.bidder_claim_before.money > UINT_MAX)
			return error::corrupt_evidence;
		const int64_t claim_credit_used =
			std::min(to_pay, authority.bidder_claim_before.money);
		const int64_t wallet_to_pay = to_pay - claim_credit_used;
		int64_t wallet_before = 0;
		status = economic_coin_value(authority.balances_before.wallet.amount,
					     &wallet_before);
		if (status != error::ok)
			return status;
		if (wallet_before < wallet_to_pay)
			return error::negative_holding;
		if (authority.balances_before.wallet_revision != payload.expected_wallet_revision ||
		    authority.balances_before.bank_revision != payload.expected_bank_revision)
			return error::stale_revision;
		if (authority.balances_before.wallet_revision == UINT64_MAX ||
		    authority.balances_before.bank_revision == UINT64_MAX ||
		    (claim_credit_used && authority.bidder_claim_before.revision == UINT64_MAX) ||
		    (outbid && authority.previous_claim_before.revision == UINT64_MAX) ||
		    (sold && authority.seller_claim_before.revision == UINT64_MAX))
			return error::overflow;
		if ((outbid && (authority.previous_claim_before.money < 0 ||
				authority.previous_claim_before.money >
					static_cast<int64_t>(UINT_MAX) - listing.current_price)) ||
		    (sold && (authority.seller_claim_before.money < 0 ||
			      authority.seller_claim_before.money >
				      static_cast<int64_t>(UINT_MAX) - proceeds)))
			return error::overflow;
		const auto wallet_after = canonical_wallet(wallet_before - wallet_to_pay);
		if (result.action != auction_action::bid ||
		    result.event_type !=
			    (sold ? auction_event_type::sold : auction_event_type::bid_placed) ||
		    result.auction_id != listing.auction_id || result.status != (sold ? 2U : 1U) ||
		    result.seller_pid != listing.seller_pid ||
		    result.winner_pid != payload.actor_pid ||
		    result.previous_bidder_pid != listing.winning_bidder_pid ||
		    result.final_price != bid || result.wallet_value_delta != -wallet_to_pay ||
		    result.claim_credit_used != claim_credit_used ||
		    result.wallet.amount != wallet_after ||
		    result.bank.amount != authority.balances_before.bank.amount ||
		    result.wallet_revision != authority.balances_before.wallet_revision + 1 ||
		    result.bank_revision != authority.balances_before.bank_revision + 1 ||
		    result.auction_revision != listing.revision + 1 || result.item_count ||
		    result.player_owner_revision || result.auction_owner_revision)
			return error::corrupt_evidence;
		if (std::any_of(result.item_uids.begin(), result.item_uids.end(),
				[](uint64_t value) { return value != 0; }) ||
		    std::any_of(result.item_revisions.begin(), result.item_revisions.end(),
				[](uint64_t value) { return value != 0; }))
			return error::corrupt_evidence;
		economic_accounting_plan candidate;
		status = economic_intent_plan_metadata(command, intent, &candidate.metadata);
		if (status != error::ok)
			return status;
		candidate.accounts.push_back(
			{ accounts.wallet, authority.balances_before.wallet.amount,
			  result.wallet.amount, authority.balances_before.wallet_revision,
			  result.wallet_revision });
		uint16_t bidder_claim_index = 0;
		if (claim_credit_used)
		{
			bidder_claim_index = static_cast<uint16_t>(candidate.accounts.size());
			candidate.accounts.push_back(
				{ accounts.bidder_claim,
				  copper(authority.bidder_claim_before.money),
				  copper(authority.bidder_claim_before.money - claim_credit_used),
				  authority.bidder_claim_before.revision,
				  authority.bidder_claim_before.revision + 1 });
		}
		uint16_t escrow_index = static_cast<uint16_t>(candidate.accounts.size());
		candidate.accounts.push_back(
			{ accounts.escrow,
			  copper(listing.winning_bidder_pid ? listing.current_price : 0),
			  copper(sold ? 0 : bid), listing.revision, listing.revision + 1 });
		uint16_t previous_index = 0, seller_index = 0;
		if (outbid)
		{
			previous_index = static_cast<uint16_t>(candidate.accounts.size());
			candidate.accounts.push_back(
				{ accounts.previous_claim,
				  copper(authority.previous_claim_before.money),
				  copper(authority.previous_claim_before.money +
					 listing.current_price),
				  authority.previous_claim_before.revision,
				  authority.previous_claim_before.revision + 1 });
		}
		if (sold)
		{
			seller_index = static_cast<uint16_t>(candidate.accounts.size());
			candidate.accounts.push_back(
				{ accounts.seller_claim,
				  copper(authority.seller_claim_before.money),
				  copper(authority.seller_claim_before.money + proceeds),
				  authority.seller_claim_before.revision,
				  authority.seller_claim_before.revision + 1 });
		}
		economic_coin_vector wallet_delta = {};
		status = economic_coin_delta(candidate.accounts[0].before,
					     candidate.accounts[0].after, &wallet_delta);
		if (status != error::ok)
			return status;
		if (wallet_to_pay)
			posting(&candidate, 0, wallet_delta, -wallet_to_pay);
		if (claim_credit_used)
			posting(&candidate, bidder_claim_index, copper(-claim_credit_used),
				-claim_credit_used);
		posting(&candidate, escrow_index, copper(to_pay), to_pay);
		if (outbid)
		{
			posting(&candidate, escrow_index, copper(-listing.current_price),
				-listing.current_price);
			posting(&candidate, previous_index, copper(listing.current_price),
				listing.current_price);
		}
		if (sold)
		{
			posting(&candidate, escrow_index, copper(-bid), -bid);
			if (proceeds)
				posting(&candidate, seller_index, copper(proceeds), proceeds);
			if (fee)
			{
				candidate.accounts.push_back(
					{ { accounts.wallet.lineage, economic_account_kind::sink,
					    ECONOMIC_AUCTION_CLOSING_FEE_SINK_ID, 0 },
					  {},
					  {},
					  0,
					  0 });
				posting(&candidate,
					static_cast<uint16_t>(candidate.accounts.size() - 1),
					copper(fee), fee);
			}
		}
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
