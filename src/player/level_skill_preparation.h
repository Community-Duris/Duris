#ifndef DURIS_LEVEL_SKILL_PREPARATION_H
#define DURIS_LEVEL_SKILL_PREPARATION_H

#include "core/structs.h"
#include "player/skill_selection_preparation.h"
#include <array>

struct player_level_skill_policy_row
{
	int required_level = 0;
	::byte maximum_base = 0, maximum_specialization = 0;
	bool epic = false;
};

struct player_level_skill_policy
{
	unsigned int primary_class = 0, secondary_class = 0;
	int specialization = 0;
	bool chaos_mode = false, chaos_starter_epic_skills = false;
	std::array<player_level_skill_policy_row, MAX_SKILLS> rows{};
};

// Actual initialized original GET_LVL_FOR_SKILL/SKILL_DATA_ALL tables and chaos
// policies. Caller owns the actor, complete configuration cut and table lifetime.
// Required/max policies use only PC class/spec, not its BEFORE race/level/skills;
// the original level owner must prove those class/spec values remain unchanged.
// Invalid source classes/spec refuse before the original getter's live fallback.
// Values grant no source, gameplay, native transaction, publication or ACK power.
bool player_level_skill_observe_policy(P_char, player_level_skill_policy *) noexcept;

// Same policy for explicitly prepared PC class/spec values and the genuine
// complete initialized skill table. The original owner retains the immutable
// source/configuration cut, including both original property modifiers. Strong
// output refusal; no old actor, live getters, fallback or source capability.
bool player_level_skill_prepare_policy_values(const player_skill_selection_input &,
					      std::span<const Skill>,
					      const player_skill_selection_modifiers &,
					      bool chaos_mode, bool chaos_starter_epic_skills,
					      player_level_skill_policy *) noexcept;

// Original byte assignments in original order, shared by live update_skills and
// the pure complete-array preparation. Neither function acquires source/level
// authority. The supplied row is the actual initialized original policy.
void player_level_skill_apply_regular(char_skill_data &, int level, int minimum_learning,
				      bool chaos_mode, bool chaos_starter_epic_skills,
				      const player_level_skill_policy_row &) noexcept;
void player_level_skill_apply_song(char_skill_data &, int level, int minimum_learning,
				   bool chaos_mode, const player_level_skill_policy_row &) noexcept;

// Original native song getter order, including short-circuit and MAX repetition.
// Literal preparation supplies value readers; the original live wrapper supplies
// its actual initialized getters. Neither route supplies source/owner capability.
struct player_level_skill_song_reader
{
	const void *context;
	int (*required_level)(const void *);
	::byte (*maximum_base)(const void *);
	::byte (*maximum_specialization)(const void *);
};
void player_level_skill_apply_song_with_reader(char_skill_data &, int level, int minimum_learning,
					       bool chaos_mode,
					       const player_level_skill_song_reader &);

// Full original skill store, including unchanged slots outside both loops. No
// shadow character, RNG, live mutation, notch/affect, item/boon/guild or save work.
// Result is unchanged on refusal. Caller separately owns its full level/source
// proposal; a value result is not final AFTER or native readiness evidence.
bool player_level_skill_prepare(const player_level_skill_policy &, int prepared_level,
				const std::array<char_skill_data, MAX_SKILLS> &before,
				std::array<char_skill_data, MAX_SKILLS> *after) noexcept;

#endif
