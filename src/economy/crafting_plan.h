#ifndef DURIS_CRAFTING_PLAN_H
#define DURIS_CRAFTING_PLAN_H

#include <climits>
#include <cmath>
#include <cstdint>

struct crafting_plan
{
	int item_value;
	int low_material_vnum;
	int high_material_vnum;
	int low_material_count;
	int high_material_count;
	bool magical;
};

inline bool crafting_scale_material_count(int64_t count, double quantity_multiplier, int *scaled)
{
	if (count < 0 || !std::isfinite(quantity_multiplier) || quantity_multiplier <= 0.0)
		return false;
	const double quote = std::ceil(static_cast<double>(count) * quantity_multiplier);
	if (!std::isfinite(quote) || quote < 0.0 || quote > INT_MAX)
		return false;
	*scaled = static_cast<int>(quote);
	return true;
}

// Item valuation/material/affect capture and configuration ownership stay with
// the caller. This quote grants no authority to consume materials or mint output.
inline bool crafting_prepare_plan(int item_value, int low_material_vnum, bool magical,
				  double quantity_multiplier, crafting_plan *plan)
{
	if (!plan || item_value < 1 || low_material_vnum <= 0 || low_material_vnum > INT_MAX - 4)
		return false;
	const int64_t total = static_cast<int64_t>(item_value) + 4;
	crafting_plan quoted = {};
	if (!crafting_scale_material_count(total / 5, quantity_multiplier,
					   &quoted.high_material_count) ||
	    !crafting_scale_material_count(total % 5, quantity_multiplier,
					   &quoted.low_material_count))
		return false;
	quoted.item_value = item_value;
	quoted.low_material_vnum = low_material_vnum;
	quoted.high_material_vnum = low_material_vnum + 4;
	quoted.magical = magical;
	*plan = quoted;
	return true;
}

#endif
