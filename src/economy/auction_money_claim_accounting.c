#include "economy/auction_money_claim_accounting.h"

#include <openssl/sha.h>

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
	for (size_t index = 0; index < 2; ++index)
		bytes->push_back(static_cast<uint8_t>(value >> (index * 8)));
}

bool valid(const auction_command_payload &payload, const economic_account_key &wallet,
	   const economic_account_key &bank, const economic_account_key &claim_account,
	   const auction_money_claim_state &claim)
{
	if (payload.action != auction_action::claim_money || !payload.actor_pid ||
	    payload.actor_pid != claim.beneficiary_pid || claim.money <= 0 ||
	    claim.money > INT_MAX || claim.revision == UINT64_MAX || claim.sources.empty() ||
	    claim.sources.size() > ECONOMIC_AUCTION_CLAIM_MAX_SOURCES ||
	    !economic_account_key_valid(wallet) || wallet.kind != economic_account_kind::wallet ||
	    wallet.context_id || !economic_account_key_valid(bank) ||
	    bank.kind != economic_account_kind::bank || bank.context_id != payload.racewar ||
	    !economic_account_key_valid(claim_account) ||
	    claim_account.kind != economic_account_kind::pending_claim ||
	    claim_account.context_id || wallet.lineage.bytes != bank.lineage.bytes ||
	    wallet.lineage.bytes != claim_account.lineage.bytes ||
	    wallet.authority_id == bank.authority_id ||
	    wallet.authority_id == claim_account.authority_id ||
	    bank.authority_id == claim_account.authority_id)
		return false;
	uint64_t total = 0;
	for (size_t index = 0; index < claim.sources.size(); ++index)
	{
		const auto &source = claim.sources[index];
		if (critical_operation_id_is_zero(source.operation) || !source.slot ||
		    source.beneficiary_pid != payload.actor_pid ||
		    source.claim_mapping_id != claim_account.authority_id || !source.amount ||
		    source.amount > static_cast<uint64_t>(INT_MAX) - total)
			return false;
		if (index)
		{
			const auto &previous = claim.sources[index - 1];
			if (source.operation.bytes < previous.operation.bytes ||
			    (source.operation.bytes == previous.operation.bytes &&
			     source.slot <= previous.slot))
				return false;
		}
		total += source.amount;
	}
	return total == static_cast<uint64_t>(claim.money);
}

economic_digest source_digest(const auction_money_claim_state &claim)
{
	static constexpr char tag[] = "DURIS-PENDING-CLAIM-SOURCES-V1";
	std::vector<uint8_t> bytes(tag, tag + sizeof(tag) - 1);
	append_u32(&bytes, static_cast<uint32_t>(claim.sources.size()));
	for (const auto &source : claim.sources)
	{
		bytes.insert(bytes.end(), source.operation.bytes.begin(),
			     source.operation.bytes.end());
		append_u16(&bytes, source.slot);
		append_u32(&bytes, source.beneficiary_pid);
		append_u64(&bytes, source.claim_mapping_id);
		append_u64(&bytes, source.amount);
	}
	economic_digest digest = {};
	SHA256(bytes.data(), bytes.size(), digest.data());
	return digest;
}

