/* Shared combat damage support and output helpers. */
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/safe_format.h"
#include "classes/paladins.h"
#include "classes/summoner_pet.h"
#include "combat/damage.h"
#include "combat/dam_mods.h"
#include "net/comm.h"
#include "net/output_style.h"
#include "magic/spells.h"
#include "telemetry/telemetry_runtime.h"
#include "world/achievements.h"
#include "world/db.h"
#include "world/events.h"
#include "world/epic.h"
#include "world/hardcore_config.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

extern P_room world;
extern P_char get_dragoon_mount(P_char ch);

struct affected_type *get_ward_from_char(P_char ch)
{
	return get_first_affect_with_flag(ch, AFFTYPE_DAM_WARD);
}

int check_damage_ward(P_char attacker, P_char ch, int dam)
{
	struct affected_type *paf = get_ward_from_char(ch);
	int absorbed = 0;
	float wardMitigation = get_property("ward.mitigation", 0.80);
	dam = (int)ceil(dam * wardMitigation);

	// loop through all wards
	while (paf && absorbed < dam)
	{
		paf->modifier -= (dam - absorbed);
		if (paf->modifier < 0)
		{
			absorbed += (dam + paf->modifier);
			wear_off_message(ch, paf);
			affect_remove(ch, paf);
		}
		else
		{
			absorbed += dam;
		}

		paf = get_ward_from_char(ch);

		act("&+CThe ward around you flashes briefly as it absorbs $n&+C's assault!&n",
		    FALSE, attacker, 0, ch, TO_VICT | ACT_NOTTERSE);
		act("&+CThe ward around&n $N&+C flashes briefly as it absorbs your assault!&n",
		    FALSE, attacker, 0, ch, TO_CHAR | ACT_NOTTERSE);
		act("&+CThe ward around&n $N&+C flashes briefly as it absorbs &n$n&+C's assault!&n",
		    FALSE, attacker, 0, ch, TO_NOTVICT | ACT_NOTTERSE);
	}

	// check for innate wards
	if (GET_WARD(ch) > 0)
	{
		GET_WARD(ch) -= (dam - absorbed);
		if (GET_WARD(ch) < 0)
		{
			absorbed += (dam + GET_WARD(ch));
			GET_WARD(ch) = 0;
		}
		else
		{
			absorbed += (dam - absorbed);
		}

		act("&+CThe ward around you flashes briefly as it absorbs $n&+C's assault!&n",
		    FALSE, attacker, 0, ch, TO_VICT | ACT_NOTTERSE);
		act("&+CThe ward around&n $N&+C flashes briefly as it absorbs your assault!&n",
		    FALSE, attacker, 0, ch, TO_CHAR | ACT_NOTTERSE);

		if (GET_CHAR_SKILL(ch, SKILL_EPIC_WARDING_FAITH) > 0)
			notch_skill(ch, SKILL_EPIC_WARDING_FAITH, 2);
	}

	return (int)ceil(absorbed / wardMitigation);
}

