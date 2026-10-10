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

#include "economy/auction_native_command_context.h"

#include <type_traits>
namespace
{
constexpr size_t auction_codec_allocator_frames =
	// _M_allocate, allocator_traits::allocate, allocator::allocate (C++20):
	// each this/allocator reference, n and returned pointer; new_allocator
	// adds its genuine hint pointer; operator new n and returned pointer.
	3 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(void *) + sizeof(size_t) +
	// _M_deallocate/traits/allocator/new_allocator: allocator/this+p+n,
	// then sized operator delete p+n. Trivial element _Destroy closures.
	4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) +
	(3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *)) +
	// vector max_size/_S_max_size/traits max_size/new_allocator::_M_max_size
	// references/results and actual diffmax/allocmax locals. C++20 allocator
	// has no max_size member; that inactive C++17 branch is not counted.
	4 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(size_t) +
	// traits::construct -> construct_at -> forward -> placement-new; all
	// constructor arguments here are real references to trivial values.
	3 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t);
constexpr size_t auction_codec_copy_frames =
	// __uninitialized_move_if_noexcept_a and __uninitialized_copy_a: 3
	// iterators+allocator-reference+returned iterator each. Runtime ordinary
	// uninitialized_copy's two boolean locals and __uninit_copy carrier.
	2 * (4 * sizeof(void *) + sizeof(void *)) + 3 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(bool) + 3 * sizeof(void *) + sizeof(void *) +
	// copy/copy_move_a/a1/a2/copy_m, each3 iterator params+return; real
	// miter/niter/wrap/assign_one and memmove argument/result scopes.
	5 * (3 * sizeof(void *) + sizeof(void *)) + 2 * (sizeof(void *) + sizeof(void *)) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// distance/__distance and normal-iterator subtraction/base/dereference/
	// ++/comparison/constructor source parameter/return scopes.
	2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) + sizeof(char) +
	6 * (2 * sizeof(void *)) + sizeof(std::ptrdiff_t) + sizeof(bool) +
	// Fitting forward insert reaches advance(__mid,__elems_after), even zero.
	// advance: iterator-reference, size_t n, real local difference_type __d;
	// __iterator_category: iterator-reference and actual returned RA tag;
	// __advance: iterator-reference, difference n and by-value RA tag;
	// actual += this/n/reference-return, plus source ++/-- alternatives.
	sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	4 * sizeof(void *);
constexpr size_t auction_codec_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t auction_codec_default_frames =
	// Runtime default_n_a/default_n/default_n_1<true>: real first/n/allocator
	// reference, can_fill and val locals, actual returned pointer carriers.
	(3 * sizeof(void *) + sizeof(size_t)) +
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(3 * sizeof(void *) + sizeof(size_t)) +
	// _Construct's real location plus placement-new n/location/result.
	sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// fill_n/__fill_n_a<random_access>: first/n/value/result/tag;
	// __size_to_integer argument/result; __fill_a/__fill_a1 scalar __tmp.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(char) + 2 * sizeof(size_t) +
	2 * (3 * sizeof(void *)) + sizeof(uint64_t);
constexpr size_t auction_codec_vector_frames =
	auction_codec_allocator_frames + auction_codec_copy_frames + auction_codec_relocate_frames +
	auction_codec_default_frames +
	// reserve this/n/old_size/tmp; assign public/forward-aux and exact
	// _M_allocate_and_copy's this/n/first/last/result/returned pointer.
	2 * sizeof(void *) + 2 * sizeof(size_t) + 7 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(char) + 5 * sizeof(void *) + sizeof(size_t) +
	// push_back/emplace_back and real realloc_insert old/new start/finish,
	// len/elems_before/position/forward value reference; _M_check_len.
	2 * sizeof(void *) + 3 * sizeof(void *) + 7 * sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(void *) + 3 * sizeof(size_t) +
	// C++20 forward insert public/range-insert (no old dispatch), offset/elems_after/
	// len/old-start/finish/mid/new-start/finish/iterator return/tag scopes.
	15 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(char) +
	// default_append's n/size/navail/len and real old/new/destroy pointers.
	5 * sizeof(void *) + 4 * sizeof(size_t) +
	// begin/end/cbegin/size/capacity/get-allocator declared carriers and
	// iterator-category/std::max arguments/results on the real call paths.
	7 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(char) + 3 * sizeof(void *);
