#include "economy/economic_currency_adapter.h"

#include <cerrno>
#include <cstring>
#include <new>
#include <utility>

namespace
{
uint32_t writer_for(currency_reason_type reason)
{
	if (reason == currency_reason_type::atm_deposit)
		return ECONOMIC_WRITER_BANK_DEPOSIT;
	if (reason == currency_reason_type::atm_withdraw)
		return ECONOMIC_WRITER_BANK_WITHDRAW;
	return 0;
}
bool account_pair(const economic_account_key &wallet, const economic_account_key &bank)
{
	return economic_account_key_valid(wallet) && economic_account_key_valid(bank) &&
	       wallet.kind == economic_account_kind::wallet &&
	       bank.kind == economic_account_kind::bank && wallet.context_id == 0 &&
	       wallet.lineage.bytes == bank.lineage.bytes;
}
std::vector<uint8_t> bank_facts(const economic_account_key &wallet,
				const economic_account_key &bank)
{
	std::vector<uint8_t> result(ECONOMIC_BANK_FACT_BYTES);
	const uint64_t fields[] = { wallet.authority_id, bank.authority_id, bank.context_id };
	for (size_t field = 0; field < 3; ++field)
		for (size_t byte = 0; byte < 8; ++byte)
			result[field * 8 + byte] =
				static_cast<uint8_t>(fields[field] >> (byte * 8));
	return result;
}
economic_accounting_error mutation_error(unsigned int error)
{
	switch (error)
	{
	case 0:
		return economic_accounting_error::ok;
	case ESTALE:
		return economic_accounting_error::stale_revision;
	case ERANGE:
		return economic_accounting_error::overflow;
	case ENOSPC:
		return economic_accounting_error::negative_holding;
	case EILSEQ:
		return economic_accounting_error::corrupt_evidence;
	default:
		return economic_accounting_error::invalid_identity;
	}
}
economic_accounting_error transfer_policy(const currency_command_payload &payload)
{
	const auto writer = writer_for(payload.reason);
	if (!writer)
		return economic_accounting_error::unauthorized;
	// ATM writers transfer matching denominations; conversion requires a separate
	// capability even when the resulting copper value balances.
	for (size_t part = 0; part < payload.wallet_delta.amount.size(); ++part)
	{
		const auto debit = payload.wallet_delta.amount[part];
		const auto credit = payload.bank_delta.amount[part];
		if ((writer == ECONOMIC_WRITER_BANK_DEPOSIT && (debit > 0 || credit < 0)) ||
		    (writer == ECONOMIC_WRITER_BANK_WITHDRAW && (debit < 0 || credit > 0)))
			return economic_accounting_error::unauthorized;
		// The direction check makes this sum overflow-safe without negation.
		if (debit + credit != 0)
			return economic_accounting_error::unbalanced;
	}
	int64_t wallet = 0, bank = 0;
	auto result = economic_coin_value(payload.wallet_delta.amount, &wallet);
	if (result != economic_accounting_error::ok)
		return result;
	result = economic_coin_value(payload.bank_delta.amount, &bank);
	if (result != economic_accounting_error::ok)
		return result;
	// Avoid negating an unbounded integer. Opposite nonzero signs ensure the
	// sum cannot overflow, even at the endpoints of the signed range.
	if ((writer == ECONOMIC_WRITER_BANK_DEPOSIT && !(wallet < 0 && bank > 0)) ||
	    (writer == ECONOMIC_WRITER_BANK_WITHDRAW && !(wallet > 0 && bank < 0)))
		return economic_accounting_error::unauthorized;
	if (wallet + bank != 0)
		return economic_accounting_error::unbalanced;
	return economic_accounting_error::ok;
}
economic_accounting_error chaos_starter_bank_source(const critical_command &command,
						    const currency_command_payload &payload,
						    economic_source_event *source)
{
	if (!source || payload.reason != currency_reason_type::chaos_starter_reward ||
	    payload.reason_id != payload.pid ||
	    command.source_site != critical_source_site::login ||
	    command.deadline_class != critical_deadline_class::recovery ||
	    payload.wallet_delta.amount != economic_coin_vector{} ||
	    payload.bank_delta.amount != (economic_coin_vector{ 0, 0, 0, 1000000 }))
		return economic_accounting_error::unauthorized;
	critical_operation_id seed = {};
	memcpy(seed.bytes.data(), "CHAOSEED", 8);
	for (size_t byte = 0; byte < 8; ++byte)
		seed.bytes[8 + byte] = static_cast<uint8_t>(uint64_t(payload.pid) >> (8 * byte));
	critical_operation_id operation = {};
	if (!critical_operation_id_derive(seed, 0x43484250, 1, &operation) ||
	    !critical_operation_id_equal(command.operation_id, operation))
		return economic_accounting_error::unauthorized;
	*source = { economic_source_kind::starter_grant, seed, seed, 1, 1 };
	return economic_accounting_error::ok;
}
bool same_source(const economic_source_event &left, const economic_source_event &right)
{
	return left.kind == right.kind && left.source.bytes == right.source.bytes &&
	       left.generation.bytes == right.generation.bytes && left.sequence == right.sequence &&
	       left.slot == right.slot;
}
bool quest_wallet_reward_payload(const critical_command &command,
				 const currency_command_payload &payload)
{
	return command.type == critical_command_type::account_bank &&
	       payload.reason == currency_reason_type::wallet_reward && payload.reason_id > 0 &&
	       command.source_site == critical_source_site::recovery &&
	       command.deadline_class == critical_deadline_class::recovery &&
	       !critical_operation_id_is_zero(command.operation_id) &&
	       currency_command_is_rebasable_wallet_reward(payload);
}
economic_source_event quest_wallet_reward_source(const critical_command &command,
						 const currency_command_payload &payload)
{
	return { economic_source_kind::quest_completion, command.operation_id, command.operation_id,
		 static_cast<uint64_t>(payload.reason_id), 1 };
}
}