void dam_message(double fdam, P_char ch, P_char victim, struct damage_messages *messages)
{
	int dam = (int)fdam;
	const auto role = fdam > 0 ? OutputRole::Hit : OutputRole::Miss;
	char buf_char[160], buf_vict[160], buf_notvict[160];
	int w_percent, h_percent, max_dam = 0, w_loop, h_loop;
	int msg_flags = messages->type;
	static int dam_ref[] = { 0, 2, 7, 10, 15, 25, 40, 55, 70, 85, 9999 };
	const char *weapon_damage[] = {
		"",	     " feeble", " weak",    " crude",	" decent", " fine", " impressive",
		" powerful", " mighty", " awesome", " amazing",
	};

	const char *victim_damage[] = { "grazes $w",
					"wounds $w",
					"strikes $w",
					"strikes $w hard",
					"strikes $w very hard",
					"seriously wounds $w",
					"enshrouds $w in a mist of blood",
					"causes $w to grimace in pain",
					"grievously wounds $w",
					"critically injures $w",
					"hits $w" };

	const char *victim_damage2[] = { "graze $w",
					 "wound $w",
					 "strike $w",
					 "strike $w hard",
					 "strike $w very hard",
					 "seriously wound $w",
					 "cause $w to grimace in pain",
					 "enshroud $w in a mist of blood",
					 "grievously wound $w",
					 "critically injure $w",
					 "hit $w" };

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
		return;

	if (ch->equipment[WIELD])
	{
		max_dam = ch->equipment[WIELD]->value[1] * ch->equipment[WIELD]->value[2];
	}
	// else if(messages->obj)
	// {
	// wield = messages->obj;

	// if((messages->obj->value[1] * messages->obj->value[2]) >= 0)
	// max_dam = messages->obj->value[1] * messages->obj->value[2];
	// else
	// max_dam = 0;
	// }
	else
	{
		max_dam = ch->points.damnodice * ch->points.damsizedice;
	}

	h_percent = BOUNDED(0, (int)((dam * 100) / (GET_HIT(victim) + dam + 10)), 100);
	w_percent = BOUNDED(0, (int)((dam * 100) / (max_dam + dam + 10)), 100);

	for (h_loop = 0; (h_percent > dam_ref[h_loop]); h_loop++)
		;
	if (h_loop > 10)
		h_loop = 10; /* h and w require reverse. dont ask why */
	for (w_loop = 0; (w_percent > dam_ref[w_loop]); w_loop++)
		;
	if (w_loop > 10)
		w_loop = 0;

	if (!number(0, 3))
		w_loop = 0;

	/*
	  char showdam[MAX_STRING_LENGTH];
	  snprintf(showdam, MAX_STRING_LENGTH, " [&+wDamage: %d&n] ", dam);
	*/
	if (msg_flags & DAMMSG_HIT_EFFECT)
	{
		checked_snprintf_runtime(buf_char, 160, messages->attacker, weapon_damage[w_loop],
					 victim_damage[h_loop]);
		checked_snprintf_runtime(buf_vict, 160, messages->victim, weapon_damage[w_loop],
					 victim_damage[h_loop]);
		checked_snprintf_runtime(buf_notvict, 160, messages->room, weapon_damage[w_loop],
					 victim_damage[h_loop]);
	}
	else if (msg_flags & DAMMSG_EFFECT_HIT)
	{
		checked_snprintf_runtime(buf_char, 160, messages->attacker, victim_damage2[h_loop],
					 weapon_damage[w_loop]);
		checked_snprintf_runtime(buf_vict, sizeof buf_vict, messages->victim,
					 victim_damage[h_loop], weapon_damage[w_loop]);
		checked_snprintf_runtime(buf_notvict, sizeof buf_notvict, messages->room,
					 victim_damage[h_loop], weapon_damage[w_loop]);
	}
	else if ((msg_flags & DAMMSG_EFFECT))
	{
		checked_snprintf_runtime(buf_char, sizeof buf_char, messages->attacker,
					 victim_damage[h_loop]);
		checked_snprintf_runtime(buf_vict, sizeof buf_vict, messages->victim,
					 victim_damage[h_loop]);
		checked_snprintf_runtime(buf_notvict, sizeof buf_notvict, messages->room,
					 victim_damage[h_loop]);
	}
	else if (msg_flags & DAMMSG_HIT)
	{
		checked_snprintf_runtime(buf_char, sizeof buf_char, messages->attacker,
					 weapon_damage[w_loop]);
		checked_snprintf_runtime(buf_vict, sizeof buf_vict, messages->victim,
					 weapon_damage[w_loop]);
		checked_snprintf_runtime(buf_notvict, sizeof buf_notvict, messages->room,
					 weapon_damage[w_loop]);
	}
	/* if (IS_PC(ch) && IS_SET(ch->specials.act2, PLR2_DAMAGE) )
	   strcat(buf_char, showdam);*/

#if ENABLE_TERSE
	act(buf_notvict, FALSE, ch, messages->obj, victim, TO_NOTVICTROOM | ACT_NOTTERSE,
	    recipient_role_context(OutputChannel::CombatObserved, role));
#else
	act(buf_notvict, FALSE, ch, messages->obj, victim, TO_NOTVICTROOM,
	    recipient_role_context(OutputChannel::CombatObserved, role));
#endif

	if (IS_PC(ch) && !IS_SET(ch->specials.act2, PLR2_BATTLEALERT) &&
	    !IS_SET(ch->specials.act2, PLR2_TERSE))
	{
		strcat(buf_char, "&+G]=-&N");
		send_to_char("&+G-=[&N", ch);
	}
#if ENABLE_TERSE
	act(buf_char, FALSE, ch, messages->obj, victim, TO_CHAR | ACT_NOTTERSE,
	    recipient_role_context(OutputChannel::CombatOutgoing, role));
#else
	act(buf_char, FALSE, ch, messages->obj, victim, TO_CHAR,
	    recipient_role_context(OutputChannel::CombatOutgoing, role));
#endif

	if (IS_PC(victim) && !IS_SET(victim->specials.act2, PLR2_BATTLEALERT) &&
	    !IS_SET(victim->specials.act2, PLR2_TERSE))
	{
		strcat(buf_vict, "&+R]=-&N");
		send_to_char("&+R-=[&N", victim);
	}
#if ENABLE_TERSE
	act(buf_vict, FALSE, ch, messages->obj, victim, TO_VICT | ACT_NOTTERSE,
	    recipient_role_context(OutputChannel::CombatIncoming, role));
#else
	act(buf_vict, FALSE, ch, messages->obj, victim, TO_VICT,
	    recipient_role_context(OutputChannel::CombatIncoming, role));
#endif
}