constexpr size_t auction_codec_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + auction_codec_allocator_frames;
constexpr size_t auction_codec_vector_constructor_frames =
	2 * sizeof(void *) + 3 * sizeof(std::allocator<uint8_t>) + 2 * sizeof(void *) +
	sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	8 * (sizeof(void *) + sizeof(size_t)) + auction_codec_vector_frames;

// Genuine additional selected typed library scopes beside the vector's
// reserve/forward-insert profile. The owning inline DTOs remain in the actual
// enclosing object sizes, rather than a fabricated encoded-envelope baseline.
static_assert(std::is_trivially_copyable_v<economic_source_event>);
static_assert(std::is_trivially_destructible_v<economic_source_event>);
static_assert(std::is_trivially_copyable_v<auction_command_payload>);
static_assert(std::is_trivially_copyable_v<economic_account_key>);
constexpr size_t auction_codec_optional_frames =
	// Actual metadata/frozen/admission default/generated move/copy member
	// functions: this/source refs; optional/_Optional_base/_payload/_Storage
	// default constructors and trivial storage destructor this carriers.
	6 * sizeof(void *) + 5 * sizeof(void *) + sizeof(void *) +
	// source_event assignment operator=(T&&): this/u/ref-result; real
	// is_engaged/get/construct wrappers, payload _M_construct and forward.
	3 * sizeof(void *) + (sizeof(void *) + sizeof(bool)) + 2 * (2 * sizeof(void *)) +
	2 * sizeof(void *) + 2 * sizeof(void *) +
	// __addressof -> _Construct -> forward -> placement new -> trivial
	// economic_source_event generated move this/source. No extra DTO copy.
	2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(void *) +
	sizeof(size_t) + sizeof(void *) + 2 * sizeof(void *) +
	// Actual optional operator bool/operator->/base get/payload get and
	// addressof parameter and reference/pointer/bool result carriers.
	2 * (sizeof(void *) + sizeof(bool)) + 3 * (2 * sizeof(void *));
constexpr size_t auction_codec_equal_frames =
	// array/vector operator== actual lhs/rhs and returned bool; genuine
	// container size/begin/end and array_traits::_S_ptr pointer returns.
	2 * sizeof(void *) + sizeof(bool) + 2 * (sizeof(void *) + sizeof(size_t)) +
	6 * (2 * sizeof(void *)) +
	// equal/__equal_aux/__equal_aux1/__equal<true>::equal each three
	// iterator arguments and returned bool; __simple and __len locals;
	// niter_base calls and __memcmp's genuine pointers/length/int result.
	4 * (3 * sizeof(void *) + sizeof(bool)) + sizeof(bool) + sizeof(size_t) +
	3 * (2 * sizeof(void *)) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(int) +
	// Actual normal_iterator copied argument/ctor/base source carriers.
	4 * (2 * sizeof(void *));
constexpr size_t auction_codec_copy_n_frames =
	// Original copy_n(count literal16) owns first/count/result/__n2/result,
	// __size_to_integer(int), iterator_category and __copy_n<RA> tag.
	3 * sizeof(void *) + 2 * sizeof(int) + 2 * sizeof(int) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + 3 * sizeof(void *) + sizeof(int) +
	sizeof(std::random_access_iterator_tag);
