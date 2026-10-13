#include "player/level_skill_preparation.h"
#include "core/prototypes.h"
#include "core/utility.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "combat/chaos_config.h"
#include <limits>

extern Skill skills[];

bool player_level_skill_observe_policy(P_char actor, player_level_skill_policy *output) noexcept
{
	if (!output || !actor || !IS_PC(actor) || !actor->only.pc || actor->player.spec > MAX_SPEC)
		return false;
	const int primary = flag2idx(actor->player.m_class) - 1;
	const int secondary = flag2idx(actor->player.secondary_class) - 1;
	if (primary < 0 || primary >= CLASS_COUNT || secondary >= CLASS_COUNT)
		return false;
	try
	{
		const float primary_modifier =
			get_property("skill.cap.multi.mod.primarySkill", 100.0);
		const float secondary_modifier =
			get_property("skill.cap.multi.mod.secondarySkill", 95.0);
		const bool chaos_mode = chaos_mud_enabled();
		const bool starter_epics = chaos_mode && chaos_starter_epic_skills_enabled();
		return player_level_skill_prepare_policy_values(
			{ actor->player.m_class, actor->player.secondary_class, actor->player.spec,
			  false, false },
			std::span<const Skill>(skills, MAX_SKILLS),
			{ primary_modifier, secondary_modifier }, chaos_mode, starter_epics,
			output);
	}
	catch (...)
	{
		return false;
	}
}

bool player_level_skill_prepare_policy_values(const player_skill_selection_input &input,
					      std::span<const Skill> initialized_skills,
					      const player_skill_selection_modifiers &modifiers,
					      bool chaos_mode, bool chaos_starter_epic_skills,
					      player_level_skill_policy *output) noexcept
{
	if (!output || input.npc || initialized_skills.size() != MAX_SKILLS)
		return false;
	player_level_skill_policy observed;
	observed.primary_class = input.primary_classes;
	observed.secondary_class = input.secondary_classes;
	observed.specialization = input.specialization;
	observed.chaos_mode = chaos_mode;
	observed.chaos_starter_epic_skills = chaos_starter_epic_skills;
	const auto capture = [&](int skill)
	{
		ClassSkillInfo selected;
		if (!player_skill_select_prepared_values(input, initialized_skills[skill].m_class,
							 modifiers, &selected))
			return false;
		auto &row = observed.rows[skill];
		row.required_level =
			player_skill_required_level_original_values(selected, input.specialization);
		row.maximum_base = selected.maxlearn[0];
		row.maximum_specialization = selected.maxlearn[input.specialization];
		row.epic = IS_SKILL(skill) && (initialized_skills[skill].targets & TAR_EPIC);
		return true;
	};
	for (int skill = FIRST_SKILL; skill <= LAST_SKILL; ++skill)
		if (!capture(skill))
			return false;
	for (int skill = FIRST_INSTRUMENT; skill <= LAST_SONG; ++skill)
		if (!capture(skill))
			return false;
	*output = observed;
	return true;
}

void player_level_skill_apply_regular(char_skill_data &value, int level, int minimum_learning,
				      bool chaos_mode, bool chaos_starter_epic_skills,
				      const player_level_skill_policy_row &policy) noexcept
{
	if (policy.epic)
	{
		if (chaos_mode && !chaos_starter_epic_skills)
		{
			value.taught = 100;
			value.learned = 100;
		}
	}
	else if (policy.required_level > 0 && level >= policy.required_level)
	{
		value.taught = policy.maximum_specialization;
		if (chaos_mode)
		{
			value.learned = 100;
			value.taught = 100;
		}
		else
		{
			if (value.taught < minimum_learning)
				value.taught = minimum_learning;
			if (value.learned < minimum_learning)
				value.learned = minimum_learning;
		}
	}
	else
		value.taught = value.learned = 0;
	if (!policy.epic && value.taught < value.learned)
	{
		if (chaos_mode)
			value.taught = value.learned = 100;
		else
			value.learned = value.taught;
	}
}

void player_level_skill_apply_song_with_reader(char_skill_data &value, int level,
					       int minimum_learning, bool chaos_mode,
					       const player_level_skill_song_reader &reader)
{
	if (reader.required_level(reader.context) > 0 &&
	    level >= reader.required_level(reader.context))
	{
		if (chaos_mode)
			value.taught = value.learned = 100;
		else
		{
			value.taught = MAX(value.taught,
					   MAX(reader.maximum_base(reader.context),
					       reader.maximum_specialization(reader.context)));
			value.learned = MAX(minimum_learning, value.learned);
		}
	}
	else
		value.taught = value.learned = 0;
	if (reader.maximum_base(reader.context) < value.taught &&
	    reader.maximum_specialization(reader.context) < value.taught)
	{
		if (chaos_mode)
			value.taught = value.learned = 100;
		else
			value.taught = MAX(0, MAX(reader.maximum_base(reader.context),
						  reader.maximum_specialization(reader.context)));
	}
}

void player_level_skill_apply_song(char_skill_data &value, int level, int minimum_learning,
				   bool chaos_mode,
				   const player_level_skill_policy_row &policy) noexcept
{
	const player_level_skill_song_reader reader{
		&policy,
		[](const void *context) {
			return static_cast<const player_level_skill_policy_row *>(context)
				->required_level;
		},
		[](const void *context) {
			return static_cast<const player_level_skill_policy_row *>(context)
				->maximum_base;
		},
		[](const void *context) {
			return static_cast<const player_level_skill_policy_row *>(context)
				->maximum_specialization;
		}
	};
	player_level_skill_apply_song_with_reader(value, level, minimum_learning, chaos_mode,
						  reader);
}

bool player_level_skill_prepare(const player_level_skill_policy &policy, int prepared_level,
				const std::array<char_skill_data, MAX_SKILLS> &before,
				std::array<char_skill_data, MAX_SKILLS> *after) noexcept
{
	if (!after || prepared_level < 0 || prepared_level > std::numeric_limits<ubyte>::max())
		return false;
	auto prepared = before;
	const int minimum_learning = MIN(40, (3 * prepared_level) / 2);
	for (int skill = FIRST_SKILL; skill <= LAST_SKILL; ++skill)
		player_level_skill_apply_regular(prepared[skill], prepared_level, minimum_learning,
						 policy.chaos_mode,
						 policy.chaos_starter_epic_skills,
						 policy.rows[skill]);
	for (int skill = FIRST_INSTRUMENT; skill <= LAST_SONG; ++skill)
		player_level_skill_apply_song(prepared[skill], prepared_level, minimum_learning,
					      policy.chaos_mode, policy.rows[skill]);
	*after = prepared;
	return true;
}
