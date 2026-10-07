#ifndef DURIS_ENHANCEMENT_SUPERIOR_PLAN_H
#define DURIS_ENHANCEMENT_SUPERIOR_PLAN_H

#include "core/config.h"
#include "economy/enhancement_material_quote.h"

#include <cstring>

#define MAX_SUPERIOR_MATERIALS (MAX_OBJ_AFFECT * 2)

struct superior_material_requirement
{
	int vnum;
	int count;
};

struct superior_enhancement_plan
{
	int slots[MAX_OBJ_AFFECT];
	int slot_count;
	int remaining_enhancements;
	struct superior_material_requirement materials[MAX_SUPERIOR_MATERIALS];
	int material_count;
};

struct enhancement_superior_target
{
	bool available;
	int low_vnum;
	int item_value;
	double material_multiplier;
};

inline bool enhancement_superior_plan_add_material(struct superior_enhancement_plan *plan, int vnum,
						   int count)
{
	if (count <= 0)
		return true;
	for (int i = 0; i < plan->material_count; i++)
	{
		if (plan->materials[i].vnum == vnum)
		{
			if (plan->materials[i].count > INT_MAX - count)
				return false;
			plan->materials[i].count += count;
			return true;
		}
	}
	if (plan->material_count >= MAX_SUPERIOR_MATERIALS)
		return false;
	plan->materials[plan->material_count].vnum = vnum;
	plan->materials[plan->material_count].count = count;
	plan->material_count++;
	return true;
}

// Observe synchronously at each original point; an early failure retains the
// prepared prefix, including low material admitted before high material refuses.
template <typename Observations> inline bool
enhancement_prepare_superior_plan(Observations &observed, struct superior_enhancement_plan *plan)
{
	std::memset(plan, 0, sizeof(*plan));
	for (int slot = 0; slot < MAX_OBJ_AFFECT; slot++)
	{
		if (!observed.eligible(slot))
			continue;
		const int base = observed.base(slot);
		const int cap = observed.cap(base);
		if (base <= 0 || observed.modifier(slot) >= cap)
			continue;
		const enhancement_superior_target target = observed.target(slot);
		if (!target.available)
			continue;
		const int high_vnum = target.low_vnum + 4;
		enhancement_material_quote quote;
		if (!enhancement_prepare_material_quote(target.item_value,
							target.material_multiplier, &quote))
			return false;
		if (!enhancement_superior_plan_add_material(plan, target.low_vnum,
							    quote.low_count) ||
		    !enhancement_superior_plan_add_material(plan, high_vnum, quote.high_count))
			return false;
		plan->slots[plan->slot_count++] = slot;
		const int remaining = observed.remaining(slot, cap);
		if (remaining > plan->remaining_enhancements)
			plan->remaining_enhancements = remaining;
	}
	return plan->slot_count > 0;
}

#endif
