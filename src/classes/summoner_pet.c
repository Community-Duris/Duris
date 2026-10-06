#include "classes/summoner_pet.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include "magic/spells.h"
#include "world/events.h"
#include "player/pet_restore_runtime.h"
#include "classes/necromancy.h"
#include "combat/chaos_config.h"
#include "sql/sql_spellbook.h"
#include "player/player_save_pipeline.h"
#include "economy/tradeskill.h"
#include <algorithm>
#include <cmath>

extern Skill skills[];
bool has_skin_spell(P_char ch);
extern P_room world;
extern unsigned long long ne_event_tick;
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
int conjure_terrain_check(P_char ch, P_char mob);

namespace
{
constexpr int capture_hp_cap = 5000;
constexpr int bank_width = 16;
constexpr int mana_bank = 13;
constexpr int capacity_bank = 14;
constexpr int resource_scale = 1000;

struct capture_buff
{
	int vector;
	uint64_t bit;
	int spell;
};

// Spell-backed prototype buffs become ordinary casts on non-elemental captures.
constexpr capture_buff capture_buffs[] = {
	{ 0, AFF_INVISIBLE, SPELL_INVISIBILITY },
	{ 0, AFF_FARSEE, SPELL_FARSEE },
	{ 0, AFF_DETECT_INVISIBLE, SPELL_DETECT_INVISIBLE },
	{ 0, AFF_HASTE, SPELL_HASTE },
	{ 0, AFF_SENSE_LIFE, SPELL_SENSE_LIFE },
	{ 0, AFF_MINOR_GLOBE, SPELL_MINOR_GLOBE },
	{ 0, AFF_STONE_SKIN, SPELL_STONE_SKIN },
	{ 0, AFF_ARMOR, SPELL_ARMOR },
	{ 0, AFF_WRAITHFORM, SPELL_WRAITHFORM },
	{ 0, AFF_WATERBREATH, SPELL_WATERBREATH },
	{ 0, AFF_PROTECT_EVIL, SPELL_PROTECT_FROM_EVIL },
	{ 0, AFF_SLOW_POISON, SPELL_SLOW_POISON },
	{ 0, AFF_PROTECT_GOOD, SPELL_PROTECT_FROM_GOOD },
	{ 0, AFF_BARKSKIN, SPELL_BARKSKIN },
	{ 0, AFF_LEVITATE, SPELL_LEVITATE },
	{ 0, AFF_FLY, SPELL_FLY },
	{ 0, AFF_PROT_FIRE, SPELL_PROTECT_FROM_FIRE },
	{ 0, AFF_BIOFEEDBACK, SPELL_BIOFEEDBACK },
	{ 0, AFF_INFERNAL_FURY, SPELL_INFERNAL_FURY },
	{ 0, AFF_FREEDOM_OF_MVMNT, SPELL_FREEDOM_OF_MOVEMENT },
	{ 0, AFF_SANCTUM_DRACONIS, SPELL_SANCTUM_DRACONIS },
	{ 1, AFF2_FIRESHIELD, SPELL_FIRESHIELD },
	{ 1, AFF2_DETECT_EVIL, SPELL_DETECT_EVIL },
	{ 1, AFF2_DETECT_GOOD, SPELL_DETECT_GOOD },
	{ 1, AFF2_DETECT_MAGIC, SPELL_DETECT_MAGIC },
	{ 1, AFF2_PROT_COLD, SPELL_PROTECT_FROM_COLD },
	{ 1, AFF2_PROT_LIGHTNING, SPELL_PROTECT_FROM_LIGHTNING },
	{ 1, AFF2_GLOBE, SPELL_GLOBE },
	{ 1, AFF2_PROT_GAS, SPELL_PROTECT_FROM_GAS },
	{ 1, AFF2_PROT_ACID, SPELL_PROTECT_FROM_ACID },
	{ 1, AFF2_SOULSHIELD, SPELL_SOULSHIELD },
	{ 1, AFF2_CONCEALMENT, SPELL_CONCEALMENT },
	{ 1, AFF2_VAMPIRIC_TOUCH, SPELL_VAMPIRIC_TOUCH },
	{ 1, AFF2_EARTH_AURA, SPELL_ELEMENTAL_AURA },
	{ 1, AFF2_WATER_AURA, SPELL_ELEMENTAL_AURA },
	{ 1, AFF2_FIRE_AURA, SPELL_ELEMENTAL_AURA },
	{ 1, AFF2_AIR_AURA, SPELL_ELEMENTAL_AURA },
	{ 1, AFF2_PASSDOOR, SPELL_MOLECULAR_CONTROL },
	{ 2, AFF3_ECTOPLASMIC_FORM, SPELL_ECTOPLASMIC_FORM },
	{ 2, AFF3_PROT_ANIMAL, SPELL_PROTECT_FROM_ANIMAL },
	{ 2, AFF3_SPIRIT_WARD, SPELL_SPIRIT_WARD },
	{ 2, AFF3_GR_SPIRIT_WARD, SPELL_GREATER_SPIRIT_WARD },
	{ 2, AFF3_INERTIAL_BARRIER, SPELL_INERTIAL_BARRIER },
	{ 2, AFF3_LIGHTNINGSHIELD, SPELL_LIGHTNINGSHIELD },
	{ 2, AFF3_COLDSHIELD, SPELL_COLDSHIELD },
	{ 2, AFF3_TOWER_IRON_WILL, SPELL_TOWER_IRON_WILL },
	{ 2, AFF3_BLUR, SPELL_BLUR },
	{ 2, AFF3_PASS_WITHOUT_TRACE, SPELL_PASS_WITHOUT_TRACE },
	{ 2, AFF3_ENLARGE, SPELL_ENLARGE },
	{ 2, AFF3_REDUCE, SPELL_REDUCE },
	{ 2, AFF3_ELEMENTAL_FORM, SPELL_ELEMENTAL_FORM },
	{ 2, AFF3_VIVERNAE_CONCORDIA, SPELL_VIVERNAE_CONCORDIA },
	{ 3, AFF4_SENSE_FOLLOWER, SPELL_SENSE_FOLLOWER },
	{ 3, AFF4_STORNOGS_SPHERES, SPELL_STORNOGS_SPHERES },
	{ 3, AFF4_STORNOGS_GREATER_SPHERES, SPELL_STORNOGS_GREATER_SPHERES },
	{ 3, AFF4_VAMPIRE_FORM, SPELL_VAMPIRE },
	{ 3, AFF4_BATTLE_ECSTASY, SPELL_BATTLE_ECSTASY },
	{ 3, AFF4_DAZZLER, SPELL_DAZZLE },
	{ 3, AFF4_NOFEAR, SPELL_INDOMITABILITY },
	{ 3, AFF4_REGENERATION, SPELL_REGENERATION },
	{ 3, AFF4_BATTLETIDE, SPELL_BATTLETIDE },
	{ 3, AFF4_MAGE_FLAME, SPELL_MAGE_FLAME },
	{ 3, AFF4_GLOBE_OF_DARKNESS, SPELL_GLOBE_OF_DARKNESS },
	{ 3, AFF4_HAWKVISION, SPELL_HAWKVISION },
	{ 3, AFF4_SANCTUARY, SPELL_SANCTUARY },
	{ 3, AFF4_HELLFIRE, SPELL_HELLFIRE },
	{ 3, AFF4_SENSE_HOLINESS, SPELL_SENSE_HOLINESS },
	{ 3, AFF4_PROT_LIVING, SPELL_PROTECT_FROM_LIVING },
	{ 3, AFF4_DETECT_ILLUSION, SPELL_DETECT_ILLUSION },
	{ 3, AFF4_REV_POLARITY, SPELL_ALTER_ENERGY_POLARITY },
	{ 3, AFF4_NEG_SHIELD, SPELL_NEG_ENERGY_BARRIER },
	{ 3, AFF4_PHANTASMAL_FORM, SPELL_ETHEREAL_FORM },
	{ 4, AFF5_FLESH_ARMOR, SPELL_FLESH_ARMOR },
	{ 4, AFF5_PROT_UNDEAD, SPELL_PROT_FROM_UNDEAD },
	{ 4, AFF5_THORNSKIN, SPELL_THORNSKIN },
	{ 4, AFF5_HOLY_DHARMA, SPELL_HOLY_DHARMA },
	{ 4, AFF5_OBSCURING_MIST, SPELL_OBSCURING_MIST },
};

std::array<uint64_t, 5> intrinsic_capture_traits(P_char pet, std::array<uint64_t, 5> traits)
{
	// Keep the old converter's transient-state exclusions and capture's Deflect exclusion.
	traits[0] &= ~(AFF_KNOCKED_OUT | AFF_BOUND | AFF_CHARM | AFF_FEAR | AFF_MEDITATE |
		       AFF_CAMPING | AFF_SLEEP);
	traits[1] &= ~(AFF2_MINOR_PARALYSIS | AFF2_MAJOR_PARALYSIS | AFF2_POISONED | AFF2_SILENCED |
		       AFF2_STUNNED | AFF2_HOLDING_BREATH | AFF2_MEMORIZING | AFF2_IS_DROWNING |
		       AFF2_CASTING | AFF2_SCRIBING | AFF2_HUNTER);
	traits[2] &= ~(AFF3_TRACKING | AFF3_FAMINE | AFF3_SWIMMING);
	traits[3] &= ~AFF4_SACKING;
	traits[4] &= ~(AFF5_IMPRISON | AFF5_MEMORY_BLOCK);
	if (!IS_ELEMENTAL(pet))
	{
		traits[3] &= ~AFF4_DEFLECT;
		for (const auto &buff : capture_buffs)
			traits[buff.vector] &= ~buff.bit;
	}
	return traits;
}

void mark_resources(P_char owner)
{
	player_save_pipeline_mark(GET_PID(owner), PLAYER_COMPONENT_AFFECTS | PLAYER_COMPONENT_PETS);
}

affected_type *bank(P_char owner, int slot, int circle, bool create)
{
	const int key = (slot - 1) * bank_width + circle;
	for (auto *af = owner->affected; af; af = af->next)
		if (af->type == TAG_SUMMONER_RESOURCE && af->level == key)
			return af;
	if (!create)
		return nullptr;
	affected_type af = {};
	af.type = TAG_SUMMONER_RESOURCE;
	af.flags = AFFTYPE_STORE;
	af.duration = -1;
	af.level = key;
	affect_to_char(owner, &af);
	return bank(owner, slot, circle, false);
}

int specialization_level(P_char pet, P_char owner, int specialization)
{
	if (!summoner_capture(pet))
		return 0;
	if (!owner)
		owner = GET_MASTER(pet);
	if (!owner || !IS_PC(owner) || !GET_SPEC(owner, CLASS_SUMMONER, specialization) ||
	    GET_LEVEL(owner) < 30 || (specialization == SPEC_MENTALIST && !IS_ELEMENTAL(pet)) ||
	    (specialization == SPEC_NATURALIST && !(IS_ANIMAL(pet) || IS_PLANT(pet))))
		return 0;
	return GET_LEVEL(owner);
}

int mana_capacity(P_char pet, P_char owner = nullptr)
{
	const int level = specialization_level(pet, owner, SPEC_MENTALIST);
	return GET_LEVEL(pet) * (IS_ELEMENTAL(pet)	    ? (level >= 41 ? 6 :
							       level	   ? 5 :
									     4) :
				 GET_CLASS(pet, CLASS_BARD) ? 6 :
							      8);
}

bool owner_busy(P_char owner)
{
	return !owner || IS_FIGHTING(owner) || IS_CASTING(owner) ||
	       (owner->summoner_last_move_tick &&
		ne_event_tick < owner->summoner_last_move_tick + 20 * WAIT_SEC);
}

P_char pet_in_slot(P_char owner, int slot)
{
	for (auto *f = owner->followers; f; f = f->next)
		if (summoner_owned_pet(f->follower) &&
		    f->follower->only.npc->summoner_resource_slot == slot)
			return f->follower;
	return nullptr;
}

void event_summoner_recovery(P_char owner, P_char, P_obj, void *)
{
	if (!IS_ALIVE(owner))
		return;
	for (int slot = 1; slot <= 4; ++slot)
	{
		auto *debt = bank(owner, slot, mana_bank, false);
		auto *capacity = bank(owner, slot, capacity_bank, false);
		P_char pet = pet_in_slot(owner, slot);
		if (!debt || !capacity || owner_busy(owner) ||
		    (pet && (IS_FIGHTING(pet) || IS_CASTING(pet) || SINGING(pet))))
			continue;
		// Two percent each six seconds, including fractional mana. Dismissal
		// leaves this owner-side debt intact and cannot refill a prepared slot.
		const int previous = debt->modifier;
		debt->modifier = std::max(0, debt->modifier - capacity->modifier * 20);
		if (debt->modifier != previous)
			mark_resources(owner);
		if (pet)
		{
			GET_MANA(pet) =
				std::max(0, GET_MAX_MANA(pet) - (debt->modifier + resource_scale -
								 1) / resource_scale);
			pet->only.npc->summoner_expected_mana = GET_MANA(pet);
		}
	}
	add_event(event_summoner_recovery, 6 * WAIT_SEC, owner, nullptr, nullptr, 0, nullptr, 0);
}

bool spend_scaled(P_char pet, int cost)
{
	if (!summoner_owned_pet(pet) || cost == 0)
		return true;
	P_char owner = GET_MASTER(pet);
	const int slot = pet->only.npc->summoner_resource_slot;
	if (slot < 1 || slot > 4)
		return false;
	summoner_pet_sync_resources(pet);
	auto *debt = bank(owner, slot, mana_bank, true);
	if (cost < 0 || cost > GET_MAX_MANA(pet) * resource_scale - debt->modifier)
	{
		summoner_pet_exhausted(pet);
		return false;
	}
	debt->modifier += cost;
	if (cost)
		mark_resources(owner);
	GET_MANA(pet) = std::max(0, GET_MAX_MANA(pet) -
					    (debt->modifier + resource_scale - 1) / resource_scale);
	pet->only.npc->summoner_expected_mana = GET_MANA(pet);
	return true;
}

std::pair<double, double> racial_range(int race)
{
	switch (race)
	{
	case RACE_HALFLING:
	case RACE_GNOME:
	case RACE_GOBLIN:
	case RACE_KOBOLD:
	case RACE_DROW:
	case RACE_GREY:
	case RACE_FAERIE:
		return { 0.70, 0.90 };
	case RACE_MOUNTAIN:
	case RACE_DUERGAR:
	case RACE_TROLL:
	case RACE_WIGHT:
	case RACE_REVENANT:
	case RACE_GARGOYLE:
	case RACE_GOLEM:
	case RACE_CONSTRUCT:
		return { 1.15, 1.40 };
	case RACE_OGRE:
	case RACE_MINOTAUR:
	case RACE_FIRBOLG:
		return { 1.60, 2.00 };
	case RACE_SGIANT:
	case RACE_GIANT:
	case RACE_FIREGIANT:
	case RACE_FROSTGIANT:
	case RACE_TITAN:
	case RACE_AVATAR:
		return { 2.10, 2.60 };
	case RACE_SNOW_OGRE:
		return { 3.00, 4.00 };
	case RACE_INSECT:
	case RACE_ARACHNID:
	case RACE_FLYING_ANIMAL:
	case RACE_PARASITE:
		return { 0.65, 0.85 };
	case RACE_DRAGON:
	case RACE_DRAGONKIN:
	case RACE_DRACOLICH:
	case RACE_QUADRUPED:
	case RACE_CARNIVORE:
	case RACE_HERBIVORE:
	case RACE_CENTAUR:
	case RACE_PWORM:
		return { 1.20, 1.65 };
	case RACE_PLANT:
	case RACE_SLIME:
		return { 1.45, 1.90 };
	default:
		return { 0.95, 1.15 };
	}
}

// Frozen normal-mode body values from the pre-rework configuration. The old
// ordinary summon (before its random Charisma bonus and elite multiplier) is
// a conservative ceiling; wild NPC properties, zone dials and Chaos cannot
// change it. Infuse Life still uses the owner's skill as the old summon did.
int normal_capture_hp(P_char pet, P_char owner)
{
	constexpr int racial_con[LAST_RACE + 1] = {
		100, 100, 165, 90,  90,	 120, 135, 95,	90,  200, 160, 102, 85,	 125, 105, 155, 100,
		170, 100, 169, 100, 70,	 100, 120, 106, 150, 155, 97,  109, 170, 100, 115, 95,	82,
		140, 135, 200, 100, 110, 82,  100, 150, 100, 165, 150, 150, 150, 120, 100, 100, 110,
		170, 105, 165, 95,  170, 140, 120, 110, 90,  100, 100, 100, 120, 115, 110, 120, 175,
		100, 100, 60,  140, 155, 130, 140, 100, 100, 130, 225, 70,  140, 90,  130, 85,	95,
		195, 120, 150, 100, 130, 90,  155, 155, 110, 85,  185, 100, 100, 100, 170, 95
	};
	constexpr float class_hp[CLASS_COUNT + 1] = { 1,   1.2, .9, .55, 1,  1,	  .8, .7,
						      .7,  .7,	.6, .5,	 .5, .8,  .8, .9,
						      .85, .8,	.8, .4,	 1,  .85, .9, .5,
						      .7,  1,	.8, 1,	 .5, .5,  1 };
	const int level = GET_LEVEL(pet);
	const int con = racial_con[std::clamp<int>(GET_RACE(pet), 0, LAST_RACE)];
	const int type = std::clamp(flag2idx(pet->player.m_class), 0, CLASS_COUNT);
	int hp = static_cast<int>((0.00000045 * con * con * level * level + 2) * level);
	hp -= static_cast<int>(0.5 * hp * (1.0 - class_hp[type]));
	const int life = std::clamp(GET_CHAR_SKILL(owner, SKILL_INFUSE_LIFE), 0, 100);
	return std::clamp(static_cast<int>(hp * (1 + life / 500.0)), 1, 8000);
}

std::pair<int, int> normal_capture_dice(P_char pet)
{
	// Frozen ordinary prototype dice from convertMob, without elite or zone boosts.
	// Recompute at the trained level so restored dice never compound the blend.
	constexpr int bodies[][3] = { { 1, 5, 0 },  { 2, 3, 1 },  { 3, 3, 5 },	{ 4, 4, 10 },
				      { 5, 5, 10 }, { 5, 5, 15 }, { 6, 6, 20 }, { 6, 6, 25 },
				      { 6, 7, 30 }, { 7, 7, 35 }, { 8, 9, 40 }, { 9, 9, 45 } };
	const int level = GET_LEVEL(pet);
	const auto &body = bodies[std::clamp((level - 1) / 5, 0, 11)];
	const int count = body[0] + (IS_MELEE_CLASS(pet) ? level / 15 : 0);
	if (body[2] + count * (1 + body[1]) / 2 > 90)
		return { std::max(1, (90 - (30 + level / 2)) / 4), 7 };
	return { count, body[1] };
}

void blend_capture_dice(P_char pet, P_char owner)
{
	const auto original = normal_capture_dice(pet);
	const bool naturalist = specialization_level(pet, owner, SPEC_NATURALIST);
	if (naturalist)
		++pet->points.damnodice;
	const int count = pet->points.damnodice, sides = pet->points.damsizedice;
	const int trained_average = count * (sides + 1);
	const int original_average = original.first * (original.second + 1);
	if (original_average <= trained_average)
		return;
	const int weight = naturalist ? 80 : 50;
	int blended_count = (count * (100 - weight) + original.first * weight + 50) / 100;
	int blended_sides = (sides * (100 - weight) + original.second * weight + 50) / 100;
	// Whole dice must stay between the trained floor and the original average.
	if (blended_count * (blended_sides + 1) > original_average)
	{
		blended_count = original.first;
		blended_sides = original.second;
	}
	if (blended_count * (blended_sides + 1) >= trained_average)
	{
		pet->points.damnodice = blended_count;
		pet->points.damsizedice = blended_sides;
	}
}

double role_factor(P_char pet)
{
	if (GET_CLASS(pet, CLASS_BARD))
		return 0.85;
	const auto classes = pet->player.m_class;
	if (classes && (classes & (classes - 1)))
		return 0.90;
	if (IS_CASTER_CLASS(classes))
		return 0.80;
	if (classes & (CLASS_ROGUE | CLASS_THIEF | CLASS_ASSASSIN))
		return 0.90;
	return 1.0;
}

bool necro_spell_available(int spell, int level)
{
	const auto &info = skills[spell].m_class[flag2idx(CLASS_NECROMANCER) - 1];
	for (int spec = 0; spec <= MAX_SPEC; ++spec)
		if (info.rlevel[spec] > 0 && level >= (info.rlevel[spec] - 1) * 5 + 1)
			return true;
	return false;
}

int necro_hp_ceiling(P_char owner)
{
	const int level = GET_LEVEL(owner);
	const int life = std::clamp(GET_CHAR_SKILL(owner, SKILL_INFUSE_LIFE), 0, 100);
	constexpr int raises[] = { SPELL_ANIMATE_DEAD, SPELL_ANIMATE_DEAD,  SPELL_RAISE_SPECTRE,
				   SPELL_RAISE_WRAITH, SPELL_RAISE_VAMPIRE, SPELL_RAISE_LICH,
				   SPELL_RAISE_SHADOW };
	int ceiling = 1;
	for (int type = 0; type <= NECROPET_END; ++type)
	{
		if (!necro_spell_available(raises[type], level))
			continue;
		const auto &body = undead_data[type];
		const int cap =
			level < 50 ?
				static_cast<int>((level / 4.0 + 37.5) * body.max_level / 50.0) :
				body.max_level;
		ceiling = std::max(ceiling, static_cast<int>(std::min(level, cap) * 6.5 +
							     level * body.hps + life * 3));
	}
	if (necro_spell_available(SPELL_CREATE_GOLEM, level))
		for (int type = 0; type < NECROGOLEM_LAST; ++type)
		{
			const auto &body = golem_data[type];
			if (body.corpse_lvl > level)
				continue;
			const int cap = level < 50 ? static_cast<int>((level / 4.0 + 37.5) *
								      body.max_level / 50.0) :
						     body.max_level;
			ceiling = std::max(ceiling, static_cast<int>(std::min(level, cap) * 6 +
								     level * body.hps + life * 3));
		}
	if (necro_spell_available(SPELL_CREATE_DRACOLICH, level))
		ceiling = std::max(ceiling, 1000 + life * 5);
	if (necro_spell_available(SPELL_CREATE_GREATER_DRACOLICH, level))
		ceiling = std::max(ceiling, 2520 + life * 5);
	return ceiling;
}

bool useful_song(P_char bard, P_char target, int song)
{
	if (song == SONG_HEALING)
	{
		if (affected_by_spell(target, SONG_HEALING) &&
		    get_linked_char(target, LNK_SNG_HEALING) != bard)
			return false;
		return GET_HIT(target) < GET_MAX_HIT(target) - 4 ||
		       (GET_SPEC(bard, CLASS_BARD, SPEC_MINSTREL) &&
			((IS_AFFECTED(target, AFF_BLIND) &&
			  GET_CHAR_SKILL(bard, SONG_HEALING) >= 90) ||
			 (IS_AFFECTED2(target, AFF2_POISONED) &&
			  GET_CHAR_SKILL(bard, SONG_HEALING) >= 50) ||
			 (GET_CHAR_SKILL(bard, SONG_HEALING) >= 70 &&
			  (affected_by_spell(target, SPELL_DISEASE) ||
			   affected_by_spell(target, SPELL_PLAGUE))))) ||
		       (target == bard && GET_SPEC(bard, CLASS_BARD, SPEC_DISHARMONIST) &&
			IS_AFFECTED2(bard, AFF2_SILENCED) &&
			GET_CHAR_SKILL(bard, SONG_HEALING) >= 90);
	}
	if (song == SONG_FLIGHT)
		return !is_linked_to(bard, target, LNK_SONG);
	if (song == SONG_PROTECTION)
		return !affected_by_spell(target, song) ||
		       (GET_LEVEL(bard) >= 51 && !IS_AFFECTED2(target, AFF2_GLOBE)) ||
		       (GET_LEVEL(bard) >= 31 && GET_LEVEL(bard) < 51 &&
			!IS_AFFECTED(target, AFF_MINOR_GLOBE)) ||
		       (GET_LEVEL(bard) >= 46 && !has_skin_spell(target));
	if (song == SONG_HEROISM)
		return !affected_by_spell(target, song) || !affected_by_spell(bard, song) ||
		       (GET_LEVEL(bard) >= 21 && !IS_AFFECTED(target, AFF_HASTE));
	return !affected_by_spell(target, song);
}
}

