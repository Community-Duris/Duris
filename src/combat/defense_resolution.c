/* Defensive response and evasion resolution. */
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include "combat/defense_resolution.h"
#include "combat/attack_continuation.h"
#include "combat/damage.h"
#include "combat/dam_mods.h"
#include "world/difficulty.h"
#include "world/events.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "item/objmisc.h"
#include "classes/paladins.h"

extern void event_wait(P_char ch, P_char victim, P_obj obj, void *data);
extern struct dex_app_type dex_app[];
extern struct str_app_type str_app[];
extern struct wis_app_type wis_app[];
extern bool is_dragoon_mounted(P_char ch);
extern bool innate_two_daggers(P_char ch);

static int try_riposte(P_char ch, P_char victim, P_obj wpn);

/* Shared generic defensive item procedures. */
int generic_parry_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
	struct proc_data *data;
	P_char vict;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	// 1/20 chance.
	if (cmd != CMD_GOTHIT || number(0, 19))
	{
		return FALSE;
	}
	// important! can do this cast (next line) ONLY if cmd was CMD_GOTHIT or CMD_GOTNUKED
	if (!(data = legacy_proc_arg<struct proc_data *>(arg)))
	{
		return FALSE;
	}
	vict = data->victim;
	if (!IS_ALIVE(vict))
	{
		return FALSE;
	}
	act("Your $q parries $N's lunge at you.", FALSE, ch, obj, vict, TO_CHAR | ACT_NOTTERSE);
	act("$n's $q parries your futile lunge at $m.", FALSE, ch, obj, vict,
	    TO_VICT | ACT_NOTTERSE);
	act("$n's $q parries $N's lunge at $n.", FALSE, ch, obj, vict, TO_NOTVICT | ACT_NOTTERSE);

	return TRUE;
}

int generic_riposte_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
	struct proc_data *data;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	// 1/20 chance.
	if (cmd != CMD_GOTHIT || number(0, 19))
	{
		return FALSE;
	}

	if (!(data = legacy_proc_arg<struct proc_data *>(arg)))
	{
		return FALSE;
	}
	victim = data->victim;
	if (!IS_ALIVE(victim))
	{
		return FALSE;
	}

	act("$n's $q deflects $N's blow and strikes $M!", TRUE, ch, obj, victim,
	    TO_NOTVICT | ACT_NOTTERSE);
	act("$n's $q deflects your blow and strikes YOU!", TRUE, ch, obj, victim,
	    TO_VICT | ACT_NOTTERSE);
	act("Your $q deflects $N's blow and strikes $M!", TRUE, ch, obj, victim,
	    TO_CHAR | ACT_NOTTERSE);
	hit(ch, victim, obj);

	return TRUE;
}

static int WeaponSkill(P_char ch, P_obj weapon)
{
	if (IS_NPC(ch))
		return BOUNDED(0, (GET_LEVEL(ch) * 7 / 3), 98);
	else
		return GET_CHAR_SKILL(ch, required_weapon_skill(weapon));
}

int leapSucceed(P_char victim, P_char attacker)
{
	int chance;

	chance = 0;
	if (!IS_THRIKREEN(victim) && !IS_HARPY(victim)) /* only bugs */
		return false;

	if (!MIN_POS(victim, POS_STANDING + STAT_NORMAL) || affected_by_spell(victim, SKILL_GAZE))
		return false;

	chance = (GET_C_AGI(victim) / 7);
	chance -= load_modifier(victim) / 100;
	chance += (GET_LEVEL(victim) - GET_LEVEL(attacker)) / 2;
	chance = BOUNDED(1, chance, 20);

	if (number(1, 100) > chance)
		return false;

	if (IS_THRIKREEN(victim))
	{
		act("You leap into the air, avoiding $n's attack.", FALSE, attacker, 0, victim,
		    TO_VICT);
		act("$N leaps into the air, avoiding your attack.", FALSE, attacker, 0, victim,
		    TO_CHAR);
		act("$N leaps over $n's attack.", FALSE, attacker, 0, victim, TO_NOTVICT);
	}
	else
	{
		act("You move with lightning speed, quickly evading $n's attack.", FALSE, attacker,
		    0, victim, TO_VICT);
		act("$N moves with lightning speed, evading your attack.", FALSE, attacker, 0,
		    victim, TO_CHAR);
		act("$N moves with lightning speed, evading $n's attack.", FALSE, attacker, 0,
		    victim, TO_NOTVICT);
	}
	return true;
}

/*
	 * This function does all the necessary details for implementation
	 * of skill "dodging".  Details include print out messages.
	 * Return 1 if successful dodge, or 0 otherwise.
	 */

