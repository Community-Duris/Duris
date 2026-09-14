#include "world/generated_npc_state.h"
#include "core/prototypes.h"
#include "core/utils.h"

// Generated world NPCs use the bounded state codec without pet ownership,
// summon markers, charm removal, or lifetime scheduling.
bool generated_npc_capture(P_char pet, std::string *encoded, bool include_currency)
{
	if (!pet || !IS_NPC(pet) || !encoded)
		return false;
	encoded->clear();
	if (!generated_npc_vnum(mob_index[GET_RNUM(pet)].virtual_number))
		return true;
	const unsigned required_strings = STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2;
	if ((pet->only.npc->str_mask & required_strings) != required_strings)
		return true; // Retain legacy state without blocking the rest of the world snapshot.
	const auto &npc = *pet->only.npc;
	pet_restore_state s;
	s.kind = summoned_pet_kind::ordinary;
	s.name = pet->player.name ? pet->player.name : "";
	s.short_description = pet->player.short_descr ? pet->player.short_descr : "";
	s.long_description = pet->player.long_descr ? pet->player.long_descr : "";
	for (size_t i = 0; i < s.base_stats.size(); ++i)
		s.base_stats[i] = pet->base_stats[i];
	s.base_points = { pet->points.base_hit,	     pet->points.base_mana,
			  pet->points.base_vitality, pet->points.base_armor,
			  pet->points.base_hitroll,  pet->points.base_damroll,
			  pet->points.base_ward };
	s.damage_dice = { pet->points.damnodice, pet->points.damsizedice };
	static_assert(MAX_CIRCLE < PET_RESTORE_STATE_SLOTS);
	for (size_t i = 0; i <= MAX_CIRCLE; ++i)
		s.spell_slots[i] = pet->specials.undead_spell_slots[i];
	s.intrinsic_affects = { pet->specials.affected_by, pet->specials.affected_by2,
				pet->specials.affected_by3, pet->specials.affected_by4,
				pet->specials.affected_by5 };
	s.aggression = { npc.aggro_flags, npc.aggro2_flags, npc.aggro3_flags };
	s.act = pet->specials.act;
	s.primary_class = pet->player.m_class;
	s.secondary_class = pet->player.secondary_class;
	s.level = GET_LEVEL(pet);
	s.race = GET_RACE(pet);
	s.sex = GET_SEX(pet);
	s.size = GET_SIZE(pet);
	s.alignment = GET_ALIGNMENT(pet);
	const std::array<int32_t, 4> wallet =
		include_currency ? std::array<int32_t, 4>{ GET_COPPER(pet), GET_SILVER(pet),
							   GET_GOLD(pet), GET_PLATINUM(pet) } :
				   std::array<int32_t, 4>{};
	return generated_npc_state_encode(s, wallet, encoded) &&
	       generated_npc_state_valid(mob_index[GET_RNUM(pet)].virtual_number, *encoded);
}

bool generated_npc_apply(P_char pet, const std::string &encoded)
{
	if (!pet || !IS_NPC(pet) ||
	    !generated_npc_state_valid(mob_index[GET_RNUM(pet)].virtual_number, encoded))
		return false;
	if (encoded.empty())
		return true;
	pet_restore_state s;
	std::array<int32_t, 4> wallet;
	if (!generated_npc_state_decode(encoded, &s, &wallet))
		return false;
	if (!pet || !IS_NPC(pet) || s.race > LAST_RACE || !(s.act & ACT_ISNPC))
		return false;
	auto &npc = *pet->only.npc;
	const auto replace = [&](char **target, const std::string &text, unsigned flag)
	{
		if ((npc.str_mask & flag) && *target)
			str_free(*target);
		*target = str_dup(text.c_str());
		npc.str_mask |= flag;
	};
	replace(&pet->player.name, s.name, STRUNG_KEYS);
	replace(&pet->player.short_descr, s.short_description, STRUNG_DESC2);
	replace(&pet->player.long_descr, s.long_description, STRUNG_DESC1);
	for (size_t i = 0; i < s.base_stats.size(); ++i)
		pet->base_stats[i] = s.base_stats[i];
	pet->points.base_hit = s.base_points[0];
	pet->points.base_mana = s.base_points[1];
	pet->points.base_vitality = s.base_points[2];
	pet->points.base_armor = s.base_points[3];
	pet->points.base_hitroll = s.base_points[4];
	pet->points.base_damroll = s.base_points[5];
	pet->points.base_ward = s.base_points[6];
	pet->points.damnodice = s.damage_dice[0];
	pet->points.damsizedice = s.damage_dice[1];
	for (size_t i = 0; i <= MAX_CIRCLE; ++i)
		pet->specials.undead_spell_slots[i] = s.spell_slots[i];
	pet->specials.affected_by = s.intrinsic_affects[0];
	pet->specials.affected_by2 = s.intrinsic_affects[1];
	pet->specials.affected_by3 = s.intrinsic_affects[2];
	pet->specials.affected_by4 = s.intrinsic_affects[3];
	pet->specials.affected_by5 = s.intrinsic_affects[4];
	npc.aggro_flags = s.aggression[0];
	npc.aggro2_flags = s.aggression[1];
	npc.aggro3_flags = s.aggression[2];
	pet->specials.act = s.act;
	pet->player.m_class = s.primary_class;
	pet->player.secondary_class = s.secondary_class;
	pet->player.level = s.level;
	GET_RACE(pet) = s.race;
	GET_SEX(pet) = s.sex;
	GET_SIZE(pet) = s.size;
	GET_ALIGNMENT(pet) = s.alignment;
	GET_COPPER(pet) = wallet[0];
	GET_SILVER(pet) = wallet[1];
	GET_GOLD(pet) = wallet[2];
	GET_PLATINUM(pet) = wallet[3];

	return true;
}
