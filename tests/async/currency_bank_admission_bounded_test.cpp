// Genuine immutable bank-admission component; fresh ATM authority remains open.
#include "economy/economic_command_admission.h"
#include <openssl/opensslv.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>

#ifndef EXPECT_BANK_ADMISSION_SUPPORTED
#error EXPECT_BANK_ADMISSION_SUPPORTED must reflect the real production profile
#endif

namespace
{
using error = economic_accounting_error;
constexpr size_t limit_bytes = 32 * 1024 * 1024;
size_t cases = 0, denied_ordinals = 0, refreshed_after_refusal = 0;

critical_operation_id id(uint8_t first)
{
	critical_operation_id value{};
	value.bytes[0] = first;
	return value;
}

size_t heap(const critical_command &value)
{
	return value.keys.capacity() * sizeof(critical_entity_key) +
	       value.expected_revisions.capacity() * sizeof(critical_expected_revision) +
	       value.payload.capacity() + value.accounting_intent.capacity();
}

bool same_key(const critical_entity_key &left, const critical_entity_key &right)
{
	return left.type == right.type && left.id == right.id;
}

struct snapshot
{
	std::array<unsigned char, sizeof(critical_command)> object;
	critical_command copy;
	explicit snapshot(const critical_command &value)
		: copy(value)
	{
		std::memcpy(object.data(), &value, sizeof(value));
	}
	bool unchanged(const critical_command &value) const
	{
		if (std::memcmp(object.data(), &value, sizeof(value)) != 0 ||
		    copy.keys.size() != value.keys.size() ||
		    copy.expected_revisions.size() != value.expected_revisions.size() ||
		    copy.payload != value.payload ||
		    copy.accounting_intent != value.accounting_intent)
			return false;
		for (size_t index = 0; index < copy.keys.size(); ++index)
			if (!same_key(copy.keys[index], value.keys[index]))
				return false;
		for (size_t index = 0; index < copy.expected_revisions.size(); ++index)
			if (!same_key(copy.expected_revisions[index].key,
				      value.expected_revisions[index].key) ||
			    copy.expected_revisions[index].revision !=
				    value.expected_revisions[index].revision)
				return false;
		return true;
	}
};

struct profile
{
	size_t full = 0, initial = 0, supplement = 0, owned = 0;
};

struct budget
{
	size_t limit = limit_bytes, outer = 0, deny = 0, calls = 0, peak = 0;
	size_t refused_at = 0, refusal_hits = 0, refresh_callbacks = 0;
	bool entered = false;
	const critical_command *input = nullptr;
	const snapshot *saved = nullptr;
	std::array<size_t, 1024> requests{};
	profile metadata;
	static bool reserve(size_t need, void *opaque) noexcept
	{
		auto &owner = *static_cast<budget *>(opaque);
		assert(owner.input && owner.saved && owner.saved->unchanged(*owner.input));
		assert(need >= owner.outer);
		owner.peak = std::max(owner.peak, need);
		if (owner.entered)
		{
			if (owner.calls == owner.requests.size())
				return false;
			owner.requests[owner.calls++] = need;
			if (owner.deny && owner.calls == owner.deny)
			{
				owner.refused_at = owner.calls;
				++owner.refusal_hits;
				return false;
			}
			if (owner.refusal_hits)
				++owner.refresh_callbacks;
		}
		return need <= owner.limit;
	}
};

bool checked_add(size_t &total, size_t extra)
{
	if (extra > SIZE_MAX - total)
		return false;
	total += extra;
	return true;
}

bool invoke(const critical_command &command, bool flat, budget &owner, const snapshot &saved,
	    size_t enclosing_live = 0)
{
	owner.input = &command;
	owner.saved = &saved;
	// Fixed fixture query scalars remain caller-owned; this is a component
	// observer boundary, not a production caller Source/ABI qualification.
	owner.outer = sizeof(command) + heap(command) + sizeof(owner) + sizeof(saved) +
		      heap(saved.copy) + 7 * sizeof(size_t) + sizeof(bool);
	if (!checked_add(owner.outer, enclosing_live))
		return false;
	size_t query = owner.outer;
	if (!checked_add(query, economic_bank_command_admission_source_query_frame_bytes()) ||
	    !budget::reserve(query, &owner))
		return false;
	assert(economic_bank_command_admission_source_frame_bytes(&owner.metadata.full));
	assert(budget::reserve(query, &owner));
	assert(economic_bank_command_admission_initial_inline_bytes(&owner.metadata.initial));
	assert(budget::reserve(query, &owner));
	assert(economic_bank_command_admission_source_supplement_frame_bytes(
		&owner.metadata.supplement));
	assert(owner.metadata.supplement == 0 && owner.metadata.initial > 0);
	size_t child[4]{};
	// The full parent query already bounds the same four genuine child query laws.
	assert(budget::reserve(query, &owner));
	assert(economic_intent_decode_source_frame_bytes(&child[0]));
	assert(currency_command_decode_payload_source_frame_bytes(&child[1]));
	assert(currency_command_build_source_frame_bytes(&child[2]));
	assert(economic_currency_intent_source_frame_bytes(&child[3]));
	size_t sum = 0;
	for (auto value : child)
		assert(checked_add(sum, value));
	assert(owner.metadata.full > sum);
	owner.metadata.owned = owner.metadata.full - sum;
	size_t entry = owner.outer;
	if (!checked_add(entry, owner.metadata.full) ||
	    !checked_add(entry, owner.metadata.initial) || !budget::reserve(entry, &owner))
		return false;
	owner.entered = true;
	const bool result = flat ? economic_flatfile_bank_command_admission_supported_bounded(
					   command, budget::reserve, &owner,
					   owner.outer + owner.metadata.supplement) :
				   economic_bank_command_admission_supported_bounded(
					   command, budget::reserve, &owner,
					   owner.outer + owner.metadata.supplement);
	owner.entered = false;
	assert(saved.unchanged(command));
	return result;
}

error original_intent(const critical_command &command, currency_reason_type reason,
		      std::vector<uint8_t> *encoded)
{
	const economic_account_key wallet{ id(1), economic_account_kind::wallet, 1001, 0 };
	const economic_account_key bank{ id(1), economic_account_kind::bank, 2001, 1 };
	if (reason == currency_reason_type::chaos_starter_reward)
		return economic_chaos_starter_bank_intent(command, id(2), wallet, bank, encoded);
	if (reason == currency_reason_type::wallet_reward)
		return economic_quest_wallet_reward_intent(command, id(2), wallet, bank, encoded);
	return economic_bank_transfer_intent(command, id(2), wallet, bank, encoded);
}

critical_command command(currency_reason_type reason, uint8_t opaque = 0)
{
	currency_command_payload payload{};
	payload.pid = 7;
	payload.racewar = 1;
	payload.reason = reason;
	std::strcpy(payload.account_name.data(), "MiXeD-bank-fixture");
	payload.wallet_delta.amount[0] = reason == currency_reason_type::atm_withdraw ? 7 : -7;
	payload.bank_delta.amount[0] = -payload.wallet_delta.amount[0];
	auto operation = id(3);
	auto source = critical_source_site::command;
	auto deadline = critical_deadline_class::interactive;
	if (reason == currency_reason_type::wallet_reward)
	{
		payload.reason_id = 99;
		payload.wallet_delta.amount = { 9, 0, 0, 0 };
		payload.bank_delta.amount = {};
		source = critical_source_site::recovery;
		deadline = critical_deadline_class::recovery;
	}
	if (reason == currency_reason_type::chaos_starter_reward)
	{
		payload.reason_id = payload.pid;
		payload.wallet_delta.amount = {};
		payload.bank_delta.amount = { 0, 0, 0, 1000000 };
		source = critical_source_site::login;
		deadline = critical_deadline_class::recovery;
		critical_operation_id seed{};
		std::memcpy(seed.bytes.data(), "CHAOSEED", 8);
		for (size_t byte = 0; byte < 8; ++byte)
			seed.bytes[8 + byte] =
				static_cast<uint8_t>(uint64_t(payload.pid) >> (8 * byte));
		assert(critical_operation_id_derive(seed, 0x43484250, 1, &operation));
	}
	critical_command value;
	assert(currency_command_build(&value, operation, payload, UINT64_MAX, 9, source, deadline));
	assert(value.payload.size() == 136);
	value.payload[5] = opaque;
	value.accepted_at_usec = 123456;
	assert(original_intent(value, reason, &value.accounting_intent) == error::ok);
	value.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	value.keys.reserve(19);
	value.expected_revisions.reserve(23);
	value.payload.reserve(251);
	value.accounting_intent.reserve(1021);
	return value;
}

size_t accepted(const critical_command &input, size_t enclosing_live = 0)
{
	const snapshot saved(input);
	size_t calls = 0;
	for (bool flat : { false, true })
	{
		budget owner;
		assert(invoke(input, flat, owner, saved, enclosing_live));
		assert(owner.calls >= 4 && owner.peak <= limit_bytes);
		assert(owner.requests[0] == owner.outer + owner.metadata.owned);
		assert(owner.requests[1] ==
		       owner.outer + owner.metadata.full + owner.metadata.initial);
		assert(owner.requests[2] ==
		       owner.outer + owner.metadata.owned + owner.metadata.initial +
			       3 * currency_command_decode_payload_source_query_frame_bytes());
		++cases;
		calls = owner.calls;
	}
	return calls;
}

void rejected(const critical_command &input, size_t enclosing_live = 0)
{
	const snapshot saved(input);
	for (bool flat : { false, true })
	{
		budget owner;
		assert(!invoke(input, flat, owner, saved, enclosing_live));
		assert(saved.unchanged(input));
		++cases;
	}
}

void refreeze(critical_command &value)
{
	economic_frozen_intent previous;
	assert(economic_intent_decode(value.accounting_intent, &previous) == error::ok);
	value.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	value.accounting_intent.clear();
	value.publication_required = false;
	assert(economic_intent_freeze(value, previous.admission, &value.accounting_intent) ==
	       error::ok);
	value.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
}

void unsupported(const critical_command &input)
{
	const snapshot saved(input);
	for (auto getter : { economic_bank_command_admission_source_frame_bytes,
			     economic_bank_command_admission_initial_inline_bytes,
			     economic_bank_command_admission_source_supplement_frame_bytes })
	{
		size_t value = 123;
		assert(!getter(&value) && value == 123 && !getter(nullptr));
	}
	for (bool flat : { false, true })
	{
		budget owner;
		owner.input = &input;
		owner.saved = &saved;
		owner.outer = sizeof(input) + heap(input) + sizeof(owner) + sizeof(saved) +
			      heap(saved.copy);
		const bool result =
			flat ? economic_flatfile_bank_command_admission_supported_bounded(
				       input, budget::reserve, &owner, owner.outer) :
			       economic_bank_command_admission_supported_bounded(
				       input, budget::reserve, &owner, owner.outer);
		assert(!result && saved.unchanged(input));
		++cases;
	}
}
} // namespace