int dodgeSucceed(P_char char_dodger, P_char attacker, P_obj wpn)
{
	P_char mount;
	int percent = 0, learned = 0, minimum = 0;

	if (!(char_dodger) || !(attacker) || IS_IMMOBILE(char_dodger))
		return 0;

	mount = get_linked_char(char_dodger, LNK_RIDING);

	if (mount && MIN_POS(mount, POS_STANDING + STAT_NORMAL) &&
	    MIN_POS(char_dodger, POS_STANDING + STAT_NORMAL) && !IS_STUNNED(mount) &&
	    !IS_BLIND(mount) && !IS_STUNNED(char_dodger) && !IS_BLIND(char_dodger) &&
	    (notch_skill(char_dodger, SKILL_SIDESTEP, get_property("skill.notch.defensive", 17)) ||
	     GET_CHAR_SKILL(char_dodger, SKILL_SIDESTEP) / 5 > number(0, 100)))
	{
		act("Your mount sidesteps $n's blow.", FALSE, attacker, 0, char_dodger,
		    TO_VICT | ACT_NOTTERSE);
		act("$N sidesteps your attack causing you to miss.", FALSE, attacker, 0, mount,
		    TO_CHAR | ACT_NOTTERSE);
		act("$N's mount sidesteps $n's blow.", FALSE, attacker, 0, char_dodger,
		    TO_NOTVICT | ACT_NOTTERSE);
		return 1;
	}

	// Cannot dodge while on the ground except with groundfighting.
	if ((GET_POS(char_dodger) < POS_STANDING) && !GROUNDFIGHTING_CHECK(char_dodger))
		return 0;

	if (affected_by_spell(char_dodger, SKILL_RAGE) && attacker != GET_OPPONENT(char_dodger))
		return 0;

	// Notching dodge fails dodge check.
	/* -Changing dodge to an innate skill, with c_agility as basis for check - Drannak 12/12/2012
		   if(notch_skill
		   (char_dodger, SKILL_DODGE, get_property("skill.notch.defensive", 17)))
		   {
		   return 0;
		   }
		   */
	// Generating base dodge value.
	/*
		   learned = (int) ((GET_CHAR_SKILL(char_dodger, SKILL_DODGE)) * 1.25) -
		   (WeaponSkill(attacker, wpn));
		   */
	learned = (int)((GET_C_AGI(char_dodger)) * dam_factor[DF_DODGE_AGI_MODIFIER]) -
		  (WeaponSkill(attacker, wpn));

	// Dwarves now get the DnD 3.5 dodgeroll bonus vs giant races
	if (has_innate(char_dodger, INNATE_GIANT_AVOIDANCE) &&
	    ((GET_RACE(attacker) == RACE_TROLL) || (GET_RACE(attacker) == RACE_OGRE) ||
	     (GET_RACE(attacker) == RACE_GIANT)))
	{
		learned *= 1.10;
	}

	// Everybody receives these values. -maybe change this? Drannak
	learned +=
		(int)(((STAT_INDEX(GET_C_AGI(char_dodger))) - (STAT_INDEX(GET_C_DEX(attacker)))) /
		      2);

	// Simple level comparison adjustment.
	learned += (int)(GET_LEVEL(char_dodger) - GET_LEVEL(attacker));

	// NPCs receive a dodge bonus.
	/* Nah - Drannak
		   if(IS_NPC(char_dodger))
		   {
		   learned += (int) (GET_LEVEL(char_dodger) / 2);
		   }
		   */
	// Minimum dodge is 1/10th of the skill.
	/*
		   minimum = (int) (GET_CHAR_SKILL(char_dodger, SKILL_DODGE) / 10);
		   */
	minimum = (int)(GET_C_AGI(char_dodger) / 10);

	percent = BOUNDED(minimum, learned, 50);

	// Modifiers
	if ((has_innate(char_dodger, INNATE_GROUNDFIGHTING) &&
	     !MIN_POS(char_dodger, POS_STANDING + STAT_NORMAL)) ||
	    affected_by_spell(char_dodger, SKILL_GAZE))
	{
		percent = (int)(percent * 0.50);
	}

	// 1/5 the chance if dazzled (typical 2% max 10%).
	if (IS_AFFECTED5(char_dodger, AFF5_DAZZLEE))
		percent /= 5;

	// 6% of 50% is 3% chance to dodge stunned (6% of 10% is 0%).
	if (IS_STUNNED(char_dodger))
		percent = (6 * percent) / 100;

	if (GET_CLASS(char_dodger, CLASS_MONK))
		percent = (int)(percent * 1.20);

	// Improved Zealot/Scoundrel dodge.
	if (GET_SPEC(char_dodger, CLASS_CLERIC, SPEC_ZEALOT) ||
	    GET_SPEC(char_dodger, CLASS_BARD, SPEC_SCOUNDREL))
	{
		percent += 5;
	}

	if (IS_STUNNED(attacker))
		percent = (int)(percent * 1.10);

	bool dragoon_dodge = false;

	// Weight affects dodge. This simulates mobility.
	// Harder to dodge when char_dodger's weight is increased.
	// Easier to dodge when attacker's weight is increased.
	// Tweak as needed.
	/*
		   percent -= (int) (load_modifier(char_dodger) / 100);
		   percent += (int) (load_modifier(attacker) / 100);
		   */

	// Drows receive special dodge bonus now based on level.
	// Level 56 drow has 10% innate dodge.  Excessive weight will negate this bonus.
	// This is actually a 9% dodge for all !stunned drow within weight limit..
	//   I set the weight limit to 100 since the range for load_modifier is between 75 and 300.
	if (GET_RACE(char_dodger) == RACE_DROW && !IS_STUNNED(char_dodger) &&
	    (load_modifier(char_dodger) < 100) && !number(0, 10))
	{
		// debug("Drow dodge (%s).", GET_NAME(char_dodger));
	}
	else if (affected_by_spell(char_dodger, SKILL_BATTLE_SENSES))
	{
		// debug("dodging (%s) affected by battle senses.", GET_NAME(char_dodger));
	}
	else if (affected_by_spell(char_dodger, SPELL_SANGUINIS_IGNIS_AFF) &&
		 is_dragoon_mounted(char_dodger))
	{
		dragoon_dodge = true;
		debug("dodging (%s) affected by sanguinis ignis.", GET_NAME(char_dodger));
	}
	else if (number(1, 101) > percent) // Dodge success or failure.
	{
		return 0; // Failed dodge.
	}

	// Dodge success.
	if (GET_CLASS(char_dodger, CLASS_MONK))
	{
		if (number(0, 1))
		{
			act("You easily lean out of range of $n's vicious attack.", FALSE, attacker,
			    0, char_dodger, TO_VICT | ACT_NOTTERSE);
			act("$N nimbly swivels out of the path of your attack.", FALSE, attacker, 0,
			    char_dodger, TO_CHAR | ACT_NOTTERSE);
			act("$N nimbly swivels out of the path of $n's attack.", FALSE, attacker, 0,
			    char_dodger, TO_NOTVICT | ACT_NOTTERSE);
		}
		else
		{
			act("You whirl around, just avoiding $n's vicious attack.", FALSE, attacker,
			    0, char_dodger, TO_VICT | ACT_NOTTERSE);
			act("$N gracefully whirls around your attack.", FALSE, attacker, 0,
			    char_dodger, TO_CHAR | ACT_NOTTERSE);
			act("$N gracefully whirls around $n's attack.", FALSE, attacker, 0,
			    char_dodger, TO_NOTVICT | ACT_NOTTERSE);
		}
	}
	else if (!dragoon_dodge)
	{
		act("You dodge $n's vicious attack.", FALSE, attacker, 0, char_dodger,
		    TO_VICT | ACT_NOTTERSE);
		act("$N dodges your futile attack.", FALSE, attacker, 0, char_dodger,
		    TO_CHAR | ACT_NOTTERSE);
		act("$N dodges $n's attack.", FALSE, attacker, 0, char_dodger,
		    TO_NOTVICT | ACT_NOTTERSE);
	}
	else
	{
		act("Your sense $n's vicious attack, as your blood burns with &+rpower&n.", FALSE,
		    attacker, 0, char_dodger, TO_VICT | ACT_NOTTERSE);
		act("$N senses your attack and swiftly manuevers away.", FALSE, attacker, 0,
		    char_dodger, TO_CHAR | ACT_NOTTERSE);
		act("$N senses $n's attack and swiftly manuevers away.", FALSE, attacker, 0,
		    char_dodger, TO_NOTVICT | ACT_NOTTERSE);
	}
	return 1;
}