constexpr size_t auction_codec_scalar_source_frames =
	// Original read lambda (facts-reference capture/this, offset,width,
	// byte index,value/result); typed read_number alternatives; span data,
	// index,size/constructor parameters and returned pointer/reference.
	2 * sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(uint64_t) +
	sizeof(std::span<const uint8_t>) + 2 * sizeof(size_t) + 2 * sizeof(uint64_t) +
	4 * (sizeof(void *) + sizeof(size_t) + sizeof(void *)) +
	// Actual account/empty/key_valid/kind_valid helper params/results,
	// operation_id_equal two refs/result and zero's byte range loop.
	3 * sizeof(void *) + sizeof(economic_account_kind) + sizeof(uint64_t) + sizeof(bool) +
	2 * (sizeof(void *) + sizeof(bool)) + sizeof(economic_account_kind) + sizeof(bool) +
	2 * sizeof(void *) + sizeof(bool) + 4 * sizeof(void *) + sizeof(uint8_t) + sizeof(bool) +
	// Original append_u64/u32/u16 and native_fact_append<T> pointer/value/
	// index/byte result lifetimes, plus initializer-list begin/end/size.
	4 * (sizeof(void *) + sizeof(uint64_t) + sizeof(size_t) + sizeof(uint8_t)) +
	3 * (sizeof(void *) + sizeof(void *)) + sizeof(std::initializer_list<uint64_t>) +
	// Original metadata assignment generated function this/source and the
	// returned source_for event temporary, optional typed path above.
	2 * sizeof(void *) + sizeof(economic_source_event) + auction_codec_optional_frames +
	auction_codec_equal_frames + auction_codec_copy_n_frames;