void summoner_chaos_recipes(P_char owner)
{
	if (!owner || IS_NPC(owner) || !chaos_mud_enabled() || !GET_CLASS(owner, CLASS_SUMMONER) ||
	    GET_LEVEL(owner) < 56 || owner->player.spec < SPEC_CONTROLLER ||
	    owner->player.spec > SPEC_NATURALIST)
		return;
	constexpr int recipes[][3] = {
		{ 142408, 27035, 82507 }, // Bran Boru, A'den, Xavier
		{ 35543, 35542, 30623 }, // earth/air Library sentries, pech
		{ 135214, 42204, 78483 } // dragonkin seer, scorpion, warg
	};
	for (int vnum : recipes[owner->player.spec - 1])
		if (!sql_has_spellbook_mob(GET_PID(owner), vnum))
			sql_add_spellbook_mob(GET_PID(owner), vnum);
}

bool summoner_capture(P_char pet)
{
	return pet && IS_NPC(pet) &&
	       pet->only.npc->summon_kind ==
		       static_cast<uint32_t>(summoned_pet_kind::summoner_capture);
}

bool summoner_balanced_body(P_char pet)
{
	return summoner_capture(pet);
}

bool summoner_owned_pet(P_char pet)
{
	return summoner_capture(pet) && IS_PC_PET(pet);
}