int blockSucceed(P_char victim, P_char attacker, P_obj wpn)
{
	int learned, attackerweaponskill, percent;
	int room = victim->in_room;
	P_obj shield;

	if (!(attacker) || !(victim) || !IS_ALIVE(victim) || !IS_ALIVE(attacker))
		return false;

	if (!GET_CHAR_SKILL(victim, SKILL_SHIELD_BLOCK) ||
	    !(shield = victim->equipment[WEAR_SHIELD]))
		return false;

	if (affected_by_spell(victim, SKILL_RAGE) && attacker != GET_OPPONENT(victim))
		return false;

	if (notch_skill(victim, SKILL_SHIELD_BLOCK, get_property("skill.notch.defensive", 17)))
		return false;

	learned = GET_CHAR_SKILL(victim, SKILL_SHIELD_BLOCK) / 4;
	learned += dex_app[STAT_INDEX(GET_C_DEX(victim))].reaction * 2;

	if (GET_CLASS(attacker, CLASS_MONK))
		learned = (int)(learned * 1.75);

	// Shield block works well versus an attacker that is using a whip/flail.
	if ((wpn && IS_FLAYING(wpn)) ||
	    (attacker->equipment[WIELD] && IS_FLAYING(attacker->equipment[WIELD])))
		learned = (int)(learned * 1.5);

	if (IS_AFFECTED5(victim, AFF5_DAZZLEE))
		learned -= 10;
	// Victim benefits from having a shield and not standing.
	// This is intentional as parry and dodge values are
	// greatly reduced when victim is not standing. The victim
	// focuses their defense with the shield, if present.
	if (!MIN_POS(victim, POS_STANDING + STAT_NORMAL))
		learned = (int)(learned * 1.2);

	if (!MIN_POS(attacker, POS_STANDING + STAT_NORMAL))
		percent = (int)(learned * 1.1);

	if (GET_C_LUK(victim) / 10 > number(0, 100))
		percent += number(1, 4);

	/* Bless spell modifier can be adjusted on the fly - Lucrot */
	if (IS_SET(shield->extra2_flags, ITEM2_BLESS))
		percent = (int)(percent * get_property("skill.shieldBlock.blessBonus", 1.2));

	attackerweaponskill = WeaponSkill(attacker, wpn);
	attackerweaponskill += str_app[STAT_INDEX(GET_C_STR(attacker))].tohit * 4;

	/* Standardizing values to a 1 to 100 scale. May need tweaking. -Lucrot */

	if (number(0, attackerweaponskill) > number(0, learned))
		return false;

	if (affected_by_spell(victim, SPELL_STORMSHIELD) && !number(0, 2))
	{
		act("Your shield &+Yflares&n up and discharges a bolt of &+Blightning&n towards $n.",
		    FALSE, attacker, 0, victim, TO_VICT);
		act("$N's shield &+Yflares&n up and discharges a bolt of &+Blightning&n towards YOU!",
		    FALSE, attacker, 0, victim, TO_CHAR);
		act("$N's shield &+Yflares&n up and discharges a bolt of &+Blightning&n towards $n.",
		    FALSE, attacker, 0, victim, TO_NOTVICT);

		spell_lightning_bolt(GET_LEVEL(victim), victim, 0, SPELL_TYPE_SPELL, attacker, 0);
		if (!is_char_in_room(attacker, room) || !is_char_in_room(victim, room))
			return true;
	}

	/* Bless wearing off shield. */
	if (IS_SET(shield->extra2_flags, ITEM2_BLESS) && !number(0, 299))
	{
		act("A &+Cblessed glow&n around your $q fades.", FALSE, victim, shield, 0, TO_CHAR);
		affect_from_obj(shield, SPELL_BLESS);
		REMOVE_BIT(shield->extra2_flags, ITEM2_BLESS);
	}

	if (GET_CHAR_SKILL(victim, SKILL_IMPROVED_SHIELD_COMBAT) &&
	    number(1, 400) < MAX(20, GET_CHAR_SKILL(victim, SKILL_IMPROVED_SHIELD_COMBAT)) &&
	    MIN_POS(victim, POS_STANDING + STAT_NORMAL))
	{
		struct damage_messages messages = {
			"You block $N's lunge at you and immediately slam $M with your shield!",
			"$n blocks your lunge and then slams $s shield into you.",
			"$n blocks $N's lunge and then immediately slams $M with $s shield!",
			"You block $N and then slam your shield into $M, crushing $S skull.",
			"$n blocks your attack and then slams $s shield into you! The lights go out.",
			"$n blocks $M's attack and then slams $s shield into $S body crunching it like a soft egg."
		};

		/* Improved shield combat property can be adjusted on the fly - Lucrot */
		int dmg = number(
			40,
			MAX(41, GET_CHAR_SKILL(victim, SKILL_IMPROVED_SHIELD_COMBAT) *
					(int)get_property(
						"skill.improvedShieldCombat.damageMultiplier", 1)));

		if (melee_damage(victim, attacker, dmg, 0, &messages) != DAM_NONEDEAD)
			return true;
		else
			return false;
	}
	else if (learned > 45 && !number(0, 2))
	{
		act("You expertly block $n's lunge at you.", FALSE, attacker, 0, victim,
		    TO_VICT | ACT_NOTTERSE);
		act("$N easily blocks your futile lunge at $M.", FALSE, attacker, 0, victim,
		    TO_CHAR | ACT_NOTTERSE);
		act("$N easily blocks $n's lunge at $M.", FALSE, attacker, 0, victim,
		    TO_NOTVICT | ACT_NOTTERSE);
		return true;
	}
	else if (learned < 20)
	{
		act("You barely block $n's lunge at you.", FALSE, attacker, 0, victim,
		    TO_VICT | ACT_NOTTERSE);
		act("$N just barely gets $S shield up to block your attack.", FALSE, attacker, 0,
		    victim, TO_CHAR | ACT_NOTTERSE);
		act("$N barely blocks $n's lunge at $M.", FALSE, attacker, 0, victim,
		    TO_NOTVICT | ACT_NOTTERSE);
		return true;
	}
	else
	{
		act("You block $n's lunge at you.", FALSE, attacker, 0, victim,
		    TO_VICT | ACT_NOTTERSE);
		act("$N blocks your futile lunge at $M.", FALSE, attacker, 0, victim,
		    TO_CHAR | ACT_NOTTERSE);
		act("$N blocks $n's lunge at $M.", FALSE, attacker, 0, victim,
		    TO_NOTVICT | ACT_NOTTERSE);
		return true;
	}
	return false;
}

