#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include "magic/spell_words_of_power.h"
#include "classes/necromancy.h"

#define WISPY_BAND_VNUM 424

extern P_room world;
extern P_index mob_index;

void astral_banishment(P_char ch, P_char victim, int hwordtype, int level)
{
	// hwordtype: 1 = holy word
	//            2 = unholy word
	P_obj wispy_band;
	bool has_band;
	int new_room, lev = GET_LEVEL(victim);

	if (IS_NPC(ch) || IS_NPC(victim))
		return;

	has_band = FALSE;
	for (int i = 0; i < MAX_WEAR; i++)
	{
		if (((wispy_band = victim->equipment[i]) != NULL) &&
		    (OBJ_VNUM(wispy_band) == WISPY_BAND_VNUM))
		{
			has_band = TRUE;
			break;
		}
	}

	if (GET_RACE(victim) ==
		    (hwordtype == BANISHMENT_HOLY_WORD ? RACE_GITHYANKI : RACE_GITHZERAI) &&
	    number(0, 100) < 2 + BOUNDED(-2, level - lev, 2) && !IS_HOMETOWN(ch->in_room) &&
	    !IS_ROOM(ch->in_room, ROOM_NO_TELEPORT) &&
	    !(world[ch->in_room].sector_type == SECT_OCEAN))
	{
		if (has_band)
		{
			act("You are protected by a powerful force from $p, and the banishment fails.",
			    FALSE, ch, wispy_band, victim, TO_VICT);
			return;
		}

		if (hwordtype == BANISHMENT_HOLY_WORD)
		{
			act("You banish the evil $N to the astral plane with your holy word.",
			    FALSE, ch, 0, victim, TO_CHAR);
			act("The power of the holy word causes you to flee to the astral plane.",
			    FALSE, ch, 0, victim, TO_VICT);
		}
		if (hwordtype == BANISHMENT_UNHOLY_WORD)
		{
			act("You banish the good $N to the astral plane with your unholy word.",
			    FALSE, ch, 0, victim, TO_CHAR);
			act("The power of the unholy word causes you to flee to the astral plane.",
			    FALSE, ch, 0, victim, TO_VICT);
		}
		act("Terror flashes in $N's eyes and $E is no more.", FALSE, ch, 0, victim,
		    TO_NOTVICT);
		affect_from_char(victim, TAG_PVPDELAY);
		char_from_room(victim);
		new_room = real_room(number(ASTRAL_VNUM_BEGIN, ASTRAL_VNUM_END));
		char_to_room(victim, new_room, -1);
	}
}