int summoner_pet_strength_factor(int race)
{
	// Frozen normal prototype Strength factors; wild NPC config cannot retrain pets.
	constexpr int racial_strength[LAST_RACE + 1] = {
		100, 100, 155, 90,  90,	 135, 130, 95,	85,  230, 160, 105, 70,	 120, 115, 140, 100,
		165, 85,  145, 100, 70,	 120, 120, 120, 150, 135, 90,  80,  160, 100, 95,  90,	83,
		135, 125, 230, 110, 110, 82,  80,  120, 90,  150, 190, 150, 150, 110, 120, 90,	110,
		250, 90,  200, 60,  200, 140, 145, 120, 120, 85,  75,  100, 250, 190, 110, 120, 170,
		100, 140, 100, 120, 250, 130, 120, 100, 100, 140, 250, 60,  115, 95,  100, 55,	75,
		185, 125, 200, 100, 100, 95,  350, 350, 105, 55,  35,  100, 100, 100, 250, 75
	};
	return 100 + (std::max(100, racial_strength[std::clamp(race, 0, LAST_RACE)]) - 100 + 1) / 2;
}

void summoner_pet_apply_traits(P_char pet, const std::array<uint64_t, 5> &traits)
{
	const auto intrinsic = intrinsic_capture_traits(pet, traits);
	pet->specials.affected_by |= intrinsic[0];
	pet->specials.affected_by2 |= intrinsic[1];
	pet->specials.affected_by3 |= intrinsic[2];
	pet->specials.affected_by4 |= intrinsic[3];
	pet->specials.affected_by5 |= intrinsic[4];
}

