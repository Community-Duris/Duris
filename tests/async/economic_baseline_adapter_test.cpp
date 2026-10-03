#include "economy/economic_baseline_adapter.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <new>
#include <type_traits>

using error = economic_accounting_error;
#include "baseline_reference.inc"
size_t allocation_target = 0, allocation_seen = 0;
extern "C" void *__real__Znwm(size_t);
extern "C" void *__real__Znam(size_t);
extern "C" void *__wrap__Znwm(size_t size)
{
	if (allocation_target && ++allocation_seen == allocation_target)
		throw std::bad_alloc();
	return __real__Znwm(size);
}
extern "C" void *__wrap__Znam(size_t size)
{
	if (allocation_target && ++allocation_seen == allocation_target)
		throw std::bad_alloc();
	return __real__Znam(size);
}
critical_operation_id id(uint8_t n)
{
	critical_operation_id result = {};
	result.bytes[0] = n;
	return result;
}
economic_digest digest(uint8_t n)
{
	economic_digest result = {};
	result[0] = n;
	return result;
}
economic_baseline_batch fixture()
{
	economic_baseline_batch result;
	result.lineage = id(1);
	result.epoch = id(2);
	result.preparation_id = id(3);
	result.actor_id = 7;
	result.batch_index = 8;
	result.opening_account = { id(1), economic_account_kind::opening, 9001, 0 };
	result.boundary_digest = digest(11);
	result.coverage_digest = digest(12);
	for (uint64_t n = 1; n <= 6; ++n)
		result.holdings.push_back({ { id(1), static_cast<economic_account_kind>(n),
					      1000 + n, n == 2 ? 1u : 0u },
					    { static_cast<int64_t>(n), 2, 3, 4 },
					    n - 1,
					    digest(20 + n) });
	result.items = {
		{ { 1, { { item_owner_type::player, 7, 0 }, 1, 0, 0, item_custody_state::active } },
		  digest(31) },
		{ { 2,
		    { { item_owner_type::player, 7, 0 }, 1, 1, 8, item_custody_state::quarantined } },
		  digest(31) },
		{ { 3,
		    { { item_owner_type::destruction, 0, 0 },
		      1,
		      1,
		      9,
		      item_custody_state::destroyed } },
		  digest(32) }
	};
	return result;
}
std::optional<economic_prepared_baseline> prepare(const economic_baseline_batch &value)
{
	std::optional<economic_prepared_baseline> result;
	assert(economic_baseline_prepare(value, &result) == error::ok && result);
	return result;
}
void rejected(const economic_baseline_batch &value, error expected)
{
	auto output = prepare(fixture());
	const auto before = output->encoded_plan();
	assert(economic_baseline_prepare(value, &output) == expected);
	assert(output->encoded_plan() == before);
}
#include "economic_baseline_command_test.h"
#include "economic_baseline_codec_test.h"