int MonkRiposte(P_char victim, P_char attacker, P_obj wpn)
{
	// int percent = 0, learned = 0; // learned was never set, broke minimum floor for riposte
	int percent = 0, learned;
	int skl = GET_CHAR_SKILL(victim, SKILL_MARTIAL_ARTS);
	learned = skl; // this sets min riposte chance based on martial arts skill

	if (!(attacker) || !(victim) || !GET_CLASS(victim, CLASS_MONK) || !IS_ALIVE(victim) ||
	    !(skl) || !IS_ALIVE(attacker) || IS_IMMOBILE(victim) || IS_BLIND(victim) ||
	    !IS_AWAKE(victim) || IS_STUNNED(victim))
	{
		return 0;
	}

	if (IS_PC(victim) &&
	    notch_skill(victim, SKILL_MARTIAL_ARTS, get_property("skill.notch.defensive", 17)))
		return 0;

	if (IS_ELITE(victim))
		skl = (int)(skl * 1.1);

	percent = (int)(skl / 2) - (int)(WeaponSkill(attacker, wpn) / 3);

	percent += dex_app[STAT_INDEX(GET_C_DEX(victim))].reaction * 2;
	percent += str_app[STAT_INDEX(GET_C_STR(victim))].tohit * 1.5;
	percent -= (str_app[STAT_INDEX(GET_C_STR(attacker))].tohit +
		    str_app[STAT_INDEX(GET_C_STR(attacker))].todam);

	if (!MIN_POS(victim, POS_STANDING + STAT_NORMAL))
	{
		// This allows monks a chance to regain their feet based on agil and
		// martial arts skill. Aug09 -Lucrot

		// All this does is set someone up to be repeatedly bashed and lagged,
		// utterly and insanely stupid. -- Jexni 1/21/11

		if (GET_C_AGI(victim) > number(1, 1000) &&
		    !get_spell_from_char(victim, SKILL_MARTIAL_ARTS) &&
		    GET_CHAR_SKILL(victim, SKILL_MARTIAL_ARTS) > number(1, 100))
		{
			act("$n tucks in $s arms, rolls quickly away, then thrust $s feet skywards, leaping back to $s feet!",
			    TRUE, victim, 0, attacker, TO_NOTVICT);
			act("$n tucks in $s arms, rolls away from $N's attack, then thrust $s feet skywards, and leaps to $s feet!",
			    TRUE, victim, 0, attacker, TO_VICT);
			act("You tuck in your arms, roll away from $N's blow, then leap to your feet!",
			    TRUE, victim, 0, attacker, TO_CHAR);
			SET_POS(victim, POS_STANDING + GET_STAT(victim));
			// Clear the lag!
			disarm_char_nevents(victim, event_wait);
			set_short_affected_by(victim, SKILL_MARTIAL_ARTS, PULSE_VIOLENCE);
			REMOVE_BIT(victim->specials.act2, PLR2_WAIT);
			update_pos(victim);
			return FALSE;
		}

		percent = 5;
	}

	if (!MIN_POS(attacker, POS_STANDING + STAT_NORMAL))
		percent *= 1.1;

	percent = BOUNDED(learned / 25, percent, 15);

	if (IS_GREATER_RACE(attacker) || IS_ELITE(attacker))
		percent /= 2;

	if (number(1, 150) > percent)
		return 0;

	if (number(1, 150) > skl)
	{
		act("$n arches around $N's blow, and deals a brutal counterattack!", TRUE, victim,
		    0, attacker, TO_NOTVICT);
		act("$n arches around your blow, and deals a brutal counterattack to YOU!", TRUE,
		    victim, 0, attacker, TO_VICT);
		act("You arch around $N's blow, and deal a brutal counterattack!", TRUE, victim, 0,
		    attacker, TO_CHAR);
		hit(victim, attacker, NULL);
		return false;
	}

	act("$n completely sidesteps $N's blow, shifts $s balance, and strikes back at $M!", TRUE,
	    victim, 0, attacker, TO_NOTVICT);
	act("$n completely sidesteps your blow, shifts $s balance, and strikes back at YOU!", TRUE,
	    victim, 0, attacker, TO_VICT);
	act("You completely sidestep $N's blow, shift your balance, and strike back at $M!", TRUE,
	    victim, 0, attacker, TO_CHAR);

	hit(victim, attacker, NULL);
	return true;
}

static bool rapier_dirk(P_char victim, P_char attacker)
{
	int chance, i, f;
	if (!victim || !char_in_list(victim) || !IS_ALIVE(victim) || !attacker ||
	    !char_in_list(attacker) || !IS_ALIVE(attacker))
		return FALSE;

	// The assumption made here is that the victim has 100 agi and wis.
	chance = ((GET_C_AGI(victim) + GET_C_WIS(victim)) / 2 + GET_LEVEL(victim));

	// Rapier_dirk_check() verifies the correct weapons in each hand. We now confirm
	// there are two weapon objects.
	P_obj wep1 = victim->equipment[PRIMARY_WEAPON];
	P_obj wep2 = victim->equipment[SECONDARY_WEAPON];
	auto refresh_dirk_participants = [&](const attack_continuation &continuation)
	{
		const attack_continuation_result after_callback =
			check_attack_continuation(continuation);
		if (!after_callback.can_continue())
			return false;

		victim = after_callback.actor;
		attacker = after_callback.target;
		wep1 = after_callback.weapon;
		return true;
	};

	// Off-hand special riposte
	if (rapier_dirk_check(victim) && chance > number(1, 1000) &&
	    MIN_POS(victim, POS_STANDING + STAT_NORMAL) && !IS_DRAGON(attacker) &&
	    GET_OPPONENT(victim) == attacker)
	{
		if (number(0, 1))
		{
			if (GET_CLASS(victim, CLASS_BARD) ||
			    GET_SPEC(victim, CLASS_ROGUE, SPEC_THIEF))
			{
				act("$n's $q &=LCflashes&n into the path of $N's attack, then $n delivers a graceful counter-attack!",
				    TRUE, victim, wep1, attacker, TO_NOTVICT);
				act("With &=LWunbelievable speed&n, $n's $q intercepts your attack, and then $n steps wide to deliver a graceful counter-attack!",
				    TRUE, victim, wep1, attacker, TO_VICT);
				act("With superb grace and ease, you intercept $N's attack with $q and counter-attack!",
				    TRUE, victim, wep1, attacker, TO_CHAR);
			}
			else
			{
				act("$n's $q &=LCflashes&n into the path of $N's attack, then $n delivers a graceful counter-attack!",
				    TRUE, victim, wep2, attacker, TO_NOTVICT);
				act("With &=LWunbelievable speed&n, $n's $q intercepts your attack, and then $n steps wide to deliver a graceful counter-attack!",
				    TRUE, victim, wep2, attacker, TO_VICT);
				act("With superb grace and ease, you intercept $N's attack with $q and counter-attack!",
				    TRUE, victim, wep2, attacker, TO_CHAR);
			}
		}
		else
		{
			act("$n's intercepts $N's attack with $q with eloquent ease.", TRUE, victim,
			    wep1, attacker, TO_NOTVICT);
			act("$n's skillfully shifts $s stance and $q &+Wglistens&n with insane speed to thwart your attack!",
			    TRUE, victim, wep1, attacker, TO_VICT);
			act("You play with $N before lunging into an offensive routine!", TRUE,
			    victim, wep1, attacker, TO_CHAR);
		}

		const attack_continuation first_dirk =
			begin_attack_continuation(victim, attacker, wep1, PRIMARY_WEAPON);
		hit(victim, attacker, wep1);
		if (!refresh_dirk_participants(first_dirk))
			return TRUE;

		// if(wep1->craftsmanship == OBJCRAFT_HIGHEST &&
		if (!number(0, 3) && CanDoFightMove(victim, attacker))
		{
			act("$p slices through the air with incredible ease!", TRUE, victim, wep1,
			    attacker, TO_CHAR);
			act("$p slices through the air with incredible ease!", TRUE, victim, wep1,
			    attacker, TO_ROOM);

			f = number(1, (int)(GET_LEVEL(victim) / 28));

			for (i = 0; i < f; i++)
			{
				const attack_continuation dirk_swing = begin_attack_continuation(
					victim, attacker, wep1, PRIMARY_WEAPON);
				hit(victim, attacker, wep1);
				if (!refresh_dirk_participants(dirk_swing))
					break;
			}
		}
		else
		{
			const attack_continuation followup_dirk =
				begin_attack_continuation(victim, attacker, wep1, PRIMARY_WEAPON);
			hit(victim, attacker, wep1);
			if (!refresh_dirk_participants(followup_dirk))
				return TRUE;
		}

		return TRUE;
	}

	return FALSE;
}