std::vector<uint8_t> facts(const economic_account_key &wallet, const economic_account_key &bank,
			   const economic_account_key &claim_account,
			   const auction_money_claim_state &claim)
{
	std::vector<uint8_t> result;
	result.reserve(84);
	append_u64(&result, wallet.authority_id);
	append_u64(&result, bank.authority_id);
	append_u64(&result, claim_account.authority_id);
	append_u32(&result, claim.beneficiary_pid);
	append_u64(&result, static_cast<uint64_t>(claim.money));
	append_u64(&result, claim.revision);
	append_u32(&result, static_cast<uint32_t>(claim.sources.size()));
	const auto digest = source_digest(claim);
	result.insert(result.end(), digest.begin(), digest.end());
	return result;
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

economic_accounting_error auction_money_claim_accounting_intent(
	const critical_command &command, const critical_operation_id &epoch,
	const economic_account_key &wallet, const economic_account_key &bank,
	const economic_account_key &claim_account, const auction_money_claim_state &claim,
	std::vector<uint8_t> *encoded)
{
	if (!encoded || command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    critical_operation_id_is_zero(epoch))
		return error::invalid_version;
	auction_command_payload payload = {};
	if (!auction_command_decode_payload(command, &payload))
		return error::corrupt_evidence;
	if (!valid(payload, wallet, bank, claim_account, claim) ||
	    critical_operation_id_equal(command.operation_id, claim.sources.front().operation))
		return error::invalid_identity;
	try
	{
		economic_admission_facts admission;
		admission.metadata.lineage = wallet.lineage;
		admission.metadata.epoch = epoch;
		admission.metadata.original_operation_id = claim.sources.front().operation;
		admission.metadata.actor_kind = economic_actor_kind::domain;
		admission.metadata.actor_id = payload.actor_pid;
		admission.metadata.writer_id = ECONOMIC_WRITER_AUCTION_MONEY_CLAIM;
		admission.metadata.reason = economic_reason::auction_claim;
		admission.metadata.source_event = { economic_source_kind::service,
						    claim.sources.front().operation,
						    claim.sources.front().operation, claim.revision,
						    claim.sources.front().slot };
		admission.facts = facts(wallet, bank, claim_account, claim);
		return economic_intent_freeze(command, admission, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error auction_money_claim_accounting_decode(const critical_command &command,
								economic_frozen_intent *intent,
								auction_command_payload *payload,
								economic_account_key *wallet,
								economic_account_key *bank,
								economic_account_key *claim_account)
{
	if (!intent || !payload || !wallet || !bank || !claim_account ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return error::invalid_version;
	try
	{
		auction_command_payload parsed_payload = {};
		if (!auction_command_decode_payload(command, &parsed_payload) ||
		    parsed_payload.action != auction_action::claim_money ||
		    !parsed_payload.actor_pid)
			return error::invalid_identity;
		economic_frozen_intent parsed_intent;
		if (economic_intent_decode(command.accounting_intent, &parsed_intent) !=
			    error::ok ||
		    economic_intent_verify_binding(command, parsed_intent) != error::ok)
			return error::corrupt_evidence;
		const auto facts = std::span<const uint8_t>(parsed_intent.admission.facts);
		if (facts.size() != 80)
			return error::invalid_identity;
		const auto number = [&](size_t offset, size_t width)
		{
			uint64_t value = 0;
			for (size_t byte = 0; byte < width; ++byte)
				value |= static_cast<uint64_t>(facts[offset + byte]) << (byte * 8);
			return value;
		};
		const auto &meta = parsed_intent.admission.metadata;
		if (meta.writer_id != ECONOMIC_WRITER_AUCTION_MONEY_CLAIM ||
		    meta.reason != economic_reason::auction_claim ||
		    meta.actor_kind != economic_actor_kind::domain ||
		    meta.actor_id != parsed_payload.actor_pid ||
		    number(24, 4) != parsed_payload.actor_pid || !number(28, 8) ||
		    number(28, 8) > INT_MAX || !number(44, 4) ||
		    number(44, 4) > ECONOMIC_AUCTION_CLAIM_MAX_SOURCES || !meta.source_event ||
		    meta.source_event->kind != economic_source_kind::service ||
		    meta.source_event->source.bytes != meta.original_operation_id.bytes ||
		    meta.source_event->generation.bytes != meta.original_operation_id.bytes ||
		    meta.source_event->sequence != number(36, 8) || !meta.source_event->slot)
			return error::invalid_identity;
		const economic_account_key parsed_wallet = { meta.lineage,
							     economic_account_kind::wallet,
							     number(0, 8), 0 };
		const economic_account_key parsed_bank = { meta.lineage,
							   economic_account_kind::bank,
							   number(8, 8), parsed_payload.racewar };
		const economic_account_key parsed_claim = { meta.lineage,
							    economic_account_kind::pending_claim,
							    number(16, 8), 0 };
		if (!economic_account_key_valid(parsed_wallet) ||
		    !economic_account_key_valid(parsed_bank) ||
		    !economic_account_key_valid(parsed_claim) ||
		    parsed_wallet.authority_id == parsed_bank.authority_id ||
		    parsed_wallet.authority_id == parsed_claim.authority_id ||
		    parsed_bank.authority_id == parsed_claim.authority_id)
			return error::invalid_identity;
		*intent = std::move(parsed_intent);
		*payload = parsed_payload;
		*wallet = parsed_wallet;
		*bank = parsed_bank;
		*claim_account = parsed_claim;
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error auction_money_claim_accounting_plan(
	const critical_command &command, const economic_frozen_intent &intent,
	const auction_money_claim_authority &authority, const auction_command_result &result,
	economic_accounting_plan *plan)
{
	if (!plan)
		return error::corrupt_evidence;
	try
	{
		auto status = economic_intent_verify_binding(command, intent);
		if (status != error::ok)
			return status;
		const auto &meta = intent.admission.metadata;
		if (meta.writer_id != ECONOMIC_WRITER_AUCTION_MONEY_CLAIM ||
		    meta.reason != economic_reason::auction_claim ||
		    meta.actor_kind != economic_actor_kind::domain ||
		    meta.epoch.bytes != authority.epoch.bytes ||
		    meta.lineage.bytes != authority.wallet.lineage.bytes)
			return error::unauthorized;
		critical_command projected = command;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		status = auction_money_claim_accounting_intent(projected, authority.epoch,
							       authority.wallet, authority.bank,
							       authority.claim_account,
							       authority.claim, &expected);
		if (status != error::ok || expected != command.accounting_intent)
			return error::stale_revision;
		const auto &before = authority.balances_before;
		int64_t wallet_before = 0;
		status = economic_coin_value(before.wallet.amount, &wallet_before);
		if (status != error::ok)
			return status;
		auction_command_payload payload = {};
		if (!auction_command_decode_payload(command, &payload))
			return error::corrupt_evidence;
		if (wallet_before > INT64_MAX - authority.claim.money)
			return error::overflow;
		if (before.wallet_revision != payload.expected_wallet_revision ||
		    before.bank_revision != payload.expected_bank_revision ||
		    before.wallet_revision == UINT64_MAX || before.bank_revision == UINT64_MAX)
			return error::stale_revision;
		const auto after = canonical_wallet(wallet_before + authority.claim.money);
		if (result.action != auction_action::claim_money ||
		    result.event_type != auction_event_type::money_claimed || result.auction_id ||
		    result.status || result.seller_pid || result.winner_pid ||
		    result.previous_bidder_pid || result.final_price ||
		    result.wallet_value_delta != authority.claim.money ||
		    result.wallet.amount != after || result.bank.amount != before.bank.amount ||
		    result.wallet_revision != before.wallet_revision + 1 ||
		    result.bank_revision != before.bank_revision + 1 ||
		    result.auction_revision != result.wallet_revision ||
		    result.player_owner_revision || result.auction_owner_revision ||
		    result.item_count ||
		    std::any_of(result.item_uids.begin(), result.item_uids.end(),
				[](uint64_t uid) { return uid != 0; }) ||
		    std::any_of(result.item_revisions.begin(), result.item_revisions.end(),
				[](uint64_t rev) { return rev != 0; }))
			return error::corrupt_evidence;
		economic_accounting_plan candidate;
		status = economic_intent_plan_metadata(command, intent, &candidate.metadata);
		if (status != error::ok)
			return status;
		candidate.accounts.push_back({ authority.wallet, before.wallet.amount, after,
					       before.wallet_revision, result.wallet_revision });
		candidate.accounts.push_back({ authority.claim_account,
					       { authority.claim.money, 0, 0, 0 },
					       {},
					       authority.claim.revision,
					       authority.claim.revision + 1 });
		economic_coin_vector wallet_delta = {};
		status = economic_coin_delta(before.wallet.amount, after, &wallet_delta);
		if (status != error::ok)
			return status;
		candidate.postings.push_back({ 0, 0, 0, wallet_delta, authority.claim.money });
		candidate.postings.push_back(
			{ 1, 1, 0, { -authority.claim.money, 0, 0, 0 }, -authority.claim.money });
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
economic_accounting_error auction_money_claim_accounting_decode_bounded(
	const critical_command &command, economic_frozen_intent *intent,
	auction_command_payload *payload, economic_account_key *wallet, economic_account_key *bank,
	economic_account_key *claim_account, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	const size_t frames =
		sizeof(auction_command_payload) + 3 * sizeof(economic_account_key) +
		sizeof(economic_frozen_intent) + sizeof(std::span<const uint8_t>) +
		// Public parameters/status/prefix/request, original typed read lambdas,
		// loop indices/references, fixed arrays/comparisons and helper results.
		18 * sizeof(void *) + 13 * sizeof(size_t) + 8 * sizeof(bool) + 3 * sizeof(error) +
		6 * sizeof(uint64_t) + 4 * sizeof(uint32_t) + 3 * sizeof(uint16_t) +
		sizeof(std::array<uint8_t, 4>) + auction_codec_scalar_source_frames +
		auction_codec_vector_frames + auction_codec_move_frames +
		auction_codec_vector_constructor_frames;
	auction_codec_budget budget{ reserve, context, outer_live, frames };
	size_t nested = 0;
	if (!auction_codec_policy() || !budget.peak(critical_command_valid_frame_bytes()))
		return error::capacity;
	if (!intent || !payload || !wallet || !bank || !claim_account ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return error::invalid_version;
	try
	{
		auction_command_payload parsed_payload = {};
		if (!auction_codec_payload(command, &parsed_payload, budget, false) ||
		    parsed_payload.action != auction_action::claim_money ||
		    !parsed_payload.actor_pid)
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
		if (facts.size() != 80)
			return budget.denied ? error::capacity : error::invalid_identity;
		const auto number = [&](size_t offset, size_t width)
		{
			uint64_t value = 0;
			for (size_t byte = 0; byte < width; ++byte)
				value |= static_cast<uint64_t>(facts[offset + byte]) << (byte * 8);
			return value;
		};
		const auto &meta = parsed_intent.admission.metadata;
		if (meta.writer_id != ECONOMIC_WRITER_AUCTION_MONEY_CLAIM ||
		    meta.reason != economic_reason::auction_claim ||
		    meta.actor_kind != economic_actor_kind::domain ||
		    meta.actor_id != parsed_payload.actor_pid ||
		    number(24, 4) != parsed_payload.actor_pid || !number(28, 8) ||
		    number(28, 8) > INT_MAX || !number(44, 4) ||
		    number(44, 4) > ECONOMIC_AUCTION_CLAIM_MAX_SOURCES || !meta.source_event ||
		    meta.source_event->kind != economic_source_kind::service ||
		    meta.source_event->source.bytes != meta.original_operation_id.bytes ||
		    meta.source_event->generation.bytes != meta.original_operation_id.bytes ||
		    meta.source_event->sequence != number(36, 8) || !meta.source_event->slot)
			return budget.denied ? error::capacity : error::invalid_identity;
		const economic_account_key parsed_wallet = { meta.lineage,
							     economic_account_kind::wallet,
							     number(0, 8), 0 };
		const economic_account_key parsed_bank = { meta.lineage,
							   economic_account_kind::bank,
							   number(8, 8), parsed_payload.racewar };
		const economic_account_key parsed_claim = { meta.lineage,
							    economic_account_kind::pending_claim,
							    number(16, 8), 0 };
		if (!economic_account_key_valid(parsed_wallet) ||
		    !economic_account_key_valid(parsed_bank) ||
		    !economic_account_key_valid(parsed_claim) ||
		    parsed_wallet.authority_id == parsed_bank.authority_id ||
		    parsed_wallet.authority_id == parsed_claim.authority_id ||
		    parsed_bank.authority_id == parsed_claim.authority_id)
			return budget.denied ? error::capacity : error::invalid_identity;
		*intent = std::move(parsed_intent);
		*payload = parsed_payload;
		*wallet = parsed_wallet;
		*bank = parsed_bank;
		*claim_account = parsed_claim;
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

// PRIVATE UNSEALED own-source draft. Only actual money-claim decoder scopes.
// Genuine vector lifecycle controls reused byte-for-byte (names only) from
// independent reviewed SHOP identity27; no SHOP algorithm/authorization imported.
// Lower command full preentry/source ownership seam remains OPEN.
namespace
{
template <class T> constexpr size_t auction_money_fixed_vector_default = 6 * sizeof(void *);
// Vector destructor -> _Destroy trivial dispatch -> base destructor,
// _M_deallocate -> traits/allocator/new_allocator -> sized delete, followed
// by actual allocator and new_allocator base cleanup. No nontrivial T here.
template <class T> constexpr size_t auction_money_fixed_vector_cleanup =
	sizeof(std::vector<T> *) + 2 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(void *) + 4 * (2 * sizeof(void *) + sizeof(size_t)) +
	// Sized delete plus real _Vector_impl, allocator, new_allocator and
	// _Vector_impl_data cleanup receivers (base destructor above owns P).
	sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) +
	// Genuine C++20 _Destroy and allocator::deallocate runtime false
	// constant-evaluation result carriers; both selected calls still occur.
	2 * sizeof(bool);
// Move assignment operator=(this,source), true_type -> _M_move_assign:
// real tmp(get_allocator()), allocator temporary, two _M_swap_data calls
// with actual _Vector_impl_data temporary, copy_data receivers, std::move,
// allocator_on_move and the tmp/allocator cleanup. std::allocator propagates.
template <class T> constexpr size_t auction_money_fixed_vector_move =
	// Public operator= receivers/result and its actual constexpr policy bool;
	// _M_move_assign(this,source,true_type) formals.
	3 * sizeof(void *) + sizeof(bool) +
	// Public std::move(__x) argument/reference result before _M_move_assign.
	2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(std::true_type) + sizeof(std::vector<T>) +
	sizeof(std::allocator<T>) +
	// get_allocator + const _M_get_Tp_allocator, allocator/new_allocator copy;
	// vector(allocator) -> base -> impl -> allocator/base copy -> data default.
	sizeof(void *) + sizeof(std::allocator<T>) + 2 * sizeof(void *) + 4 * sizeof(void *) +
	5 * 2 * sizeof(void *) + sizeof(void *) +
	2 * (2 * sizeof(void *) + sizeof(typename std::vector<T>::pointer) * 3 + sizeof(void *) +
	     3 * 2 * sizeof(void *) +
	     // Each actual swap temporary's trivial _Vector_impl_data cleanup.
	     sizeof(void *)) +
	// Two _M_get_Tp_allocator scopes; __alloc_on_move -> std::move and
	// genuine defaulted allocator/new_allocator copy-assignment results.
	4 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + 2 * 3 * sizeof(void *) +
	auction_money_fixed_vector_cleanup<T> + 2 * sizeof(void *);

// Source scopes of the actual locally owned frozen DTO: containing frozen,
// admission and metadata default/cleanup receivers; four critical ID wrappers
// with member arrays; two direct digest arrays; the real trivial-source optional
// seven-class default/cleanup chain. The stored source-event is trivial: its
// optional storage cleanup does not invoke a reset/destroy dispatch.
constexpr size_t auction_money_fixed_frozen_lifetime_source =
	3 * (2 * sizeof(void *)) + 4 * (4 * sizeof(void *)) + 2 * (2 * sizeof(void *)) +
	7 * (2 * sizeof(void *)) + auction_money_fixed_vector_default<uint8_t> +
	auction_money_fixed_vector_cleanup<uint8_t>;
// Final strong frozen output: containing generated assignments, actual
// metadata ID-wrapper/member-array assignments, direct digest arrays, real
// optional's seven selected defaulted assignment scopes and actual vector move.
// std::move(parsed_intent) owns its separate input/reference result pair.
constexpr size_t auction_money_fixed_frozen_output_source =
	3 * (3 * sizeof(void *)) + 4 * (2 * 3 * sizeof(void *)) + 2 * (3 * sizeof(void *)) +
	7 * (3 * sizeof(void *)) + 2 * sizeof(void *) + auction_money_fixed_vector_move<uint8_t>;
// Each of three genuine returned account aggregates copies its lineage through
// critical ID-wrapper+member-array scopes; final account copy assignments own
// this/source/reference-result in account, ID wrapper and its array. Actual
// three key cleanups visit key+ID+array receivers, no vector/string heap.
constexpr size_t auction_money_fixed_account_lifetime_output_source =
	3 * (2 * 2 * sizeof(void *) + 3 * 3 * sizeof(void *) + 3 * sizeof(void *));
} // namespace

namespace
{
constexpr size_t auction_money_fixed_array_equal =
	// array<byte,16> operator==(two refs,bool), size(), begin/data/_S_ptr
	// for the two first iterators, and end's data+size. This is the actual
	// lineage comparison, distinct from vector<byte> equality below.
	2 * sizeof(void *) + sizeof(bool) + sizeof(void *) + sizeof(size_t) +
	2 * 4 * sizeof(void *) + 6 * sizeof(void *) + sizeof(size_t) +
	// std::equal -> equal_aux -> equal_aux1 -> equal<true>::equal; genuine
	// pointer niter bases, constexpr simple/integer bools, len, memcmp leaf.
	4 * (3 * sizeof(void *) + sizeof(bool)) + 3 * 2 * sizeof(void *) + 2 * sizeof(bool) +
	sizeof(std::ptrdiff_t) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(int);
constexpr size_t auction_money_fixed_span_source =
	// span(vector&): this/range -> ranges::_Data(this,range)->vector.data
	// -> _M_data_ptr(this,pointer); ranges::_Size(this,range)->vector.size.
	// Actual ranges noexcept expressions are required constant expressions.
	2 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) + 3 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(size_t) + sizeof(void *) + sizeof(size_t) +
	// Delegating span(pointer,count) -> std::to_address(pointer) and actual
	// dynamic __extent_storage(this,count), not a static extent surrogate.
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	// span.size -> extent::_M_extent and operator[] receiver/index/reference.
	2 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(void *) + sizeof(size_t);
}

// PRIVATE candidate: complete money-decoder OWN SOURCE only. The full called
// command/native-source dependency remains an explicit separate join.
namespace
{
constexpr size_t auction_money_fixed_P = sizeof(void *), auction_money_fixed_N = sizeof(size_t),
		 auction_money_fixed_B = sizeof(bool);
// Actual economic_account_key_valid -> ID zero range -> std::array
// begin/end/data/_S_ptr and size, then kind_valid. No stored DTO copy.
constexpr size_t auction_money_fixed_key_valid_source =
	(sizeof(void *) + sizeof(bool)) +
	(sizeof(void *) + 2 * sizeof(const uint8_t *) + sizeof(uint8_t) + sizeof(bool)) +
	2 * 2 * sizeof(void *) + 2 * 2 * sizeof(void *) + 2 * 2 * sizeof(void *) + sizeof(void *) +
	sizeof(size_t) + sizeof(economic_account_kind) + sizeof(bool);
// Trivial optional has seven actual defaulted wrapper/base/payload classes.
// operator bool -> _M_is_engaged owns two receiver/result pairs; operator->
// -> base_impl::_M_get -> payload::_M_get -> __addressof owns
// four real receiver/argument/result pairs. std::addressof is not called. No assert path under this policy.
constexpr size_t auction_money_fixed_optional_access_source =
	2 * (sizeof(void *) + sizeof(bool)) + 4 * 2 * sizeof(void *);
// Strong payload output: actual containing assignment, seven array member
// assignments, and the nine selected generated auction_item_entry assignments.
// Its local aggregate's trivial cleanup has containing/array/item receivers.
constexpr size_t auction_money_fixed_payload_output_lifetime_source =
	(1 + 7 + AUCTION_COMMAND_MAX_ITEMS) * 3 * sizeof(void *) +
	(1 + 7 + AUCTION_COMMAND_MAX_ITEMS) * sizeof(void *);
// Full exact own scalar union. Complete child SOURCE is not aliased here;
// the actual child-owned workspaces and heap remain in child admissions.
constexpr size_t auction_money_fixed_scalar_source =
	// Eight public pointer/reference formals and the outer argument; local
	// own-source/entry/query/nested and two statuses plus catch/result.
	8 * sizeof(void *) + 8 * sizeof(size_t) + 3 * sizeof(error) + sizeof(void *) +
	// Three intent entry/source/supplement locals, their constexpr query;
	// two proof source/initial locals and its constexpr query.
	11 * sizeof(size_t) +
	// Original vector-to-span intent argument is temporary; facts is the
	// later real span. Their sequential scopes are explicitly inventoried.
	2 * sizeof(std::span<const uint8_t>) + auction_money_fixed_span_source +
	// number lambda: reference capture/this, offset/width/byte, value/result.
	2 * sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(uint64_t) +
	// Actual metadata reference and converted uint8_t value. The actual
	// span indexing graph is already in span_source, not duplicated here.
	sizeof(void *) + sizeof(uint8_t) + auction_money_fixed_optional_access_source +
	auction_money_fixed_array_equal + auction_money_fixed_key_valid_source +
	auction_money_fixed_payload_output_lifetime_source +
	auction_money_fixed_frozen_lifetime_source + auction_money_fixed_frozen_output_source +
	auction_money_fixed_account_lifetime_output_source;
struct auction_money_fixed_budget;
constexpr size_t auction_money_fixed_observer_source =
	// checked-add(ref,value,result), prefix(this,out,extra,value,result),
	// peak(this,extra,value,result), forward(amount,opaque,ref,result).
	(sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
	(sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	// Actual sole retained heap: facts vector capacity receiver/size result.
	sizeof(void *) + sizeof(size_t) +
	// Brace aggregate initialization has no constructor call. Actual
	// generated trivial budget cleanup owns its receiver separately.
	sizeof(void *) +
	// Policy bool and source getter's output/result/critical getter N.
	2 * sizeof(bool) + sizeof(void *) + sizeof(size_t);
constexpr size_t auction_money_fixed_workspace_inline = sizeof(auction_command_payload) +
							sizeof(economic_frozen_intent) +
							3 * sizeof(economic_account_key);
struct auction_money_fixed_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, source;
	const economic_frozen_intent *intent = nullptr;
	bool denied = false;
	static bool forward(size_t amount, void *opaque) noexcept
	{
		auto &b = *static_cast<auction_money_fixed_budget *>(opaque);
		if (b.denied || !b.reserve || !b.reserve(amount, b.context))
		{
			b.denied = true;
			return false;
		}
		return true;
	}
	bool prefix(size_t &value, size_t extra = 0) noexcept
	{
		value = outer;
		if (denied || !auction_codec_add(value, source) ||
		    !auction_codec_add(value, sizeof(*this)) ||
		    !auction_codec_add(value, auction_money_fixed_workspace_inline) ||
		    (intent && !auction_codec_add(value, intent->admission.facts.capacity())) ||
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
}
bool auction_money_claim_accounting_decode_own_source_frame_bytes(size_t *out) noexcept
{
	if (!out || !auction_codec_policy())
		return false;
	*out = auction_money_fixed_scalar_source + auction_money_fixed_observer_source +
	       critical_command_valid_frame_bytes();
	return true;
}
bool auction_money_claim_accounting_decode_initial_inline_bytes(size_t *out) noexcept
{
	if (!out || !auction_codec_policy())
		return false;
	*out = sizeof(auction_money_fixed_budget) + auction_money_fixed_workspace_inline;
	return true;
}

// PRIVATE UNSEALED: algorithm/source joins in progress, not selected or qualified.
economic_accounting_error auction_money_claim_accounting_decode_fixed_bounded(
	const critical_command &command, economic_frozen_intent *intent,
	auction_command_payload *payload, economic_account_key *wallet, economic_account_key *bank,
	economic_account_key *claim_account, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	// First query admission owns the real public entry/query scalar scopes.
	size_t own_source = 0, entry_inline = 0, nested = 0, query_peak = outer_live;
	constexpr size_t query_frames =
		auction_money_claim_accounting_decode_own_source_query_frame_bytes();
	constexpr size_t entry_query_source =
		8 * sizeof(void *) + 7 * sizeof(size_t) + sizeof(bool) + sizeof(error);
	if (!reserve || !auction_codec_add(query_peak, entry_query_source) ||
	    !auction_codec_add(query_peak, query_frames) || !reserve(query_peak, context) ||
	    !auction_money_claim_accounting_decode_own_source_frame_bytes(&own_source) ||
	    !auction_money_claim_accounting_decode_initial_inline_bytes(&entry_inline))
		return error::capacity;
	size_t entry_peak = outer_live;
	if (!auction_codec_add(entry_peak, own_source) ||
	    !auction_codec_add(entry_peak, entry_inline) || !reserve(entry_peak, context))
		return error::capacity;
	auction_money_fixed_budget budget{ reserve, context, outer_live, own_source };
	if (!intent || !payload || !wallet || !bank || !claim_account ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return error::invalid_version;
	try
	{
		auction_command_payload parsed_payload = {};
		// Full original v1/v2 lower dispatcher; complete SOURCE/initial
		// is transient. Its fixed companion owns the returned closure once.
		size_t payload_source = 0, payload_initial = 0, payload_supplement = 0;
		constexpr size_t payload_query =
			auction_command_decode_payload_source_query_frame_bytes();
		if (!budget.peak(payload_query) ||
		    !auction_command_decode_payload_source_frame_bytes(&payload_source) ||
		    !auction_command_decode_payload_initial_inline_bytes(&payload_initial) ||
		    !auction_command_decode_payload_source_supplement_frame_bytes(
			    &payload_supplement) ||
		    !auction_codec_add(payload_source, payload_initial) ||
		    !budget.peak(payload_source) || !budget.prefix(nested) ||
		    !auction_codec_add(nested, payload_supplement))
			return error::capacity;
		if (!auction_command_decode_payload_fixed_bounded(
			    command, &parsed_payload, auction_money_fixed_budget::forward, &budget,
			    nested, &budget.denied) ||
		    parsed_payload.action != auction_action::claim_money ||
		    !parsed_payload.actor_pid)
			return budget.denied ? error::capacity : error::invalid_identity;
		economic_frozen_intent parsed_intent;
		budget.intent = &parsed_intent;
		// Complete published intent-decode SOURCE is transient at entry;
		// only its genuine uncovered supplement remains in the child's outer.
		size_t decode_source = 0, decode_initial = 0, decode_supplement = 0;
		constexpr size_t decode_query = economic_intent_decode_source_query_frame_bytes();
		if (!budget.peak(decode_query) ||
		    !economic_intent_decode_source_frame_bytes(&decode_source) ||
		    !economic_intent_decode_initial_inline_bytes(&decode_initial) ||
		    !economic_intent_decode_source_supplement_frame_bytes(&decode_supplement) ||
		    !auction_codec_add(decode_source, decode_initial) ||
		    !budget.peak(decode_source) || !budget.prefix(nested) ||
		    !auction_codec_add(nested, decode_supplement))
			return error::capacity;
		const auto decoded = economic_intent_decode_bounded(
			command.accounting_intent, &parsed_intent,
			auction_money_fixed_budget::forward, &budget, nested);
		if (decoded != error::ok)
			return budget.denied ? error::capacity : error::corrupt_evidence;
		// Fixed proof preserves original canonical binding bytes and completed
		// error mapping; the fixed callee self-retains its own SOURCE once.
		size_t proof_source = 0, proof_initial = 0;
		constexpr size_t proof_query =
			economic_intent_verify_binding_fixed_source_query_frame_bytes();
		if (!budget.peak(proof_query) ||
		    !economic_intent_verify_binding_fixed_source_frame_bytes(&proof_source) ||
		    !economic_intent_verify_binding_fixed_initial_inline_bytes(&proof_initial) ||
		    !auction_codec_add(proof_source, proof_initial) || !budget.peak(proof_source) ||
		    !budget.prefix(nested))
			return error::capacity;
		const auto binding = economic_intent_verify_binding_fixed_bounded(
			command, parsed_intent, auction_money_fixed_budget::forward, &budget,
			nested);
		if (binding != error::ok)
			return budget.denied ? error::capacity : error::corrupt_evidence;
		const auto facts = std::span<const uint8_t>(parsed_intent.admission.facts);
		if (facts.size() != 80)
			return budget.denied ? error::capacity : error::invalid_identity;
		const auto number = [&](size_t offset, size_t width)
		{
			uint64_t value = 0;
			for (size_t byte = 0; byte < width; ++byte)
				value |= static_cast<uint64_t>(facts[offset + byte]) << (byte * 8);
			return value;
		};
		const auto &meta = parsed_intent.admission.metadata;
		if (meta.writer_id != ECONOMIC_WRITER_AUCTION_MONEY_CLAIM ||
		    meta.reason != economic_reason::auction_claim ||
		    meta.actor_kind != economic_actor_kind::domain ||
		    meta.actor_id != parsed_payload.actor_pid ||
		    number(24, 4) != parsed_payload.actor_pid || !number(28, 8) ||
		    number(28, 8) > INT_MAX || !number(44, 4) ||
		    number(44, 4) > ECONOMIC_AUCTION_CLAIM_MAX_SOURCES || !meta.source_event ||
		    meta.source_event->kind != economic_source_kind::service ||
		    meta.source_event->source.bytes != meta.original_operation_id.bytes ||
		    meta.source_event->generation.bytes != meta.original_operation_id.bytes ||
		    meta.source_event->sequence != number(36, 8) || !meta.source_event->slot)
			return budget.denied ? error::capacity : error::invalid_identity;
		const economic_account_key parsed_wallet = { meta.lineage,
							     economic_account_kind::wallet,
							     number(0, 8), 0 };
		const economic_account_key parsed_bank = { meta.lineage,
							   economic_account_kind::bank,
							   number(8, 8), parsed_payload.racewar };
		const economic_account_key parsed_claim = { meta.lineage,
							    economic_account_kind::pending_claim,
							    number(16, 8), 0 };
		if (!economic_account_key_valid(parsed_wallet) ||
		    !economic_account_key_valid(parsed_bank) ||
		    !economic_account_key_valid(parsed_claim) ||
		    parsed_wallet.authority_id == parsed_bank.authority_id ||
		    parsed_wallet.authority_id == parsed_claim.authority_id ||
		    parsed_bank.authority_id == parsed_claim.authority_id)
			return budget.denied ? error::capacity : error::invalid_identity;
		*intent = std::move(parsed_intent);
		*payload = parsed_payload;
		*wallet = parsed_wallet;
		*bank = parsed_bank;
		*claim_account = parsed_claim;
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

// Complete SOURCE getter for the exact fixed money decoder. Lower command
// definitions are prospective until the separate exclusive owner's immutable
// handoff is authenticated. Never qualify the original old bounded proof.
bool auction_money_claim_accounting_decode_source_frame_bytes(size_t *output) noexcept
{
	size_t own = 0, payload = 0, intent = 0, proof = 0, total = 0;
	if (!output || !auction_money_claim_accounting_decode_own_source_frame_bytes(&own) ||
	    !auction_command_decode_payload_source_frame_bytes(&payload) ||
	    !economic_intent_decode_source_frame_bytes(&intent) ||
	    !economic_intent_verify_binding_fixed_source_frame_bytes(&proof))
		return false;
	// Actual child phases are sequential: payload, original intent decode,
	// fixed proof. Parent own SOURCE persists throughout each child.
	total = payload;
	if (intent > total)
		total = intent;
	if (proof > total)
		total = proof;
	if (!auction_codec_add(total, own))
		return false;
	*output = total;
	return true;
}
bool auction_money_claim_accounting_decode_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !auction_codec_policy())
		return false;
	// Exact fixed callee self-retains all own SOURCE and actual child joins.
	*output = 0;
	return true;
}