static void spell_single_banish(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
				P_obj /*obj*/)
{
	int chance;

	if (!ch)
	{
		logit(LOG_EXIT, "spell_single_banish called in magic.c with no ch");
		return;
	}

	if (IS_TRUSTED(victim) || IS_PC(victim))
	{
		return;
	}

	chance = (int)(((40 * level) + (10 * GET_CHAR_SKILL(ch, SKILL_DEVOTION))) /
		       (GET_LEVEL(victim) + 1));

	if (GET_LEVEL(ch) > (10 + GET_LEVEL(victim)))
	{
		chance = 95;
	}

	if (GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT))
	{
		chance = (int)(chance * 1.25);
	}

	if (IS_GREATER_DRACO(victim) || IS_GREATER_AVATAR(victim))
	{
		chance = (int)(chance * 1 / 3);
	}

	chance = BOUNDED(1, chance, 95);

	debug("(%s) attempts to banish (%s) with (%d) chance.", GET_NAME(ch), GET_NAME(victim),
	      chance);

	if (chance > number(1, 100))
	{
		if (IS_UNDEADRACE(victim) || IS_ANGEL(victim))
		{
			act("You break the binding energies and watch as $N crumbles to dust.",
			    FALSE, ch, 0, victim, TO_CHAR);
			act("$N &+Lsuddenly becomes lifeless once more and crumbles to dust.&n",
			    FALSE, ch, 0, victim, TO_NOTVICT);
			check_saved_corpse(victim);
			extract_char(victim);
			return;
		}

		switch (GET_RACE(victim))
		{
		case RACE_W_ELEMENTAL:
			act("You banish $N back to the plane of water.", FALSE, ch, 0, victim,
			    TO_CHAR);
			act("$n banishes $N back to the plane of water.", FALSE, ch, 0, victim,
			    TO_NOTVICT);
			break;
		case RACE_A_ELEMENTAL:
			act("You banish $N back to the plane of air.", FALSE, ch, 0, victim,
			    TO_CHAR);
			act("$n banishes $N back to the plane of air.", FALSE, ch, 0, victim,
			    TO_NOTVICT);
			break;
		case RACE_E_ELEMENTAL:
			act("You banish $N back to the plane of earth.", FALSE, ch, 0, victim,
			    TO_CHAR);
			act("$n banishes $N back to the plane of earth.", FALSE, ch, 0, victim,
			    TO_NOTVICT);
			break;
		case RACE_EFREET:
			act("You banish $N back to the plane of fire.", FALSE, ch, 0, victim,
			    TO_CHAR);
			act("$n banishes $N back to the plane of fire.", FALSE, ch, 0, victim,
			    TO_NOTVICT);
			break;
		case RACE_F_ELEMENTAL:
			act("You banish $N back to the plane of fire.", FALSE, ch, 0, victim,
			    TO_CHAR);
			act("$n banishes $N back to the plane of fire.", FALSE, ch, 0, victim,
			    TO_NOTVICT);
			break;
		case RACE_DRACOLICH:
			act("Arcs of divine flames leap from your hands reducing $N to ash.", FALSE,
			    ch, 0, victim, TO_CHAR);
			act("Divine flames coil around $N reducing it to a pile of ash.", FALSE, ch,
			    0, victim, TO_NOTVICT);
			break;
		case RACE_GOLEM:
			act("Having lost its binding magic $N topples over and falls apart.", FALSE,
			    ch, 0, victim, TO_CHAR);
			act("$N abruply stiffens and then simply falls apart.", FALSE, ch, 0,
			    victim, TO_NOTVICT);
			break;
		default:
			act("The magic binding $N is unraveled and $E simply ceases to be.", FALSE,
			    ch, 0, victim, TO_CHAR);
			act("Divine power sweeps over $N and $E is no more.", FALSE, ch, 0, victim,
			    TO_NOTVICT);
			break;
		}
		check_saved_corpse(victim);
		extract_char(victim);
	}
}

bool can_banish(P_char ch, P_char victim)
{
	if (!ch) // Something is amiss.
	{
		logit(LOG_EXIT, "can_banish called in magic.c with no ch");
		return FALSE;
	}

	if (IS_NPC(victim))
	{
		if (mob_index[GET_RNUM(victim)].virtual_number == 250 ||
		    mob_index[GET_RNUM(victim)].virtual_number == 63)
		{
			return TRUE;
		}

		if (GET_RACE(victim) == RACE_E_ELEMENTAL &&
		    world[victim->in_room].sector_type == SECT_EARTH_PLANE)
		{
			send_to_char("This is the earth plane. Your banish spell fails!", ch);
			return FALSE;
		}

		if ((GET_RACE(victim) == RACE_F_ELEMENTAL || GET_RACE(victim) == RACE_EFREET) &&
		    world[victim->in_room].sector_type == SECT_FIREPLANE)
		{
			send_to_char("This is the fire plane. Your banish spell fails!", ch);
			return FALSE;
		}

		if (GET_RACE(victim) == RACE_A_ELEMENTAL &&
		    world[victim->in_room].sector_type == SECT_AIR_PLANE)
		{
			send_to_char("This is the air plane. Your banish spell fails!", ch);
			return FALSE;
		}

		if (GET_RACE(victim) == RACE_W_ELEMENTAL &&
		    world[victim->in_room].sector_type == SECT_WATER_PLANE)
		{
			send_to_char("This is the water plane. Your banish spell fails!", ch);
			return FALSE;
		}

		if (IS_ANGEL(victim))
		{
			if (world[victim->in_room].sector_type == SECT_ETHEREAL)
			{
				send_to_char("This is the ethereal plane. Your banish spell fails!",
					     ch);
				return FALSE;
			}
			else
			{
				return TRUE;
			}
		}

		if (IS_UNDEADRACE(victim))
		{
			if (world[victim->in_room].sector_type == SECT_NEG_PLANE)
			{
				send_to_char("This is the negative plane. Your banish spell fails!",
					     ch);
				return FALSE;
			}
			else
			{
				return TRUE;
			}
		}

		switch (GET_RACE(victim))
		{
		case RACE_E_ELEMENTAL:
		case RACE_F_ELEMENTAL:
		case RACE_EFREET:
		case RACE_A_ELEMENTAL:
		case RACE_W_ELEMENTAL:
		case RACE_I_ELEMENTAL:
			return TRUE;
			break;
		default:
			if (affected_by_spell(victim, TAG_CONJURED_PET))
			{
				return TRUE;
			}
			return FALSE;
			break;
		}
	}

	return FALSE;
}