int parrySucceed(P_char victim, P_char attacker, P_obj wpn)
{
	int learnedvictim;
	int learnedattacker;
	int blindfightskl;
	bool npcepicparry = false;
	int expertparry = 0;

	if (!victim || !char_in_list(victim) || !IS_ALIVE(victim) || !attacker ||
	    !char_in_list(attacker) || !IS_ALIVE(attacker))
		return FALSE;

	learnedvictim = GET_CHAR_SKILL(victim, SKILL_PARRY);
	blindfightskl = GET_CHAR_SKILL(victim, SKILL_BLINDFIGHTING);

	if (GET_POS(victim) != POS_STANDING)
		return FALSE;

	if (affected_by_spell(victim, SPELL_COMBAT_MIND) && GET_CLASS(victim, CLASS_PSIONICIST))
		learnedvictim += GET_LEVEL(victim);

	if (learnedvictim < 1)
		return FALSE;

	// Monks and immaterial (ghosts, phantoms, etc...) may be parried when attacker
	// has a weapon.
	if (GET_CLASS(attacker, CLASS_MONK) || IS_IMMATERIAL(attacker))
		if (!attacker->equipment[WIELD] && !attacker->equipment[WIELD2])
			return FALSE;

	// Ensure the victim has a weapon for parrying.
	// May want to expand this in the future by adding modifiers
	// based on weapon types (e.g. maces are harder to parry with or
	// parry against).
	if (!victim->equipment[WIELD] && !victim->equipment[WIELD2] && !victim->equipment[WIELD3] &&
	    !victim->equipment[WIELD4])
		return FALSE;

	// Flaying weapons are !parry.
	if ((wpn && IS_FLAYING(wpn)) ||
	    (victim->equipment[WIELD] && IS_FLAYING(victim->equipment[WIELD])))
		return FALSE;

	if (affected_by_spell(victim, SKILL_RAGE) && attacker != GET_OPPONENT(victim))
		return FALSE;

	// Notching the parry skill fails the parry check.
	if (notch_skill(victim, SKILL_PARRY, get_property("skill.notch.defensive", 17)) &&
	    !affected_by_spell(victim, SPELL_COMBAT_MIND))
		return FALSE;

	// Victim's parry:

	if (affected_by_spell(victim, SPELL_COMBAT_MIND) && GET_CLASS(victim, CLASS_PSIONICIST))
		learnedvictim += (int)(GET_LEVEL(victim) / 2);
	learnedvictim += dex_app[STAT_INDEX(GET_C_DEX(victim))].reaction * 15;
	learnedvictim += wis_app[STAT_INDEX(GET_C_WIS(victim))].bonus * 5;

	if (learnedvictim < 1)
		return FALSE;

	// Attacker's parry:
	learnedattacker = WeaponSkill(attacker, wpn);
	learnedattacker += str_app[STAT_INDEX(GET_C_STR(attacker))].tohit * 10;
	learnedattacker += str_app[STAT_INDEX(GET_C_STR(attacker))].todam * 15;

	// If attacker is significantly stronger than the defender, parry is reduced.
	// This will benefit giants, dragons, etc... which is logical.
	if (GET_C_STR(attacker) > GET_C_STR(victim) + 35)
		learnedattacker += GET_C_STR(attacker) - 35 - GET_C_STR(victim);

	// Harder to parry incoming attacks when not standing.
	if (!MIN_POS(victim, POS_STANDING + STAT_NORMAL))
		learnedvictim = (int)(learnedvictim * 0.75);

	// Attackers are easier to parry when they are not standing.
	if (!MIN_POS(attacker, POS_STANDING + STAT_NORMAL))
		learnedattacker = (int)(learnedattacker * 0.65); // old 0.75

	if (IS_AFFECTED5(victim, AFF5_DAZZLEE))
		learnedvictim = (int)(learnedvictim * 0.00);

	if (affected_by_spell(victim, SKILL_GAZE))
		learnedvictim = (int)(learnedvictim * 0.80);

	// adding blindfighting check - Jexni 1/21/11
	if (IS_BLIND(victim))
		learnedvictim = (int)(blindfightskl > 0 ? (learnedvictim * (blindfightskl / 200)) :
							  (learnedvictim * 0.05));

	if (IS_STUNNED(victim))
		learnedvictim = (int)(learnedvictim * 0.90);

	// Elite warriors and paladins have maximum expert parry.
	if (IS_ELITE(victim))
	{
		if (GET_CLASS(victim, CLASS_WARRIOR) || GET_CLASS(victim, CLASS_PALADIN) ||
		    GET_CLASS(victim, CLASS_RANGER))
		{
			learnedvictim = (int)(learnedvictim * 1.25);
			npcepicparry = TRUE;
		}
	}

	// Player expert parry.
	if (!IS_NPC(victim) && GET_CHAR_SKILL(victim, SKILL_EXPERT_PARRY))
	{
		expertparry = GET_CHAR_SKILL(victim, SKILL_EXPERT_PARRY);
		// 125 percent max bonus

		learnedvictim = (int)(learnedvictim * (1 + expertparry / 400));
	}

	//  Blademasters and swashbucklers have a better chance.
	if (GET_SPEC(victim, CLASS_RANGER, SPEC_BLADEMASTER) ||
	    GET_SPEC(victim, CLASS_WARRIOR, SPEC_SWASHBUCKLER))
		learnedvictim = (int)(learnedvictim * 1.10);

	//  Better chance for ap's
	if ((GET_CLASS(victim, CLASS_ANTIPALADIN) || GET_CLASS(victim, CLASS_PALADIN)) &&
	    is_wielding_paladin_sword(victim))
		learnedvictim = (int)(learnedvictim * 1.15);

	// Harder to parry a swashbuckler.
	if (GET_SPEC(attacker, CLASS_WARRIOR, SPEC_SWASHBUCKLER))
		learnedattacker = (int)(learnedattacker * 0.80);

	// Random 5% change based on random luck comparison.
	if (number(0, GET_C_LUK(victim)) > number(0, GET_C_LUK(attacker)))
		learnedvictim = (int)(learnedvictim * 1.05);

	// Dragons are more difficult to parry, but not impossible.
	if (IS_GREATER_RACE(attacker))
		learnedattacker += GET_LEVEL(attacker);

	// Much harder to parry with fireweapons like a bow, but not impossible.
	P_obj weapon = victim->equipment[WIELD];

	if (weapon && GET_ITEM_TYPE(weapon) == ITEM_FIREWEAPON)
		learnedvictim /= 10;

	if (weapon)
		learnedattacker += (GET_OBJ_WEIGHT(weapon) * 2);

	// Harder to parry something you are not fighting.
	if (IS_PC(victim) && GET_OPPONENT(victim) != attacker)
		learnedvictim = (int)(learnedvictim * 0.90);

	// Generate attacker and victim ranges.
	int defroll, attroll;
	defroll = MAX(5, number(1, learnedvictim));
	attroll = MAX(5, number(1, learnedattacker));

	// debug("Defroll (%d), Attroll (%d)", defroll, attroll);

	// Simple parry success check via comparison of two constrained random numbers.
	if (attroll > defroll)
		return FALSE;

	if (rapier_dirk(victim, attacker))
		return TRUE;

	// Riposte check.
	if (try_riposte(victim, attacker, wpn))
		return TRUE;

	if (expertparry > number(1, 250) && !IS_NPC(victim))
	{
		act("You anticipate $n's maneuver and &+wmasterfully&n parry the attack.", FALSE,
		    attacker, 0, victim, TO_VICT | ACT_NOTTERSE);
		act("$N anticipates your attack and &+wmasterfully&n parries your blow.", FALSE,
		    attacker, 0, victim, TO_CHAR | ACT_NOTTERSE);
		act("$N anticipates $n's attack and &+wmasterfully&n parries the incoming blow.",
		    FALSE, attacker, 0, victim, TO_NOTVICT | ACT_NOTTERSE);
	}
	else if (((npcepicparry == true) && !number(0, 12)) && IS_NPC(victim))
	{
		//  act("You anticipate $n's maneuver and &+wmasterfully&n parry the attack.", FALSE, attacker, 0, victim,
		//      TO_VICT | ACT_NOTTERSE);
		act("$N anticipates your attack and &+wmasterfully&n parries your blow.", FALSE,
		    attacker, 0, victim, TO_CHAR | ACT_NOTTERSE);
		act("$N anticipates $n's attack and &+wmasterfully&n parries the incoming blow.",
		    FALSE, attacker, 0, victim, TO_NOTVICT | ACT_NOTTERSE);
	}
	else
	{
		act("You parry $n's lunge at you.", FALSE, attacker, 0, victim,
		    TO_VICT | ACT_NOTTERSE);
		act("$N parries your futile lunge at $M.", FALSE, attacker, 0, victim,
		    TO_CHAR | ACT_NOTTERSE);
		act("$N parries $n's lunge at $M.", FALSE, attacker, 0, victim,
		    TO_NOTVICT | ACT_NOTTERSE);
	}
	return TRUE;
}