double orc_horde_dam_modifier(P_char ch, double dam, int attacking)
{
	P_char horde, next;
	float c = 0.00;

	if (!ch)
		return dam;

	if (ch->in_room < 1)
		return dam;

	for (horde = world[ch->in_room].people; horde; horde = next)
	{
		next = horde->next_in_room;

		if (attacking)
		{
			if (GET_RACE(horde) == RACE_ORC && ch != horde)
				c++;
		}
		else
		{
			if (GET_RACE(horde) == RACE_ORC)
				c--;
		}
	}

	// Remove ourself
	if (!attacking)
		c++;

	c *= get_property("orc.horde.bonus.modifier", 1.5);

	if (attacking)
		return (dam * (1.0 + (c / 100.00)));
	else
		return (dam * (1.0 - (c / 100.00)));
}

/* Shared healing and protective-skin support. */

int vamp(P_char ch, double fhits, double fcap)
{
	struct affected_type *af;
	static char buf[100];
	int hits = (int)fhits, cap = summoner_pet_heal_cap(ch, (int)fcap), blocked;

	if (!IS_ALIVE(ch))
		return 0;

	if (affected_by_spell(ch, TAG_BUILDING))
		return 0;

	if (hits <= 0)
		return 0;

	if ((af = get_spell_from_char(ch, SPELL_PLAGUE)) && !IS_AFFECTED4(ch, AFF4_CARRY_PLAGUE))
	{
		blocked = (int)(MIN(hits * (((float)number(30, 40)) / 100), af->modifier));
		hits -= blocked;
		if (af->modifier <= blocked)
		{
			wear_off_message(ch, af);
			affect_remove(ch, af);
		}
		else
		{
			af->modifier -= blocked;
		}
	}
	else if (IS_AFFECTED4(ch, AFF4_CARRY_PLAGUE))
	{
		blocked = (int)(hits * (((float)number(50, 60)) / 100));
		hits -= blocked;
	}
	else if ((af = get_spell_from_char(ch, SPELL_BMANTLE)) != NULL ||
		 (af = get_spell_from_char(ch, SPELL_FLAMESTRIKE)) != NULL)
	{
		blocked = (int)(MIN(hits * (GET_LEVEL(ch) / 100), af->modifier));
		hits -= blocked;
		if (af->modifier <= blocked)
		{
			wear_off_message(ch, af);
			affect_remove(ch, af);
		}
		else
		{
			af->modifier -= blocked;
		}
	}

	if (IS_PC(ch) && IS_AFFECTED3(ch, AFF3_PALADIN_AURA) && has_aura(ch, AURA_HEALING))
	{
		// This stops the massive healing when used with the following: Nov08 -Lucrot
		if (!IS_SET(ch->specials.affected_by4, AFF4_REGENERATION) &&
		    !affected_by_spell(ch, SPELL_ACCEL_HEALING) &&
		    !affected_by_skill(ch, SKILL_REGENERATE))
		{
			hits += (int)(hits *
				      ((get_property("innate.paladin_aura.healing_mod", 0.2) *
					aura_mod(ch, AURA_HEALING)) /
				       100));
		}
	}

	hits = MAX(0, MIN(hits, cap - GET_HIT(ch)));
	GET_HIT(ch) = GET_HIT(ch) + hits;

	if (hits > 1 && IS_SET(ch->specials.act2, PLR2_HEAL))
	{
		snprintf(buf, ARRAY_SIZE(buf), "&+w[Heal: &+G%2d&+w ]&n ", hits);
		send_to_char(buf, ch);
	}

	update_groupies(ch, true);

	return hits;
}

