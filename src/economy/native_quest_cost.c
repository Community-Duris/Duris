#include "economy/native_quest_cost.h"

#include <climits>
#include <bit>
#include <algorithm>
#include <new>
#include <utility>

namespace
{
constexpr std::array<int64_t, 4> units{ 1, 10, 100, 1000 };
bool native_value(const std::array<int64_t, 4> &cash, int64_t *value) noexcept
{
	int64_t total = 0;
	for (size_t i = 0; i < cash.size(); ++i)
	{
		if (cash[i] < 0 || cash[i] > INT_MAX || cash[i] > (INT_MAX - total) / units[i])
			return false;
		total += cash[i] * units[i];
	}
	*value = total;
	return true;
}
void spend(std::array<int64_t, 4> &cash, int64_t amount) noexcept
{
	if (amount > cash[0])
	{
		amount -= cash[0];
		cash[0] = 0;
	}
	else
	{
		cash[0] -= amount;
		return;
	}
	for (size_t i = 1; i < cash.size() && amount > 0; ++i)
	{
		const int64_t available = cash[i] * units[i];
		if (amount >= available)
		{
			amount -= available;
			cash[i] = 0;
		}
		else
		{
			const int64_t removed = amount / units[i] + 1;
			cash[i] -= removed;
			amount -= removed * units[i];
		}
	}
	// Actual ADD_MONEY decomposes negative remainder from largest coin down.
	if (amount < 0)
	{
		int64_t change = -amount;
		for (size_t i = cash.size(); i-- > 0;)
		{
			cash[i] += change / units[i];
			change %= units[i];
		}
	}
}
}

native_quest_cost_projection_result
native_quest_cost_project(const std::array<int64_t, 4> &before, uint64_t revision,
			  std::span<const native_quest_cost_requirement> requirements,
			  native_quest_cost_projection *output) noexcept
{
	using result = native_quest_cost_projection_result;
	int64_t value = 0;
	if (!output || !revision || !native_value(before, &value))
		return result::invalid;
	try
	{
		native_quest_cost_projection candidate;
		candidate.before = candidate.after = before;
		candidate.before_revision = candidate.after_revision = revision;
		candidate.attempts.reserve(requirements.size());
		bool changed = false;
		for (size_t i = 0; i < requirements.size(); ++i)
		{
			if (i && requirements[i - 1].slot >= requirements[i].slot)
				return result::invalid;
			native_quest_cost_attempt attempt;
			attempt.requirement = requirements[i];
			attempt.before = candidate.after;
			if (requirements[i].copper <= 0)
				attempt.outcome = native_quest_cost_attempt_outcome::nonpositive;
			else if (requirements[i].copper > value)
				attempt.outcome = native_quest_cost_attempt_outcome::insufficient;
			else
			{
				spend(candidate.after, requirements[i].copper);
				attempt.outcome = native_quest_cost_attempt_outcome::charged;
				changed = true;
			}
			attempt.after = candidate.after;
			if (!native_value(candidate.after, &value))
				return result::overflow;
			candidate.attempts.push_back(std::move(attempt));
		}
		if (changed)
		{
			if (revision == UINT64_MAX)
				return result::overflow;
			++candidate.after_revision;
		}
		*output = std::move(candidate);
		return result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return result::allocation_failure;
	}
	catch (...)
	{
		return result::invalid;
	}
}

namespace
{
using cost_result = native_quest_cost_projection_result;
constexpr size_t max_attempts =
	(CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - NATIVE_QUEST_COST_HEADER_BYTES) /
	NATIVE_QUEST_COST_ATTEMPT_BYTES;
void put_cost(uint8_t *out, uint64_t value, size_t size) noexcept
{
	for (size_t i = 0; i < size; ++i)
		out[i] = static_cast<uint8_t>(value >> (8 * i));
}
uint64_t get_cost(const uint8_t *in, size_t size) noexcept
{
	uint64_t value = 0;
	for (size_t i = 0; i < size; ++i)
		value |= static_cast<uint64_t>(in[i]) << (8 * i);
	return value;
}
bool read_cost_cash(const uint8_t *in, std::array<int64_t, 4> &cash) noexcept
{
	for (size_t i = 0; i < cash.size(); ++i)
	{
		const uint64_t value = get_cost(in + i * 8, 8);
		if (value > INT_MAX)
			return false;
		cash[i] = static_cast<int64_t>(value);
	}
	return true;
}
void put_cost_cash(uint8_t *out, const std::array<int64_t, 4> &cash) noexcept
{
	for (size_t i = 0; i < cash.size(); ++i)
		put_cost(out + i * 8, static_cast<uint64_t>(cash[i]), 8);
}
cost_result verify_cost_projection(const native_quest_cost_projection &value)
{
	if (value.attempts.size() > max_attempts)
		return cost_result::overflow;
	std::vector<native_quest_cost_requirement> requirements;
	requirements.reserve(value.attempts.size());
	for (const auto &attempt : value.attempts)
		requirements.push_back(attempt.requirement);
	native_quest_cost_projection expected;
	auto status = native_quest_cost_project(value.before, value.before_revision, requirements,
						&expected);
	return status != cost_result::ok ? status :
	       expected == value	 ? cost_result::ok :
					   cost_result::invalid;
}
}