// Attempt by a berserker to slam weapon aside and deal damage, possibly disarming opponent in the process.
// Similar to riposte, does not require a successful parry though.
// ch is the mangler, and victim is the person attacking (that might be mangled).  It's important
//   to have it this way for the help files to be correct.
bool mangleSucceed(P_char ch, P_char victim, P_obj weap)
{
	bool skill_notch = FALSE;
	int skl, wloc;
	struct damage_messages messages = { "You &+rmangle&n $S forearm with $q.",
					    "$n &+rmangles&n you with $q.",
					    "$n &+rmangles&n $N with $q.",
					    "You &+Rmangle&N $N to death.",
					    "$n &+Rmangles&n you to death.",
					    "$n &+Rmangles&n $N to death.",
					    DAMMSG_TERSE,
					    weap };

	// Can't mangle vs a non-weapon.
	if (!weap || (weap->type != ITEM_WEAPON) || IS_SET(weap->extra_flags, ITEM_NODROP))
		return FALSE;

	// Make sure weap is wielded.
	if (weap == victim->equipment[WIELD])
		wloc = WIELD;
	else if (weap == victim->equipment[WIELD2])
		wloc = WIELD2;
	else if (weap == victim->equipment[WIELD3])
		wloc = WIELD3;
	else if (weap == victim->equipment[WIELD4])
		wloc = WIELD4;
	else
		return FALSE;

	/* Checked above now, and has weapon as argument to function.
	// Find a weapon to disarm (must exist and be a weapon and not be cursed).
	if( (weap = victim->equipment[WIELD]) == NULL || (weap->type != ITEM_WEAPON) || IS_SET(weap->extra_flags, ITEM_NODROP) )
	  if( (weap = victim->equipment[WIELD2]) == NULL || (weap->type != ITEM_WEAPON) || IS_SET(weap->extra_flags, ITEM_NODROP) )
	    if( (weap = victim->equipment[WIELD3]) == NULL || (weap->type != ITEM_WEAPON) || IS_SET(weap->extra_flags, ITEM_NODROP) )
	      if( (weap = victim->equipment[WIELD4]) == NULL || (weap->type != ITEM_WEAPON) || IS_SET(weap->extra_flags, ITEM_NODROP) )
	        return FALSE;
	*/

	skl = GET_CHAR_SKILL(ch, SKILL_MANGLE);

	if (IS_TRUSTED(victim) || IS_IMMOBILE(ch) || !IS_HUMANOID(victim) ||
	    !MIN_POS(ch, POS_STANDING + STAT_NORMAL) ||
	    (skl < 20 && !(skill_notch = notch_skill(ch, SKILL_MANGLE,
						     get_property("skill.notch.defensive", 17)))))
	{
		return FALSE;
	}

	if (!skill_notch)
	{
		// Epic parry now reduces mangle percentage. Jan08 -Lucrot
		if (GET_CHAR_SKILL(victim, SKILL_EXPERT_PARRY))
		{
			if ((skl -= GET_CHAR_SKILL(victim, SKILL_EXPERT_PARRY) / 2) < 20)
				return FALSE;
		}

		// Elite mobs are not affected as much. Jan08 -Lucrot
		if (IS_ELITE(victim))
			skl = 20;

		if (IS_ELITE(ch))
			skl += 100;

		if ((skl -= (GET_C_DEX(victim) - GET_C_DEX(ch)) * 5) < 20)
			return FALSE;

		skl /= 20;
		skl = BOUNDED(0, skl, 5);

		// 5% max chance.
		if (number(1, 100) > skl)
			return FALSE;
	}

	act("$n blocks your attack and slashes viciously at your arm.", TRUE, ch, 0, victim,
	    TO_VICT);
	act("$n blocks $N's attack and slashes viciously at $S arm.", TRUE, ch, 0, victim,
	    TO_NOTVICT);
	act("You block $N's attack and slash viciously at $S arm.", TRUE, ch, 0, victim, TO_CHAR);

	// 2-8 damage - can be mitigated.
	if (melee_damage(ch, victim, 4. * dice(2, 4), PHSDAM_NONE, &messages) != DAM_NONEDEAD)
		return TRUE;

	// 25% chance to actually disarm.
	if (weap && !number(0, 3) && !affected_by_spell(victim, SPELL_COMBAT_MIND))
	{
		send_to_char(
			"&=LYYou swing at your foe _really_ badly, losing control of your weapon!\r\n",
			victim);
		act("$n stumbles with $s attack, losing control of $s weapon!", TRUE, victim, 0, 0,
		    TO_ROOM);

		set_short_affected_by(victim, SKILL_DISARM, 2 * PULSE_VIOLENCE);

		unequip_char(victim, wloc);
		obj_to_char(weap, victim);

		char_light(victim);
		room_light(victim->in_room, REAL);
	}
	else
	{
		send_to_char("You stumble, but recover in time!\r\n", victim);
	}

	return TRUE;
}

