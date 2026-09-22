#include "economy/economic_baseline_adapter.h"
#include <algorithm>
#include <new>
#include <openssl/sha.h>
#include <set>
#include <utility>

namespace
{
using error = economic_accounting_error;
bool zero(std::span<const uint8_t> value)
{
	return std::all_of(value.begin(), value.end(), [](auto byte) { return byte == 0; });
}
struct encoding
{
	std::vector<uint8_t> bytes;
	void integer(uint64_t value, size_t width = 8)
	{
		for (size_t byte = 0; byte < width; ++byte)
			bytes.push_back(static_cast<uint8_t>(value >> (8 * byte)));
	}
	void block(std::span<const uint8_t> value)
	{
		bytes.insert(bytes.end(), value.begin(), value.end());
	}
	void account(const economic_account_key &key)
	{
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> value;
		economic_account_key_encode(key, &value);
		block(value);
	}
	template <size_t N> economic_digest digest(const char (&tag)[N]) const
	{
		std::vector<uint8_t> value(tag, tag + N); // Include the NUL delimiter.
		value.insert(value.end(), bytes.begin(), bytes.end());
		economic_digest result;
		SHA256(value.data(), value.size(), result.data());
		return result;
	}
};
}

economic_prepared_baseline::economic_prepared_baseline(economic_baseline_batch witness,
						       economic_accounting_plan plan,
						       std::vector<uint8_t> encoded)
	: witness_(std::move(witness))
	, plan_(std::move(plan))
	, encoded_(std::move(encoded))
{
}

economic_accounting_error
economic_baseline_prepare(const economic_baseline_batch &input,
			  std::optional<economic_prepared_baseline> *prepared)
{
	if (!prepared)
		return error::invalid_identity;
	if (input.holdings.size() > ECONOMIC_BASELINE_MAX_HOLDINGS ||
	    input.items.size() > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES)
		return error::capacity;
	if (critical_operation_id_is_zero(input.lineage) ||
	    critical_operation_id_is_zero(input.epoch) ||
	    critical_operation_id_is_zero(input.preparation_id) || !input.actor_id ||
	    !economic_account_key_valid(input.opening_account) ||
	    input.opening_account.kind != economic_account_kind::opening ||
	    input.opening_account.lineage.bytes != input.lineage.bytes ||
	    zero(input.boundary_digest) || zero(input.coverage_digest))
		return error::invalid_identity;
	try
	{
		auto witness = input;
		std::sort(witness.holdings.begin(), witness.holdings.end(),
			  [](const auto &a, const auto &b)
			  { return economic_account_key_less(a.account, b.account); });
		std::sort(witness.items.begin(), witness.items.end(),
			  [](const auto &a, const auto &b)
			  { return a.snapshot.uid < b.snapshot.uid; });
		economic_accounting_plan plan;
		auto &meta = plan.metadata;
		meta.lineage = input.lineage;
		meta.epoch = input.epoch;
		if (!critical_operation_id_derive(input.preparation_id,
						  ECONOMIC_BASELINE_OPERATION_DOMAIN,
						  input.batch_index, &meta.operation_id))
			return error::invalid_identity;
		meta.actor_kind = economic_actor_kind::operator_action;
		meta.actor_id = input.actor_id;
		meta.writer_id = ECONOMIC_WRITER_BASELINE;
		meta.reason = economic_reason::baseline;
		meta.source_event = economic_source_event{ economic_source_kind::baseline,
							   input.preparation_id, input.epoch,
							   input.batch_index, 0 };
		encoding intent;
		intent.integer(1, 2);
		intent.block(input.lineage.bytes);
		intent.block(input.epoch.bytes);
		intent.block(input.preparation_id.bytes);
		intent.integer(input.actor_id);
		intent.integer(input.batch_index);
		intent.account(input.opening_account);
		intent.block(input.boundary_digest);
		intent.block(input.coverage_digest);
		intent.integer(witness.holdings.size(), 4);
		intent.integer(witness.items.size(), 4);
		encoding domain;
		std::set<uint64_t> lifetimes;
		std::vector<std::pair<economic_coin_vector, int64_t>> equity;
		for (const auto &holding : witness.holdings)
		{
			if (!economic_account_key_valid(holding.account) ||
			    !economic_account_is_ordinary(holding.account.kind) ||
			    holding.account.lineage.bytes != input.lineage.bytes ||
			    zero(holding.source_digest))
				return error::invalid_identity;
			if (!lifetimes.insert(holding.account.authority_id).second)
				return error::duplicate_event;
			for (auto amount : holding.balance)
				if (amount < 0)
					return error::negative_holding;
			int64_t value = 0;
			auto status = economic_coin_value(holding.balance, &value);
			if (status != error::ok)
				return status;
			domain.account(holding.account);
			for (auto amount : holding.balance)
				domain.integer(static_cast<uint64_t>(amount));
			domain.integer(holding.native_revision);
			domain.block(holding.source_digest);
			const auto index = static_cast<uint16_t>(plan.accounts.size());
			plan.accounts.push_back({ holding.account, {}, holding.balance, 0, 1 });
			if (value)
			{
				plan.postings.push_back(
					{ static_cast<uint32_t>(plan.postings.size()), index, 0,
					  holding.balance, value });
				auto opposite = holding.balance;
				for (auto &amount : opposite)
					amount = -amount;
				equity.emplace_back(opposite, -value);
			}
		}
		if (!equity.empty())
		{
			const auto index = static_cast<uint16_t>(plan.accounts.size());
			plan.accounts.push_back({ input.opening_account, {}, {}, 0, 0 });
			for (const auto &[delta, value] : equity)
				plan.postings.push_back(
					{ static_cast<uint32_t>(plan.postings.size()), index, 0,
					  delta, value });
		}
		for (const auto &item : witness.items)
		{
			if (zero(item.source_digest) ||
			    item.snapshot.position.state == item_custody_state::absent)
				return error::invalid_identity;
			const auto &position = item.snapshot.position;
			domain.integer(item.snapshot.uid);
			domain.integer(static_cast<uint8_t>(position.owner.type), 1);
			domain.integer(position.owner.id);
			domain.integer(position.owner.context_id);
			domain.integer(position.root_uid);
			domain.integer(position.parent_uid);
			domain.integer(position.revision);
			domain.integer(static_cast<uint8_t>(position.state), 1);
			domain.block(item.source_digest);
			plan.items_before.push_back(item.snapshot);
		}
		plan.items_after = plan.items_before;
		meta.intent_digest = intent.digest("DURIS-ECONOMIC-BASELINE-INTENT-V1");
		meta.domain_digest = domain.digest("DURIS-ECONOMIC-BASELINE-DOMAIN-V1");
		auto status = economic_plan_normalize(&plan);
		if (status != error::ok)
			return status;
		std::vector<uint8_t> encoded;
		status = economic_plan_encode(plan, &encoded);
		if (status != error::ok)
			return status;
		*prepared = economic_prepared_baseline(std::move(witness), std::move(plan),
						       std::move(encoded));
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}