void summoner_pet_cast_buffs(P_char pet, const std::array<uint64_t, 5> &traits)
{
	if (!summoner_owned_pet(pet) || IS_ELEMENTAL(pet))
		return;
	// Initial prototype buffs are part of creation, not a fresh prepared spell pool.
	int slots[MAX_CIRCLE + 1];
	std::copy(std::begin(pet->specials.undead_spell_slots),
		  std::end(pet->specials.undead_spell_slots), slots);
	for (const auto &buff : capture_buffs)
		if ((traits[buff.vector] & buff.bit) && skills[buff.spell].spell_pointer &&
		    !affected_by_spell(pet, buff.spell))
			// Use normal durations/modifiers and avoid the command/casting delay path.
			skills[buff.spell].spell_pointer(GET_LEVEL(pet), pet, nullptr,
							 SPELL_TYPE_SPELL, pet, nullptr);
	// Vampire's player assimilation must not wipe the capture's finite NPC slots.
	std::copy(std::begin(slots), std::end(slots), pet->specials.undead_spell_slots);
	summoner_pet_finish_affects(pet);
}

int summoner_pet_level(P_char pet, P_char owner)
{
	// The existing greater-orb path is required for prototypes above level 56.
	// Their unclamped level also identifies the exception after restoration.
	return GET_LEVEL(pet) > CONJURE_MAXLVL_NO_ORB ? GET_LEVEL(pet) :
							std::min(GET_LEVEL(pet), GET_LEVEL(owner));
}