void spell_banish(int level, P_char ch, char *arg, int type, P_char /*victim*/, P_obj /*obj*/)
{
	P_char tch, next;
	char buf[256];

	snprintf(buf, 256,
		 "You raise your holy symbol and send a prayer to %s to aid you in battle.",
		 get_god_name(ch));
	act(buf, FALSE, ch, 0, 0, TO_CHAR);
	snprintf(buf, 256, "$n raises $s holy symbol sending a battle prayer to %s.",
		 get_god_name(ch));
	act(buf, FALSE, ch, 0, 0, TO_ROOM);

	for (tch = world[ch->in_room].people; tch; tch = next)
	{
		next = tch->next_in_room;

		if (get_linked_char(tch, LNK_PET) && can_banish(ch, tch))
		{
			attack_back(ch, tch, FALSE);
		}
	}

	for (tch = world[ch->in_room].people; tch; tch = next)
	{
		next = tch->next_in_room;

		if (get_linked_char(tch, LNK_PET) && can_banish(ch, tch))
		{
			spell_single_banish(level, ch, arg, type, tch, 0);
		}
	}

	// cast_as_damage_area(ch, spell_single_banish, level, victim,
	// get_property("spell.area.minChance.banish", 10),
	// get_property("spell.area.chanceStep.banish", 25), can_banish);
}

void single_unholy_word(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			P_obj /*obj*/)
{
	int lev;

	struct damage_messages messages = { "You send $N reeling with your word of power.",
					    "You are sent reeling by $n's unholy word.",
					    "$n sends $N reeling with an unholy word.",
					    "$N dies instantly from the power of your unholy word.",
					    "You hear a word of power, and die instantly.",
					    "$N hears $n's word of power, and nothing more.",
					    0 };

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim) || ch->in_room != victim->in_room)
	{
		return;
	}

	if (IS_NPC(victim) && IS_AFFECTED4(victim, AFF4_HELLFIRE))
	{
		int rand1 = (number(1, 100));
		if (rand1 > victim->base_stats.Pow)
		{
			act("&+LThe &+rHells&+L seem to heed the &+Ccall&+L of your &+rword&+W...",
			    FALSE, ch, 0, victim, TO_CHAR);
			act("&+LThe &+rHells&+L seem to heed the &+Ccall&+L of $n&+L's &+rword&+W...",
			    TRUE, ch, 0, victim, TO_NOTVICT);
			act("&+LA &+Ldark &+Ylight &+Ldecends upon $N&+L, returning its &+rfl&+Ram&+Yes &+Wof &+Rhell &+Lback into the abyss.",
			    FALSE, ch, 0, victim, TO_CHAR);
			act("&+LA &+Ldark &+Ylight &+Ldecends upon $N&+W, returning its &+rfl&+Ram&+Yes &+Wof &+Rhell &+Lback into the abyss.",
			    TRUE, ch, 0, victim, TO_NOTVICT);
			REMOVE_BIT(victim->specials.affected_by4, AFF4_HELLFIRE);
		}
	}

	if (GET_ALIGNMENT(victim) < 0 && !(IS_PC(victim) && opposite_racewar(victim, ch)))
	{
		act("$N is not good enough to be affected!", TRUE, ch, 0, victim, TO_CHAR);
		return;
	}

	if ((lev = GET_LEVEL(victim)) < (level / 3))
	{ /* < 7-15 death */
		if (spell_damage(ch, victim, 1000, SPLDAM_HOLY, 0, &messages) != DAM_NONEDEAD)
			return;
	}
	else
	{
		int dam = level * 3 + 20;
		dam = GET_RACE(victim) == RACE_GITHZERAI ? (int)(dam * 1.5) : dam;
		dam = dam * get_property("spell.area.damage.factor.unholyWord", 1.000);

		if (GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT))
		{
			dam = (int)(dam * 1.25);
		}

		if (spell_damage(ch, victim, dam, SPLDAM_HOLY, 0, &messages) == DAM_NONEDEAD)
		{
			if (lev < (level / 2 + 2)) /* 14-27 blind */
				spell_blindness(level, ch, 0, 0, victim, NULL); /* no save */
			if (lev < (level / 2 - 3)) /* 9-22 para */
				spell_minor_paralysis(level, ch, NULL, 0, victim, NULL);
		}
		astral_banishment(ch, victim, BANISHMENT_UNHOLY_WORD, level);
	}
}

