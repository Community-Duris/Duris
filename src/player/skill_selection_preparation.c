#include "player/skill_selection_preparation.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include <cmath>
#include <limits>

bool player_skill_select_original_values(const player_skill_selection_input &input,
					 std::span<const ClassSkillInfo> classes,
					 const player_skill_selection_modifiers &modifiers,
					 ClassSkillInfo *output) noexcept
{
	if (!output || classes.size() != CLASS_COUNT)
		return false;
	ClassSkillInfo selected;
	const int primary = flag2idx(input.primary_classes) - 1;
	const int secondary = flag2idx(input.secondary_classes) - 1;
	if (!input.npc && secondary >= 0)
	{
		const int primary_level = classes[primary].rlevel[input.specialization];
		const int secondary_level = classes[secondary].rlevel[input.specialization];
		const int primary_cap = classes[primary].maxlearn[input.specialization];
		const int secondary_cap = classes[secondary].maxlearn[input.specialization];
		int required_level, new_cap;
		if (primary_level > 0 && secondary_level < 1)
		{
			new_cap = (int)(primary_cap * modifiers.primary) / 100;
			required_level = primary_level;
		}
		else if (secondary_level > 0 && primary_level < 1)
		{
			new_cap = (int)(secondary_cap * modifiers.secondary) / 100;
			required_level = secondary_level + 5;
		}
		else if (primary_level > 0 && secondary_level > 0)
		{
			new_cap = MAX((int)(primary_cap * modifiers.primary),
				      (int)(secondary_cap * modifiers.secondary)) /
				  100;
			required_level = MIN(primary_level, secondary_level + 5);
		}
		else
			new_cap = required_level = 0;
		selected.maxlearn[input.specialization] = new_cap;
		selected.rlevel[input.specialization] = required_level;
	}
	else if (input.npc && input.multiclass_npc)
	{
		int highest = 0;
		const ClassSkillInfo *highest_class = classes.data();
		for (int i = 0; i < CLASS_COUNT; ++i)
			if (input.primary_classes & (1U << i))
				if (classes[i].maxlearn[0] > highest)
				{
					highest_class = &classes[i];
					highest = classes[i].maxlearn[0];
				}
		selected = *highest_class;
	}
	else if (primary >= 0 && primary < CLASS_COUNT)
		selected = classes[primary];
	else
		return false;
	*output = selected;
	return true;
}

int player_skill_required_level_original_values(const ClassSkillInfo &selected,
						int specialization) noexcept
{
	return selected.rlevel[specialization];
}

bool player_skill_select_prepared_values(const player_skill_selection_input &input,
					 std::span<const ClassSkillInfo> classes,
					 const player_skill_selection_modifiers &modifiers,
					 ClassSkillInfo *output) noexcept
{
	if (!output || classes.size() != CLASS_COUNT || input.specialization < 0 ||
	    input.specialization > MAX_SPEC || (!input.npc && input.multiclass_npc))
		return false;
	const int primary = flag2idx(input.primary_classes) - 1;
	const int secondary = flag2idx(input.secondary_classes) - 1;
	if (!input.npc && secondary >= 0)
	{
		if (primary < 0 || primary >= CLASS_COUNT || secondary >= CLASS_COUNT)
			return false;
		const auto conversion_defined = [](int cap, float modifier)
		{
			const float product = cap * modifier;
			return std::isfinite(product) &&
			       static_cast<double>(product) >= std::numeric_limits<int>::min() &&
			       static_cast<double>(product) <
				       static_cast<double>(std::numeric_limits<int>::max()) + 1.0;
		};
		const int primary_level = classes[primary].rlevel[input.specialization];
		const int secondary_level = classes[secondary].rlevel[input.specialization];
		if (primary_level > 0 &&
		    !conversion_defined(classes[primary].maxlearn[input.specialization],
					modifiers.primary))
			return false;
		if (secondary_level > 0 &&
		    !conversion_defined(classes[secondary].maxlearn[input.specialization],
					modifiers.secondary))
			return false;
	}
	else if (!(input.npc && input.multiclass_npc) && (primary < 0 || primary >= CLASS_COUNT))
		return false;
	return player_skill_select_original_values(input, classes, modifiers, output);
}

bool player_skill_required_level_prepared_values(const player_skill_selection_input &input,
						 std::span<const ClassSkillInfo> classes,
						 const player_skill_selection_modifiers &modifiers,
						 int *output) noexcept
{
	if (!output)
		return false;
	ClassSkillInfo selected;
	if (!player_skill_select_prepared_values(input, classes, modifiers, &selected))
		return false;
	*output = player_skill_required_level_original_values(selected, input.specialization);
	return true;
}
