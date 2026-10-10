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
economic_accounting_error auction_settlement_accounting_intent_bounded(
	const critical_command &command, const critical_operation_id &epoch,
	const auction_settlement_listing &listing, const auction_settlement_accounts &accounts,
	std::vector<uint8_t> *encoded, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept
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
		// valid(): ids[4]/lineage/sale/index/prior/item reference. Original
		// facts(): mapping[4], values[6], values64[4], initializer_list and
		// loop references/indices/item reference; fresh returned vector.
		4 * sizeof(uint64_t) + sizeof(bool) + 2 * sizeof(size_t) + 2 * sizeof(void *) +
		8 * sizeof(uint64_t) + 6 * sizeof(uint32_t) +
		2 * sizeof(std::initializer_list<uint64_t>) +
		sizeof(std::initializer_list<uint32_t>) + 9 * sizeof(void *) + sizeof(uint64_t) +
		sizeof(uint32_t) + sizeof(size_t) + sizeof(std::vector<uint8_t>);
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
	if (!valid(payload, listing, accounts) ||
	    critical_operation_id_equal(command.operation_id, listing.listing_operation) ||
	    critical_operation_id_equal(command.operation_id, listing.winning_bid_operation))
		return budget.denied ? error::capacity : error::invalid_identity;
	try
	{
		economic_admission_facts admission;
		budget.admission = &admission;
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
		const size_t fact_request = 122 + listing.item_count * 27;
		size_t fact_peak = fact_request;
		if (accounts.absent_seller_pid && (!auction_codec_add(fact_peak, fact_request) ||
						   !auction_codec_add(fact_peak, fact_request)))
			return error::capacity;
		if (!budget.peak(fact_peak))
			return error::capacity;
		admission.facts = facts(listing, accounts);
		if (!budget.prefix(nested))
			return error::capacity;
		return economic_intent_freeze_fixed_bounded(command, admission, encoded,
							    auction_codec_budget::forward, &budget,
							    nested);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error auction_settlement_accounting_decode_bounded(
	const critical_command &command, economic_frozen_intent *intent,
	auction_command_payload *payload, auction_settlement_listing *listing,
	auction_settlement_accounts *accounts, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	const size_t frames =
		sizeof(auction_command_payload) + sizeof(auction_settlement_listing) +
		sizeof(auction_settlement_accounts) + sizeof(economic_frozen_intent) +
		sizeof(std::span<const uint8_t>) + sizeof(critical_command) +
		sizeof(std::vector<uint8_t>) +
		// Public parameters/status/prefix/request, original typed read lambdas,
		// loop indices/references, fixed arrays/comparisons and helper results.
		18 * sizeof(void *) + 13 * sizeof(size_t) + 8 * sizeof(bool) + 3 * sizeof(error) +
		6 * sizeof(uint64_t) + 4 * sizeof(uint32_t) + 3 * sizeof(uint16_t) +
		sizeof(std::array<uint8_t, 4>) + auction_codec_scalar_source_frames +
		auction_codec_vector_frames + auction_codec_move_frames +
		auction_codec_vector_constructor_frames +
		// valid(): ids[4]/lineage/sale/index/prior/item reference. Original
		// facts(): mapping[4], values[6], values64[4], initializer_list and
		// loop references/indices/item reference; fresh returned vector.
		4 * sizeof(uint64_t) + sizeof(bool) + 2 * sizeof(size_t) + 2 * sizeof(void *) +
		8 * sizeof(uint64_t) + 6 * sizeof(uint32_t) +
		2 * sizeof(std::initializer_list<uint64_t>) +
		sizeof(std::initializer_list<uint32_t>) + 9 * sizeof(void *) + sizeof(uint64_t) +
		sizeof(uint32_t) + sizeof(size_t) + sizeof(std::vector<uint8_t>);
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
		    (parsed_payload.action != auction_action::finalize &&
		     parsed_payload.action != auction_action::remove))
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
		if (facts.size() < 122)
			return budget.denied ? error::capacity : error::invalid_identity;
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
			return budget.denied ? error::capacity : error::invalid_identity;
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
				return budget.denied ? error::capacity : error::invalid_identity;
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
				return budget.denied ? error::capacity : error::invalid_identity;
			item.claimed = facts[offset + 26] == 1;
		}
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
		const auto frozen = auction_settlement_accounting_intent_bounded(
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