void heal(P_char ch, P_char healer, int hits, int cap)
{
	if (!IS_ALIVE(ch))
		return;

	// healers get a 20% bonus to all heals
	if (GET_SPEC(healer, CLASS_CLERIC, SPEC_HEALER))
		hits = (int)(hits * get_property("healer.healing.mod", 1.2));

	if (IS_HARDCORE(healer))
		hits = (int)(hits * hardcore_config_get()->bonus_healing_multiplier);

	if (affected_by_spell(ch, SPELL_BMANTLE) || affected_by_spell(ch, SPELL_FLAMESTRIKE))
		hits = (int)(hits * get_property("blackmantle.healing.mod", .75));
	if (IS_AFFECTED3(healer, AFF3_ENHANCE_HEALING))
		hits = (int)(hits * get_property("enhancement.healing.mod", 1.5));
	/*
	   if(affected_by_spell(ch, TAG_NOMISFIRE)) //misfire code - drannak
	   {
	   hits = hits;
	//send_to_char("damage output normal\r\n", ch);
	}
	else
	{
	hits = (hits * .5);
	//send_to_char("&+Rdamage output halved\r\n", ch);
	}
	*/
	const int attempted_hits = MAX(0, hits);
	hits = vamp(ch, hits, cap);
	telemetry_runtime_game_combat_healing(healer, ch,
					      static_cast<std::uint64_t>(attempted_hits),
					      static_cast<std::uint64_t>(MAX(0, hits)), 0U);
	update_achievements(healer, ch, hits, 1);

	if (hits > 1 && healer != ch && ch->in_room == healer->in_room &&
	    IS_SET(healer->specials.act2, PLR2_HEAL))
	{
		char buf[100];
		snprintf(buf, ARRAY_SIZE(buf), "&+w[Heal: &+G%2d&+w ]&n ", hits);
		send_to_char(buf, healer);
	}

	if (GET_SPEC(healer, CLASS_CLERIC, SPEC_HEALER))
	{
		// healing replenishes ward
		GET_WARD(healer) = BOUNDED(0, GET_WARD(healer) + (hits / 5), GET_MAX_WARD(healer));
	}
	// debug("Hitting heal function in fight with (%d) hits.", hits);
	if (IS_PC(healer) && IS_FIGHTING(ch))
		gain_exp(healer, ch, hits, EXP_HEALING);
}

/*
 * used for stone skin type spells, decreases modifier by 1 and
 * shows wear off message when it reaches 0
 * function returns true if a corresponding affect structure was found
 */
bool decrease_skin_counter(P_char ch, unsigned int skin)
{
	struct affected_type *af, *af2;

	for (af = ch->affected; af; af = af2)
	{
		af2 = af->next;
		if (static_cast<unsigned int>(af->type) == skin)
		{
			af->modifier--;
			if (af->modifier <= 0)
			{
				wear_off_message(ch, af);
				affect_remove(ch, af);
			}
			return TRUE;
		}
	}

	return FALSE;
}