int main()
{
	const auto normal = command(currency_reason_type::atm_deposit);
	const size_t normal_live = sizeof(normal) + heap(normal);
	if (!EXPECT_BANK_ADMISSION_SUPPORTED)
	{
		unsupported(normal);
		std::cout << "BANK_ADMISSION_COMPONENT {\"supported\":false,\"cases\":" << cases
			  << ",\"denied_ordinals\":0,\"wire_bytes\":136}\n";
		return 0;
	}
	for (auto getter : { economic_bank_command_admission_source_frame_bytes,
			     economic_bank_command_admission_initial_inline_bytes,
			     economic_bank_command_admission_source_supplement_frame_bytes })
		assert(!getter(nullptr));
	for (auto reason :
	     { currency_reason_type::atm_deposit, currency_reason_type::atm_withdraw,
	       currency_reason_type::chaos_starter_reward, currency_reason_type::wallet_reward })
		for (uint8_t byte : { uint8_t(0), uint8_t(0xA5) })
			accepted(command(reason, byte), normal_live);
	{
		auto changed_timestamp = normal;
		changed_timestamp.accepted_at_usec += 987;
		accepted(changed_timestamp,
			 normal_live); // Original binding excludes transport timestamp.
	}
	for (auto reason :
	     { currency_reason_type::wallet_reward, currency_reason_type::chaos_starter_reward })
		for (unsigned variant = 0; variant < 3; ++variant)
		{
			auto bad = command(reason);
			if (variant == 0)
				bad.source_site = critical_source_site::command;
			if (variant == 1)
				bad.deadline_class = critical_deadline_class::interactive;
			if (variant == 2)
				std::fill(bad.payload.begin() + 8, bad.payload.begin() + 16, 0);
			refreeze(bad);
			rejected(bad, normal_live);
		}
	for (size_t offset : { size_t(0), size_t(4), size_t(5), size_t(6), size_t(16), size_t(17),
			       size_t(67), size_t(68), size_t(100), size_t(132), size_t(135) })
	{
		auto bad = normal;
		bad.payload[offset] ^= 1;
		rejected(bad, normal_live);
	}
	for (size_t offset : { size_t(12), size_t(16), size_t(64), size_t(160), size_t(224) })
	{
		auto bad = normal;
		bad.accounting_intent[offset] ^= 1;
		rejected(bad, normal_live);
	}
	for (size_t length : { size_t(0), size_t(135), size_t(137) })
	{
		auto bad = normal;
		bad.payload.resize(length);
		rejected(bad, normal_live);
	}
	for (size_t length : { size_t(0), size_t(255), size_t(279) })
	{
		auto bad = normal;
		bad.accounting_intent.resize(length);
		rejected(bad, normal_live);
	}
	for (unsigned variant = 0; variant < 6; ++variant)
	{
		auto bad = normal;
		if (variant == 0)
			bad.expected_revisions[0].revision++;
		if (variant == 1)
			std::swap(bad.keys[0], bad.keys[1]);
		if (variant == 2)
			bad.expected_revisions.pop_back();
		if (variant == 3)
			bad.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		if (variant == 4)
			bad.type = critical_command_type::test;
		if (variant == 5)
			bad.operation_id = {};
		rejected(bad, normal_live);
	}
	// Reason-selected original intent rejects each unrelated currency reason even
	// when genuine structural freeze creates a well-formed matching command binding.
	{
		economic_frozen_intent frozen;
		assert(economic_intent_decode(normal.accounting_intent, &frozen) == error::ok);
		const size_t witnesses =
			normal_live + sizeof(frozen) + frozen.admission.facts.capacity();
		for (uint16_t reason = 3;
		     reason <= static_cast<uint16_t>(currency_reason_type::corpse_lifecycle);
		     ++reason)
		{
			if (reason == static_cast<uint16_t>(currency_reason_type::wallet_reward) ||
			    reason == static_cast<uint16_t>(
					      currency_reason_type::chaos_starter_reward))
				continue;
			auto bad = normal;
			bad.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
			bad.accounting_intent.clear();
			bad.payload[6] = static_cast<uint8_t>(reason);
			bad.payload[7] = 0;
			assert(economic_intent_freeze(bad, frozen.admission,
						      &bad.accounting_intent) == error::ok);
			bad.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
			rejected(bad, witnesses);
		}
	}
	for (bool flat : { false, true })
	{
		const snapshot saved(normal);
		{
			budget baseline;
			assert(invoke(normal, flat, baseline, saved));
			assert(baseline.calls <= baseline.requests.size());
			for (size_t ordinal = 1; ordinal <= baseline.calls; ++ordinal)
			{
				budget denial;
				denial.deny = ordinal;
				assert(!invoke(normal, flat, denial, saved, sizeof(baseline)));
				// Original bounded codecs reserve again while unwinding after
				// refusal. Keep those genuine cleanup requests observable;
				// the exact injected ordinal must still be reached once.
				assert(denial.refused_at == ordinal && denial.refusal_hits == 1);
				assert(denial.calls >= ordinal &&
				       denial.refresh_callbacks == denial.calls - ordinal);
				assert(saved.unchanged(normal));
				refreshed_after_refusal += denial.refresh_callbacks;
				++cases;
				++denied_ordinals;
			}
		}
		{
			budget capacity;
			capacity.limit = 1;
			assert(!invoke(normal, flat, capacity, saved) && capacity.calls == 0);
			++cases;
		}
		budget overflow;
		overflow.input = &normal;
		overflow.saved = &saved;
		overflow.outer = SIZE_MAX;
		assert(!economic_bank_command_admission_supported_bounded(normal, budget::reserve,
									  &overflow, SIZE_MAX));
		assert(!economic_flatfile_bank_command_admission_supported_bounded(
			normal, budget::reserve, &overflow, SIZE_MAX));
		assert(saved.unchanged(normal));
		++cases;
	}
	assert(!economic_bank_command_admission_supported_bounded(normal, nullptr, nullptr, 0));
	assert(!economic_flatfile_bank_command_admission_supported_bounded(normal, nullptr, nullptr,
									   0));
	cases += 2;
	std::cout << "BANK_ADMISSION_COMPONENT {\"supported\":true,\"cases\":" << cases
		  << ",\"denied_ordinals\":" << denied_ordinals
		  << ",\"refresh_callbacks\":" << refreshed_after_refusal
		  << ",\"wire_bytes\":136}\n";
}