int summoner_pet_memtime(P_char pet, int time)
{
	if (specialization_level(pet, nullptr, SPEC_MENTALIST))
		return std::max(1, time * 4 / 5);
	return specialization_level(pet, nullptr, SPEC_NATURALIST) ? time * 6 / 5 : time;
}

double summoner_pet_melee_damage(P_char pet, double damage)
{
	const int level = specialization_level(pet, nullptr, SPEC_NATURALIST);
	return damage * (level >= 41 ? 1.10 : level ? 1.05 : 1.0);
}

double summoner_pet_physical_damage(P_char pet, double damage)
{
	const int level = specialization_level(pet, nullptr, SPEC_NATURALIST);
	return damage * (level >= 51 ? 0.85 : level >= 41 ? 0.90 : level ? 0.95 : 1.0);
}

int summoner_pet_hit_regen(P_char pet, int gain)
{
	const int level = specialization_level(pet, nullptr, SPEC_NATURALIST);
	if (!level || gain <= 0 || IS_FIGHTING(pet) || GET_STAT(pet) < STAT_SLEEPING)
		return gain;
	return gain + gain * (level >= 51 ? 20 : level >= 41 ? 15 : 10) / 100;
}

int summoner_pet_heal_cap(P_char pet, int requested)
{
	return summoner_capture(pet) ? std::min(requested, GET_MAX_HIT(pet) * 11 / 10) : requested;
}

double summoner_pet_vamp_rate(P_char pet, double requested, bool undead)
{
	return summoner_capture(pet) ? std::min(requested, undead ? 0.10 : 0.25) : requested;
}

void summoner_pet_start_recovery(P_char owner)
{
	if (!owner || !IS_PC(owner) || get_scheduled(owner, event_summoner_recovery))
		return;
	for (auto *af = owner->affected; af; af = af->next)
		if (af->type == TAG_SUMMONER_RESOURCE)
		{
			add_event(event_summoner_recovery, 6 * WAIT_SEC, owner, nullptr, nullptr, 0,
				  nullptr, 0);
			return;
		}
}

void summoner_pet_exhausted(P_char pet)
{
	P_char owner = pet ? GET_MASTER(pet) : nullptr;
	if (owner && IS_PC(owner))
		act("$N looks too exhausted for that.", FALSE, owner, nullptr, pet, TO_CHAR);
}

bool summoner_pet_spend(P_char pet, int mana)
{
	return spend_scaled(pet, mana * resource_scale);
}

bool summoner_pet_spell_ready(P_char pet, int circle)
{
	if (!summoner_owned_pet(pet) || circle == -1)
		return true;
	if (circle >= 1 && circle <= get_max_circle(pet) &&
	    (USES_MANA(pet) ? GET_MANA(pet) >= circle * MANA_PER_CIRCLE :
			      pet->specials.undead_spell_slots[circle] > 0))
		return true;
	summoner_pet_exhausted(pet);
	return false;
}

