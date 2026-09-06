#include "core/prototypes.h"
#include "core/structs.h"
#include "world/db.h"
#include "core/utils.h"
#include "combat/chaos_config.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "combat/damage.h"
#include "item/objmisc.h"
#include "classes/specializations.h"
#include "magic/spells.h"

extern P_index mob_index; /* for IS_SHOPKEEPER() macro */
extern struct stat_data stat_factor[];
extern float class_hitpoints[];
float hp_mob_con_factor;
float hp_mob_npc_pc_ratio;
extern P_room world;
extern struct zone_data *zone_table;
extern char *specdata[][MAX_SPEC];
extern int spl_table[TOTALLVLS][MAX_CIRCLE];

void set_npc_multi(P_char ch)
{
	int i, flag = FALSE;

	for (i = 0; i < CLASS_COUNT; i++)
	{
		if (ch->player.m_class & (1 << i))
		{
			if (flag)
			{
				ch->specials.affected_by4 |= AFF4_MULTI_CLASS;
				return;
			}

			flag = TRUE;
		}
	}

	ch->specials.affected_by4 &= ~AFF4_MULTI_CLASS;
}

/*
 * Give an NPC the spell budget of the level it is actually standing at.
 *
 * A mob's castable circles are undead_spell_slots[], and read_mobile() fills
 * them once from spl_table[] using the level in the .mob file. Nothing ever
 * refilled them, so any mob whose level changed after it was read -- the class
 * default and the two clamps in convertMob(), and every later promotion --
 * kept casting at the level it was loaded with. A kingdom guard promoted from
 * 45 to 56 never gained a single circle.
 *
 * Circle availability is (level - 1) / 5 + 1, so circle 12 opens at level 56:
 * this is what "a mob of 56 or better has its level-56 spells" means, and it
 * is the same table the player memorisation code reads. Skills need no
 * equivalent: GET_CHAR_SKILL_P() computes an NPC's percentage from its class
 * and its CURRENT level every time it is asked.
 */
void refresh_npc_spell_slots(P_char ch)
{
	if (!ch || IS_PC(ch))
		return;

	int level = GET_LEVEL(ch);

	if (level < 0)
		level = 0;
	if (level >= TOTALLVLS)
		level = TOTALLVLS - 1;

	ch->specials.undead_spell_slots[0] = 0;
	for (int circle = 1; circle <= MAX_CIRCLE; circle++)
		ch->specials.undead_spell_slots[circle] = spl_table[level][circle - 1];
}