static int try_riposte(P_char ch, P_char victim, P_obj wpn)
{
	int expertriposte = 0, victim_dead;
	int randomnumber = number(1, 1000);
	double skl;
	bool npcepicriposte = FALSE;

	if (!char_in_list(ch) || !char_in_list(victim) || !IS_ALIVE(victim) || !IS_ALIVE(ch))
		return FALSE;

	const attack_continuation participants = begin_attack_continuation(ch, victim);
	const auto participants_valid = [&]()
	{
		const attack_continuation_result checked = check_attack_continuation(participants);
		if (!checked.can_continue())
			return false;
		ch = checked.actor;
		victim = checked.target;
		return true;
	};
	if (!participants_valid())
		return FALSE;

	// Innate two daggers is static at 5%.
	if (innate_two_daggers(ch) && !number(0, 19))
	{
		act("$n flourishes $s dagger, intercepting $N's attack, and gracefully counters!",
		    TRUE, ch, 0, victim, TO_NOTVICT);
		act("$n thrusts forth $s dagger, intercepting your attack, and counters it gracefully!",
		    TRUE, ch, 0, victim, TO_VICT);
		act("You brandish your offhand dagger, intercepting $N's blow, and countering his attack!",
		    TRUE, ch, 0, victim, TO_CHAR);

		P_obj secondary = ch->equipment[SECONDARY_WEAPON];
		const attack_continuation secondary_continuation =
			begin_attack_continuation(ch, victim, secondary, SECONDARY_WEAPON);
		hit(ch, victim, ch->equipment[PRIMARY_WEAPON]);
		if (participants_valid())
		{
			const attack_continuation_result checked =
				check_attack_continuation(secondary_continuation);
			if (checked.can_continue())
				hit(checked.actor, checked.target, checked.weapon);
		}
		return TRUE;
	}

	// No longer able to riposte without the skill.
	if ((skl = GET_CHAR_SKILL(ch, SKILL_RIPOSTE)) < 1)
		return FALSE;

	// Notching the skill means failing the riposte.
	if (notch_skill(ch, SKILL_RIPOSTE, get_property("skill.notch.defensive", 17)))
		return FALSE;

	// Skill range is 1 to 100.
	expertriposte = GET_CHAR_SKILL(ch, SKILL_EXPERT_RIPOSTE);

	// Elite mobiles receive the expert riposte skill.
	if (IS_ELITE(ch))
	{
		if (GET_CLASS(ch, CLASS_WARRIOR) || GET_CLASS(ch, CLASS_ANTIPALADIN) ||
		    GET_CLASS(ch, CLASS_DREADLORD) || GET_CLASS(ch, CLASS_AVENGER))
		{
			skl += 100;
			npcepicriposte = TRUE;
		}
	}

	/*  Blademasters are slightly better, though they don't get
	 *  the full riposte skill
	 */
	if (GET_SPEC(ch, CLASS_RANGER, SPEC_BLADEMASTER))
		skl *= 1.05;

	/*  Hey, lets do the same for Swashbucklers */
	if (GET_SPEC(ch, CLASS_WARRIOR, SPEC_SWASHBUCKLER))
		skl *= 1.10;

	/* lucky or unlucky? */
	if (number(0, GET_C_LUK(ch)) > number(0, GET_C_LUK(victim)))
		skl *= 1.05;
	else
		skl *= 0.95;

	// Harder to riposte while stunned.
	if (IS_STUNNED(ch))
		skl *= 0.50;

	// Easier to riposte versus stunned attacker.
	if (IS_STUNNED(victim))
		skl *= 1.15;
	// Much harder to riposte while knocked down.
	if (!MIN_POS(ch, POS_STANDING + STAT_NORMAL))
		skl *= 0.50;

	// Much harder to riposte something you are not fighting.
	if (GET_OPPONENT(ch) != victim)
		skl *= 0.50;

	if (IS_AFFECTED5(ch, AFF5_DAZZLEE))
		skl *= 0.95;

	// Expert riposte.
	if (expertriposte)
		skl += expertriposte;

	// Simple comparison.
	if (randomnumber > skl)
		return FALSE;

	int weapon_slot = PRIMARY_WEAPON;
	if (ch->equipment[FOURTH_WEAPON] && !number(0, 4))
		weapon_slot = FOURTH_WEAPON;
	else if (ch->equipment[THIRD_WEAPON] && !number(0, 3))
		weapon_slot = THIRD_WEAPON;
	else if (ch->equipment[SECONDARY_WEAPON] && !number(0, 2))
		weapon_slot = SECONDARY_WEAPON;
	wpn = ch->equipment[weapon_slot];
	const attack_continuation continuation =
		begin_attack_continuation(ch, victim, wpn, weapon_slot);
	const auto continuation_valid = [&]()
	{
		const attack_continuation_result checked = check_attack_continuation(continuation);
		if (!checked.can_continue())
			return false;
		ch = checked.actor;
		victim = checked.target;
		wpn = checked.weapon;
		return true;
	};

	if (expertriposte > number(1, 500) && GET_OPPONENT(ch) == victim)
	{
		act("$n &+wslams aside&n $N's attack and then pounds $M!", TRUE, ch, 0, victim,
		    TO_NOTVICT);
		act("$n &+wslams aside&n your attack and counters!", TRUE, ch, 0, victim, TO_VICT);
		act("You &+wslam aside&n $N's attack and counter!", TRUE, ch, 0, victim, TO_CHAR);

		hit(ch, victim, wpn);

		if (!continuation_valid())
			return TRUE;
		if (expertriposte > number(1, 500))
			hit(ch, victim, wpn);
	}
	else if ((npcepicriposte == TRUE) && !number(0, 4) && GET_OPPONENT(ch) == victim)
	{
		act("$n &+wslams aside&n $N's attack and then pounds $M!", TRUE, ch, 0, victim,
		    TO_NOTVICT);
		act("$n &+wslams aside&n your attack and counters!", TRUE, ch, 0, victim, TO_VICT);
		act("You &+wslam aside&n $N's attack and counter!", TRUE, ch, 0, victim, TO_CHAR);

		hit(ch, victim, wpn);

		if (!continuation_valid())
			return TRUE;
		if (!number(0, 1))
			hit(ch, victim, wpn);
	}
	else
	{
		act("$n deflects $N's blow and strikes back at $M!", TRUE, ch, 0, victim,
		    TO_NOTVICT);
		act("$n deflects your blow and strikes back at YOU!", TRUE, ch, 0, victim, TO_VICT);
		act("You deflect $N's blow and strike back at $M!", TRUE, ch, 0, victim, TO_CHAR);
	}

	if (!continuation_valid())
		return TRUE;
	hit(ch, victim, wpn);

	if (!continuation_valid())
		return TRUE;
	if (GET_CLASS(ch, CLASS_BERSERKER))
	{
		if (affected_by_spell(ch, SKILL_BERSERK))
			hit(ch, victim, wpn);
	} // new zerker stuff

	if (!continuation_valid())
		return TRUE;
	if ((skl = GET_CHAR_SKILL(ch, SKILL_FOLLOWUP_RIPOSTE)) > 0)
	{
		notch_skill(ch, SKILL_FOLLOWUP_RIPOSTE, get_property("skill.notch.defensive", 17));

		if (number(0, 1) == 0)
		{
			if (skl / 3 > number(0, 100))
			{
				act("Before $N can recover $n steps forward and slams $s fist into $S face.",
				    TRUE, ch, 0, victim, TO_NOTVICT);
				act("Before $E can recover you step forward and slam your fist into $S face.",
				    TRUE, ch, 0, victim, TO_CHAR);
				act("Before you can recover $n steps forward and slams $s fist into your face!",
				    TRUE, ch, 0, victim, TO_VICT);
				victim_dead = damage(ch, victim,
						     static_cast<double>(GET_DAMROLL(ch)) *
							     ch->specials.damage_mod,
						     SKILL_FOLLOWUP_RIPOSTE);

				if (!victim_dead && continuation_valid() &&
				    skl / 3 > number(0, 100))
				{
					act("As $N staggers back $n follows-up with a well-placed kick.",
					    TRUE, ch, 0, victim, TO_NOTVICT);
					act("As $E staggers back you follow-up with a well-placed kick.",
					    TRUE, ch, 0, victim, TO_CHAR);
					act("As you stagger from the blow $n follows-up with a well-placed kick.",
					    TRUE, ch, 0, victim, TO_VICT);
					melee_damage(ch, victim, dice(20, 6), 0, 0);
				}
			}
		}
		else if (skl / 3 > number(0, 100))
		{
			act("Spinning around $n slams his elbow into $N's throat.", TRUE, ch, 0,
			    victim, TO_NOTVICT);
			act("Before you can react $n spins around and slams $s elbow into your throat.",
			    TRUE, ch, 0, victim, TO_VICT);
			act("Spinning around you slam your elbow into $N's throat.", TRUE, ch, 0,
			    victim, TO_CHAR);
			victim_dead = damage(ch, victim,
					     static_cast<double>(GET_DAMROLL(ch)) *
						     ch->specials.damage_mod,
					     SKILL_FOLLOWUP_RIPOSTE);

			if (!victim_dead && continuation_valid() && skl / 3 > number(0, 100))
			{
				act("...then steps forward and brutally slams $s head into $N's face.",
				    TRUE, ch, 0, victim, TO_NOTVICT);
				act("As $E gasps for breath you step forward and slam your head into $S face. ",
				    TRUE, ch, 0, victim, TO_CHAR);
				act("As you stagger and gasp for breath $e steps forward and slams $s head into your face. Yikes!",
				    TRUE, ch, 0, victim, TO_VICT);
				damage(ch, victim, dice(20, 6), SKILL_FOLLOWUP_RIPOSTE);
			}
		}
	}
	return TRUE;
}