int main(int argc, char **argv)
{
	assert(argc == 2);
	static_assert(!std::is_default_constructible_v<economic_prepared_baseline>);
	static_assert(!std::is_aggregate_v<economic_prepared_baseline>);
	auto input = fixture();
	auto output = prepare(input);
	const auto &plan = output->plan();
	assert(plan.metadata.intent_digest == REFERENCE_INTENT);
	assert(plan.metadata.domain_digest == REFERENCE_DOMAIN);
	assert(plan.accounts.size() == 7 && plan.postings.size() == 12);
	assert(plan.children.empty() && plan.item_events.empty());
	assert(plan.metadata.writer_id == 4 && plan.metadata.reason == economic_reason::baseline);
	critical_operation_id expected;
	assert(critical_operation_id_derive(input.preparation_id, 0x42415345, 8, &expected));
	assert(expected.bytes == plan.metadata.operation_id.bytes);
	assert(plan.metadata.source_event->source.bytes == input.preparation_id.bytes);
	for (size_t n = 0; n < 6; ++n)
	{
		assert(plan.accounts[n].before == economic_coin_vector{});
		assert(plan.accounts[n].after == input.holdings[n].balance);
		assert(plan.accounts[n].before_revision == 0 &&
		       plan.accounts[n].after_revision == 1);
		assert(output->witness().holdings[n].native_revision == n);
		assert(plan.postings[n].copper == 4321 + static_cast<int64_t>(n));
		assert(plan.postings[n + 6].copper == -plan.postings[n].copper);
		for (size_t d = 0; d < 4; ++d)
			assert(plan.postings[n + 6].delta[d] == -plan.postings[n].delta[d]);
	}
	for (size_t n = 0; n < input.items.size(); ++n)
	{
		assert(plan.items_before[n].uid == input.items[n].snapshot.uid);
		assert(economic_item_position_equal(plan.items_before[n].position,
						    input.items[n].snapshot.position));
		assert(economic_item_position_equal(plan.items_before[n].position,
						    plan.items_after[n].position));
	}
	economic_accounting_plan decoded;
	assert(economic_plan_decode(output->encoded_plan(), &decoded) == error::ok);
	std::vector<uint8_t> encoded;
	assert(economic_plan_encode(decoded, &encoded) == error::ok &&
	       encoded == output->encoded_plan());
	auto shuffled = input;
	std::reverse(shuffled.holdings.begin(), shuffled.holdings.end());
	std::reverse(shuffled.items.begin(), shuffled.items.end());
	assert(prepare(shuffled)->encoded_plan() == encoded);
	// Observe the maximum native revision without advancing or wrapping it.
	auto changed = input;
	changed.holdings[0].native_revision = UINT64_MAX;
	auto alternate = prepare(changed);
	assert(alternate->witness().holdings[0].native_revision == UINT64_MAX);
	assert(alternate->plan().metadata.operation_id.bytes == expected.bytes);
	assert(alternate->plan().metadata.domain_digest != plan.metadata.domain_digest);
	changed = input;
	changed.boundary_digest[0]++;
	assert(prepare(changed)->plan().metadata.intent_digest != plan.metadata.intent_digest);
	changed = input;
	changed.coverage_digest[0]++;
	assert(prepare(changed)->plan().metadata.intent_digest != plan.metadata.intent_digest);
	changed = input;
	changed.holdings[0].source_digest[0]++;
	assert(prepare(changed)->plan().metadata.domain_digest != plan.metadata.domain_digest);
	changed = input;
	changed.items[0].source_digest[0]++;
	assert(prepare(changed)->plan().metadata.domain_digest != plan.metadata.domain_digest);
	changed = input;
	changed.batch_index++;
	assert(prepare(changed)->plan().metadata.operation_id.bytes != expected.bytes);
	changed = input;
	changed.holdings.push_back(input.holdings[1]);
	rejected(changed, error::duplicate_event);
	changed = input;
	changed.holdings[1].account.authority_id = changed.holdings[0].account.authority_id;
	rejected(changed, error::duplicate_event);
	changed = input;
	changed.holdings[0].balance[0] = -1;
	rejected(changed, error::negative_holding);
	changed = input;
	changed.holdings[0].balance[3] = INT64_MAX;
	rejected(changed, error::overflow);
	changed = input;
	changed.holdings[0].source_digest = {};
	rejected(changed, error::invalid_identity);
	changed = input;
	changed.boundary_digest = {};
	rejected(changed, error::invalid_identity);
	changed = input;
	changed.coverage_digest = {};
	rejected(changed, error::invalid_identity);
	changed = input;
	changed.holdings[0].account.lineage = id(19);
	rejected(changed, error::invalid_identity);
	changed = input;
	changed.holdings[0].account.kind = economic_account_kind::issuance;
	rejected(changed, error::invalid_identity);
	changed = input;
	changed.opening_account.kind = economic_account_kind::sink;
	rejected(changed, error::invalid_identity);
	changed = input;
	changed.items[1].snapshot.position.parent_uid = 99;
	rejected(changed, error::topology);
	changed = input;
	changed.items.push_back(changed.items[0]);
	rejected(changed, error::invalid_identity);
	changed = input;
	changed.items[0].snapshot.position = {};
	rejected(changed, error::invalid_identity);
	changed = input;
	changed.items[0].source_digest = {};
	rejected(changed, error::invalid_identity);
	changed = input;
	changed.holdings.clear();
	changed.items.clear();
	assert(prepare(changed)->plan().accounts.empty());
	changed.holdings.push_back(input.holdings[0]);
	changed.holdings[0].balance = {};
	auto empty = prepare(changed);
	assert(empty->plan().accounts.size() == 1 && empty->plan().postings.empty());
	changed = input;
	changed.items.clear();
	changed.holdings.clear();
	for (size_t n = 0; n < ECONOMIC_BASELINE_MAX_HOLDINGS; ++n)
		changed.holdings.push_back({ { id(1), economic_account_kind::bank, n + 1, n % 2 },
					     { INT64_MAX, 0, 0, 0 },
					     UINT64_MAX,
					     digest(1) });
	assert(prepare(changed)->plan().postings.size() == 2 * ECONOMIC_BASELINE_MAX_HOLDINGS);
	changed.holdings.push_back(input.holdings[0]);
	rejected(changed, error::capacity);
	changed = input;
	changed.holdings.clear();
	changed.items.clear();
	for (uint64_t n = 1; n <= ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES; ++n)
		changed.items.push_back({ { n,
					    { { item_owner_type::player, 7, 0 },
					      1,
					      n - 1,
					      0,
					      item_custody_state::active } },
					  digest(1) });
	assert(prepare(changed)->plan().items_before.size() ==
	       ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES);
	changed.items.push_back(input.items[0]);
	rejected(changed, error::capacity);
	const auto retained = output->encoded_plan();
	size_t failed = 0;
	for (size_t target = 1; target < 1024; ++target)
	{
		allocation_seen = 0;
		allocation_target = target;
		const auto status = economic_baseline_prepare(input, &output);
		allocation_target = 0;
		assert(output->encoded_plan() == retained);
		if (status == error::ok)
			break;
		assert(status == error::capacity);
		++failed;
	}
	assert(failed > 20 && failed < 1023);
	codec_tests();
	command_tests(argv[1]);
	std::cout
		<< "baseline preparation: exact openings, zero/empty holdings, unchanged custody, source binding, limits and "
		<< failed << " allocation failures passed\n";
}
