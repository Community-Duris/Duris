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
constexpr int bank_width = 16;
constexpr int mana_bank = 13;
constexpr int capacity_bank = 14;
constexpr int resource_scale = 1000;

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

int mana_capacity(P_char pet)
{
	return GET_LEVEL(pet) * (IS_ELEMENTAL(pet) ? 4 : GET_CLASS(pet, CLASS_BARD) ? 6 : 8);
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
	case RACE_SNOW_OGRE:
	case RACE_FIREGIANT:
	case RACE_FROSTGIANT:
	case RACE_TITAN:
	case RACE_AVATAR:
		return { 2.10, 2.60 };
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
	return summoner_capture(pet) ||
	       (pet && IS_NPC(pet) &&
		pet->only.npc->summon_kind ==
			static_cast<uint32_t>(summoned_pet_kind::conjurer_elemental));
}

bool summoner_owned_pet(P_char pet)
{
	return summoner_capture(pet) && IS_PC_PET(pet);
}

void summoner_pet_start_recovery(P_char owner)
{
	if (owner && IS_PC(owner) && !get_scheduled(owner, event_summoner_recovery))
		add_event(event_summoner_recovery, 6 * WAIT_SEC, owner, nullptr, nullptr, 0,
			  nullptr, 0);
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
	const int cap = greater ? (owner_level >= 56 ? 55 : 53) : 45;
	pet->player.level = std::min<int>(GET_LEVEL(pet), cap);
	const int level = GET_LEVEL(pet);
	const int life = std::clamp(GET_CHAR_SKILL(owner, SKILL_INFUSE_LIFE), 0, 100);
	const int charisma = GET_C_CHA(owner) + owner_level / 5;
	const double quality = std::clamp((GET_C_CHA(owner) - 60) / 70.0, 0.0, 1.0);
	// Heater is the common upper body benchmark for greater elementals.
	template_hits = std::min(template_hits, 700);
	template_damage = std::min(template_damage, 25);
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
	hp = std::min(hp, necro_hp_ceiling(owner));
	pet->points.base_armor = -level;
	pet->points.base_hit = GET_MAX_HIT(pet) = GET_HIT(pet) = std::max(1, hp);
	pet->only.npc->summoner_hp_ceiling = hp;
	pet->points.base_hitroll = pet->points.base_damroll =
		greater ? (template_damage + (terrain == 1 ? 20 : 0) +
			   static_cast<int>(quality * (terrain == 1 ? 10 : 5))) :
			  level / (terrain == 1 ? 2 : 3);
	MonkSetSpecialDie(pet);
	if (greater || terrain != 1)
		pet->points.damsizedice = static_cast<int>(pet->points.damsizedice * 0.8);
	pet->specials.affected_by = (pet->specials.affected_by & AFF_CHARM) | AFF_INFRAVISION |
				    (terrain == 1 ? AFF_HASTE : 0);
	GET_SIZE(pet) = !greater || terrain == -1 ? SIZE_MEDIUM :
			terrain == 1		  ? SIZE_HUGE :
						    SIZE_LARGE;
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
	const int hit = GET_HIT(pet), mana = GET_MANA(pet);
	int slots[MAX_CIRCLE + 1];
	std::copy(std::begin(pet->specials.undead_spell_slots),
		  std::begin(pet->specials.undead_spell_slots) + MAX_CIRCLE + 1, slots);
	pet->player.level =
		std::min<int>(GET_LEVEL(pet), std::min<int>(GET_LEVEL(owner), MAXLVLMORTAL));
	pet->only.npc->summon_kind = static_cast<uint32_t>(summoned_pet_kind::summoner_capture);
	pet->specials.act &=
		~(ACT_ELITE | ACT_IGNORE | ACT_NO_BASH | ACT_IMMUNE_TO_PARA | ACT_BREATHES_FIRE |
		  ACT_BREATHES_LIGHTNING | ACT_BREATHES_FROST | ACT_BREATHES_ACID |
		  ACT_BREATHES_GAS | ACT_BREATHES_SHADOW | ACT_BREATHES_BLIND_GAS);
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
		const int ceiling = necro_hp_ceiling(owner);
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
		pet->points.base_damroll = level * 55 / 100;
	}
	pet->points.base_armor = -GET_LEVEL(pet);
	pet->points.base_mana = GET_MAX_MANA(pet) = GET_MANA(pet) = mana_capacity(pet);
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
	const int missing_hp = GET_MAX_HIT(pet) - GET_HIT(pet);
	if (pet->only.npc->summoner_hp_ceiling > 0)
		GET_MAX_HIT(pet) =
			std::min<int>(GET_MAX_HIT(pet), pet->only.npc->summoner_hp_ceiling);
	GET_HIT(pet) = GET_MAX_HIT(pet) - missing_hp;
	if (summoner_capture(pet))
	{
		GET_MAX_MANA(pet) = mana_capacity(pet);
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
		return true;
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