void summoner_pet_sync_resources(P_char pet)
{
	if (!summoner_owned_pet(pet))
		return;
	P_char owner = GET_MASTER(pet);
	const int slot = pet->only.npc->summoner_resource_slot;
	if (slot < 1 || slot > 4)
		return;
	auto *debt = bank(owner, slot, mana_bank, true);
	// Keep any fractional flight expenditure when another action rebuilds affects.
	auto &npc = *pet->only.npc;
	bool changed = false;
	const int external_spending = npc.summoner_expected_mana - GET_MANA(pet);
	changed |= external_spending != 0;
	debt->modifier = std::max(0, debt->modifier + external_spending * resource_scale);
	GET_MANA(pet) = std::max(0, GET_MAX_MANA(pet) -
					    (debt->modifier + resource_scale - 1) / resource_scale);
	npc.summoner_expected_mana = GET_MANA(pet);
	auto *capacity = bank(owner, slot, capacity_bank, true);
	changed |= capacity->modifier != GET_MAX_MANA(pet);
	capacity->modifier = GET_MAX_MANA(pet);
	for (int circle = 1; circle <= MAX_CIRCLE; ++circle)
	{
		auto *used = bank(owner, slot, circle, true);
		changed |= npc.summoner_expected_slots[circle] !=
			   pet->specials.undead_spell_slots[circle];
		used->modifier = std::max(0, used->modifier + npc.summoner_expected_slots[circle] -
						     pet->specials.undead_spell_slots[circle]);
		pet->specials.undead_spell_slots[circle] =
			std::max(0, max_spells_in_circle(pet, circle) - used->modifier);
		npc.summoner_expected_slots[circle] = pet->specials.undead_spell_slots[circle];
	}
	if (changed)
		mark_resources(owner);
	summoner_pet_start_recovery(owner);
}

bool summoner_pet_recovery_blocked(P_char pet)
{
	return summoner_owned_pet(pet) &&
	       (owner_busy(GET_MASTER(pet)) || IS_FIGHTING(pet) || IS_CASTING(pet) || SINGING(pet));
}

void summoner_pet_note_command(P_char ch, int command)
{
	if (ch && IS_PC(ch) && command >= CMD_NORTH && command <= CMD_DOWN)
		ch->summoner_last_move_tick = ne_event_tick;
}

bool summoner_pet_skill(P_char pet, int command)
{
	if (!summoner_owned_pet(pet))
		return true;
	int cost = 0;
	switch (command)
	{
	case CMD_KICK:
	case CMD_RESCUE:
	case CMD_DISARM:
	case CMD_DIRTTOSS:
	case CMD_GUARD:
	case CMD_FEIGNDEATH:
	case CMD_SUBTERFUGE:
	case CMD_NECKBITE:
	case CMD_TRUE_STRIKE:
		cost = 12;
		break;
	case CMD_BASH:
	case CMD_TRIP:
	case CMD_TACKLE:
	case CMD_HEADBUTT:
	case CMD_BODYSLAM:
	case CMD_BACKSTAB:
	case CMD_CIRCLE:
	case CMD_HITALL:
	case CMD_ROUNDKICK:
	case CMD_MAUL:
	case CMD_SHIELDPUNCH:
	case CMD_SWEEPING_THRUST:
	case CMD_FLANK:
	case CMD_BEARHUG:
	case CMD_CHARGE:
	case CMD_STAMPEDE:
	case CMD_DRAGONPUNCH:
	case CMD_SPRINGLEAP:
	case CMD_LEGSWEEP:
	case CMD_HAMSTRING:
	case CMD_GARROTE:
	case CMD_COMBINATION:
	case CMD_BERSERK:
	case CMD_RAGE:
	case CMD_INFURIATE:
	case CMD_MUG:
	case CMD_SNEAKY_STRIKE:
	case CMD_HEADLOCK:
	case CMD_LEGLOCK:
	case CMD_TRAMPLE:
	case CMD_RUSH:
	case CMD_CHI:
	case CMD_CHANT:
	case CMD_WARCRY:
		cost = 24;
		break;
	case CMD_BREATH:
		// Ordered and autonomous breath share the admission check in BreathWeapon.
		return true;
	case CMD_GAZE:
	case CMD_RESTRAIN:
	case CMD_WHIRLWIND:
	case CMD_THROAT_CRUSH:
	case CMD_BLADE:
	case CMD_BARRAGE:
	case CMD_RAMPAGE:
	case CMD_DREADNAUGHT:
	case CMD_SHADOWSTEP:
	case CMD_OGRE_ROAR:
	case CMD_SHRIEK:
	case CMD_GROUNDSLAM:
	case CMD_ROAR_OF_HEROES:
	case CMD_SMITE:
	case CMD_LAYHAND:
	case CMD_DRAGON_ROAR:
	case CMD_DRAGON_BREATH:
	case CMD_DRAGON_STRIKE:
		cost = 42;
		break;
	default:
		break;
	}
	return summoner_pet_spend(pet, cost);
}

