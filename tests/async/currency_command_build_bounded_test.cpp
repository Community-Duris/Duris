// Private genuine-provider component regression; no original producer selected.
#include "economy/currency_command.h"
#include <openssl/opensslv.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>

#ifndef EXPECT_CURRENCY_BUILD_SUPPORTED
#error EXPECT_CURRENCY_BUILD_SUPPORTED must reflect the real whole-TU profile
#endif

namespace
{
using result = currency_command_bounded_result;
constexpr size_t limit_bytes = 32 * 1024 * 1024;
static_assert(currency_command_build_source_query_frame_bytes() == sizeof(size_t *) + sizeof(bool));
size_t cases = 0;
size_t denied_ordinals = 0;

bool same_key(const critical_entity_key &left, const critical_entity_key &right)
{
	return left.type == right.type && left.id == right.id;
}

bool same_command(const critical_command &left, const critical_command &right)
{
	if (left.schema_version != right.schema_version ||
	    left.operation_id.bytes != right.operation_id.bytes || left.type != right.type ||
	    left.payload_version != right.payload_version ||
	    left.source_site != right.source_site || left.deadline_class != right.deadline_class ||
	    left.accepted_at_usec != right.accepted_at_usec ||
	    left.publication_required != right.publication_required ||
	    left.payload != right.payload || left.accounting_intent != right.accounting_intent ||
	    left.keys.size() != right.keys.size() ||
	    left.expected_revisions.size() != right.expected_revisions.size())
		return false;
	for (size_t index = 0; index < left.keys.size(); ++index)
		if (!same_key(left.keys[index], right.keys[index]))
			return false;
	for (size_t index = 0; index < left.expected_revisions.size(); ++index)
		if (!same_key(left.expected_revisions[index].key,
			      right.expected_revisions[index].key) ||
		    left.expected_revisions[index].revision !=
			    right.expected_revisions[index].revision)
			return false;
	return true;
}

critical_operation_id operation()
{
	critical_operation_id value{};
	for (size_t index = 0; index < value.bytes.size(); ++index)
		value.bytes[index] = static_cast<uint8_t>(index + 1);
	return value;
}

critical_command previous()
{
	critical_command value{};
	value.schema_version = 2;
	value.operation_id = operation();
	value.type = critical_command_type::test;
	value.payload_version = 7;
	value.source_site = critical_source_site::recovery;
	value.deadline_class = critical_deadline_class::terminal;
	value.accepted_at_usec = 123456;
	value.publication_required = true;
	value.keys = { { critical_entity_type::player, 88 },
		       { critical_entity_type::account, 99 } };
	value.expected_revisions = { { { critical_entity_type::player, 88 }, 13 },
				     { { critical_entity_type::account, 99 }, 17 } };
	value.payload = { 9, 8, 7, 6 };
	value.accounting_intent = { 3, 2, 1 };
	value.keys.reserve(19);
	value.expected_revisions.reserve(23);
	value.payload.reserve(31);
	value.accounting_intent.reserve(37);
	return value;
}

struct snapshot
{
	std::array<unsigned char, sizeof(critical_command)> object;
	std::array<critical_entity_key, 2> keys;
	std::array<critical_expected_revision, 2> revisions;
	std::array<uint8_t, 4> payload;
	std::array<uint8_t, 3> intent;
	explicit snapshot(const critical_command &value)
	{
		assert(value.keys.size() == keys.size() &&
		       value.expected_revisions.size() == revisions.size());
		assert(value.payload.size() == payload.size() &&
		       value.accounting_intent.size() == intent.size());
		std::memcpy(object.data(), &value, sizeof(value));
		std::copy(value.keys.begin(), value.keys.end(), keys.begin());
		std::copy(value.expected_revisions.begin(), value.expected_revisions.end(),
			  revisions.begin());
		std::copy(value.payload.begin(), value.payload.end(), payload.begin());
		std::copy(value.accounting_intent.begin(), value.accounting_intent.end(),
			  intent.begin());
	}
	bool unchanged(const critical_command &value) const
	{
		if (std::memcmp(object.data(), &value, sizeof(value)) != 0)
			return false;
		for (size_t index = 0; index < keys.size(); ++index)
			if (!same_key(keys[index], value.keys[index]) ||
			    !same_key(revisions[index].key, value.expected_revisions[index].key) ||
			    revisions[index].revision != value.expected_revisions[index].revision)
				return false;
		return std::equal(payload.begin(), payload.end(), value.payload.begin()) &&
		       std::equal(intent.begin(), intent.end(), value.accounting_intent.begin());
	}
};

struct budget
{
	size_t limit = limit_bytes;
	size_t outer = 0;
	size_t deny = 0;
	size_t calls = 0;
	size_t peak = 0;
	bool entered = false;
	const critical_command *old = nullptr;
	const snapshot *saved = nullptr;
	std::array<size_t, 256> requests{};
	static bool reserve(size_t need, void *opaque) noexcept
	{
		auto &self = *static_cast<budget *>(opaque);
		assert(self.old && self.saved && self.saved->unchanged(*self.old));
		assert(need >= self.outer);
		self.peak = std::max(self.peak, need);
		if (self.entered)
		{
			if (self.calls == self.requests.size())
				return false;
			self.requests[self.calls++] = need;
			if (self.deny && self.calls == self.deny)
				return false;
		}
		return need <= self.limit;
	}
};

size_t heap(const critical_command &value)
{
	return value.keys.capacity() * sizeof(critical_entity_key) +
	       value.expected_revisions.capacity() * sizeof(critical_expected_revision) +
	       value.payload.capacity() + value.accounting_intent.capacity();
}

struct profile
{
	size_t full = 0;
	size_t supplement = 0;
	size_t initial = 0;
};

profile query(budget &owner)
{
	profile value;
	assert(budget::reserve(owner.outer + currency_command_build_source_query_frame_bytes(),
			       &owner));
	assert(currency_command_build_source_frame_bytes(&value.full));
	assert(budget::reserve(owner.outer + currency_command_build_source_query_frame_bytes(),
			       &owner));
	assert(currency_command_build_source_supplement_frame_bytes(&value.supplement));
	assert(budget::reserve(owner.outer + currency_command_build_source_query_frame_bytes(),
			       &owner));
	assert(currency_command_build_initial_inline_bytes(&value.initial));
	assert(value.supplement == 0);
	assert(value.initial == sizeof(critical_command) + sizeof(std::vector<uint8_t>) +
					2 * sizeof(critical_entity_key));
	assert(value.full > 0 && value.full < owner.limit);
	return value;
}

currency_command_payload payload(currency_reason_type reason)
{
	currency_command_payload value{};
	value.pid = 42;
	value.racewar = 1;
	value.reason = reason;
	value.reason_id = -123;
	std::strcpy(value.account_name.data(), "MiXeD-fixture");
	value.wallet_delta.amount = { -7, -3, -1, 0 };
	value.bank_delta.amount = { 7, 3, 1, 0 };
	return value;
}

result invoke(critical_command &output, const critical_operation_id &id,
	      const currency_command_payload &input, budget &owner, const snapshot &saved,
	      size_t enclosing_live = 0)
{
	owner.old = &output;
	owner.saved = &saved;
	// Fixed observer/snapshot and authentic input/output capacities are retained.
	// This fixture boundary is not a production caller Source/ABI qualification.
	owner.outer = sizeof(input) + sizeof(id) + sizeof(output) + heap(output) + sizeof(owner) +
		      sizeof(saved) + sizeof(profile) + sizeof(result) + enclosing_live;
	const auto metadata = query(owner);
	assert(metadata.full <= SIZE_MAX - owner.outer);
	assert(metadata.initial <= SIZE_MAX - owner.outer - metadata.full);
	assert(budget::reserve(owner.outer + metadata.full + metadata.initial, &owner));
	owner.entered = true;
	const auto outcome = currency_command_build_bounded_status(
		&output, id, input, UINT64_MAX, 9, critical_source_site::command,
		critical_deadline_class::interactive, budget::reserve, &owner,
		owner.outer + metadata.supplement);
	owner.entered = false;
	return outcome;
}

size_t equivalent(const currency_command_payload &input)
{
	auto original = previous();
	auto bounded = previous();
	const snapshot saved(bounded);
	const auto id = operation();
	assert(currency_command_build(&original, id, input, UINT64_MAX, 9,
				      critical_source_site::command,
				      critical_deadline_class::interactive));
	budget owner;
	assert(invoke(bounded, id, input, owner, saved, sizeof(original) + heap(original)) ==
	       result::ok);
	assert(owner.calls > 0 && owner.peak <= owner.limit);
	assert(same_command(original, bounded));
	assert(bounded.payload.size() == 136);
	assert(std::all_of(bounded.payload.end() - 4, bounded.payload.end(),
			   [](uint8_t byte) { return byte == 0; }));
	assert(bounded.keys.size() == 2 && bounded.expected_revisions.size() == 2);
	assert(bounded.keys[0].type == critical_entity_type::player &&
	       bounded.keys[0].id == input.pid);
	assert(bounded.expected_revisions[0].revision == UINT64_MAX &&
	       bounded.expected_revisions[1].revision == 9);
	critical_entity_key canonical{};
	assert(currency_account_key(input.account_name.data(), input.racewar, &canonical));
	assert(same_key(canonical, bounded.keys[1]));
	assert(bounded.schema_version == 1 && bounded.accounting_intent.empty() &&
	       !bounded.publication_required && bounded.accepted_at_usec == 0);
	++cases;
	return owner.calls;
}

void refused(const currency_command_payload &input, critical_operation_id id, result expected)
{
	auto output = previous();
	const snapshot saved(output);
	budget owner;
	assert(invoke(output, id, input, owner, saved) == expected);
	assert(saved.unchanged(output));
	++cases;
}

[[maybe_unused]] void supported()
{
	assert(!currency_command_build_source_frame_bytes(nullptr));
	assert(!currency_command_build_source_supplement_frame_bytes(nullptr));
	assert(!currency_command_build_initial_inline_bytes(nullptr));
	for (uint16_t reason = 1;
	     reason <= static_cast<uint16_t>(currency_reason_type::corpse_lifecycle); ++reason)
		equivalent(payload(static_cast<currency_reason_type>(reason)));
	auto longest = payload(currency_reason_type::atm_deposit);
	longest.account_name.fill('A');
	longest.account_name.back() = 0;
	longest.racewar = 255;
	const size_t ordinals = equivalent(longest);
	for (size_t ordinal = 1; ordinal <= ordinals; ++ordinal)
	{
		auto output = previous();
		const snapshot saved(output);
		budget owner;
		owner.deny = ordinal;
		assert(invoke(output, operation(), longest, owner, saved) == result::capacity);
		assert(saved.unchanged(output));
		assert(owner.calls >= ordinal);
		++denied_ordinals;
		++cases;
	}
	auto extremes = payload(currency_reason_type::wallet_reward);
	extremes.reason_id = INT64_MIN;
	extremes.wallet_delta.amount = { INT64_MAX, -INT64_MAX, 0, 1 };
	extremes.bank_delta.amount = { -INT64_MAX, INT64_MAX, 0, -1 };
	equivalent(extremes);
	auto zero = payload(currency_reason_type::corpse_lifecycle);
	zero.wallet_delta.amount = {};
	zero.bank_delta.amount = {};
	equivalent(zero);
	zero.reason = currency_reason_type::atm_deposit;
	refused(zero, operation(), result::invalid);
	refused(payload(currency_reason_type::atm_deposit), {}, result::invalid);
	for (int fault = 0; fault < 8; ++fault)
	{
		auto bad = longest;
		if (fault == 0)
			bad.pid = 0;
		if (fault == 1)
			bad.reason = currency_reason_type::unknown;
		if (fault == 2)
			bad.reason = static_cast<currency_reason_type>(UINT16_MAX);
		if (fault == 3)
			bad.account_name.fill(0);
		if (fault == 4)
			bad.account_name.fill('x');
		if (fault == 5)
			bad.account_name[0] = '\x1f';
		if (fault == 6)
			bad.wallet_delta.amount[1] = INT64_MIN;
		if (fault == 7)
			bad.bank_delta.amount[1] = INT64_MIN;
		refused(bad, operation(), result::invalid);
	}
	// Defensive provider checks deliberately use malformed entry arguments;
	// they do not claim an admitted original producer can carry SIZE_MAX outer.
	for (int fault = 0; fault < 4; ++fault)
	{
		auto output = previous();
		const snapshot saved(output);
		budget owner;
		owner.old = &output;
		owner.saved = &saved;
		owner.entered = true;
		if (fault == 2)
			owner.limit = 1;
		const auto id = operation();
		const auto input = longest;
		const auto outcome = currency_command_build_bounded_status(
			fault == 3 ? nullptr : &output, id, input, 4, 9,
			critical_source_site::command, critical_deadline_class::interactive,
			fault == 1 ? nullptr : budget::reserve, &owner, fault == 0 ? SIZE_MAX : 0);
		assert(outcome == (fault == 3 ? result::invalid : result::capacity));
		assert(saved.unchanged(output));
		assert(owner.calls == (fault < 2 ? 0 : 1));
		++cases;
	}
}

[[maybe_unused]] void unsupported()
{
	for (auto getter : { currency_command_build_source_frame_bytes,
			     currency_command_build_source_supplement_frame_bytes,
			     currency_command_build_initial_inline_bytes })
	{
		size_t sentinel = 987654;
		assert(!getter(&sentinel) && sentinel == 987654);
		assert(!getter(nullptr));
		++cases;
	}
	auto output = previous();
	const snapshot saved(output);
	budget owner;
	owner.old = &output;
	owner.saved = &saved;
	owner.entered = true;
	assert(currency_command_build_bounded_status(
		       &output, operation(), payload(currency_reason_type::atm_deposit), 4, 9,
		       critical_source_site::command, critical_deadline_class::interactive,
		       budget::reserve, &owner, 0) == result::capacity);
	assert(saved.unchanged(output) && owner.calls == 0);
	++cases;
}
}

int main()
{
	if constexpr (EXPECT_CURRENCY_BUILD_SUPPORTED)
		supported();
	else
		unsupported();
	std::cout << "CURRENCY_BUILD_COMPONENT {\"supported\":"
		  << (EXPECT_CURRENCY_BUILD_SUPPORTED ? "true" : "false") << ",\"cases\":" << cases
		  << ",\"denied_ordinals\":" << denied_ordinals << ",\"wire_bytes\":136}"
		  << std::endl;
}