native_quest_cost_projection_result
native_quest_cost_projection_encode(const native_quest_cost_projection &value,
				    std::vector<uint8_t> *output) noexcept
{
	if (!output)
		return cost_result::invalid;
	try
	{
		auto status = verify_cost_projection(value);
		if (status != cost_result::ok)
			return status;
		std::vector<uint8_t> bytes(NATIVE_QUEST_COST_HEADER_BYTES +
					   value.attempts.size() * NATIVE_QUEST_COST_ATTEMPT_BYTES);
		bytes[0] = 'N';
		bytes[1] = 'Q';
		bytes[2] = 'C';
		bytes[3] = '1';
		put_cost(bytes.data() + 4, 1, 2);
		put_cost(bytes.data() + 8, value.attempts.size(), 4);
		put_cost(bytes.data() + 16, value.before_revision, 8);
		put_cost(bytes.data() + 24, value.after_revision, 8);
		put_cost_cash(bytes.data() + 32, value.before);
		put_cost_cash(bytes.data() + 64, value.after);
		for (size_t i = 0; i < value.attempts.size(); ++i)
		{
			auto *row = bytes.data() + NATIVE_QUEST_COST_HEADER_BYTES +
				    i * NATIVE_QUEST_COST_ATTEMPT_BYTES;
			const auto &attempt = value.attempts[i];
			put_cost(row, attempt.requirement.slot, 4);
			put_cost(row + 4, std::bit_cast<uint32_t>(attempt.requirement.copper), 4);
			row[8] = static_cast<uint8_t>(attempt.outcome);
			put_cost_cash(row + 16, attempt.before);
			put_cost_cash(row + 48, attempt.after);
		}
		*output = std::move(bytes);
		return cost_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return cost_result::allocation_failure;
	}
	catch (...)
	{
		return cost_result::invalid;
	}
}

native_quest_cost_projection_result
native_quest_cost_projection_decode(std::span<const uint8_t> bytes,
				    native_quest_cost_projection *output) noexcept
{
	if (!output || bytes.size() < NATIVE_QUEST_COST_HEADER_BYTES ||
	    bytes.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES || bytes[0] != 'N' ||
	    bytes[1] != 'Q' || bytes[2] != 'C' || bytes[3] != '1' ||
	    get_cost(bytes.data() + 4, 2) != 1 || get_cost(bytes.data() + 6, 2) ||
	    get_cost(bytes.data() + 12, 4))
		return cost_result::invalid;
	const auto count = get_cost(bytes.data() + 8, 4);
	if (count > max_attempts || bytes.size() != NATIVE_QUEST_COST_HEADER_BYTES +
							    count * NATIVE_QUEST_COST_ATTEMPT_BYTES)
		return cost_result::invalid;
	try
	{
		native_quest_cost_projection value;
		value.before_revision = get_cost(bytes.data() + 16, 8);
		value.after_revision = get_cost(bytes.data() + 24, 8);
		if (!read_cost_cash(bytes.data() + 32, value.before) ||
		    !read_cost_cash(bytes.data() + 64, value.after))
			return cost_result::invalid;
		value.attempts.reserve(static_cast<size_t>(count));
		for (size_t i = 0; i < count; ++i)
		{
			const auto *row = bytes.data() + NATIVE_QUEST_COST_HEADER_BYTES +
					  i * NATIVE_QUEST_COST_ATTEMPT_BYTES;
			if (std::any_of(row + 9, row + 16, [](uint8_t byte) { return byte != 0; }))
				return cost_result::invalid;
			native_quest_cost_attempt attempt;
			attempt.requirement.slot = static_cast<uint32_t>(get_cost(row, 4));
			attempt.requirement.copper =
				std::bit_cast<int32_t>(static_cast<uint32_t>(get_cost(row + 4, 4)));
			attempt.outcome = static_cast<native_quest_cost_attempt_outcome>(row[8]);
			if (!read_cost_cash(row + 16, attempt.before) ||
			    !read_cost_cash(row + 48, attempt.after))
				return cost_result::invalid;
			value.attempts.push_back(std::move(attempt));
		}
		auto status = verify_cost_projection(value);
		if (status != cost_result::ok)
			return status;
		*output = std::move(value);
		return cost_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		return cost_result::allocation_failure;
	}
	catch (...)
	{
		return cost_result::invalid;
	}
}