void convertMob(P_char ch)
{
	float xp, copp, silv, gold, plat;
	int damN, damS, damA, hits, level;

	if (!ch || IS_PC(ch))
		return;

	set_npc_multi(ch);

	/* default pos of sleeping = bad */

	if ((ch->only.npc->default_pos & STAT_MASK) == STAT_SLEEPING)
		ch->only.npc->default_pos = STAT_RESTING + POS_SITTING;

	/* ok we now set size and check for exception */

	if (ch->player.size == SIZE_DEFAULT)
	{
		GET_SIZE(ch) = race_size(GET_RACE(ch));
	}
	/* end of size */

	/* an ugly hack here - instead of going through hundreds of zones
	   to assign properly newly created undead races, lets just assume
	   that everything that walks and is RACE_UNDEAD and has keywords
	   of skeleton or zombie is assigned to RACE_SKELETON or RACE_ZOMBIE*/
	if (GET_RACE(ch) == RACE_UNDEAD && isname("skeleton", GET_NAME(ch)))
		GET_RACE(ch) = RACE_SKELETON;

	if (GET_RACE(ch) == RACE_UNDEAD && isname("zombie", GET_NAME(ch)))
		GET_RACE(ch) = RACE_ZOMBIE;

	/* assign specialization */
	if (!IS_MULTICLASS_NPC(ch) && ch->player.spec == 0)
	{
		if (isname("_spec1_", GET_NAME(ch)) && *GET_SPEC_NAME(ch->player.m_class, 0))
		{
			ch->player.spec = 1;
		}
		else if (isname("_spec2_", GET_NAME(ch)) && *GET_SPEC_NAME(ch->player.m_class, 1))
		{
			ch->player.spec = 2;
		}
		else if (isname("_spec3_", GET_NAME(ch)) && *GET_SPEC_NAME(ch->player.m_class, 2))
		{
			ch->player.spec = 3;
		}
		else if (isname("_spec4_", GET_NAME(ch)) && *GET_SPEC_NAME(ch->player.m_class, 3))
		{
			ch->player.spec = 4;
		}
		/* No spec means no spec.  Not random spec!
		    else if( !isname("_nospec_", GET_NAME(ch)) &&
		             GET_LEVEL(ch) > number(20,50) )
		    {
		      int i = number(0,MAX_SPEC-1);
		      if( *GET_SPEC_NAME(ch->player.m_class, i) &&
		          is_allowed_race_spec(GET_RACE(ch), ch->player.m_class, i+1) )
		      {
		        ch->player.spec = i+1;
		      }
		    }
		*/
	}

	/* for guild golem and mob that should not move set cover
	   so they don't get attack by range */

	if (strstr(ch->player.name, "assoc"))
		SET_BIT(ch->specials.affected_by3, AFF3_COVER);
	else if (strstr(ch->player.name, "_no_move_"))
	{
		SET_BIT(ch->specials.act2, ACT2_NO_LURE);
		SET_BIT(ch->specials.affected_by3, AFF3_COVER);
	}
	else if (IS_SET(ch->specials.act, ACT_SENTINEL))
		SET_BIT(ch->specials.affected_by3, AFF3_COVER);

	/* some mobs, we do NOT convert! */
	if (IS_SET(ch->specials.act, ACT_IGNORE) || strstr(ch->player.name, "_ignore_"))
	{
		ch->points.hit = ch->points.max_hit = ch->points.base_hit =
			MAX(1, ch->points.base_hit / 4);
		affect_total(ch, FALSE);
		return;
	}

	if ((ch->player.m_class == 0) && (GET_LEVEL(ch) >= 15))
		ch->player.m_class = CLASS_WARRIOR;

	// Minimum mob level
	if (GET_LEVEL(ch) < 1)
		ch->player.level = 1;
	// Max mob level
	if (GET_LEVEL(ch) > MAXLVL)
		ch->player.level = MAXLVL;
	level = GET_LEVEL(ch);

	/* Re-stamp the spell budget for the level this mob actually ended up at.
	 * read_mobile() takes the snapshot from the .mob file's level, which is
	 * right for a mob nobody moves, but the class default above and either
	 * clamp can change the level after it -- and anything that promotes a mob
	 * later leaves it casting at the level it was loaded with. */
	refresh_npc_spell_slots(ch);

	xp = copp = silv = gold = plat = 0;

	/* find multipliers for mob xp/money */
	if (GET_LEVEL(ch) > 50)
	{
		//    GET_EXP(ch) = (int) (GET_LEVEL(ch) * 2500);
		xp = 4000;
		copp = 0;
		silv = 0;
		gold = .4292;
		plat = .0950;
	}
	else if (GET_LEVEL(ch) > 40)
	{
		xp = 1500;
		copp = 0;
		silv = 0;
		gold = .3637;
		plat = .0667;
	}
	else if (GET_LEVEL(ch) > 30)
	{
		xp = 800;
		copp = 0;
		silv = 0;
		gold = .2857;
		plat = .0500;
	}
	else if (GET_LEVEL(ch) > 20)
	{
		xp = 550;
		copp = .6667;
		silv = .4546;
		gold = .2223;
		plat = .0400;
	}
	else if (GET_LEVEL(ch) > 10)
	{
		xp = 300;
		copp = .5000;
		silv = .4000;
		gold = .1667;
		plat = 0.0;
	}
	else
	{
		xp = 80;
		copp = .4000;
		silv = .3334;
		gold = 0.0;
		plat = 0.0;
	}

	if (IS_ACT(ch, ACT_BREATHES_FIRE | ACT_BREATHES_LIGHTNING | ACT_BREATHES_FROST |
			       ACT_BREATHES_ACID | ACT_BREATHES_GAS | ACT_BREATHES_SHADOW |
			       ACT_BREATHES_BLIND_GAS))
	{
		xp *= 2;
	}

	/* apply multipliers */
	GET_EXP(ch) = (int)(level * xp);
	GET_PLATINUM(ch) = (int)(level * plat * number(75, 125) / 100);
	GET_GOLD(ch) = (int)(level * gold * number(75, 125) / 100);
	GET_SILVER(ch) = (int)(level * silv * number(75, 125) / 100);
	GET_COPPER(ch) = (int)(level * copp * number(75, 125) / 100);

	// EXP modifiers are found in limits.c in gain_exp().

	/* handle special situations for special races in regards to
	   money */
	if (IS_GREATER_RACE(ch) || IS_ELITE(ch))
		GET_PLATINUM(ch) *= 4;

	/* finally, rarefy valuable platinum */
	/* make sure they get at least 1 coin... */
	if (!GET_MONEY(ch))
		GET_COPPER(ch) = 1;

	if ((GET_RACE(ch) == RACE_F_ELEMENTAL) || (GET_RACE(ch) == RACE_A_ELEMENTAL) ||
	    (GET_RACE(ch) == RACE_W_ELEMENTAL) || (GET_RACE(ch) == RACE_E_ELEMENTAL) ||
	    (GET_RACE(ch) == RACE_V_ELEMENTAL) || (GET_RACE(ch) == RACE_I_ELEMENTAL) ||
	    IS_UNDEADRACE(ch) || (GET_RACE(ch) == RACE_INSECT) || (GET_RACE(ch) == RACE_REPTILE) ||
	    (GET_RACE(ch) == RACE_SNAKE) || (GET_RACE(ch) == RACE_ARACHNID) ||
	    (GET_RACE(ch) == RACE_AQUATIC_ANIMAL) || (GET_RACE(ch) == RACE_FLYING_ANIMAL) ||
	    (GET_RACE(ch) == RACE_QUADRUPED) || (GET_RACE(ch) == RACE_ANIMAL) ||
	    (GET_RACE(ch) == RACE_PLANT) || (GET_RACE(ch) == RACE_HERBIVORE) ||
	    (GET_RACE(ch) == RACE_CARNIVORE) || (GET_RACE(ch) == RACE_PARASITE) ||
	    (GET_RACE(ch) == RACE_SLIME) || (GET_RACE(ch) == RACE_CONSTRUCT) ||
	    (GET_RACE(ch) == RACE_GOLEM) || (isname("_nomoney_", GET_NAME(ch))))
		GET_PLATINUM(ch) = GET_GOLD(ch) = GET_SILVER(ch) = GET_COPPER(ch) = 0;

	/* adjust for mana */
	if (GET_CLASS(ch, CLASS_PSIONICIST))
	{
		// 30 : double, 45 : triple, 60 : quadruple mana. - This helps with cannibalize spell.
		ch->points.mana = ch->points.base_mana = ch->points.max_mana =
			level * 25 * (level > 29 ? level / 15 : 1);
	}
	else
	{
		ch->points.mana = ch->points.base_mana = ch->points.max_mana =
			level * 10 * (level > 29 ? level / 15 : 1);
	}

	/* thac0 */
	if (IS_MELEE_CLASS(ch) || IS_DRAGON(ch) || IS_DEMON(ch) || IS_GIANT(ch))
		ch->points.base_hitroll = BOUNDED(2, (level / 2), 35);
	else
		ch->points.base_hitroll = BOUNDED(0, (level / 3), 25);

	ch->points.hitroll = ch->points.base_hitroll;

	/* AC computations... first base AC */
	ch->points.base_armor = (int)(level * -3 + 100);

	/* then additions based on level... */
	if (level > 57)
		ch->points.base_armor -= 150;
	else if (level > 49)
		ch->points.base_armor -= 100;
	else if (level > 40)
		ch->points.base_armor -= 50;

	/* racial conditions to AC */
	switch (GET_RACE(ch))
	{
	case RACE_F_ELEMENTAL:
	case RACE_W_ELEMENTAL:
	case RACE_A_ELEMENTAL:
	case RACE_E_ELEMENTAL:
	case RACE_V_ELEMENTAL:
	case RACE_GHOST:
	case RACE_DRAGONKIN:
		ch->points.base_armor -= 50;
		break;
	case RACE_GIANT:
		ch->points.base_armor += 10;
		break;
	case RACE_DEMON:
	case RACE_DEVIL:
		ch->points.base_armor -= 100;
		break;
	case RACE_DRAGON:
	case RACE_I_ELEMENTAL:
		ch->points.base_armor -= 150;
		break;
	}
	ch->points.base_armor = BOUNDED(-750, ch->points.base_armor, 100);

	/* hitpoints, and damage dice */

	damN = damS = damA = 0;

	if (IS_ELITE(ch))
	{
		damN = 9;
		damS = 9;
		damA = 45;
	}
	else if (level <= 5)
	{
		damN = 1;
		damS = 5;
		damA = 0;
	}
	else if (level <= 10)
	{
		damN = 2;
		damS = 3;
		damA = 1;
	}
	else if (level <= 15)
	{
		damN = 3;
		damS = 3;
		damA = 5;
	}
	else if (level <= 20)
	{
		damN = 4;
		damS = 4;
		damA = 10;
	}
	else if (level <= 25)
	{
		damN = 5;
		damS = 5;
		damA = 10;
	}
	else if (level <= 30)
	{
		damN = 5;
		damS = 5;
		damA = 15;
	}
	else if (level <= 35)
	{
		damN = 6;
		damS = 6;
		damA = 20;
	}
	else if (level <= 40)
	{
		damN = 6;
		damS = 6;
		damA = 25;
	}
	else if (level <= 45)
	{
		damN = 6;
		damS = 7;
		damA = 30;
	}
	else if (level <= 50)
	{
		damN = 7;
		damS = 7;
		damA = 35;
	}
	else if (level <= 55)
	{
		damN = 8;
		damS = 9;
		damA = 40;
	}
	else
	{
		damN = 9;
		damS = 9;
		damA = 45;
	}

	if (IS_MELEE_CLASS(ch))
	{
		damN += GET_LEVEL(ch) / 15;
	}

	ch->points.base_damroll = ch->points.damroll = damA + GET_LEVEL(ch) / 2;
	ch->points.damnodice = damN;
	ch->points.damsizedice = damS;

	/* this formula calculates mob hitpoints with an assumption
	 * hp_mob_npc_pc_ratio provided in property hitpoints.mob.NpcPcRatio
	 * tells us how many times more hps should have a 50 level mob
	 * with con 100 in comparison to a PC. hp_mob_con_factor provided
	 * in property hp_mob_con_factor ranging from 0 to 1 tells us how much
	 * racial con affects hitpoints. when it's 0, then all mob races have
	 * same hitpoints, when it's 1 then differences are as big as for the
	 * PC races, so ogres having twice human and nearly four times drow
	 * hitpoints etc. the goal was to make it intuitively adjustable via
	 * two provided properties. /tharkun
	 */
	hits = (int)((0.00000045 *
			      (stat_factor[GET_RACE(ch)].Con * stat_factor[GET_RACE(ch)].Con *
				       hp_mob_con_factor +
			       100 * 100 * (1 - hp_mob_con_factor)) *
			      level * level +
		      2) *
		     level * hp_mob_npc_pc_ratio);

	hits -= (int)(0.5 * hits * (1.0 - class_hitpoints[flag2idx(ch->player.m_class)]));

	/*
	 * A chaos mud runs mobs at a tenth of their hitpoints.
	 *
	 * This was `hits * (1 / 10)`, and `1 / 10` is integer division: it is 0,
	 * so every mob on a chaos server was built with 0 hitpoints and handed
	 * the one the line below added. Every mob in the game had 1 hit point.
	 * The divide is now the integer divide it always meant to be, and the
	 * floor is only what a divide needs -- a level-1 mob rounding to nothing
	 * still has to be alive.
	 */
	if (chaos_mud_enabled())
	{
		hits /= 10;
		if (hits < 1)
			hits = 1;
	}

	ch->points.base_hit = hits;
	ch->points.hit = ch->points.max_hit = ch->points.base_hit;
	ch->only.npc->lowest_hit = INT_MAX;
	ch->points.base_vitality = dice(5, 10) + 80;
	ch->points.vitality = ch->points.base_vitality = ch->points.max_vitality;

	damA += damN * (1 + damS) / 2;

	damN = MIN(damA, 90);

	if (damA > damN)
	{
		damA = damN;
		ch->points.base_damroll = ch->points.damroll = damA / 3 + GET_LEVEL(ch) / 2;
		damA -= ch->points.base_damroll;
		ch->points.damsizedice = 7;
		ch->points.damnodice = MAX(1, damA / 4);
	}

	ch->curr_stats = ch->base_stats;

	/* if they don't have memory, and we thing they should, give it to
	   them */
	if (!IS_SET(ch->specials.act, ACT_MEMORY) && !IS_ANIMAL(ch) && !IS_INSECT(ch) &&
	    (GET_RACE(ch) != RACE_PLANT))
	{
		SET_BIT(ch->specials.act, ACT_MEMORY);
	}

	/* druids are neutral aligned... */
	if (GET_CLASS(ch, CLASS_DRUID) ||
	    (IS_MULTICLASS_PC(ch) && GET_SECONDARY_CLASS(ch, CLASS_DRUID)))
		GET_ALIGNMENT(ch) = BOUNDED(-349, GET_ALIGNMENT(ch), 349);

	/* paladins and rangers are good aligned */
	else if (GET_CLASS(ch, CLASS_PALADIN) || GET_CLASS(ch, CLASS_RANGER))
		GET_ALIGNMENT(ch) = MAX(750, GET_ALIGNMENT(ch));

	/* anti paladins are evil aligned */
	else if (GET_CLASS(ch, CLASS_ANTIPALADIN))
		GET_ALIGNMENT(ch) = MIN(-1000, GET_ALIGNMENT(ch));

	/* necro's are evil aligned (undead spells won't work otherwise) */
	else if (GET_CLASS(ch, CLASS_NECROMANCER))
		GET_ALIGNMENT(ch) = MIN(-1000, GET_ALIGNMENT(ch));

	if (GET_ALIGNMENT(ch) > 1000)
		GET_ALIGNMENT(ch) = 1000;
	if (GET_ALIGNMENT(ch) < -1000)
		GET_ALIGNMENT(ch) = -1000;

	/* horses get ridden.. */
	if (((GET_RACE(ch) == RACE_QUADRUPED) || isname("horse", GET_NAME(ch))) &&
	    (GET_RACE(ch) != RACE_UNDEAD) && !IS_SET(ch->specials.act, ACT_MOUNT) &&
	    !IS_SET(ch->only.npc->aggro_flags, AGGR_ALL) && GET_LEVEL(ch) < 26)
		SET_BIT(ch->specials.act, ACT_MOUNT);

	if (IS_SET(ch->specials.act, ACT_MOUNT))
	{
		GET_MAX_VITALITY(ch) *= 2;
		GET_MAX_VITALITY(ch) += 50;
		ch->points.vitality = ch->points.base_vitality = ch->points.max_vitality;
	}

	if (IS_SHOPKEEPER(ch) || has_quest(ch))
	{
		REMOVE_BIT(ch->specials.act, ACT_SCAVENGER);
	}
	/* thieves and neutral folks pick up stuff laying around... */
	else if ((GET_ALIGNMENT(ch) == 0 || isname("thief", GET_NAME(ch)) ||
		  GET_CLASS(ch, CLASS_ROGUE | CLASS_THIEF)) &&
		 IS_HUMANOID(ch))
	{
		SET_BIT(ch->specials.act, ACT_SCAVENGER);
	}

	/* guards are protectors... */
	if (strstr(ch->player.name, "guard") || (strstr(ch->player.name, "militia")))
		SET_BIT(ch->specials.act, ACT_PROTECTOR);

	/* now that the STONE_SKIN affect does something without the spell
	   being cast, need to make sure area builders didn't use it
	   improperly... so... */
	if (IS_AFFECTED(ch, AFF_STONE_SKIN) &&
	    !((GET_LEVEL(ch) > 39) &&
	      ((GET_RACE(ch) == RACE_GOLEM) || (GET_RACE(ch) == RACE_CONSTRUCT) ||
	       isname("iron", GET_NAME(ch)) || isname("stone", GET_NAME(ch)))))
	{
		REMOVE_BIT(ch->specials.affected_by, AFF_STONE_SKIN);
	}

	/* earth elems get perm stoneskin */
	if (GET_RACE(ch) == RACE_E_ELEMENTAL)
	{
		SET_BIT(ch->specials.affected_by, AFF_STONE_SKIN);
	}

	/* remove ALL affects that don't belong! */
	REMOVE_BIT(ch->specials.affected_by,
		   /*           AFF_BLIND |*/
		   AFF_KNOCKED_OUT | AFF_BOUND | AFF_CHARM | AFF_FEAR | AFF_MEDITATE | AFF_CAMPING |
			   AFF_SLEEP);

	REMOVE_BIT(ch->specials.affected_by2,
		   AFF2_MINOR_PARALYSIS | AFF2_MAJOR_PARALYSIS | AFF2_POISONED | AFF2_SILENCED |
			   AFF2_STUNNED | AFF2_HOLDING_BREATH | AFF2_MEMORIZING | AFF2_IS_DROWNING |
			   AFF2_CASTING | AFF2_SCRIBING | AFF2_HUNTER);

	REMOVE_BIT(ch->specials.affected_by3, AFF3_TRACKING | AFF3_FAMINE | AFF3_SWIMMING);
	REMOVE_BIT(ch->specials.affected_by4, AFF4_SACKING);
	REMOVE_BIT(ch->specials.affected_by5, AFF5_IMPRISON | AFF5_MEMORY_BLOCK);

	if (IS_SET(ch->specials.act, ACT_ELITE))
	{
		give_proper_stat(ch);
		ch->points.hit = ch->points.max_hit = ch->points.base_hit =
			(int)(ch->points.hit * get_property("hitpoints.mob.eliteBonus", 2.5));
		ch->points.damnodice =
			(int)(get_property("damage.eliteBonus", 1.2) * ch->points.damnodice);
		GET_EXP(ch) = (int)(GET_EXP(ch) * get_property("hitpoints.mob.eliteBonus", 2.5) *
				    get_property("damage.eliteBonus", 1.2));
	}

	affect_total(ch, FALSE);
}

