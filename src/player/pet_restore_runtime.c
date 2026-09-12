#include "player/pet_restore_runtime.h"

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "world/events.h"
#include "classes/necromancy.h"

#include <algorithm>
#include <climits>
#include <ctime>

void summoned_pet_mark(P_char pet, summoned_pet_kind kind)
{
	if (!pet || !IS_NPC(pet))
		return;
	auto &npc = *pet->only.npc;
	npc.summoned_instance = true;
	npc.summon_kind = static_cast<uint32_t>(kind);
	npc.summon_intrinsic_affects[0] = pet->specials.affected_by & ~AFF_CHARM;
	npc.summon_intrinsic_affects[1] = pet->specials.affected_by2;
	npc.summon_intrinsic_affects[2] = pet->specials.affected_by3;
	npc.summon_intrinsic_affects[3] = pet->specials.affected_by4;
	npc.summon_intrinsic_affects[4] = pet->specials.affected_by5;
}

bool summoned_pet_capture(P_char pet, std::string *encoded)
{
	if (!pet || !IS_NPC(pet) || !encoded)
		return false;
	encoded->clear();
	if (!pet->only.npc->summon_kind)
		return true;
	const auto &npc = *pet->only.npc;
	pet_restore_state s;
	s.kind = static_cast<summoned_pet_kind>(npc.summon_kind);
	s.charm_expires_at = npc.pet_charm_expires_at;
	s.death_expires_at = npc.pet_death_expires_at;
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
	std::copy(std::begin(npc.summon_intrinsic_affects), std::end(npc.summon_intrinsic_affects),
		  s.intrinsic_affects.begin());
	s.aggression = { npc.aggro_flags, npc.aggro2_flags, npc.aggro3_flags };
	s.act = pet->specials.act;
	s.primary_class = pet->player.m_class;
	s.secondary_class = pet->player.secondary_class;
	s.level = GET_LEVEL(pet);
	s.race = GET_RACE(pet);
	s.sex = GET_SEX(pet);
	s.size = GET_SIZE(pet);
	s.alignment = GET_ALIGNMENT(pet);
	return pet_restore_state_encode(s, encoded);
}

bool summoned_pet_apply(P_char pet, const pet_restore_state &s)
{
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
	pet->specials.affected_by = s.intrinsic_affects[0] & ~AFF_CHARM;
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
	summoned_pet_mark(pet, s.kind);
	npc.pet_charm_expires_at = s.charm_expires_at;
	npc.pet_death_expires_at = s.death_expires_at;
	return true;
}

int summoned_pet_capacity(P_char owner)
{
	if (IS_TRUSTED(owner))
		return INT_MAX;
	int power = GET_LEVEL(owner) * 2;
	if (GET_SPEC(owner, CLASS_NECROMANCER, SPEC_NECROLYTE) ||
	    GET_SPEC(owner, CLASS_THEURGIST, SPEC_TEMPLAR))
		power += 10;
	if (GET_CLASS(owner, CLASS_BLIGHTER))
		power /= 3;
	return power;
}

void summoned_pet_restore_lifetime(P_char pet, P_char owner, const pet_restore_state &s)
{
	const int64_t now = time(nullptr);
	const int duration =
		s.charm_expires_at ?
			static_cast<int>(std::min<int64_t>(
				INT_MAX,
				std::max<int64_t>(0, (s.charm_expires_at - now + 59) / 60))) :
			-1;
	setup_pet(pet, owner, duration, PET_NOAGGRO | PET_RESTORE);
	// setup_pet cannot turn this saved finite charm into a permanent one merely
	// because the owner now has a globe. Keep the original absolute deadlines.
	pet->only.npc->pet_charm_expires_at = s.charm_expires_at;
	pet->only.npc->pet_death_expires_at = s.death_expires_at;
	if (s.death_expires_at)
		schedule_pet_death(pet,
				   static_cast<int>(std::min<int64_t>(
					   INT_MAX, std::max<int64_t>(1, s.death_expires_at - now) *
							    WAIT_SEC)));
}