void check_vamp(P_char ch, P_char victim, double fdam, uint flags)
{
	int dam = (int)fdam, vamped = 0, wdam;
	struct group_list *group;
	P_char tch;
	double temp_dam = 0, bt_gain = 0, fcap = 0, fhits = 0, sac_gain = 0, can_mana = 0;

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim) || (dam < 1))
		return;

	// no vamping on images
	if (IS_NPC(victim) && GET_VNUM(victim) == 250)
		return;

	// Allowing cannibalize through all weapons.
	if (IS_AFFECTED3(ch, AFF3_CANNIBALIZE) && dam > 2)
	{
		can_mana = MAX(10, dam);
		can_mana = BOUNDED(0, can_mana, 100);
		can_mana *= GET_LEVEL(ch) / 30.;
		can_mana = BOUNDED(0, GET_MANA(victim), can_mana);

		GET_MANA(ch) += (int)can_mana;
		if (GET_MANA(ch) > GET_MAX_MANA(ch))
			GET_MANA(ch) = GET_MAX_MANA(ch);

		if (GET_MANA(ch) < GET_MAX_MANA(ch))
			StartRegen(ch, regen_resource::mana);

		GET_MANA(victim) -= (int)can_mana;
		// BOUNDED(0, number(1, can_mana), GET_MANA(victim));

		if (GET_MANA(victim) < GET_MAX_MANA(victim))
			StartRegen(victim, regen_resource::mana);
	}

	// Negative energy barrier prevents all forms of hitpoint vamp.
	if (affected_by_spell(victim, SPELL_NEG_ENERGY_BARRIER))
		return;

	// So does not being alive!
	if (GET_RACE(victim) == RACE_CONSTRUCT)
		return;

	if (flags & SPLDAM_NOVAMP)
		return;

	if (IS_DRAGOON(ch) && affected_by_spell(ch, SPELL_STIGMATA_DRACONICA))
	{
		P_char mount = get_dragoon_mount(ch);

		if (mount != NULL)
		{
			if (mount == victim)
			{
				vamped = vamp(
					ch, static_cast<double>(dam) * dam_factor[DF_TOUCHVAMP],
					static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));

				if (vamped)
				{
					send_to_char(
						"The &+Gdr&+Lag&+Gon&n's &+Rblood&n burns your &+Lstigmata&n as you fill with &+rpower&n!\r\n",
						ch);
				}
			}
		}
	}

	// Arrow type vamp. Apr09 -Lucrot
	if ((flags & PHSDAM_ARROW) && IS_AFFECTED2(ch, AFF2_VAMPIRIC_TOUCH) && IS_PC(ch) &&
	    !IS_AFFECTED4(ch, AFF4_BATTLE_ECSTASY) && dam >= 4)
	{
		vamped = vamp(ch, static_cast<double>(dam) * dam_factor[DF_ARROWVAMP],
			      static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
	}

	// Physical type actions that vamp
	// PC vamp touch.
	if ((flags & PHSDAM_TOUCH) && !vamped && IS_AFFECTED2(ch, AFF2_VAMPIRIC_TOUCH) &&
	    IS_PC(ch) &&
	    !IS_AFFECTED4(ch, AFF4_VAMPIRE_FORM)) // && !IS_AFFECTED4(ch, AFF4_BATTLE_ECSTASY) )
	{
		// Illithids get full regular touch vamp (since they have lousy str).
		if (IS_ILLITHID(ch))
		{
			vamped = vamp(ch, static_cast<double>(dam) * dam_factor[DF_TOUCHVAMP],
				      static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
		}
		// The class order makes a difference to multiclass chars.
		else if (GET_CLASS(ch, CLASS_ANTIPALADIN))
		{
			vamped = vamp(ch, static_cast<double>(dam) * dam_factor[DF_ANTIPALADINVAMP],
				      static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
		}
		else if (GET_CLASS(ch, CLASS_MONK))
		{
			vamped = vamp(ch, static_cast<double>(dam) * dam_factor[DF_MONKVAMP],
				      static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
		}
		else if (GET_CLASS(ch, CLASS_MERCENARY))
		{
			vamped = vamp(ch, static_cast<double>(dam) * dam_factor[DF_MERCENARYVAMP],
				      static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
		}
		else if (GET_CLASS(ch, CLASS_WARRIOR))
		{
			vamped = vamp(ch, static_cast<double>(dam) * dam_factor[DF_WARRIORVAMP],
				      static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
		}
		else if (GET_CLASS(ch, CLASS_BERSERKER))
		{
			vamped = vamp(ch, static_cast<double>(dam) * dam_factor[DF_BERSERKERVAMP],
				      static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
		}
		else if (GET_CLASS(ch, CLASS_ROGUE))
		{
			vamped = vamp(ch, static_cast<double>(dam) * dam_factor[DF_ROGUEVAMP],
				      static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
		}
		else if (GET_CLASS(ch, CLASS_PALADIN))
		{
			vamped = vamp(ch, static_cast<double>(dam) * dam_factor[DF_PALADINVAMP],
				      static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
		}
		else if (GET_CLASS(ch, CLASS_RANGER))
		{
			vamped = vamp(ch, static_cast<double>(dam) * dam_factor[DF_RANGERVAMP],
				      static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
		}
		else if (GET_CLASS(ch, CLASS_AVENGER) || GET_CLASS(ch, CLASS_DREADLORD))
		{
			vamped = vamp(ch, static_cast<double>(dam) * dam_factor[DF_DLORDAVGRVAMP],
				      static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
		}
		else
		{
			// vamped = vamp(ch, dam * dam_factor[DF_TOUCHVAMP], GET_MAX_HIT(ch) * 1.10); //113?
			vamped = vamp(ch, static_cast<double>(dam) * dam_factor[DF_TOUCHVAMP],
				      static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
		}
	}

	// NPC vamp touch.
	// This overrides undead vamp.
	if ((flags & PHSDAM_TOUCH) && IS_AFFECTED2(ch, AFF2_VAMPIRIC_TOUCH) && IS_NPC(ch) &&
	    !IS_AFFECTED4(ch, AFF4_VAMPIRE_FORM))
	{
		vamped = vamp(ch,
			      static_cast<double>(dam) *
				      summoner_pet_vamp_rate(ch, dam_factor[DF_TOUCHVAMP]),
			      static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
	}
	// end TOUCHVAMP

	// Battle X section:
	// btx vamp(also short_vamp) - Jexni

	// This is battle x vamp from your own attacks:

	if (!vamped && ch != victim && !IS_AFFECTED4(victim, AFF4_HOLY_SACRIFICE) &&
	    ((flags & RAWDAM_BTXVAMP) || (flags & RAWDAM_SHRTVMP)) &&
	    (IS_AFFECTED4(ch, AFF4_BATTLE_ECSTASY) || affected_by_spell(ch, SKILL_SHORT_VAMP)))
	{
		if (IS_PC(ch))
		{
			temp_dam = static_cast<double>(dam) *
				   get_property("vamping.self.battleEcstasy", 0.150);
			vamp(ch, temp_dam, static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
		}
		else
		{
			temp_dam =
				static_cast<double>(dam) *
				summoner_pet_vamp_rate(
					ch, get_property("vamping.self.NPCbattleEcstasy", 0.050));
			const int healed =
				vamp(ch, temp_dam,
				     static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
			if (summoner_capture(ch))
				vamped = healed;
		}
	}

	// This is battle x vamp for PC group hits and damage spells.
	if (dam >= 10 && ((flags & RAWDAM_BTXVAMP) || (flags & PHSDAM_ARROW)))
	{
		if (IS_PC(ch) || IS_PC_PET(ch))
		{
			for (group = ch->group; group; group = group->next)
			{
				tch = group->ch;

				if (IS_AFFECTED4(tch, AFF4_BATTLE_ECSTASY) &&
				    tch->in_room == ch->in_room && tch != ch)
				{
					// Have to use BOUNDEDF here for floats.. *sigh*
					vamp(tch,
					     static_cast<double>(dam) *
						     get_property("vamping.battleEcstasy", .140),
					     VAMPPERCENT(tch) *
						     static_cast<double>(GET_MAX_HIT(tch)));
				}
			}
		}

		// NPCs do not group, thus we use a room search and separate code.
		if (IS_NPC(ch) && !IS_PC_PET(ch))
		{
			temp_dam = number(1, (int)(temp_dam));

			for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
			{
				if (IS_NPC(tch) && !IS_PC_PET(tch) &&
				    IS_SET((tch)->specials.affected_by4, AFF4_BATTLE_ECSTASY) &&
				    tch->in_room == ch->in_room && tch != ch)
				{
					vamp(tch, temp_dam,
					     static_cast<double>(GET_MAX_HIT(ch)) *
						     VAMPPERCENT(ch));
				}
			}
		}
	}

	if (!vamped && IS_AFFECTED2(ch, AFF2_VAMPIRIC_TOUCH) && (flags & RAWDAM_TRANCEVAMP) &&
	    (IS_AFFECTED4(ch, AFF4_VAMPIRE_FORM)))
	{
		vamped = vamp(ch,
			      static_cast<double>(dam) *
				      summoner_pet_vamp_rate(ch, dam_factor[DF_TRANCEVAMP]),
			      static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
	}

	// hellfire vamp
	if (!vamped && (flags & PHSDAM_HELLFIRE) && IS_AFFECTED4(ch, AFF4_HELLFIRE))
	{
		P_obj weapon = ch->equipment[PRIMARY_WEAPON];
		wdam = 1;

		if (IS_NPC(ch))
			wdam = MIN(dam, dice(ch->points.damnodice, MAX(1, ch->points.damsizedice)));
		else if (IS_PC(ch) && weapon)
			wdam = MIN(dam, dice(weapon->value[1], MAX(1, weapon->value[2])));
		if (wdam)
		{
			vamped = vamp(ch,
				      static_cast<double>(wdam) *
					      summoner_pet_vamp_rate(ch, dam_factor[DF_HFIREVAMP]),
				      static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
		}
	}

	// BATTLETIDE VAMP
	if (!vamped && (flags & PHSDAM_BATTLETIDE) && IS_AFFECTED4(ch, AFF4_BATTLETIDE) &&
	    !affected_by_spell(ch, SPELL_PLAGUE))
	{
		bt_gain = static_cast<double>(dam) * dam_factor[DF_BATTLETIDEVAMP];

		for (group = ch->group; group; group = group->next)
		{
			tch = group->ch;
			if (tch->in_room == ch->in_room && tch != ch)
				vamped = vamp(tch, bt_gain, GET_MAX_HIT(tch));
		}
	}

	// Tranced vamping
	if (!vamped && (flags & RAWDAM_TRANCEVAMP) && (IS_UNDEADRACE(ch) || IS_ANGEL(ch)) &&
	    !IS_AFFECTED4(ch, AFF4_BATTLE_ECSTASY))
	{
		if (IS_DRACOLICH(ch) && !summoner_capture(ch))
		{
			fhits = static_cast<double>(dam) * dam_factor[DF_DRACOLICHVAMP];
			vamped = vamp(ch, fhits,
				      static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
		}
		if (IS_NPC(ch))
		{
			fhits = static_cast<double>(dam) *
				summoner_pet_vamp_rate(ch, dam_factor[DF_NPCVAMP], true);
			vamped = vamp(ch, fhits,
				      static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
		}
		// 10 dam * .3 = 3 points of vamp minimum. 'cause I said so.
		else if (dam >= 12 && IS_PC(ch))
		{
			fhits = static_cast<double>(dam) * dam_factor[DF_UNDEADVAMP];
			fcap = GET_MAX_HIT(ch);
			// Liches vamp from spells.
			if (flags & SPLDAM_SPELL && GET_RACE(ch) == RACE_LICH)
				fcap *= VAMPPERCENT(ch);
			vamped = vamp(ch, fhits, fcap);
		}
	}

	// Illesarus vamp through weapon, but only if they haven't vamped previous to this - Jexni 12/20/08
#define HOA_ILLESARUS_VNUM 77738
	if (!vamped && ch->equipment[WIELD] &&
	    (obj_index[ch->equipment[WIELD]->R_num].virtual_number == HOA_ILLESARUS_VNUM))
	{
		vamped = vamp(ch, MIN(dam, number(2, 7)),
			      static_cast<double>(GET_MAX_HIT(ch)) * VAMPPERCENT(ch));
	}

	if ((dam >= 2 && !IS_AFFECTED4(ch, AFF4_BATTLE_ECSTASY) &&
	     IS_AFFECTED4(victim, AFF4_HOLY_SACRIFICE) && (flags & RAWDAM_HOLYSAC) &&
	     !affected_by_spell(victim, SPELL_BMANTLE) &&
	     !affected_by_spell(victim, SPELL_PLAGUE) &&
	     !affected_by_spell(victim, SPELL_FLAMESTRIKE)))
	/* Taking this out as it doesn't seem necessary.
	    && ((GOOD_RACE(victim) && !GOOD_RACE(ch))
	    || (EVIL_RACE(victim) && !EVIL_RACE(ch))))
	*/
	{
		sac_gain = static_cast<double>(dam) * get_property("vamping.holySacrifice", 0.035);
		for (group = victim->group; group; group = group->next)
		{
			tch = group->ch;

			if (tch->in_room == victim->in_room && tch != victim &&
			    !IS_AFFECTED4(tch, AFF4_HOLY_SACRIFICE) &&
			    !affected_by_spell(tch, SPELL_PLAGUE))
			{
				vamp(tch, sac_gain,
				     GET_MAX_HIT(
					     tch)); // Holy Sac only vamps to max hp - Jexni 12/9/10
			}
		}
	}
}

bool soul_trap(P_char ch, P_char victim)
{
	int hps = GET_LEVEL(victim) * 2 * GET_CHAR_SKILL(ch, SKILL_SOUL_TRAP) / 100;
	bool himself = false;
	struct group_list *gl;

	if (GET_CHAR_SKILL(ch, SKILL_SOUL_TRAP))
	{
		if (!notch_skill(ch, SKILL_SOUL_TRAP, get_property("skill.notch.soulTrap", 1)) &&
		    number(1, 100) >= GET_CHAR_SKILL(ch, SKILL_SOUL_TRAP) / 3)
			return false;
		himself = true;
	}
	else
	{
		for (gl = ch->group; gl; gl = gl->next)
		{
			if (GET_CHAR_SKILL(gl->ch, SKILL_SOUL_TRAP) &&
			    (notch_skill(gl->ch, SKILL_SOUL_TRAP,
					 get_property("skill.notch.soulTrap", 100) * 3) ||
			     GET_CHAR_SKILL(gl->ch, SKILL_SOUL_TRAP) / 5 > number(1, 100)))
			{
				ch = gl->ch;
				break;
			}
		}
		if (!gl)
			return false;
	}

	vamp(ch, hps, (int)(GET_MAX_HIT(ch) * 1.1));

	if (himself)
	{
		act("&+LThe room seems to darken as&n $n &+Rslams&n $p &+Lthrough&n $N's &+Lchest!&n",
		    FALSE, ch, 0, victim, TO_ROOM);
		act("&+LAs $E slumps to the ground, bleeding, screaming, begging for mercy,&n $n &+Ltwist the weapon and draws the soul from the dying body.",
		    FALSE, ch, 0, victim, TO_ROOM);
		act("&+LThe air seems to vibrate with &+Wenergy &+Lwhich causes&n $n &+Lto throw back $s head and let out a mocking laughter that freezes your &+Rblood.&n",
		    FALSE, ch, 0, victim, TO_ROOM);

		act("&+LThe room seems to darken as you slam&n $p &+Lthrough&n $N's &+Lchest!&n",
		    FALSE, ch, 0, victim, TO_CHAR);
		act("&+LAs $E slumps to the ground, bleeding, screaming, begging for mercy, you twist the weapon and draw the soul from the dying body.",
		    FALSE, ch, 0, victim, TO_CHAR);
		act("&+LA sweetness fills your being and you cannot help but to let out a &=LWmocking laughter.&n&n",
		    FALSE, ch, 0, victim, TO_CHAR);
	}
	else
	{
		act("With a &+mmocking smile&n lacking any &+ywarmth&n $n takes a step towards $s &+yhelpless victim&n \r\n"
		    "and &+Rdrives&n $p &+Rthrough&n $N's &+Rchest.&n",
		    FALSE, ch, 0, victim, TO_ROOM);
		act("$s &+msmile deepens&n as $e watches the life quickly seep out of $N's body.",
		    FALSE, ch, 0, victim, TO_ROOM);
		act("&+LStrengthened by the suffering around $m&n $n &+Ljerks $s weapon free causing the now lifeless husk to crumble in a heap.&n",
		    FALSE, ch, 0, victim, TO_ROOM);

		act("&+YAmused, &+yyou take a step forward and drive&n $p &+ythrough&n $N's &+ychest.&n",
		    FALSE, ch, 0, victim, TO_CHAR);
		act("&+yAs $E slumps to the ground screaming for &+Wmercy&+y, you twist the blade causing $S &+Llifeblood &+yto pour from $S body.&n",
		    FALSE, ch, 0, victim, TO_CHAR);
		act("&+yStrengthened by the suffering you jerk your weapon free causing the now lifeless husk to crumble in a heap.&n",
		    FALSE, ch, 0, victim, TO_CHAR);
	}

	return true;
}