// apply zone difficulty modifiers
// intended to be called only once, right after mob is loaded and has birthplace set
void apply_zone_modifier(P_char ch)
{
	int difficulty =
		BOUNDED(1, zone_table[world[real_room0(GET_BIRTHPLACE(ch))].zone].difficulty, 10);

	if (difficulty == 1)
		return;

	float hit_mod =
		1.0 + ((float)get_property("hitpoints.zoneDifficulty.factor", 0.500) * difficulty);
	GET_MAX_HIT(ch) = GET_HIT(ch) = ch->points.base_hit = (int)(ch->points.base_hit * hit_mod);

	float exp_mod =
		1.0 + ((float)get_property("exp.zoneDifficulty.factor", 0.500) * difficulty);
	GET_EXP(ch) = (int)(GET_EXP(ch) * exp_mod);

	float damage_mod_mod =
		1.0 + ((float)get_property("damage.zoneDifficulty.mod.factor", 0.200) * difficulty);
	ch->specials.damage_mod = (float)(ch->specials.damage_mod * damage_mod_mod);
}

int GetFormType(P_char ch)
{
	switch (GET_RACE(ch))
	{
	case RACE_HUMAN:
	case RACE_BARBARIAN:
	case RACE_DROW:
	case RACE_GREY:
	case RACE_MOUNTAIN:
	case RACE_DUERGAR:
	case RACE_HALFLING:
	case RACE_GNOME:
	case RACE_ORC:
	case RACE_THRIKREEN:
	case RACE_CENTAUR:
	case RACE_GITHYANKI:
	case RACE_SHADE:
	case RACE_MINOTAUR:
	case RACE_HALFELF:
	case RACE_GOBLIN:
	case RACE_HALFORC:
	case RACE_ELADRIN:
	case RACE_FAERIE:
	case RACE_UNDEAD:
	case RACE_LICH:
	case RACE_PVAMPIRE:
	case RACE_PSBEAST:
	case RACE_PDKNIGHT:
	case RACE_VAMPIRE:
	case RACE_GHOST:
	case RACE_AQUATIC_ANIMAL:
	case RACE_FLYING_ANIMAL:
	case RACE_HUMANOID:
	case RACE_HERBIVORE:
		return MSG_HIT;
		break;
	case RACE_ILLITHID:
		return MSG_WHIP;
		break;
	case RACE_OGRE:
	case RACE_TROLL:
	case RACE_GOLEM:
	case RACE_PRIMATE:
	case RACE_SGIANT:
	case RACE_FIRBOLG:
	case RACE_SNOW_OGRE:
		return MSG_MAUL;
		break;
	case RACE_F_ELEMENTAL:
	case RACE_A_ELEMENTAL:
	case RACE_W_ELEMENTAL:
	case RACE_E_ELEMENTAL:
	case RACE_EFREET:
	case RACE_DEMON:
	case RACE_GIANT:
	case RACE_DEVIL:
	case RACE_PLANT:
	case RACE_CONSTRUCT:
		return MSG_CRUSH;
		break;
	case RACE_LYCANTH:
	case RACE_DRAGON:
	case RACE_DRACOLICH:
	case RACE_DRAGONKIN:
	case RACE_REPTILE:
	case RACE_SKELETON:
	case RACE_ZOMBIE:
	case RACE_REVENANT:
	case RACE_SPECTRE:
		return MSG_CLAW;
		break;
	case RACE_QUADRUPED:
		return MSG_THRASH;
		break;
	case RACE_SNAKE:
	case RACE_CARNIVORE:
	case RACE_PARASITE:
	case RACE_ANIMAL:
	case RACE_BEHOLDER:
	case RACE_PWORM:
		return MSG_BITE;
		break;
	case RACE_INSECT:
	case RACE_ARACHNID:
		return MSG_STING;
		break;
	default:
		return MSG_HIT;
	}
}
