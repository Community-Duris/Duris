#include "economy/economic_baseline_adapter.h"
#include <algorithm>
#include <bit>
#include <new>

namespace
{
using error = economic_accounting_error;
struct writer
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
};
struct reader
{
	std::span<const uint8_t> bytes;
	size_t offset = 0;
	// Length/count checks establish every bound before decoding any row.
	uint64_t integer(size_t width = 8)
	{
		uint64_t value = 0;
		for (size_t byte = 0; byte < width; ++byte)
			value |= uint64_t(bytes[offset++]) << (8 * byte);
		return value;
	}
	std::span<const uint8_t> take(size_t count)
	{
		auto result = bytes.subspan(offset, count);
		offset += count;
		return result;
	}
	template <size_t N> void block(std::array<uint8_t, N> &value)
	{
		auto input = take(N);
		std::copy(input.begin(), input.end(), value.begin());
	}
};
constexpr std::array<uint8_t, 4> MAGIC = { 'E', 'A', 'B', '1' };
}

economic_accounting_error economic_baseline_encode(const economic_prepared_baseline &prepared,
						   std::vector<uint8_t> *encoded)
{
	if (!encoded || prepared.encoded_plan().empty())
		return error::invalid_identity;
	try
	{
		const auto &value = prepared.witness();
		const size_t total = ECONOMIC_BASELINE_HEADER_BYTES +
				     value.holdings.size() * ECONOMIC_BASELINE_HOLDING_BYTES +
				     value.items.size() * ECONOMIC_BASELINE_ITEM_BYTES;
		writer out;
		out.bytes.reserve(total);
		out.block(MAGIC);
		out.integer(1, 2);
		out.integer(ECONOMIC_BASELINE_HEADER_BYTES, 2);
		out.integer(total, 4);
		out.integer(0, 4);
		out.block(value.lineage.bytes);
		out.block(value.epoch.bytes);
		out.block(value.preparation_id.bytes);
		out.integer(value.actor_id);
		out.integer(value.batch_index);
		out.account(value.opening_account);
		out.block(value.boundary_digest);
		out.block(value.coverage_digest);
		out.integer(value.holdings.size(), 4);
		out.integer(value.items.size(), 4);
		for (const auto &holding : value.holdings)
		{
			out.account(holding.account);
			for (auto amount : holding.balance)
				out.integer(static_cast<uint64_t>(amount));
			out.integer(holding.native_revision);
			out.block(holding.source_digest);
		}
		for (const auto &item : value.items)
		{
			const auto &position = item.snapshot.position;
			out.integer(item.snapshot.uid);
			out.integer(static_cast<uint8_t>(position.owner.type), 1);
			out.integer(static_cast<uint8_t>(position.state), 1);
			out.integer(0, 6);
			out.integer(position.owner.id);
			out.integer(position.owner.context_id);
			out.integer(position.root_uid);
			out.integer(position.parent_uid);
			out.integer(position.revision);
			out.block(item.source_digest);
		}
		*encoded = std::move(out.bytes);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error
economic_baseline_decode(std::span<const uint8_t> encoded,
			 std::optional<economic_prepared_baseline> *prepared)
{
	if (!prepared)
		return error::invalid_identity;
	if (encoded.size() > ECONOMIC_BASELINE_MAX_BYTES)
		return error::capacity;
	if (encoded.size() < ECONOMIC_BASELINE_HEADER_BYTES ||
	    !std::equal(MAGIC.begin(), MAGIC.end(), encoded.begin()))
		return error::corrupt_evidence;
	reader input{ encoded, 4 };
	if (input.integer(2) != 1)
		return error::invalid_version;
	if (input.integer(2) != ECONOMIC_BASELINE_HEADER_BYTES ||
	    input.integer(4) != encoded.size() || input.integer(4) != 0)
		return error::corrupt_evidence;
	try
	{
		economic_baseline_batch value;
		input.block(value.lineage.bytes);
		input.block(value.epoch.bytes);
		input.block(value.preparation_id.bytes);
		value.actor_id = input.integer();
		value.batch_index = input.integer();
		auto status = economic_account_key_decode(input.take(ECONOMIC_ACCOUNT_KEY_BYTES),
							  &value.opening_account);
		if (status != error::ok)
			return status;
		input.block(value.boundary_digest);
		input.block(value.coverage_digest);
		const auto holdings = input.integer(4), items = input.integer(4);
		if (holdings > ECONOMIC_BASELINE_MAX_HOLDINGS ||
		    items > ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES)
			return error::capacity;
		const size_t total = ECONOMIC_BASELINE_HEADER_BYTES +
				     holdings * ECONOMIC_BASELINE_HOLDING_BYTES +
				     items * ECONOMIC_BASELINE_ITEM_BYTES;
		if (total != encoded.size())
			return error::corrupt_evidence;
		value.holdings.resize(holdings);
		for (auto &holding : value.holdings)
		{
			status = economic_account_key_decode(input.take(ECONOMIC_ACCOUNT_KEY_BYTES),
							     &holding.account);
			if (status != error::ok)
				return status;
			for (auto &amount : holding.balance)
				amount = std::bit_cast<int64_t>(input.integer());
			holding.native_revision = input.integer();
			input.block(holding.source_digest);
		}
		value.items.resize(items);
		for (auto &item : value.items)
		{
			auto &position = item.snapshot.position;
			item.snapshot.uid = input.integer();
			position.owner.type = static_cast<item_owner_type>(input.integer(1));
			position.state = static_cast<item_custody_state>(input.integer(1));
			if (input.integer(6))
				return error::corrupt_evidence;
			position.owner.id = input.integer();
			position.owner.context_id = input.integer();
			position.root_uid = input.integer();
			position.parent_uid = input.integer();
			position.revision = input.integer();
			input.block(item.source_digest);
		}
		std::optional<economic_prepared_baseline> result;
		status = economic_baseline_prepare(value, &result);
		if (status != error::ok)
			return status;
		std::vector<uint8_t> canonical;
		status = economic_baseline_encode(*result, &canonical);
		if (status != error::ok)
			return status;
		if (!std::equal(encoded.begin(), encoded.end(), canonical.begin(), canonical.end()))
			return error::corrupt_evidence;
		*prepared = std::move(result);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}
