#ifndef DURIS_ENHANCEMENT_PRICE_H
#define DURIS_ENHANCEMENT_PRICE_H

#include <climits>
#include <cstdint>

static_assert(INT_MAX <= INT32_MAX && INT_MIN >= INT32_MIN);

inline bool enhancement_prepare_price(int64_t quote, int *cost)
{
	if (!cost || quote < 0 || quote > INT_MAX)
		return false;
	*cost = static_cast<int>(quote);
	return true;
}

inline bool enhancement_prepare_ordinary_price(int item_value, int low_threshold, int low_price,
					       int high_price, int *cost)
{
	return enhancement_prepare_price(item_value <= low_threshold ? low_price : high_price,
					 cost);
}

inline bool enhancement_prepare_superior_price(int item_value, int base, int per_value, int *cost)
{
	const int64_t quote =
		static_cast<int64_t>(base) + static_cast<int64_t>(item_value) * per_value;
	return enhancement_prepare_price(quote, cost);
}

inline int enhancement_essence_price(int material_value)
{
	return material_value <= 20 ? 1000 : material_value <= 30 ? 20000 : 100000;
}

#endif