economic_prepared_currency::economic_prepared_currency(currency_prepared_mutation mutation,
						       economic_accounting_plan plan,
						       std::vector<uint8_t> encoded)
	: mutation_(std::move(mutation))
	, plan_(std::move(plan))
	, encoded_(std::move(encoded))
{
}

economic_accounting_error
economic_prepared_currency::agrees_with(const economic_accounting_plan &candidate) const
{
	std::vector<uint8_t> encoded;
	const auto result = economic_plan_encode(candidate, &encoded);
	if (result != economic_accounting_error::ok)
		return result;
	return encoded == encoded_ ? economic_accounting_error::ok :
				     economic_accounting_error::payload_conflict;
}

economic_accounting_error economic_bank_transfer_intent(const critical_command &command,
							const critical_operation_id &epoch,
							const economic_account_key &wallet,
							const economic_account_key &bank,
							std::vector<uint8_t> *encoded)
{
	if (!encoded || !account_pair(wallet, bank))
		return economic_accounting_error::invalid_identity;
	currency_command_payload payload = {};
	if (!currency_command_decode_payload(command, &payload))
		return economic_accounting_error::corrupt_evidence;
	auto result = transfer_policy(payload);
	if (result != economic_accounting_error::ok)
		return result;
	if (bank.context_id != payload.racewar)
		return economic_accounting_error::invalid_identity;
	try
	{
		economic_admission_facts facts;
		facts.metadata.lineage = wallet.lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = wallet.authority_id;
		facts.metadata.writer_id = writer_for(payload.reason);
		facts.metadata.reason = economic_reason::bank_transfer;
		facts.facts = bank_facts(wallet, bank);
		return economic_intent_freeze(command, facts, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
}

economic_accounting_error economic_bank_transfer_prepare(
	const critical_command &command, const economic_frozen_intent &intent,
	const economic_currency_authority &authority, currency_revision_policy revision_policy,
	std::optional<economic_prepared_currency> *prepared)
{
	if (!prepared || !account_pair(authority.wallet_account, authority.bank_account))
		return economic_accounting_error::invalid_identity;
	try
	{
		auto result = economic_intent_verify_binding(command, intent);
		if (result != economic_accounting_error::ok)
			return result;
		currency_command_payload payload = {};
		if (!currency_command_decode_payload(command, &payload))
			return economic_accounting_error::corrupt_evidence;
		result = transfer_policy(payload);
		if (result != economic_accounting_error::ok)
			return result;
		const auto &meta = intent.admission.metadata;
		if (meta.epoch.bytes != authority.epoch.bytes ||
		    meta.writer_id != writer_for(payload.reason) ||
		    meta.reason != economic_reason::bank_transfer ||
		    meta.actor_kind != economic_actor_kind::domain ||
		    meta.actor_id != authority.wallet_account.authority_id || meta.source_event ||
		    !critical_operation_id_is_zero(meta.original_operation_id) ||
		    meta.lineage.bytes != authority.wallet_account.lineage.bytes ||
		    authority.bank_account.context_id != payload.racewar ||
		    !critical_entity_key_equal(authority.player_fence, command.keys[0]) ||
		    !critical_entity_key_equal(authority.bank_fence, command.keys[1]) ||
		    intent.admission.facts !=
			    bank_facts(authority.wallet_account, authority.bank_account))
			return economic_accounting_error::unauthorized;
		std::optional<currency_prepared_mutation> mutation;
		const auto domain_error = currency_prepare_mutation(
			payload, authority.state, command.expected_revisions[0].revision,
			command.expected_revisions[1].revision, revision_policy, &mutation);
		if (domain_error)
			return mutation_error(domain_error);
		economic_accounting_plan plan;
		result = economic_intent_plan_metadata(command, intent, &plan.metadata);
		if (result != economic_accounting_error::ok)
			return result;
		const auto &before = mutation->before();
		const auto &after = mutation->after();
		plan.accounts = { { authority.wallet_account, before.wallet.amount,
				    after.wallet.amount, before.wallet_revision,
				    after.wallet_revision },
				  { authority.bank_account, before.bank.amount, after.bank.amount,
				    before.bank_revision, after.bank_revision } };
		int64_t wallet = 0, bank = 0;
		result = economic_coin_value(payload.wallet_delta.amount, &wallet);
		if (result != economic_accounting_error::ok)
			return result;
		result = economic_coin_value(payload.bank_delta.amount, &bank);
		if (result != economic_accounting_error::ok)
			return result;
		plan.postings = { { 0, 0, 0, payload.wallet_delta.amount, wallet },
				  { 1, 1, 0, payload.bank_delta.amount, bank } };
		result = economic_plan_normalize(&plan);
		if (result != economic_accounting_error::ok)
			return result;
		std::vector<uint8_t> encoded;
		result = economic_plan_encode(plan, &encoded);
		if (result != economic_accounting_error::ok)
			return result;
		*prepared =
			economic_prepared_currency(*mutation, std::move(plan), std::move(encoded));
		return economic_accounting_error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
}

economic_accounting_error economic_chaos_starter_bank_intent(const critical_command &command,
							     const critical_operation_id &epoch,
							     const economic_account_key &wallet,
							     const economic_account_key &bank,
							     std::vector<uint8_t> *encoded)
{
	if (!encoded || !account_pair(wallet, bank))
		return economic_accounting_error::invalid_identity;
	currency_command_payload payload = {};
	if (!currency_command_decode_payload(command, &payload))
		return economic_accounting_error::corrupt_evidence;
	economic_source_event source;
	auto result = chaos_starter_bank_source(command, payload, &source);
	if (result != economic_accounting_error::ok)
		return result;
	if (bank.context_id != payload.racewar)
		return economic_accounting_error::invalid_identity;
	try
	{
		economic_admission_facts facts;
		facts.metadata.lineage = wallet.lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = wallet.authority_id;
		facts.metadata.writer_id = ECONOMIC_WRITER_CHAOS_STARTER_BANK;
		facts.metadata.reason = economic_reason::starter_reward;
		facts.metadata.source_event = source;
		facts.facts = bank_facts(wallet, bank);
		return economic_intent_freeze(command, facts, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
}

economic_accounting_error economic_chaos_starter_bank_prepare(
	const critical_command &command, const economic_frozen_intent &intent,
	const economic_currency_authority &authority, currency_revision_policy revision_policy,
	std::optional<economic_prepared_currency> *prepared)
{
	if (!prepared || !account_pair(authority.wallet_account, authority.bank_account))
		return economic_accounting_error::invalid_identity;
	try
	{
		auto result = economic_intent_verify_binding(command, intent);
		if (result != economic_accounting_error::ok)
			return result;
		currency_command_payload payload = {};
		if (!currency_command_decode_payload(command, &payload))
			return economic_accounting_error::corrupt_evidence;
		economic_source_event source;
		result = chaos_starter_bank_source(command, payload, &source);
		if (result != economic_accounting_error::ok)
			return result;
		const auto &meta = intent.admission.metadata;
		if (meta.epoch.bytes != authority.epoch.bytes ||
		    meta.writer_id != ECONOMIC_WRITER_CHAOS_STARTER_BANK ||
		    meta.reason != economic_reason::starter_reward ||
		    meta.actor_kind != economic_actor_kind::domain ||
		    meta.actor_id != authority.wallet_account.authority_id || !meta.source_event ||
		    !same_source(*meta.source_event, source) ||
		    !critical_operation_id_is_zero(meta.original_operation_id) ||
		    meta.lineage.bytes != authority.wallet_account.lineage.bytes ||
		    authority.bank_account.context_id != payload.racewar ||
		    !critical_entity_key_equal(authority.player_fence, command.keys[0]) ||
		    !critical_entity_key_equal(authority.bank_fence, command.keys[1]) ||
		    intent.admission.facts !=
			    bank_facts(authority.wallet_account, authority.bank_account))
			return economic_accounting_error::unauthorized;
		std::optional<currency_prepared_mutation> mutation;
		const auto domain_error = currency_prepare_mutation(
			payload, authority.state, command.expected_revisions[0].revision,
			command.expected_revisions[1].revision, revision_policy, &mutation);
		if (domain_error)
			return mutation_error(domain_error);
		economic_accounting_plan plan;
		result = economic_intent_plan_metadata(command, intent, &plan.metadata);
		if (result != economic_accounting_error::ok)
			return result;
		const auto &before = mutation->before();
		const auto &after = mutation->after();
		const economic_account_key issuance = { authority.bank_account.lineage,
							economic_account_kind::issuance, 1, 0 };
		if (after.wallet_revision != before.wallet_revision)
			plan.accounts.push_back({ authority.wallet_account, before.wallet.amount,
						  after.wallet.amount, before.wallet_revision,
						  after.wallet_revision });
		const uint16_t bank_index = static_cast<uint16_t>(plan.accounts.size());
		plan.accounts.push_back({ authority.bank_account, before.bank.amount,
					  after.bank.amount, before.bank_revision,
					  after.bank_revision });
		const uint16_t issuance_index = static_cast<uint16_t>(plan.accounts.size());
		plan.accounts.push_back({ issuance, {}, {}, 0, 0 });
		int64_t value = 0;
		result = economic_coin_value(payload.bank_delta.amount, &value);
		if (result != economic_accounting_error::ok)
			return result;
		economic_coin_vector issuance_delta = {};
		for (size_t part = 0; part < issuance_delta.size(); ++part)
			issuance_delta[part] = -payload.bank_delta.amount[part];
		plan.postings = { { 0, bank_index, 0, payload.bank_delta.amount, value },
				  { 1, issuance_index, 0, issuance_delta, -value } };
		result = economic_plan_normalize(&plan);
		if (result != economic_accounting_error::ok)
			return result;
		std::vector<uint8_t> encoded;
		result = economic_plan_encode(plan, &encoded);
		if (result != economic_accounting_error::ok)
			return result;
		*prepared =
			economic_prepared_currency(*mutation, std::move(plan), std::move(encoded));
		return economic_accounting_error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
}

economic_accounting_error economic_quest_wallet_reward_intent(const critical_command &command,
							      const critical_operation_id &epoch,
							      const economic_account_key &wallet,
							      const economic_account_key &bank,
							      std::vector<uint8_t> *encoded)
{
	using error = economic_accounting_error;
	if (!encoded || !account_pair(wallet, bank) || critical_operation_id_is_zero(epoch))
		return error::invalid_identity;
	currency_command_payload payload = {};
	if (!currency_command_decode_payload(command, &payload))
		return error::corrupt_evidence;
	if (!quest_wallet_reward_payload(command, payload) || bank.context_id != payload.racewar)
		return error::unauthorized;
	try
	{
		economic_admission_facts facts;
		facts.metadata.lineage = wallet.lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = payload.pid;
		facts.metadata.writer_id = ECONOMIC_WRITER_QUEST_WALLET_REWARD;
		facts.metadata.reason = economic_reason::quest_reward;
		facts.metadata.source_event = quest_wallet_reward_source(command, payload);
		facts.facts = bank_facts(wallet, bank);
		return economic_intent_freeze(command, facts, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error economic_quest_wallet_reward_prepare(
	const critical_command &command, const economic_frozen_intent &intent,
	const economic_currency_authority &authority, currency_revision_policy revision_policy,
	std::optional<economic_prepared_currency> *prepared)
{
	using error = economic_accounting_error;
	if (!prepared || !account_pair(authority.wallet_account, authority.bank_account))
		return error::invalid_identity;
	try
	{
		auto result = economic_intent_verify_binding(command, intent);
		if (result != error::ok)
			return result;
		currency_command_payload payload = {};
		if (!currency_command_decode_payload(command, &payload))
			return error::corrupt_evidence;
		const auto &meta = intent.admission.metadata;
		if (!quest_wallet_reward_payload(command, payload) ||
		    meta.writer_id != ECONOMIC_WRITER_QUEST_WALLET_REWARD ||
		    meta.reason != economic_reason::quest_reward ||
		    meta.actor_kind != economic_actor_kind::domain ||
		    meta.actor_id != payload.pid || !meta.source_event ||
		    !same_source(*meta.source_event,
				 quest_wallet_reward_source(command, payload)) ||
		    meta.epoch.bytes != authority.epoch.bytes ||
		    meta.lineage.bytes != authority.wallet_account.lineage.bytes ||
		    authority.bank_account.context_id != payload.racewar ||
		    !critical_entity_key_equal(authority.player_fence, command.keys[0]) ||
		    !critical_entity_key_equal(authority.bank_fence, command.keys[1]) ||
		    intent.admission.facts !=
			    bank_facts(authority.wallet_account, authority.bank_account))
			return error::unauthorized;
		std::optional<currency_prepared_mutation> mutation;
		const auto domain_error = currency_prepare_mutation(
			payload, authority.state, command.expected_revisions[0].revision,
			command.expected_revisions[1].revision, revision_policy, &mutation);
		if (domain_error)
			return mutation_error(domain_error);
		economic_accounting_plan plan;
		result = economic_intent_plan_metadata(command, intent, &plan.metadata);
		if (result != error::ok)
			return result;
		const auto &before = mutation->before();
		const auto &after = mutation->after();
		const economic_account_key issuance = { authority.wallet_account.lineage,
							economic_account_kind::issuance, 1, 0 };
		plan.accounts = { { authority.wallet_account, before.wallet.amount,
				    after.wallet.amount, before.wallet_revision,
				    after.wallet_revision },
				  { authority.bank_account, before.bank.amount, after.bank.amount,
				    before.bank_revision, after.bank_revision },
				  { issuance, {}, {}, 0, 0 } };
		int64_t value = 0;
		result = economic_coin_value(payload.wallet_delta.amount, &value);
		if (result != error::ok || value <= 0)
			return result == error::ok ? error::unauthorized : result;
		plan.postings = {
			{ 0, 0, 0, payload.wallet_delta.amount, value },
			{ 1,
			  2,
			  0,
			  { -payload.wallet_delta.amount[0], -payload.wallet_delta.amount[1],
			    -payload.wallet_delta.amount[2], -payload.wallet_delta.amount[3] },
			  -value }
		};
		result = economic_plan_normalize(&plan);
		if (result != error::ok)
			return result;
		std::vector<uint8_t> encoded;
		result = economic_plan_encode(plan, &encoded);
		if (result != error::ok)
			return result;
		*prepared =
			economic_prepared_currency(*mutation, std::move(plan), std::move(encoded));
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

// Additive complete typed intent builders. Original APIs above remain intact.
#include <openssl/opensslv.h>
#include <iterator>
#include <limits>
#include <type_traits>

namespace
{
bool currency_intent_profile_policy() noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(__LP64__) && !defined(_WIN32) &&          \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&            \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                            \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                         \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	return sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(std::ptrdiff_t) == 8;
#else
	return false;
#endif
}

bool currency_intent_add(size_t &value, size_t extra) noexcept
{
	if (extra > SIZE_MAX - value)
		return false;
	value += extra;
	return true;
}

using currency_intent_getter = bool (*)(size_t *) noexcept;
struct currency_intent_workspace
{
	currency_command_payload payload{};
	economic_source_event source{};
	economic_admission_facts facts;
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer;
	size_t owned_source;
	size_t current = 0;
	size_t child_source = 0;
	size_t child_supplement = 0;
	size_t child_inline = 0;
	size_t child_query = 0;
	size_t child_outer = 0;
	bool denied = false;

	bool observe() noexcept
	{
		current = outer;
		if (!currency_intent_add(current, sizeof(*this)) ||
		    !currency_intent_add(current, owned_source) ||
		    !currency_intent_add(current, facts.facts.capacity()))
		{
			denied = true;
			return false;
		}
		return true;
	}
	bool admit(size_t extra) noexcept
	{
		if (!observe() || !currency_intent_add(current, extra) ||
		    !reserve(current, context))
		{
			denied = true;
			return false;
		}
		return true;
	}
	static bool forward(size_t bytes, void *opaque) noexcept
	{
		auto &work = *static_cast<currency_intent_workspace *>(opaque);
		if (!work.reserve(bytes, work.context))
		{
			work.denied = true;
			return false;
		}
		return true;
	}
};

// Every selected lower source/entry query runs only after this owning request.
// Whole source/entry is transient; only the genuine uncovered supplement is
// retained in child_outer for unchanged older bounded children.
bool currency_intent_child_entry(currency_intent_workspace &work, currency_intent_getter source,
				 currency_intent_getter supplement, currency_intent_getter initial,
				 size_t query) noexcept
{
	work.child_query = query;
	if (!work.admit(query) || !source(&work.child_source) ||
	    (supplement && !supplement(&work.child_supplement)) || !initial(&work.child_inline) ||
	    !currency_intent_add(work.child_source, work.child_inline) ||
	    !work.admit(work.child_source) || !work.observe())
	{
		work.denied = true;
		return false;
	}
	work.child_outer = work.current;
	if (supplement && !currency_intent_add(work.child_outer, work.child_supplement))
	{
		work.denied = true;
		return false;
	}
	return true;
}

// Source graph: actual byte vector count construction, original bank_facts
// return/move assignment and destructor, including optional non-NRVO return.
// Requests/real objects are observed separately, never allocator header guesses.
constexpr size_t currency_intent_byte_vector_source() noexcept
{
	constexpr size_t P = sizeof(void *), N = sizeof(size_t), B = sizeof(bool);
	using A = std::allocator<uint8_t>;
	constexpr size_t allocation =
		// _M_allocate, traits::allocate, allocator::allocate, new_allocator::allocate
		// hint/_M_max_size, constant-evaluation predicate, operator-new boundary.
		2 * (2 * P + N) + (2 * P + N) + (3 * P + N) + (P + N) + B + (P + N);
	constexpr size_t count_constructor =
		// vector(n,a), default allocator/new_allocator and _Vector_base(n,a),
		// copied impl allocator, data default, _M_create_storage and count checks.
		(2 * P + N) + sizeof(A) + 2 * P + (2 * P + N) + 6 * P + P + (P + N) + (P + 2 * N) +
		4 * P + sizeof(A) +
		// _S_max_size diffmax/allocmax and traits/max_size/min; checked ctor throw
		// boundary carries const char*. C++20 allocator_traits specialization
		// max_size returns directly; no old allocator.max_size descendant.
		(P + 3 * N) + (P + N) + (3 * P + B) + P + allocation +
		// _M_default_initialize -> __uninitialized_default_n_a/_n/aux.
		(P + N) + (3 * P + N) + (2 * P + N) + 2 * B + (3 * P + N) +
		// Trivial byte construct first element then fill_n -> size conversion/
		// __fill_n_a/__fill_a/_a1, niter base/wrap/category, real byte temporary
		// and memset tail. Runtime _Construct maps its actual placement path.
		(2 * P) + P + B + (2 * P + N) + (3 * P + N) + 2 * N + 3 * P + 2 * P +
		(P + sizeof(std::random_access_iterator_tag)) +
		(3 * P + N + sizeof(std::random_access_iterator_tag)) + 3 * P +
		(3 * P + sizeof(uint8_t)) + B + (2 * P + sizeof(int) + N);
	constexpr size_t lifetime =
		// Default vector/base/impl/data/allocator/new_allocator ctors and matching
		// implicit impl/data/allocator/new_allocator destructor this carriers.
		6 * P + 4 * P +
		// vector destructor -> allocator range Destroy -> trivial _Destroy_aux,
		// base destructor, four genuine deallocate layers + sized operator delete.
		P + 2 * P + 3 * P + 2 * P + B + 2 * P + P + B + 4 * (2 * P + N) + (P + N);
	constexpr size_t move =
		// vector operator=(this,x,ref,B), move argument/reference, _M_move_assign,
		// genuine temporary vector and get_allocator7P+A/constalloc constructor11P.
		3 * P + B + 2 * P + 2 * P + sizeof(std::true_type) + sizeof(std::vector<uint8_t>) +
		7 * P + sizeof(A) + 11 * P +
		// Both selected _M_swap_data own this/x/three-pointer tmp/default this and
		// three _M_copy_data(this,x) scopes; both allocator getter paths.
		2 * (2 * P + 3 * P + P + 3 * (2 * P)) + 2 * (2 * P) +
		// Direct C++20 __alloc_on_move: two refs, move, generated allocator/base
		// const-copy assignments. No old true_type allocator dispatch exists.
		10 * P + lifetime +
		// Non-NRVO return vector move/base-move/impl-move/data-move and allocator
		// move/copy this/source carriers; actual returned inline is preadmitted.
		2 * P + 2 * P + 2 * P + 2 * P + 2 * P + 2 * P;
	return count_constructor + lifetime + move + 2 * P + N;
}

constexpr size_t currency_intent_array_source() noexcept
{
	constexpr size_t P = sizeof(void *), N = sizeof(size_t), B = sizeof(bool);
	// Authentic selected GNU13 array has direct _M_elems data/subscript bodies,
	// no historical _S_ptr/_S_ref helper. Equality: operator== -> three array
	// begin/end+data calls -> equal/aux/aux1 -> three __niter_base ->
	// __equal<true>(len) -> __memcmp(constant-evaluation predicate) -> memcmp.
	constexpr size_t equality = 2 * P + B + 3 * (4 * P) + 3 * (3 * P + B) + B + 3 * (2 * P) +
				    (3 * P + N + B) + (2 * P + N + sizeof(int)) + B +
				    (2 * P + N + sizeof(int));
	// Four genuine equality sites: pair lineage, operation comparison and both
	// chaos denomination vectors. Loops do not multiply SOURCE by iterations.
	constexpr size_t zero_ranges = 4 * (2 * (4 * P));
	constexpr size_t seed = 2 * P + (2 * P + N);
	constexpr size_t transfer = (P + N) + 2 * (2 * P + N);
	constexpr size_t values = 2 * ((P + N) + 2 * (2 * P + N));
	constexpr size_t reward = 3 * (2 * P + N);
	constexpr size_t facts_subscript = 2 * P + N;
	return 4 * equality + zero_ranges + seed + transfer + values + reward + facts_subscript;
}

constexpr size_t currency_intent_domain_source() noexcept
{
	constexpr size_t P = sizeof(void *), N = sizeof(size_t), B = sizeof(bool);
	constexpr size_t E = sizeof(economic_accounting_error), I = sizeof(int64_t);
	// Original account_pair; key validity -> zero-ID range -> kind validity.
	constexpr size_t accounts = 2 * P + B + 2 * (P + B + sizeof(economic_account_kind) + B);
	// Four actual critical_operation_id_is_zero lexical scopes: key validity
	// twice, quest epoch, and quest command operation. Each owns ID formal,
	// range reference, begin/end pointer locals, byte loop local and bool return.
	// Their genuine array begin/end/data descendants stay in zero_ranges above.
	constexpr size_t zero_predicates = 4 * (4 * P + sizeof(uint8_t) + B);
	// Original transfer_policy, writer_for, both wide coin-value/narrow calls.
	constexpr size_t transfer =
		P + E + sizeof(uint32_t) + N + 2 * I + 2 * I + E +
		(sizeof(currency_reason_type) + sizeof(uint32_t)) +
		2 * (2 * P + sizeof(__int128_t) + N + E + sizeof(__int128_t) + P + B + 2 * I);
	// Complete local chaos source counterpart includes seed/operation objects;
	// source-event temporary and memcpy args, equality/derive child queried below.
	constexpr size_t chaos = 4 * P + E + sizeof(critical_operation_id) * 2 + N +
				 sizeof(economic_source_event) + (3 * P + N) + 2 * P + B +
				 2 * sizeof(economic_coin_vector);
	// Original quest predicate and reward-source returned event; original
	// is_rebasable_wallet_reward positive/index/amount and zero-ID loop paths.
	constexpr size_t quest = 2 * P + B + P + B + N + B + 2 * P + sizeof(economic_source_event);
	// Optional source assignment selects actual converting assignment/reset/
	// has_value/get/construct/as-value/addressof/placement-new source-event copy.
	constexpr size_t source_assignment = 3 * P + 2 * (P + B) + 2 * (2 * P) + 2 * P + 3 * P +
					     2 * P + 2 * P + 2 * P + 2 * P + 2 * P + P;
	// Actual generated critical-ID/array assignment for lineage+epoch, generated
	// source-event/its two ID members+arrays assignment, and the event copy ctor
	// selected by optional engagement. Scalar fields have no invented heap.
	constexpr size_t value_assignment =
		2 * (3 * P + 3 * P) + (3 * P + 2 * (3 * P + 3 * P)) + (2 * P + 2 * (2 * P + 2 * P));
	// Generated workspace/admission/metadata default/destruction and optional
	// payload layers, each genuine this/result; vector graph mapped separately.
	constexpr size_t defaults = 12 * P + 4 * (P + B);
	return accounts + zero_predicates + transfer + chaos + quest + source_assignment +
	       value_assignment + defaults + currency_intent_array_source();
}

constexpr size_t currency_intent_own_source() noexcept
{
	constexpr size_t P = sizeof(void *), N = sizeof(size_t), B = sizeof(bool);
	constexpr size_t E = sizeof(economic_accounting_error);
	// Entry original formals plus reserve/context/outer; early query/source/inline/
	// initial-current locals and error results. Shared implementation selects all
	// three full routes; enum parameter/result scopes are included explicitly.
	constexpr size_t entries = 2 * (7 * P + N + E) + 4 * N + E + sizeof(unsigned int) +
				   sizeof(currency_command_bounded_result);
	// Checked-add, workspace observe/admit and forwarded reserve callback scopes;
	// facts vector capacity source getter arguments/result included separately.
	constexpr size_t observers =
		P + N + B + P + B + (P + N) + 3 * (P + N + B) + P + N + B + (N + P + B) + P;
	// Generic child preflight formals plus three checked additions and getters;
	// getter descendants use the genuine named fixed query contracts below.
	constexpr size_t preflight = 4 * P + N + B + 3 * (P + N + B);
	// Original bank_facts count result/returned vector, fields[3], indices and
	// wallet/bank refs. Candidate vector objects are prospective inline, not SOURCE.
	constexpr size_t facts = 2 * P + 3 * sizeof(uint64_t) + 2 * N;
	// Each actual child-entry queries full SOURCE, uncovered supplement and
	// initial inline. The named whole getter query dominates either smaller
	// pure getter, and three genuine calls are retained in this finite SUM law.
	constexpr size_t child_queries =
		3 * (currency_command_decode_payload_source_query_frame_bytes() +
		     economic_intent_freeze_fixed_source_query_frame_bytes() +
		     critical_operation_id_derive_source_query_frame_bytes());
	return entries + observers + preflight + facts + currency_intent_domain_source() +
	       currency_intent_byte_vector_source() + child_queries +
	       economic_currency_intent_source_query_frame_bytes();
}

// Required constant evaluation: the library/source-inventory getter graph does
// not execute before the first scalar preflight or while constructing workspace.
constexpr size_t currency_intent_owned_frames = currency_intent_own_source();

enum class currency_intent_route : unsigned int
{
	bank,
	chaos,
	quest
};

economic_accounting_error currency_intent_build_bounded(
	currency_intent_route route, const critical_command &command,
	const critical_operation_id &epoch, const economic_account_key &wallet,
	const economic_account_key &bank, std::vector<uint8_t> *encoded,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	using error = economic_accounting_error;
	// The exact original nonallocating identity/domain error order follows first
	// source admission. Caller retains every input and prior encoded output.
	if (!reserve)
		return error::capacity;
	constexpr size_t first_frames = currency_intent_owned_frames;
	size_t current = outer_live, source = 0, initial = 0;
	if (!currency_intent_add(current, first_frames) || !reserve(current, context) ||
	    !economic_currency_intent_source_frame_bytes(&source) ||
	    !economic_currency_intent_initial_inline_bytes(&initial))
		return error::capacity;
	current = outer_live;
	if (!currency_intent_add(current, source) || !currency_intent_add(current, initial) ||
	    !reserve(current, context))
		return error::capacity;
	if (!encoded || !account_pair(wallet, bank) ||
	    (route == currency_intent_route::quest && critical_operation_id_is_zero(epoch)))
		return error::invalid_identity;
	try
	{
		currency_intent_workspace work{
			{}, {}, {}, reserve, context, outer_live, currency_intent_owned_frames
		};
		constexpr size_t decode_query =
			3 * currency_command_decode_payload_source_query_frame_bytes();
		if (!currency_intent_child_entry(
			    work, currency_command_decode_payload_source_frame_bytes,
			    currency_command_decode_payload_source_supplement_frame_bytes,
			    currency_command_decode_payload_initial_inline_bytes, decode_query))
			return error::capacity;
		const auto decoded = currency_command_decode_payload_bounded_status(
			command, &work.payload, currency_intent_workspace::forward, &work,
			work.child_outer);
		if (decoded != currency_command_bounded_result::ok)
			return work.denied || decoded == currency_command_bounded_result::capacity ?
				       error::capacity :
				       error::corrupt_evidence;
		if (route == currency_intent_route::bank)
		{
			const auto status = transfer_policy(work.payload);
			if (status != error::ok)
				return status;
			if (bank.context_id != work.payload.racewar)
				return error::invalid_identity;
		}
		else if (route == currency_intent_route::chaos)
		{
			if (work.payload.reason != currency_reason_type::chaos_starter_reward ||
			    work.payload.reason_id != work.payload.pid ||
			    command.source_site != critical_source_site::login ||
			    command.deadline_class != critical_deadline_class::recovery ||
			    work.payload.wallet_delta.amount != economic_coin_vector{} ||
			    work.payload.bank_delta.amount !=
				    economic_coin_vector{ 0, 0, 0, 1000000 })
				return error::unauthorized;
			// These genuine two fixed IDs and source event are owned source locals;
			// the derive child self-owns its actual fixed SHA and input arrays.
			critical_operation_id seed{};
			memcpy(seed.bytes.data(), "CHAOSEED", 8);
			for (size_t byte = 0; byte < 8; ++byte)
				seed.bytes[8 + byte] = static_cast<uint8_t>(
					uint64_t(work.payload.pid) >> (8 * byte));
			critical_operation_id operation{};
			constexpr size_t derive_query =
				3 * critical_operation_id_derive_source_query_frame_bytes();
			if (!currency_intent_child_entry(
				    work, critical_operation_id_derive_source_frame_bytes,
				    critical_operation_id_derive_source_supplement_frame_bytes,
				    critical_operation_id_derive_initial_inline_bytes,
				    derive_query))
				return error::capacity;
			if (!critical_operation_id_derive_bounded(
				    seed, 0x43484250, 1, &operation,
				    currency_intent_workspace::forward, &work, work.child_outer))
				return work.denied ? error::capacity : error::unauthorized;
			if (!critical_operation_id_equal(command.operation_id, operation))
				return error::unauthorized;
			work.source = { economic_source_kind::starter_grant, seed, seed, 1, 1 };
			if (bank.context_id != work.payload.racewar)
				return error::invalid_identity;
		}
		else if (!quest_wallet_reward_payload(command, work.payload) ||
			 bank.context_id != work.payload.racewar)
			return error::unauthorized;

		work.facts.metadata.lineage = wallet.lineage;
		work.facts.metadata.epoch = epoch;
		work.facts.metadata.actor_kind = economic_actor_kind::domain;
		work.facts.metadata.actor_id = route == currency_intent_route::quest ?
						       work.payload.pid :
						       wallet.authority_id;
		if (route == currency_intent_route::bank)
		{
			work.facts.metadata.writer_id = writer_for(work.payload.reason);
			work.facts.metadata.reason = economic_reason::bank_transfer;
		}
		else if (route == currency_intent_route::chaos)
		{
			work.facts.metadata.writer_id = ECONOMIC_WRITER_CHAOS_STARTER_BANK;
			work.facts.metadata.reason = economic_reason::starter_reward;
			work.facts.metadata.source_event = work.source;
		}
		else
		{
			work.facts.metadata.writer_id = ECONOMIC_WRITER_QUEST_WALLET_REWARD;
			work.facts.metadata.reason = economic_reason::quest_reward;
			work.facts.metadata.source_event =
				quest_wallet_reward_source(command, work.payload);
		}
		// Original vector(n24) and byte packing are selected unchanged. Source
		// owns fields/index locals. Real return/local vectors coexist only when
		// NRVO is not used; preadmit that authentic maximum before allocating.
		if (!work.admit(2 * sizeof(std::vector<uint8_t>) + ECONOMIC_BANK_FACT_BYTES))
			return error::capacity;
		work.facts.facts = bank_facts(wallet, bank);
		constexpr size_t freeze_query =
			3 * economic_intent_freeze_fixed_source_query_frame_bytes();
		if (!currency_intent_child_entry(
			    work, economic_intent_freeze_fixed_source_frame_bytes,
			    economic_intent_freeze_fixed_source_supplement_frame_bytes,
			    economic_intent_freeze_fixed_initial_inline_bytes, freeze_query))
			return error::capacity;
		// Last fallible call: preserve child strong output. No callback after a
		// successful output transfer can turn success into failure.
		return economic_intent_freeze_fixed_bounded(command, work.facts, encoded,
							    currency_intent_workspace::forward,
							    &work, work.child_outer);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::capacity;
	}
}
}

bool economic_currency_intent_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !currency_intent_profile_policy())
		return false;
	size_t decode = 0, freeze = 0, derive = 0, total = currency_intent_owned_frames;
	if (!currency_command_decode_payload_source_frame_bytes(&decode) ||
	    !economic_intent_freeze_fixed_source_frame_bytes(&freeze) ||
	    !critical_operation_id_derive_source_frame_bytes(&derive) ||
	    !currency_intent_add(total, decode) || !currency_intent_add(total, freeze) ||
	    !currency_intent_add(total, derive))
		return false;
	*output = total;
	return true;
}
bool economic_currency_intent_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !currency_intent_profile_policy())
		return false;
	*output = sizeof(currency_intent_workspace);
	return true;
}

economic_accounting_error economic_bank_transfer_intent_bounded(
	const critical_command &command, const critical_operation_id &epoch,
	const economic_account_key &wallet, const economic_account_key &bank,
	std::vector<uint8_t> *encoded, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept
{
	return currency_intent_build_bounded(currency_intent_route::bank, command, epoch, wallet,
					     bank, encoded, reserve, context, outer_live);
}
economic_accounting_error economic_chaos_starter_bank_intent_bounded(
	const critical_command &command, const critical_operation_id &epoch,
	const economic_account_key &wallet, const economic_account_key &bank,
	std::vector<uint8_t> *encoded, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept
{
	return currency_intent_build_bounded(currency_intent_route::chaos, command, epoch, wallet,
					     bank, encoded, reserve, context, outer_live);
}
economic_accounting_error economic_quest_wallet_reward_intent_bounded(
	const critical_command &command, const critical_operation_id &epoch,
	const economic_account_key &wallet, const economic_account_key &bank,
	std::vector<uint8_t> *encoded, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept
{
	return currency_intent_build_bounded(currency_intent_route::quest, command, epoch, wallet,
					     bank, encoded, reserve, context, outer_live);
}
