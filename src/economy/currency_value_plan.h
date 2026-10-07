#ifndef CURRENCY_VALUE_PLAN_H
#define CURRENCY_VALUE_PLAN_H

#include "economy/currency_command.h"

#include <algorithm>
#include <climits>
#include <cstdint>

// Match the native wallet/bank count domain rather than accepting wide balances.
static_assert(INT_MAX <= INT32_MAX && INT_MIN >= INT32_MIN);
inline constexpr std::array<int64_t, CURRENCY_DENOMINATION_COUNT> currency_coin_values = { 1, 10,
											   100,
											   1000 };

inline currency_vector currency_canonical_value(int64_t value)
{
	currency_vector result = {};
	for (size_t index = currency_coin_values.size(); index-- > 0;)
	{
		result.amount[index] = value / currency_coin_values[index];
		value %= currency_coin_values[index];
	}
	return result;
}

inline int64_t currency_native_value(const std::array<int, CURRENCY_DENOMINATION_COUNT> &current)
{
	// Four signed native counts times these denominations fit in int64_t.
	int64_t total = 0;
	for (size_t index = 0; index < current.size(); ++index)
		total += static_cast<int64_t>(current[index]) * currency_coin_values[index];
	return total;
}

inline bool
currency_prepare_wallet_value_delta(const std::array<int, CURRENCY_DENOMINATION_COUNT> &current,
				    int64_t value_delta, currency_vector *delta)
{
	if (!delta || !value_delta || value_delta == INT64_MIN)
		return false;
	currency_vector wallet_delta = {};
	if (value_delta > 0)
		wallet_delta = currency_canonical_value(value_delta);
	else
	{
		const int64_t total = currency_native_value(current);
		const int64_t spend = -value_delta;
		if (total < spend)
			return false;
		const currency_vector after = currency_canonical_value(total - spend);
		for (size_t index = 0; index < current.size(); ++index)
			wallet_delta.amount[index] = after.amount[index] - current[index];
	}
	*delta = wallet_delta;
	return true;
}

inline bool
currency_prepare_bank_payment_deltas(const std::array<int, CURRENCY_DENOMINATION_COUNT> &current,
				     int64_t value, currency_vector *wallet, currency_vector *bank)
{
	if (!wallet || !bank || value <= 0 || currency_native_value(current) < value)
		return false;
	currency_vector bank_delta = {};
	int64_t remaining = value;
	for (size_t index = 0; index < current.size() && remaining > 0; ++index)
	{
		const int64_t needed =
			(remaining + currency_coin_values[index] - 1) / currency_coin_values[index];
		const int64_t used = std::min(static_cast<int64_t>(current[index]), needed);
		bank_delta.amount[index] = -used;
		remaining -= used * currency_coin_values[index];
	}
	currency_vector wallet_delta = {};
	if (remaining < 0)
		wallet_delta = currency_canonical_value(-remaining);
	*wallet = wallet_delta;
	*bank = bank_delta;
	return true;
}

#endif
