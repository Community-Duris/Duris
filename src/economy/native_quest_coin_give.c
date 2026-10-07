#include "economy/native_quest_coin_give.h"

#include <climits>
#include <new>
#include <utility>

namespace
{
bool native_counts(const std::array<int64_t, 4> &cash) noexcept
{
	for (const int64_t count : cash)
		if (count < 0 || count > INT_MAX)
			return false;
	return true;
}
void put(std::vector<uint8_t> &bytes, size_t offset, uint64_t value, size_t count) noexcept
{
	for (size_t i = 0; i < count; ++i)
		bytes[offset + i] = static_cast<uint8_t>(value >> (i * 8));
}
uint64_t get(std::span<const uint8_t> bytes, size_t offset, size_t count) noexcept
{
	uint64_t value = 0;
	for (size_t i = 0; i < count; ++i)
		value |= static_cast<uint64_t>(bytes[offset + i]) << (i * 8);
	return value;
}
}

native_quest_coin_give_result native_quest_coin_give_project(
	const std::array<int64_t, 4> &player_before, uint64_t player_revision,
	const std::array<int64_t, 4> &mobile_before, uint64_t mobile_revision, uint8_t denomination,
	int32_t quantity, native_quest_coin_give_projection *output) noexcept
{
	using result = native_quest_coin_give_result;
	if (!output || !mobile_revision || denomination >= player_before.size() || quantity <= 0 ||
	    !native_counts(player_before) || !native_counts(mobile_before))
		return result::invalid;
	if (player_before[denomination] < quantity)
		return result::insufficient;
	constexpr std::array<int64_t, 4> units{ 1, 10, 100, 1000 };
	if (player_revision == UINT64_MAX || mobile_revision == UINT64_MAX ||
	    quantity > INT_MAX / units[denomination])
		return result::overflow;
	native_quest_coin_give_projection candidate;
	candidate.denomination = denomination;
	candidate.quantity = quantity;
	candidate.player_before_revision = player_revision;
	candidate.player_after_revision = player_revision + 1;
	candidate.mobile_before_revision = mobile_revision;
	candidate.mobile_after_revision = mobile_revision + 1;
	candidate.player_before = candidate.player_after = player_before;
	candidate.mobile_before = candidate.mobile_after = mobile_before;
	candidate.player_after[denomination] -= quantity;
	// Original src/cmd/actobj.c begin_coin_give_credit passes copper value
	// to src/core/utility.c ADD_MONEY, largest denomination first. It adds
	// normalized credit only; existing recipient cash is not exchanged.
	int64_t copper = static_cast<int64_t>(quantity) * units[denomination];
	for (size_t i = candidate.mobile_after.size(); i-- > 0;)
	{
		const int64_t added = copper / units[i];
		if (candidate.mobile_after[i] > INT_MAX - added)
			return result::overflow;
		candidate.mobile_after[i] += added;
		copper %= units[i];
	}
	*output = candidate;
	return result::ok;
}

native_quest_coin_give_result
native_quest_coin_give_encode(const native_quest_coin_give_projection &value,
			      std::vector<uint8_t> *output) noexcept
{
	using result = native_quest_coin_give_result;
	native_quest_coin_give_projection projected;
	if (!output ||
	    native_quest_coin_give_project(value.player_before, value.player_before_revision,
					   value.mobile_before, value.mobile_before_revision,
					   value.denomination, value.quantity,
					   &projected) != result::ok ||
	    projected != value)
		return result::invalid;
	try
	{
		std::vector<uint8_t> bytes(NATIVE_QUEST_COIN_GIVE_BYTES, 0);
		bytes[0] = 'N';
		bytes[1] = 'Q';
		bytes[2] = 'G';
		bytes[3] = '1';
		put(bytes, 4, 1, 2);
		bytes[6] = value.denomination;
		put(bytes, 8, static_cast<uint32_t>(value.quantity), 4);
		put(bytes, 16, value.player_before_revision, 8);
		put(bytes, 24, value.player_after_revision, 8);
		put(bytes, 32, value.mobile_before_revision, 8);
		put(bytes, 40, value.mobile_after_revision, 8);
		const std::array<const std::array<int64_t, 4> *, 4> arrays{ &value.player_before,
									    &value.player_after,
									    &value.mobile_before,
									    &value.mobile_after };
		for (size_t a = 0; a < arrays.size(); ++a)
			for (size_t i = 0; i < arrays[a]->size(); ++i)
				put(bytes, 48 + a * 32 + i * 8,
				    static_cast<uint64_t>((*arrays[a])[i]), 8);
		*output = std::move(bytes);
		return result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return result::allocation_failure;
	}
}

native_quest_coin_give_result
native_quest_coin_give_decode(std::span<const uint8_t> bytes,
			      native_quest_coin_give_projection *output) noexcept
{
	using result = native_quest_coin_give_result;
	if (!output || bytes.size() != NATIVE_QUEST_COIN_GIVE_BYTES || bytes[0] != 'N' ||
	    bytes[1] != 'Q' || bytes[2] != 'G' || bytes[3] != '1' || get(bytes, 4, 2) != 1 ||
	    bytes[7] || get(bytes, 12, 4) || get(bytes, 8, 4) > INT_MAX)
		return result::invalid;
	native_quest_coin_give_projection value;
	value.denomination = bytes[6];
	value.quantity = static_cast<int32_t>(get(bytes, 8, 4));
	value.player_before_revision = get(bytes, 16, 8);
	value.player_after_revision = get(bytes, 24, 8);
	value.mobile_before_revision = get(bytes, 32, 8);
	value.mobile_after_revision = get(bytes, 40, 8);
	const std::array<std::array<int64_t, 4> *, 4> arrays{
		&value.player_before, &value.player_after, &value.mobile_before, &value.mobile_after
	};
	for (size_t a = 0; a < arrays.size(); ++a)
		for (size_t i = 0; i < arrays[a]->size(); ++i)
		{
			const uint64_t count = get(bytes, 48 + a * 32 + i * 8, 8);
			if (count > INT_MAX)
				return result::invalid;
			(*arrays[a])[i] = static_cast<int64_t>(count);
		}
	native_quest_coin_give_projection projected;
	if (native_quest_coin_give_project(value.player_before, value.player_before_revision,
					   value.mobile_before, value.mobile_before_revision,
					   value.denomination, value.quantity,
					   &projected) != result::ok ||
	    projected != value)
		return result::invalid;
	*output = value;
	return result::ok;
}