void spell_unholy_word(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		       P_obj /*obj*/)
{
	if (!IS_ALIVE(ch))
	{
		return;
	}

	if (GET_LEVEL(ch) < MINLVLIMMORTAL)
	{
		if (GET_ALIGNMENT(ch) > 0 && IS_PC(ch))
		{
			act("Your word of power kills you instantly.", FALSE, ch, 0, victim,
			    TO_CHAR);
			act("$n's word of power destroys $m instantly.", FALSE, ch, 0, victim,
			    TO_NOTVICT);
			die(ch, ch);
			return;
		}
		else if (GET_ALIGNMENT(ch) > -350 && IS_PC(ch))
		{
			send_to_char("You feel foolish for even trying this, &+Ygoodie&n!\n", ch);
			return;
		}
		else if (GET_ALIGNMENT(ch) > -900)
		{
			send_to_char("You are not evil enough to utter such unholiness!\n", ch);
			return;
		}
	}

	cast_as_damage_area(ch, single_unholy_word, level, victim,
			    get_property("spell.area.minChance.unholyWord", 60),
			    get_property("spell.area.chanceStep.unholyWord", 20));
}

void single_voice_of_creation(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			      P_obj /*obj*/)
{
	int lev;

	struct damage_messages messages = {
		"&n&+bR&n&+cec&n&+Cit&n&+cin&n&+bg&n a &n&+Wholy &n&+Yprayer&n, you send $N reeling with &n&+Cr&n&+ci&n&+Cght&n&+ceou&n&+Cs &n&+Rpower&n.",
		"You are sent reeling by the &n&+Cr&n&+ci&n&+Cght&n&+ceou&n&+Cs &n&+Rpower&n of $n's &n&+Wholy &n&+Yprayer&n.",
		"$n &n&+br&n&+cec&n&+Ci&n&+cte&n&+bs&n a &n&+Wholy &n&+Yprayer&n and sends $N reeling with &n&+Cr&n&+ci&n&+Cght&n&+ceou&n&+Cs &n&+Rpower&n.",
		"$N dies instantly from the power of your holy word.",
		"You hear a word of power, and die instantly.",
		"$N hears $n's word of power, and nothing more.",
		0
	};

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	if (ch->in_room != victim->in_room)
		return;

	if (GET_ALIGNMENT(victim) > 0 && !(IS_PC(victim) && opposite_racewar(victim, ch)))
	{
		act("$N is not evil enough to be affected!", TRUE, ch, 0, victim, TO_CHAR);
		return;
	}

	if ((lev = GET_LEVEL(victim)) < (level / 3))
	{ /* < 7-15 death */
		if (spell_damage(ch, victim, 1000, SPLDAM_HOLY, 0, &messages) != DAM_NONEDEAD)
			return;
	}
	else
	{
		int dam = level * 3 + 20;
		dam = GET_RACE(victim) == RACE_GITHYANKI ? (int)(dam * 1.5) : dam;
		if (spell_damage(ch, victim, dam, SPLDAM_HOLY, 0, &messages) == DAM_NONEDEAD)
		{
			if (lev < (level / 2 + 2)) /* 14-27 blind */
				spell_blindness(level, ch, 0, 0, victim, NULL); /* no save */
			if (lev < (level / 2 - 3)) /* 9-22 para */
				spell_minor_paralysis(level, ch, NULL, 0, victim, NULL);
		}
		astral_banishment(ch, victim, BANISHMENT_HOLY_WORD, level);
	}
}