bool fear_check(P_char ch, bool force)
{
	if (!IS_ALIVE(ch))
		return TRUE;

	if (IS_CONSTRUCT(ch))
		return FALSE;

	if (affected_by_spell(ch, SPELL_INDOMITABILITY))
	{
		act("&+WBeing blessed by protective spirits, you manage to withstand the fear.",
		    FALSE, ch, 0, 0, TO_CHAR);
		act("&+WBeing blessed by protective spirits, $n&+W manages to withstand the fear.",
		    FALSE, ch, 0, 0, TO_ROOM);
		return TRUE;
	}

	if (IS_SET(ch->specials.act, ACT_ELITE))
	{
		act("&+WYou are simply fearless!", FALSE, ch, 0, 0, TO_CHAR);
		act("&+W$n&+W is simply fearless!", FALSE, ch, 0, 0, TO_ROOM);
		return TRUE;
	}

	if (GET_SPEC(ch, CLASS_WARRIOR, SPEC_GUARDIAN))
	{
		act("&+WYou are simply fearless!", FALSE, ch, 0, 0, TO_CHAR);
		act("&+W$n&+W is simply fearless!", FALSE, ch, 0, 0, TO_ROOM);
		return TRUE;
	}

	if (!force && IS_AFFECTED4(ch, AFF4_NOFEAR))
	{
		act("&+WYou are simply fearless!", FALSE, ch, 0, 0, TO_CHAR);
		act("&+W$n&+W is simply fearless!", FALSE, ch, 0, 0, TO_ROOM);
		return TRUE;
	}

	if (GET_CHAR_SKILL(ch, SKILL_INDOMITABLE_RAGE) > number(1, 150) &&
	    affected_by_spell(ch, SKILL_BERSERK))
	{
		act("&+RWhat, and miss out on the glorious bloodbath? I think not....&n", FALSE, ch,
		    0, 0, TO_CHAR);
		act("&+R$n&+R snivels derisively for a moment, too caught up in his rage to acknowledge fear itself!&n",
		    FALSE, ch, 0, 0, TO_ROOM);
		return TRUE;
	}

	if (GET_CLASS(ch, CLASS_DREADLORD))
	{
		act("&+LBeing fear incarnate, you laugh derisively at the attempt to scare you into submission!&n",
		    FALSE, ch, 0, 0, TO_CHAR);
		act("&+LThe fearful visage has no affect on $n&+L!&n", FALSE, ch, 0, 0, TO_ROOM);
		return TRUE;
	}

	if (!force && IS_GREATER_RACE(ch) && GET_LEVEL(ch) >= 51)
	{
		act("&+yFear is for the weak willed... which you are not!&n", FALSE, ch, 0, 0,
		    TO_CHAR);
		return TRUE;
	}

	return FALSE;
}