void summoner_elemental_body(P_char pet, P_char owner, bool greater, int terrain, int template_hits,
			     int template_damage)
{
	const int owner_level = GET_LEVEL(owner);
	const int level = GET_LEVEL(pet);
	const int life = std::clamp(GET_CHAR_SKILL(owner, SKILL_INFUSE_LIFE), 0, 100);
	const int charisma = GET_C_CHA(owner) + owner_level / 5;
	const double quality = std::clamp((GET_C_CHA(owner) - 60) / 70.0, 0.0, 1.0);
	const int mentalist = specialization_level(pet, owner, SPEC_MENTALIST);
	// Other captures retain the original heater benchmark. Mentalists use
	// the strongest specialized template for their elemental family.
	if (!mentalist)
	{
		template_hits = std::min(template_hits, 700);
		template_damage = std::min(template_damage, 25);
	}
	else if (GET_RACE(pet) == RACE_I_ELEMENTAL)
		template_damage = 25;
	int hp;
	if (!greater)
		hp = static_cast<int>((level / 2) * (terrain == 1 ? 6.5 : 5.5) +
				      level * (terrain == 1 ? 6 : 3) + life + charisma);
	else if (terrain == 1)
		hp = owner_level * 30 + 1 + static_cast<int>(quality * 99) + life * 4 +
		     charisma * 2;
	else if (terrain == -1)
		hp = 51 + static_cast<int>(quality * 99) + life * 2 + charisma;
	else
		hp = static_cast<int>((level > 53 ? template_hits * 2 :
				       level > 49 ? template_hits :
						    310) +
				      quality * 50 + life * 3 + charisma) *
		     66 / 100;
	int attack = greater ? (template_damage + (terrain == 1 ? 20 : 0) +
				static_cast<int>(quality * (terrain == 1 ? 10 : 5))) :
			       level / (terrain == 1 ? 2 : 3);
	if (mentalist)
	{
		const int bonus = mentalist >= 41 ? 10 : 5;
		if (!greater)
			hp = (level / 2) * 10 + level * 3 + life + charisma;
		else
		{
			// Neutral conjurer HP is a small reference contribution, not a floor.
			hp = ((level > 53 ? template_hits * 2 :
			       level > 49 ? template_hits :
					    310) +
			      50 + life * 3 + charisma) *
			     66 / 100;
			const int regular =
				(level >= 53 ? 700 : 450 + std::max(0, level - 49) * 50) + 50 +
				life * 3 + charisma;
			hp = std::max(hp, regular);
			attack = template_damage + (terrain == 1 ? 30 : 5);
		}
		const int original = normal_capture_hp(pet, owner);
		const double blended =
			std::clamp((original * 4.0 + hp) / 5.0, original * 0.90, original * 1.10);
		hp = static_cast<int>(std::lround(blended * (1.0 + 0.05 * terrain)));
		attack = (attack * (100 + bonus) + 99) / 100;
	}
	hp = std::min(hp, necro_hp_ceiling(owner));
	pet->points.base_armor = -level;
	pet->points.base_hit = GET_MAX_HIT(pet) = GET_HIT(pet) = std::max(1, hp);
	pet->only.npc->summoner_hp_ceiling = hp;
	pet->points.base_hitroll = pet->points.base_damroll = attack;
	MonkSetSpecialDie(pet);
	if (greater || terrain != 1)
		pet->points.damsizedice = static_cast<int>(pet->points.damsizedice * 0.8);
	pet->specials.affected_by = (pet->specials.affected_by & AFF_CHARM) | AFF_INFRAVISION |
				    (terrain == 1 ? AFF_HASTE : 0);
	if (GET_RACE(pet) == RACE_A_ELEMENTAL || GET_RACE(pet) == RACE_V_ELEMENTAL)
		pet->specials.affected_by |= AFF_FLY;
	if (GET_RACE(pet) == RACE_E_ELEMENTAL)
		pet->specials.affected_by |= AFF_STONE_SKIN;
	pet->specials.affected_by2 = pet->specials.affected_by3 = 0;
	pet->specials.affected_by4 &= AFF4_MULTI_CLASS;
	pet->specials.affected_by5 = 0;
	for (int i = 0; i < 10; ++i)
		pet->base_stats[i] = 100;
}

void summoner_pet_configure(P_char pet, P_char owner, bool preview, bool restoring)
{
	const uint32_t classes = pet->player.m_class;
	const int spec = pet->player.spec;
	const int old_damroll = pet->points.base_damroll;
	const int hit = GET_HIT(pet), mana = GET_MANA(pet);
	const std::array<uint64_t, 5> traits = {
		pet->specials.affected_by, pet->specials.affected_by2, pet->specials.affected_by3,
		pet->specials.affected_by4, pet->specials.affected_by5
	};
	int slots[MAX_CIRCLE + 1];
	std::copy(std::begin(pet->specials.undead_spell_slots),
		  std::begin(pet->specials.undead_spell_slots) + MAX_CIRCLE + 1, slots);
	pet->player.level = summoner_pet_level(pet, owner);
	pet->only.npc->summon_kind = static_cast<uint32_t>(summoned_pet_kind::summoner_capture);
	pet->specials.act &= ~(ACT_ELITE | ACT_IGNORE | ACT_NO_BASH | ACT_IMMUNE_TO_PARA);
	for (int i = 0; i < 10; ++i)
		pet->base_stats[i] = 100;
	if (IS_ELEMENTAL(pet))
	{
		const bool greater = GET_LEVEL(owner) >= 51 && GET_LEVEL(pet) >= 46;
		const int race = GET_RACE(pet);
		const int hp = race == RACE_E_ELEMENTAL ? 800 :
			       race == RACE_F_ELEMENTAL ? 700 :
							  600;
		const int damage = race == RACE_E_ELEMENTAL				? 30 :
				   race == RACE_W_ELEMENTAL || race == RACE_F_ELEMENTAL ? 25 :
											  20;
		summoner_elemental_body(pet, owner, greater,
					preview ? 0 : conjure_terrain_check(owner, pet), hp,
					damage);
	}
	else
	{
		const auto range = racial_range(GET_RACE(pet));
		const int level = GET_LEVEL(pet);
		const double base = 120 + 0.30 * level * level;
		const double quality = std::clamp((GET_C_CHA(owner) - 60) / 70.0, 0.0, 1.0);
		const double infusion =
			1 + std::clamp(GET_CHAR_SKILL(owner, SKILL_INFUSE_LIFE), 0, 100) / 500.0;
		const int ceiling = capture_hp_cap;
		pet->points.base_hit = std::min(
			ceiling,
			static_cast<int>(base *
					 (range.first + quality * (range.second - range.first)) *
					 role_factor(pet) * infusion));
		pet->only.npc->summoner_hp_ceiling =
			std::min(ceiling, static_cast<int>(base * range.second * role_factor(pet) *
							   infusion));
		GET_MAX_HIT(pet) = GET_HIT(pet) = pet->points.base_hit;
		pet->specials.affected_by = (pet->specials.affected_by & AFF_CHARM) |
					    AFF_INFRAVISION |
					    (IS_UNDEADRACE(pet) ? AFF_UD_VISION : 0);
		pet->specials.affected_by2 = pet->specials.affected_by3 = 0;
		pet->specials.affected_by4 &= AFF4_MULTI_CLASS;
		pet->specials.affected_by5 = 0;
		MonkSetSpecialDie(pet);
		pet->points.damnodice = pet->points.damnodice / 2 + 2;
		pet->points.damsizedice = std::max(1, (GET_LEVEL(owner) - 1) / 11);
		pet->points.base_hitroll = level * 7 / 10;
		pet->points.base_damroll = std::min(old_damroll, 100);
	}
	summoner_pet_apply_traits(pet, traits);
	blend_capture_dice(pet, owner);
	const int old_hp_ceiling = specialization_level(pet, owner, SPEC_MENTALIST) ?
					   capture_hp_cap :
					   normal_capture_hp(pet, owner);
	pet->points.base_hit = std::min(pet->points.base_hit, old_hp_ceiling);
	pet->only.npc->summoner_hp_ceiling =
		std::min(pet->only.npc->summoner_hp_ceiling, old_hp_ceiling);
	GET_MAX_HIT(pet) = GET_HIT(pet) = pet->points.base_hit;
	pet->points.base_armor = -GET_LEVEL(pet);
	pet->points.base_mana = GET_MAX_MANA(pet) = GET_MANA(pet) = mana_capacity(pet, owner);
	pet->player.m_class = classes;
	pet->player.spec = spec;
	refresh_npc_spell_slots(pet);
	for (int circle = 1; circle <= MAX_CIRCLE; ++circle)
		pet->specials.undead_spell_slots[circle] = max_spells_in_circle(pet, circle);
	if (!preview)
	{
		int slot = restoring ? pet->only.npc->summoner_resource_slot : 0;
		if (slot < 1 || slot > 4 ||
		    (pet_in_slot(owner, slot) && pet_in_slot(owner, slot) != pet))
			for (int candidate = 1; candidate <= 4; ++candidate)
				if (!pet_in_slot(owner, candidate))
				{
					slot = candidate;
					break;
				}
		pet->only.npc->summoner_resource_slot = slot;
		if (slot >= 1 && slot <= 4)
		{
			auto *debt = bank(owner, slot, mana_bank, true);
			if (restoring &&
			    mana < GET_MAX_MANA(pet) -
					    (debt->modifier + resource_scale - 1) / resource_scale)
				debt->modifier =
					(GET_MAX_MANA(pet) - std::max(0, mana)) * resource_scale;
			GET_MANA(pet) =
				std::max(0, GET_MAX_MANA(pet) - (debt->modifier + resource_scale -
								 1) / resource_scale);
			bank(owner, slot, capacity_bank, true)->modifier = GET_MAX_MANA(pet);
			for (int circle = 1; circle <= MAX_CIRCLE; ++circle)
			{
				const int capacity = max_spells_in_circle(pet, circle);
				auto *used = bank(owner, slot, circle, true);
				if (restoring)
					used->modifier =
						std::max(used->modifier, capacity - slots[circle]);
				pet->specials.undead_spell_slots[circle] =
					std::max(0, capacity - used->modifier);
			}
		}
		pet->only.npc->summoner_expected_mana = GET_MANA(pet);
		for (int circle = 1; circle <= MAX_CIRCLE; ++circle)
			pet->only.npc->summoner_expected_slots[circle] =
				pet->specials.undead_spell_slots[circle];
		if (restoring)
			GET_HIT(pet) = std::min(hit, GET_MAX_HIT(pet));
		summoned_pet_mark(pet, summoned_pet_kind::summoner_capture);
		mark_resources(owner);
		summoner_pet_start_recovery(owner);
	}
}