void single_holy_word(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		      P_obj /*obj*/)
{
	int lev;

	struct damage_messages messages = { "You send $N reeling with your word of power.",
					    "You are sent reeling by $n's holy word.",
					    "$n sends $N reeling with a holy word.",
					    "$N dies instantly from the power of your holy word.",
					    "You hear a word of power, and die instantly.",
					    "$N hears $n's word of power, and nothing more.",
					    0 };

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	if (ch->in_room != victim->in_room)
		return;

	if (IS_NPC(victim) && IS_AFFECTED4(victim, AFF4_HELLFIRE))
	{
		int rand1 = (number(1, 100));
		if (rand1 > victim->base_stats.Pow)
		{
			act("&+WThe Heavens seem to heed the &+Ccall&+W of your &+Lword&+W...",
			    FALSE, ch, 0, victim, TO_CHAR);
			act("&+WThe Heavens seem to heed the &+Ccall&+W of $n&+W's &+Lword&+W...",
			    TRUE, ch, 0, victim, TO_NOTVICT);
			act("&+CA br&+Wig&+Cht &+Ylight &+Wdecends upon $N&+W, causing its &+rfl&+Ram&+Yes &+Wof &+Rhell &+Wto slowly fade away.",
			    FALSE, ch, 0, victim, TO_CHAR);
			act("&+CA br&+Wig&+Cht &+Ylight &+Wdecends upon $N&+W, causing its &+rfl&+Ram&+Yes &+Wof &+Rhell &+Wto slowly fade away.",
			    TRUE, ch, 0, victim, TO_NOTVICT);
			REMOVE_BIT(victim->specials.affected_by4, AFF4_HELLFIRE);
		}
	}

	if (GET_ALIGNMENT(victim) > 0 && !(IS_PC(victim) && opposite_racewar(victim, ch)))
	{
		act("$N is not evil enough to be affected!", TRUE, ch, 0, victim, TO_CHAR);
		return;
	}

	if ((lev = GET_LEVEL(victim)) < (level / 3))
	{ /* < 7-15 death */
		if (spell_damage(ch, victim, 1000, SPLDAM_HOLY, 0, &messages) != DAM_NONEDEAD)
			return;
	}
	else
	{
		int dam = level * 3 + 20;
		dam = GET_RACE(victim) == RACE_GITHYANKI ? (int)(dam * 1.5) : dam;
		if (GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT))
		{
			dam = (int)(dam * 1.25);
		}

		if (spell_damage(ch, victim, dam, SPLDAM_HOLY, 0, &messages) == DAM_NONEDEAD)
		{
			if (lev < (level / 2 + 2)) /* 14-27 blind */
				spell_blindness(level, ch, 0, 0, victim, NULL); /* no save */
			if (lev < (level / 2 - 3)) /* 9-22 para */
				spell_minor_paralysis(level, ch, NULL, 0, victim, NULL);
		}
		astral_banishment(ch, victim, BANISHMENT_HOLY_WORD, level);
	}
}