bool auction_codec_add(size_t &total, size_t value) noexcept
{
	if (value > SIZE_MAX - total)
		return false;
	total += value;
	return true;
}
struct auction_codec_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	size_t native_frames = 0;
	size_t payload_frames = 0;
	const economic_frozen_intent *intent = nullptr;
	const economic_admission_facts *admission = nullptr;
	const critical_command *projection = nullptr;
	const std::vector<uint8_t> *expected = nullptr;
	const auction_native_command_context *native = nullptr;
	bool denied = false;
	static bool forward(size_t amount, void *opaque) noexcept
	{
		auto &self = *static_cast<auction_codec_budget *>(opaque);
		if (self.denied || !self.reserve || !self.reserve(amount, self.context))
		{
			self.denied = true;
			return false;
		}
		return true;
	}
	bool prefix(size_t &value, size_t extra = 0) noexcept
	{
		value = outer;
		size_t heap = 0;
		// Genuine observer/relay/prefix/checked-add parameter, local and result
		// lifetimes, plus retained-vector public capacity/size/data carriers.
		constexpr size_t observer_frames = 13 * sizeof(void *) + 10 * sizeof(size_t) +
						   6 * sizeof(bool) +
						   8 * (sizeof(void *) + sizeof(size_t));
		if (!auction_codec_add(value, sizeof(*this)) || !auction_codec_add(value, frames) ||
		    !auction_codec_add(value, native_frames) ||
		    !auction_codec_add(value, payload_frames) ||
		    !auction_codec_add(value, observer_frames) ||
		    (intent && !auction_codec_add(value, intent->admission.facts.capacity())) ||
		    (admission && !auction_codec_add(value, admission->facts.capacity())) ||
		    (expected && !auction_codec_add(value, expected->capacity())) ||
		    (projection && (!critical_command_current_heap_bytes(*projection, &heap) ||
				    !auction_codec_add(value, heap))) ||
		    (native && (native->before_item_uids.capacity() > SIZE_MAX / sizeof(uint64_t) ||
				!auction_codec_add(value, native->base_v1_payload.capacity()) ||
				!auction_codec_add(value, native->before_item_uids.capacity() *
								  sizeof(uint64_t)))) ||
		    !auction_codec_add(value, extra))
		{
			denied = true;
			return false;
		}
		return true;
	}
	bool peak(size_t extra = 0) noexcept
	{
		size_t value = 0;
		return prefix(value, extra) && forward(value, this);
	}
};
// Runtime non-debug C++20 GCC13/C++11 ABI byte-vector growth law. These are
// source-level requested payload bytes; emitted/libc/native qualification is
// separate. Refuse unsupported policy before any fallible private operation.
bool auction_codec_policy() noexcept
{
#if defined(__linux__) && defined(__x86_64__) && __cplusplus == 202002L &&                        \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&    \
	!defined(_GLIBCXX_PARALLEL) && !defined(_GLIBCXX_SANITIZE_VECTOR)
	return sizeof(void *) == 8 && sizeof(size_t) == 8;
#else
	return false;
#endif
}
bool auction_codec_growth_peak(size_t size, size_t capacity, size_t count, size_t &request) noexcept
{
	if (count > SIZE_MAX - size)
		return false;
	const size_t required = size + count;
	if (required <= capacity)
	{
		request = 0;
		return true;
	}
	request = size;
	const size_t added = size > count ? size : count;
	return auction_codec_add(request, added);
}
[[maybe_unused]] bool auction_codec_native_extension_peak(const std::vector<uint8_t> &facts,
							  auction_codec_budget &budget) noexcept
{
	size_t size = facts.size(), capacity = facts.capacity(), largest = 0, request = 0;
	// Exact original ANF2 append sequence: integer bytes use individual
	// push_back; each digest uses one forward insert of 32 actual bytes.
	constexpr size_t runs[] = { 4, 2, 2, 4, 8, 32, 32, 32, 4, 2, 2 };
	for (size_t run = 0; run < std::size(runs); ++run)
	{
		const bool block = run >= 5 && run <= 7;
		const size_t count = block ? runs[run] : 1;
		const size_t iterations = block ? 1 : runs[run];
		for (size_t i = 0; i < iterations; ++i)
		{
			if (!auction_codec_growth_peak(size, capacity, count, request))
				return false;
			if (request)
			{
				size_t simultaneous = capacity;
				if (!auction_codec_add(simultaneous, request))
					return false;
				if (simultaneous > largest)
					largest = simultaneous;
				capacity = request;
			}
			if (!auction_codec_add(size, count))
				return false;
		}
	}
	// prefix already owns the actual old facts allocation; replace that term
	// by the largest authentic simultaneous old/new growth request.
	const size_t extra = largest > facts.capacity() ? largest - facts.capacity() : 0;
	// This forecast's actual automatic runs[11], size/capacity/largest/request,
	// run/count/iterations/i and growth helper required/added/ref arguments
	// remain live during its reserve callback. They are source owners, not a
	// reserve-capacity multiplier or a claim about emitted stack bytes.
	constexpr size_t forecast_frames =
		11 * sizeof(size_t) + 10 * sizeof(size_t) + 3 * sizeof(bool) + 6 * sizeof(void *);
	size_t peak_extra = extra;
	return auction_codec_add(peak_extra, forecast_frames) && budget.peak(peak_extra);
}
bool auction_codec_payload(const critical_command &command, auction_command_payload *out,
			   auction_codec_budget &budget, bool native_allowed) noexcept
{
	size_t nested = 0;
	// Keep this genuine helper's params/locals/result carriers live across
	// every nested absolute callback, including the original nonnative path.
	budget.payload_frames =
		5 * sizeof(void *) + 2 * sizeof(bool) + sizeof(size_t) + sizeof(error);
	if (!budget.peak())
		return false;
	if (!budget.prefix(nested))
		return false;
	if (native_allowed && command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)
	{
		// Original decode_payload constructs its separate native context.
		budget.native_frames = sizeof(auction_native_command_context);
		if (!budget.peak())
			return false;
		auction_native_command_context native;
		budget.native = &native;
		if (!budget.prefix(nested))
			return false;
		const auto status = auction_native_command_decode_bounded(
			command, &native, auction_codec_budget::forward, &budget, nested);
		if (status != error::ok)
			return false;
		*out = native.payload;
		budget.native = nullptr;
		budget.native_frames = 0;
		budget.payload_frames = 0;
		return true;
	}
	const bool result = auction_command_decode_payload_bounded(
		command, out, auction_codec_budget::forward, &budget, nested, &budget.denied);
	budget.payload_frames = 0;
	return result;
}
} // namespace
economic_accounting_error auction_bid_accounting_intent_bounded(
	const critical_command &command, const critical_operation_id &epoch,
	const auction_bid_accounting_listing &listing,
	const auction_bid_accounting_accounts &accounts, std::vector<uint8_t> *encoded,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	const size_t frames =
		sizeof(auction_command_payload) + sizeof(economic_admission_facts) +
		// Public parameters/status/prefix/request, original typed read lambdas,
		// loop indices/references, fixed arrays/comparisons and helper results.
		18 * sizeof(void *) + 13 * sizeof(size_t) + 8 * sizeof(bool) + 3 * sizeof(error) +
		6 * sizeof(uint64_t) + 4 * sizeof(uint32_t) + 3 * sizeof(uint16_t) +
		sizeof(std::array<uint8_t, 4>) + auction_codec_scalar_source_frames +
		auction_codec_vector_frames + auction_codec_move_frames +
		auction_codec_vector_constructor_frames +
		// valid_accounts ids[6], endpoint closure's reference capture, actual
		// lambda this/key/absent/pid/required/unused parameters and bool result;
		// outbid/index/prior plus bid_value bid/sold reference parameters.
		6 * sizeof(uint64_t) + 2 * sizeof(void *) + 2 * sizeof(uint32_t) +
		3 * sizeof(bool) + sizeof(bool) + 2 * sizeof(size_t) + 2 * sizeof(void *) +
		sizeof(int64_t) + sizeof(bool) +
		// frozen_facts reserve140: mapping[6], initializer_list, loop refs,
		// returned fresh vector and source_for value only on original intent.
		6 * sizeof(uint64_t) + sizeof(std::initializer_list<uint64_t>) +
		3 * sizeof(void *) + sizeof(uint64_t) + sizeof(std::vector<uint8_t>);
	auction_codec_budget budget{ reserve, context, outer_live, frames };
	size_t nested = 0;
	if (!auction_codec_policy() || !budget.peak(critical_command_valid_frame_bytes()))
		return error::capacity;
	if (!encoded || command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    critical_operation_id_is_zero(epoch))
		return error::invalid_version;
	auction_command_payload payload = {};
	if (!auction_codec_payload(command, &payload, budget, false))
		return budget.denied ? error::capacity : error::corrupt_evidence;
	int64_t bid = 0;
	bool sold = false;
	if (!valid_listing(listing, payload) || !bid_value(payload, listing, &bid, &sold) ||
	    !valid_accounts(accounts, listing, payload, sold) ||
	    critical_operation_id_equal(command.operation_id, listing.listing_operation) ||
	    critical_operation_id_equal(command.operation_id, listing.previous_bid_operation))
		return budget.denied ? error::capacity : error::invalid_identity;
	try
	{
		economic_admission_facts facts;
		budget.admission = &facts;
		facts.metadata.lineage = accounts.wallet.lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.original_operation_id = listing.listing_operation;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = payload.actor_pid;
		facts.metadata.writer_id = ECONOMIC_WRITER_AUCTION_BID;
		facts.metadata.reason = economic_reason::auction_bid;
		facts.metadata.source_event = source_for(listing);
		if (!budget.peak(140))
			return error::capacity;
		facts.facts = frozen_facts(listing, accounts);
		if (!budget.prefix(nested))
			return error::capacity;
		return economic_intent_freeze_fixed_bounded(
			command, facts, encoded, auction_codec_budget::forward, &budget, nested);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error auction_bid_accounting_decode_bounded(
	const critical_command &command, economic_frozen_intent *intent,
	auction_command_payload *payload, auction_bid_accounting_listing *listing,
	auction_bid_accounting_accounts *accounts, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	const size_t frames =
		sizeof(auction_command_payload) + sizeof(auction_bid_accounting_listing) +
		sizeof(auction_bid_accounting_accounts) + sizeof(economic_frozen_intent) +
		sizeof(std::span<const uint8_t>) + sizeof(critical_command) +
		sizeof(std::vector<uint8_t>) +
		// Public parameters/status/prefix/request, original typed read lambdas,
		// loop indices/references, fixed arrays/comparisons and helper results.
		18 * sizeof(void *) + 13 * sizeof(size_t) + 8 * sizeof(bool) + 3 * sizeof(error) +
		6 * sizeof(uint64_t) + 4 * sizeof(uint32_t) + 3 * sizeof(uint16_t) +
		sizeof(std::array<uint8_t, 4>) + auction_codec_scalar_source_frames +
		auction_codec_vector_frames + auction_codec_move_frames +
		auction_codec_vector_constructor_frames +
		// valid_accounts ids[6], endpoint closure's reference capture, actual
		// lambda this/key/absent/pid/required/unused parameters and bool result;
		// outbid/index/prior plus bid_value bid/sold reference parameters.
		6 * sizeof(uint64_t) + 2 * sizeof(void *) + 2 * sizeof(uint32_t) +
		3 * sizeof(bool) + sizeof(bool) + 2 * sizeof(size_t) + 2 * sizeof(void *) +
		sizeof(int64_t) + sizeof(bool) +
		// frozen_facts reserve140: mapping[6], initializer_list, loop refs,
		// returned fresh vector and source_for value only on original intent.
		6 * sizeof(uint64_t) + sizeof(std::initializer_list<uint64_t>) +
		3 * sizeof(void *) + sizeof(uint64_t) + sizeof(std::vector<uint8_t>);
	auction_codec_budget budget{ reserve, context, outer_live, frames };
	size_t nested = 0;
	if (!auction_codec_policy() || !budget.peak(critical_command_valid_frame_bytes()))
		return error::capacity;
	if (!intent || !payload || !listing || !accounts ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return error::invalid_version;
	try
	{
		auction_command_payload parsed_payload = {};
		if (!auction_codec_payload(command, &parsed_payload, budget, false) ||
		    parsed_payload.action != auction_action::bid)
			return budget.denied ? error::capacity : error::invalid_identity;
		economic_frozen_intent parsed_intent;
		budget.intent = &parsed_intent;
		if (!budget.prefix(nested))
			return error::capacity;
		const auto decoded = economic_intent_decode_bounded(command.accounting_intent,
								    &parsed_intent,
								    auction_codec_budget::forward,
								    &budget, nested);
		if (decoded != error::ok)
			return budget.denied ? error::capacity : error::corrupt_evidence;
		if (!budget.prefix(nested))
			return error::capacity;
		const auto binding = economic_intent_verify_binding_bounded(
			command, parsed_intent, auction_codec_budget::forward, &budget, nested);
		if (binding != error::ok)
			return budget.denied ? error::capacity : error::corrupt_evidence;
		const auto facts = std::span<const uint8_t>(parsed_intent.admission.facts);
		if (facts.size() != 124 && facts.size() != 140)
			return budget.denied ? error::capacity : error::invalid_identity;
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
				return budget.denied ? error::capacity : error::invalid_identity;
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
		size_t copy_request = 0;
		if (!critical_command_fresh_copy_request_bytes(command, &copy_request) ||
		    !auction_codec_add(copy_request, critical_command_copy_frame_bytes()) ||
		    !budget.peak(copy_request))
			return error::capacity;
		critical_command projected = command;
		budget.projection = &projected;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		budget.expected = &expected;
		if (!budget.prefix(nested))
			return error::capacity;
		const auto frozen = auction_bid_accounting_intent_bounded(
			projected, parsed_intent.admission.metadata.epoch, parsed_listing,
			parsed_accounts, &expected, auction_codec_budget::forward, &budget, nested);
		if (frozen != error::ok || expected != command.accounting_intent)
			return budget.denied ? error::capacity : error::unauthorized;
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
