#ifndef DURIS_ENHANCEMENT_MATERIAL_QUOTE_H
#define DURIS_ENHANCEMENT_MATERIAL_QUOTE_H

#include <climits>
#include <cmath>
#include <cstdint>

static_assert(INT_MAX <= INT32_MAX && INT_MIN >= INT32_MIN);

struct enhancement_material_quote
{
	int low_count = 0;
	int high_count = 0;
};

inline bool enhancement_scale_material_count(int count, double multiplier, int *scaled)
{
	if (!scaled || count < 0 || !std::isfinite(multiplier) || multiplier <= 0.0)
		return false;
	const double quote = count * multiplier + 0.999999;
	if (!std::isfinite(quote) || quote >= static_cast<double>(INT_MAX) + 1.0)
		return false;
	*scaled = static_cast<int>(quote);
	return true;
}

inline bool enhancement_prepare_material_quote(int item_value, double multiplier,
					       enhancement_material_quote *quote)
{
	if (!quote || item_value < 0)
		return false;
	const int64_t material_value = static_cast<int64_t>(item_value) + 4;
	enhancement_material_quote prepared;
	if (!enhancement_scale_material_count(static_cast<int>(material_value % 5), multiplier,
					      &prepared.low_count) ||
	    !enhancement_scale_material_count(static_cast<int>(material_value / 5), multiplier,
					      &prepared.high_count))
		return false;
	*quote = prepared;
	return true;
}

#endif