void spell_voice_of_creation(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			     P_obj /*obj*/)
{
	if (!IS_ALIVE(ch))
	{
		return;
	}

	if (GET_LEVEL(ch) < MINLVLIMMORTAL)
	{
		if (GET_ALIGNMENT(ch) < 0 && IS_PC(ch))
		{
			act("The voice of creation kills you instantly.", FALSE, ch, 0, victim,
			    TO_CHAR);
			act("$n hears the voice of creation and it kills $m instantly.", FALSE, ch,
			    0, victim, TO_NOTVICT);
			die(ch, ch);
			return;
		}
		else if (GET_ALIGNMENT(ch) < 350 && IS_PC(ch))
		{
			send_to_char(
				"You are far too &+revil&n to say anything as &+Wholy&n as this!\n",
				ch);
			// act(messages.death_victim, FALSE, ch, 0, victim, TO_CHAR);
			// act(messages.death_room, FALSE, ch, 0, victim, TO_NOTVICT);
			// die(ch, ch);
			return;
		}
		else if (GET_ALIGNMENT(ch) < 900)
		{
			send_to_char("You are not good enough to speak a word of such holiness!\n",
				     ch);
			return;
		}
	}

	cast_as_damage_area(ch, single_voice_of_creation, level, victim,
			    get_property("spell.area.minChance.holyWord", 60),
			    get_property("spell.area.chanceStep.holyWord", 20));
}

void spell_holy_word(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		     P_obj /*obj*/)
{
	if (!IS_ALIVE(ch))
	{
		return;
	}

	if (GET_LEVEL(ch) < MINLVLIMMORTAL)
	{
		if (GET_ALIGNMENT(ch) < 0 && IS_PC(ch))
		{
			act("Your word of power kills you instantly.", FALSE, ch, 0, victim,
			    TO_CHAR);
			act("$n's word of power destroys $m instantly.", FALSE, ch, 0, victim,
			    TO_NOTVICT);
			die(ch, ch);
			return;
		}
		else if (GET_ALIGNMENT(ch) < 350 && IS_PC(ch))
		{
			send_to_char(
				"You are far too &+revil&n to say anything as &+Wholy&n as this!\n",
				ch);
			// act(messages.death_victim, FALSE, ch, 0, victim, TO_CHAR);
			// act(messages.death_room, FALSE, ch, 0, victim, TO_NOTVICT);
			// die(ch, ch);
			return;
		}
		else if (GET_ALIGNMENT(ch) < 900)
		{
			send_to_char("You are not good enough to speak a word of such holiness!\n",
				     ch);
			return;
		}
	}
	cast_as_damage_area(ch, single_holy_word, level, victim,
			    get_property("spell.area.minChance.holyWord", 60),
			    get_property("spell.area.chanceStep.holyWord", 20));
}

void spell_pword_kill(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		      P_obj /*obj*/)
{
	int dam;

	struct damage_messages messages = {
		"$N's life force is drained slightly by the power of your word.",
		"$n's word of power causes you to sag, and you feel your vitality draining away!",
		"$N seems to sag slightly, as $n viciously attacks $S life force.",
		"$N dies instantly from the power of your word.",
		"You hear a word of power, and die instantly.",
		"$N hears $n's word of power, and nothing more."
	};

	if (!IS_ALIVE(ch))
		return;

	// The spell no longer functions against the undead races.

	if (IS_UNDEADRACE(victim))
	{
		send_to_char(
			"&+LThe living dead are no longer affected by such an incantation...&n\r\n",
			ch);
		return;
	}

	// This is now a straight line percentage check for the victim to pass
	// a save versus spell or die().
	// If the caster is more than 15 levels higher than the victim, the spell
	// save check is automatically triggered.

	if (!IS_GREATER_RACE(victim) && !IS_TRUSTED(victim) && !IS_ELITE(victim) &&
	    (GET_HIT(victim) < 1500) && !NewSaves(victim, SAVING_SPELL, 15) &&
	    (((int)(level - GET_LEVEL(victim)) >= number(0, 99)) || level > GET_LEVEL(victim) + 15))
	{
		act("&+Y$N &+Ydies instantly from the power of your word.&n", FALSE, ch, 0, victim,
		    TO_CHAR);
		act("&+YYou hear a word of power, and die instantly.&n", FALSE, ch, 0, victim,
		    TO_VICT);
		act("&+Y$N &+Yhears $n&+Y's word of power, and nothing more.&n", FALSE, ch, 0,
		    victim, TO_NOTVICT);
		die(victim, ch);
	}
	else
	{
		dam = ((dice(3, 6) + level) * 4);

		if (IS_PC_PET(ch))
			dam /= 2;

		spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
	}
}