void summoner_pet_finish_affects(P_char pet)
{
	if (!summoner_balanced_body(pet))
		return;
	// Temporary spells sharing an intrinsic bit must not erase it on expiry.
	std::array<uint64_t, 5> intrinsic;
	std::copy(std::begin(pet->only.npc->summon_intrinsic_affects),
		  std::end(pet->only.npc->summon_intrinsic_affects), intrinsic.begin());
	for (const auto *af = pet->affected; af; af = af->next)
		if (af->type == TAG_SUPPRESS_PERM_BITS)
		{
			intrinsic[0] &= ~af->bitvector;
			intrinsic[1] &= ~af->bitvector2;
			intrinsic[2] &= ~af->bitvector3;
			intrinsic[3] &= ~af->bitvector4;
			intrinsic[4] &= ~af->bitvector5;
		}
	summoner_pet_apply_traits(pet, intrinsic);
	const int missing_hp = GET_MAX_HIT(pet) - GET_HIT(pet);
	GET_MAX_HIT(pet) = std::min<int>(GET_MAX_HIT(pet), capture_hp_cap);
	if (pet->only.npc->summoner_hp_ceiling > 0)
		GET_MAX_HIT(pet) =
			std::min<int>(GET_MAX_HIT(pet), pet->only.npc->summoner_hp_ceiling);
	GET_HIT(pet) = std::min(GET_MAX_HIT(pet) - missing_hp,
				summoner_pet_heal_cap(pet, GET_MAX_HIT(pet) * 11 / 10));
	pet->points.damroll = std::min<int>(pet->points.damroll, 100);
	if (summoner_capture(pet))
	{
		// Preview and hydration rebuild affects before establishing the owner link.
		GET_MAX_MANA(pet) = GET_MASTER(pet) ? mana_capacity(pet) : pet->points.base_mana;
		if (summoner_owned_pet(pet))
		{
			auto *debt = bank(GET_MASTER(pet), pet->only.npc->summoner_resource_slot,
					  mana_bank, false);
			if (debt)
				GET_MANA(pet) = std::max(
					0, GET_MAX_MANA(pet) - (debt->modifier + resource_scale -
								1) / resource_scale);
			pet->only.npc->summoner_expected_mana = GET_MANA(pet);
		}
		else
			GET_MANA(pet) = std::clamp<int>(GET_MANA(pet), 0, GET_MAX_MANA(pet));
	}
	// The zone/race NPC damage multiplier is world tuning, not pet training.
	pet->specials.damage_mod = 1.0;
}

bool summoner_pet_song(P_char bard, int song, bool aggressive, bool self_only, bool allies_only,
		       int room, int extra_cost)
{
	if (!summoner_owned_pet(bard))
		return true;
	bool useful = aggressive;
	for (P_char target = world[room].people; target && !useful; target = target->next_in_room)
		if ((!self_only || target == bard) &&
		    (!allies_only || target == bard || grouped(bard, target)) &&
		    (!IS_TRUSTED(target) || target == bard) && useful_song(bard, target, song))
			useful = true;
	const int cost = !useful	      ? 0 :
			 aggressive	      ? 24 :
			 song == SONG_HEALING ? 20 :
			 song == SONG_FLIGHT  ? 6 :
						10;
	if (summoner_pet_spend(bard, cost + extra_cost))
	{
		if (song == SONG_FLIGHT && useful && !bard->only.npc->summoner_flight_tick)
			bard->only.npc->summoner_flight_tick = ne_event_tick;
		return true;
	}
	stop_singing(bard);
	return false;
}

bool summoner_pet_flight_regen(P_char target, unsigned long long elapsed)
{
	P_char bard = get_linked_char(target, LNK_SONG);
	if (!summoner_owned_pet(bard) || !affected_by_spell(target, SONG_FLIGHT))
		return true;
	auto &paid_until = bard->only.npc->summoner_flight_tick;
	const auto start =
		std::max(paid_until, ne_event_tick > elapsed ? ne_event_tick - elapsed : 0);
	if (ne_event_tick <= start)
		return true;
	const int cost = static_cast<int>(std::min<unsigned long long>(ne_event_tick - start,
								       60 * WAIT_SEC)) *
			 3 * resource_scale / WAIT_SEC;
	if (!spend_scaled(bard, cost))
	{
		stop_singing(bard);
		return false;
	}
	paid_until = ne_event_tick;
	return true;
}
