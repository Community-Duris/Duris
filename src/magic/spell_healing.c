#include "core/prototypes.h"
#include "telemetry/telemetry_runtime.h"
#include "core/structs.h"
#include "world/db.h"
#include "world/hardcore_config.h"
#include "net/comm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/grapple.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include <stdio.h>
#include <strings.h>

void spell_cure_serious(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			P_obj /*obj*/)
{
	int healpoints;

	healpoints = dice(3, 8);
	heal(victim, ch, healpoints, GET_MAX_HIT(victim));

	send_to_char("&+WYou feel a lot better!\n", victim);

	update_pos(victim);
}

void spell_cure_critic(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		       P_obj /*obj*/)
{
	int healpoints;

	healpoints = dice(3, 10) + 10;
	heal(victim, ch, healpoints, GET_MAX_HIT(victim));
	send_to_char("&+WYou feel MUCH better!\n", victim);
	update_pos(victim);
}

void spell_cure_light(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		      P_obj /*obj*/)
{
	int healpoints;

	healpoints = number(2, 10);
	heal(victim, ch, healpoints, GET_MAX_HIT(victim));
	update_pos(victim);
	send_to_char("&+WYou feel a little better!\n", victim);
}

void spell_full_heal(int level, P_char ch, char * /*arg*/, int type, P_char victim, P_obj obj)
{
	int num_dice = 1;

	if (level >= 36)
		num_dice += 2;

	if (level >= 41)
		num_dice += 2;

	if (level >= 46)
		num_dice += 2;

	if (level >= 51)
		num_dice += 2;

	if (GET_SPEC(ch, CLASS_CLERIC, SPEC_HEALER))
	{
		if (level >= 52)
			num_dice += 1;

		if (level >= 53)
			num_dice += 1;

		if (level >= 54)
			num_dice += 1;

		if (level >= 55)
			num_dice += 1;

		if (level >= 56)
			num_dice += 1;
	}

	int healpoints = 250 + dice(num_dice, 20);

	/*if(GET_CLASS(victim, CLASS_ANTIPALADIN) && affected_by_spell(victim, SPELL_HELLFIRE) && (GET_RACEWAR(ch) == GET_RACEWAR(victim)))
	healpoints *= .3;*/

	/* Removing holy destruction spells from holymen.
	if(GET_SPEC(ch, CLASS_CLERIC, SPEC_HOLYMAN) && IS_PC(ch) && (ch != victim) && ((GET_RACEWAR(ch) != GET_RACEWAR(victim) && !IS_PC_PET(victim)) || (IS_NPC(victim) && !IS_PC_PET(victim))))
	{
	  struct damage_messages messages = {
	    "&+cYou call upon the &+Cmight&+c of your &+Wgod &+cto &+rde&+Rst&+Wroy &+cthe &+rbody &+cof $N&+c, who stumbles from the &+Cimpact&+c!",
	    "&+cThe &+Cmight&+c of &n$n's&+W god &+cis thrust upon your &+rbody&+c, causing massive &+Cdamage&+c!",
	    "&+cThe &+Cmight&+c of &n$n's&+W god &+cis thrust upon &n$N&+c, causing massive &+Cdamage&+c!",
	    "&+cYou &+Cdestroy &+cwhat little there is left of &n$N's &+cbody, leaving only a pool of &+rblood &+cand &+ysinew&+c!",
	    "&+cThe &+Cpower &+cof &n$n's&+W god&+c is the last thing your &+rbody &+cfeels before &+Cexploding &+cinto chunky bits.",
	    "&+cThe &+Cpower &+cof &n$n's&+W god&+c destroys &n$N's &+rbody&+c which explodes, leaving only a pool of &+rblood &+cand &+ysinew&+c!", 0  };

	  int dam = 10 * level + number(1, 25);

	  if(!NewSaves(victim, SAVING_SPELL, 0))
	    dam = (int) (dam * 2.0);

	  dam = (int) (dam * .75);

	  spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
	}
	else
	{
	*/

	if (type == SPELL_TYPE_SPELL)
	{
		if (GET_CHAR_SKILL(ch, SKILL_ANATOMY) &&
		    ((GET_CHAR_SKILL(ch, SKILL_ANATOMY) + 5) / 10) > number(0, 100))
		{
			act("$n quickly diagnoses your wounds.", FALSE, ch, 0, victim, TO_VICT);
			act("$n quickly diagnoses $N&n's wounds.", FALSE, ch, 0, victim,
			    TO_NOTVICT);
			act("You quickly diagnose $N&n's wounds and apply accurate healing.", FALSE,
			    ch, 0, victim, TO_CHAR);
			healpoints += number(10, 2 * GET_CHAR_SKILL(ch, SKILL_ANATOMY));
		}
	}

	if (ch == victim)
	{
		act("&+WA torrent of divine energy flows into your body, and your wounds begin to heal!",
		    FALSE, ch, 0, victim, TO_CHAR);
	}
	else
	{
		act("&+WA torrent of divine energy flows into $N&+W's body, and $S wounds begin to heal!",
		    FALSE, ch, 0, victim, TO_CHAR);
	}

	act("A torrent of divine energy flows from $n &ninto your body, and your wounds begin to heal!",
	    FALSE, ch, 0, victim, TO_VICT);
	act("&+WA torrent of divine energy flows from $n &+Winto $N&+W's body, and $S wounds begin to heal!",
	    FALSE, ch, 0, victim, TO_NOTVICT);

	spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
	grapple_heal(victim);
	heal(victim, ch, healpoints, GET_MAX_HIT(victim) - number(1, 4));
	update_pos(victim);
}

void spell_heal(int level, P_char ch, char * /*arg*/, int type, P_char victim, P_obj obj)
{
	int num_dice = 1, healpoints;

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	if (ch->in_room != victim->in_room)
	{
		act("You cannot find $N!", FALSE, ch, 0, victim, TO_CHAR);
		return;
	}

	if (GET_SPEC(ch, CLASS_CLERIC, SPEC_HEALER))
	{
		if (level >= 33)
			num_dice += 1;

		if (level >= 39)
			num_dice += 1;

		if (level >= 43)
			num_dice += 1;

		if (level >= 51)
			num_dice += 1;

		if (level >= 56)
			num_dice += 1;
	}
	else
	{
		if (level >= 26)
			num_dice += 1;

		if (level >= 31)
			num_dice += 1;

		if (level >= 36)
			num_dice += 1;

		if (level >= 41)
			num_dice += 1;
	}

	healpoints = 100 + dice(num_dice, 5);

	/* if(GET_CLASS(victim, CLASS_ANTIPALADIN) && affected_by_spell(victim, SPELL_HELLFIRE) && (GET_RACEWAR(ch) == GET_RACEWAR(victim)))
	 healpoints *= .3;*/

	/* Removing holy destruction spells from holymen.
	if(GET_SPEC(ch, CLASS_CLERIC, SPEC_HOLYMAN) && IS_PC(ch) && ((GET_RACEWAR(ch) != GET_RACEWAR(victim) && !IS_PC_PET(victim)) || (IS_NPC(victim) && !IS_PC_PET(victim))))
	{
	  struct damage_messages messages = {
	    "&+cYou call upon the &+Cmight&+c of your &+Wgod &+cto &+rde&+Rst&+Wroy &+cthe &+rbody &+cof $N&+c, who stumbles from the &+Cimpact&+c!",
	    "&+cThe &+Cmight&+c of &n$n's&+W god &+cis thrust upon your &+rbody&+c, causing &+Wsignificant &+Cdamage&+c!",
	    "&+cThe &+Cmight&+c of &n$n's&+W god &+cis thrust upon &n$N&+c, causing &+Wsignificant &+Cdamage&+c!",
	    "&+cYou &+Cdestroy &+cwhat little there is left of &n$N's &+cbody, leaving only a pool of &+rblood &+cand &+ysinew&+c!",
	    "&+cThe &+Cpower &+cof &n$n's&+W god&+c is the last thing your &+rbody &+cfeels before &+Cexploding &+cinto chunky bits.",
	    "&+cThe &+Cpower &+cof &n$n's&+W god&+c destroys &n$N's &+rbody&+c which explodes, leaving only a pool of &+rblood &+cand &+ysinew&+c!", 0  };

	  int dam = 5 * level + number(1, 25);

	  if(!NewSaves(victim, SAVING_SPELL, 0))
	    dam = (int) (dam * 2);

	  dam = (int) (dam * .75);

	  spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
	}
	else
	{
	*/

	if (type == SPELL_TYPE_SPELL)
	{
		if (GET_CHAR_SKILL(ch, SKILL_ANATOMY) &&
		    ((GET_CHAR_SKILL(ch, SKILL_ANATOMY) + 5) / 10) > number(0, 100))
		{
			act("$n quickly diagnoses your wounds.", FALSE, ch, 0, victim, TO_VICT);
			act("$n quickly diagnoses $N&n's wounds.", FALSE, ch, 0, victim,
			    TO_NOTVICT);
			act("You quickly diagnose $N&n's wounds and apply accurate healing.", FALSE,
			    ch, 0, victim, TO_CHAR);
			healpoints += number(1, (GET_CHAR_SKILL(ch, SKILL_ANATOMY) / 2));
		}
	}

	if (ch == victim)
	{
		act("&+WA warm &+yfeeling&n &+Wfills your body!", FALSE, ch, 0, victim, TO_CHAR);
	}
	else
	{
		act("&+WYou heal $N.", FALSE, ch, 0, victim, TO_CHAR);
	}

	act("&+WHealing energy flows from $n into your body!", FALSE, ch, 0, victim, TO_VICT);
	act("&+WHealing energy flows from $n into $N's body!", FALSE, ch, 0, victim, TO_NOTVICT);

	if (IS_BLIND(victim))
	{
		spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
	}

	grapple_heal(victim);
	heal(victim, ch, healpoints, GET_MAX_HIT(victim) - number(1, 4));
	update_pos(victim);
}

void spell_cure_blind(int /*level*/, P_char /*ch*/, char * /*arg*/, int /*type*/, P_char victim,
		      P_obj /*obj*/)
{
	if (IS_AFFECTED(victim, AFF_BLIND))
	{
		affect_from_char(victim, SPELL_BLINDNESS);
		REMOVE_BIT(victim->specials.affected_by, AFF_BLIND);
		telemetry_runtime_game_control_changed(victim);
		send_to_char("&+WYour vision returns!\n", victim);
	}
}

void spell_cure_disease(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			P_obj /*obj*/)
{
	if (affected_by_spell(victim, SPELL_DISEASE))
	{
		affect_from_char(victim, SPELL_DISEASE);
		send_to_char("You suddenly feel much much better.\n", victim);
		act("$n looks markedly better.", FALSE, victim, 0, 0, TO_ROOM);
	}
	else if (affected_by_spell(victim, SPELL_CONTAGION))
	{
		affect_from_char(victim, SPELL_CONTAGION);
		send_to_char("You suddenly feel much much better.\n", victim);
		act("$n looks markedly better.", FALSE, victim, 0, 0, TO_ROOM);
	}
	else
		send_to_char("There is no noticeable effect.\n", ch);
}

static void purify_group_member(int level, P_char ch, int type, P_char victim)
{
	if (!IS_ALIVE(victim) || victim->in_room != ch->in_room)
		return;

	if (poison_common_remove(victim))
		send_to_char("&+WDivine power purges the poison from your body.&n\n", victim);
	spell_cure_blind(level, ch, nullptr, type, victim, nullptr);
	if (affected_by_spell(victim, SPELL_DISEASE))
		spell_cure_disease(level, ch, nullptr, type, victim, nullptr);
	// Cure Disease removes one ailment per call; cleanse both when they coexist.
	if (affected_by_spell(victim, SPELL_CONTAGION))
		spell_cure_disease(level, ch, nullptr, type, victim, nullptr);
}

void spell_mass_purification(int level, P_char ch, char * /*arg*/, int type, P_char /*victim*/,
			     P_obj /*obj*/)
{
	if (!IS_ALIVE(ch) || ch->in_room == NOWHERE)
		return;

	send_to_char("&+WA wave of divine purification flows from you.&n\n", ch);
	purify_group_member(level, ch, type, ch);
	for (auto *member = ch->group; member; member = member->next)
	{
		if (member->ch != ch)
			purify_group_member(level, ch, type, member->ch);
	}
}

void spell_mend_soul(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim, P_obj obj)
{
	int healpoints;

	if (!IS_ANGEL(victim) && GET_RACE(victim) != RACE_GOLEM)
	{
		act("$N chants something odd and takes a look at $n, a weird look in $S eyes.",
		    TRUE, ch, 0, victim, TO_NOTVICT);
		act("Um... $N isn't angelic...", TRUE, ch, 0, victim, TO_CHAR);
		return;
	}
	spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
	grapple_heal(victim);

	healpoints = number(150, (GET_LEVEL(ch) * 5));

	/*
	if(!GET_CLASS(ch, CLASS_WARLOCK))
	  healpoints = MIN(healpoints, GET_LEVEL(ch) * 4);
	else
	  healpoints = 100;*/

	heal(victim, ch, healpoints, GET_MAX_HIT(victim));

	if (healpoints)
		send_to_char(
			"&+WHoly &+Renergy&n flows into you from the &+Wh&+yea&+Wv&+Ye&+Wns&n, mending your wounds!\n",
			victim);
	if (victim != ch && healpoints)
	{
		act("&+Wholy &+Renergy&n flows into $N from the &+Wh&+Yea&+Wv&+Ye&+Wns&n, mending $S wounds.",
		    FALSE, ch, 0, victim, TO_NOTVICT);
		act("&+Wholy &+Renergy&n flows into $N from the &+Wh&+Yea&+Wv&+Ye&+Wns&n, mending $S wounds.",
		    FALSE, ch, 0, victim, TO_CHAR);
	}
	update_pos(victim);
}

void spell_heal_undead(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim, P_obj obj)
{
	int healpoints;

	// GET_RACE2 -> shapeshifted into skeleton (Blighters).
	if (!IS_UNDEADRACE(victim) && !(GET_RACE2(victim) == RACE_SKELETON) &&
	    (GET_RACE(victim) != RACE_GOLEM) && !GET_CLASS(victim, CLASS_NECROMANCER))
	{
		act("$N chants something odd and takes a look at $n, a weird look in $S eyes.",
		    TRUE, ch, 0, victim, TO_NOTVICT);
		act("Um... $N isn't undead...", TRUE, ch, 0, victim, TO_CHAR);
		return;
	}
	spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
	grapple_heal(victim);

	healpoints = number(150, (GET_LEVEL(ch) * 5));

	/*
	if(!GET_CLASS(ch, CLASS_WARLOCK))
	  healpoints = MIN(healpoints, GET_LEVEL(ch) * 4);
	else
	  healpoints = 100;*/

	heal(victim, ch, healpoints, GET_MAX_HIT(victim));

	if (healpoints)
	{
		send_to_char("&+WYou feel the powers of darkness strengthen you!\n", victim);
	}
	if (victim != ch && healpoints)
	{
		act("$n reaches out at $N, touching $M. ", FALSE, ch, 0, victim, TO_NOTVICT);
		act("You reach out at $N and touch $M.", TRUE, ch, 0, victim, TO_CHAR);
		act("$n appears to gain power from the sudden deadly chill around $m.", FALSE,
		    victim, 0, 0, TO_ROOM);
	}
	update_pos(victim);
}

void spell_greater_heal_undead(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			       P_obj obj)
{
	int healpoints = 300;

	// GET_RACE2 -> shapeshifted into skeleton (Blighters).
	if (!IS_UNDEADRACE(victim) && IS_PC(ch) && IS_NPC(victim) &&
	    GET_RACE2(victim) != RACE_SKELETON)
	// old guildhalls (deprecated)
	//     && mob_index[GET_RNUM(victim)].virtual_number != WARRIOR_GOLEM_VNUM &&
	//      mob_index[GET_RNUM(victim)].virtual_number != MAGE_GOLEM_VNUM &&
	//      mob_index[GET_RNUM(victim)].virtual_number != CLERIC_GOLEM_VNUM
	{
		act("$N chants something odd and takes a look at $n, a weird look in $S eyes.",
		    TRUE, ch, 0, victim, TO_NOTVICT);
		act("Um... $N isn't undead...", TRUE, ch, 0, victim, TO_CHAR);
		return;
	}
	spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
	grapple_heal(victim);

	heal(victim, ch, healpoints, GET_MAX_HIT(victim));

	if (healpoints)
		send_to_char("&+WYou feel the powers of darkness flow into you!!\n", victim);
	if (victim != ch && healpoints)
	{
		act("$n reaches out at $N, touching $M. ", FALSE, ch, 0, victim, TO_NOTVICT);
		act("You reach out at $N and touch $M.", TRUE, ch, 0, victim, TO_CHAR);
		act("$n appears to gain power from the sudden deadly chill around $m.", FALSE,
		    victim, 0, 0, TO_ROOM);
	}
	update_pos(victim);
}

void spell_group_heal(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		      P_obj /*obj*/)
{
	int healpoints = 70 + number(0, 10);

	if (!IS_ALIVE(ch))
		return;

	if (GET_SPEC(ch, CLASS_CLERIC, SPEC_HEALER))
		healpoints = 120 + number(0, 20);

	if (GET_CHAR_SKILL(ch, SKILL_DEVOTION) > 0)
		healpoints += (int)(GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 5);

	heal(ch, ch, healpoints, GET_MAX_HIT(ch) - number(1, 4));
	send_to_char("&+WA warm feeling of peace fills your body.\n", ch);

	if (ch->group)
	{
		for (struct group_list *gl = ch->group; gl; gl = gl->next)
		{
			if (ch != gl->ch && gl->ch->in_room == ch->in_room)
			{
				if (GET_HIT(gl->ch) < GET_MAX_HIT(gl->ch))
				{
					heal(gl->ch, ch, healpoints,
					     GET_MAX_HIT(gl->ch) - number(1, 4));
					update_pos(gl->ch);
					send_to_char(
						"&+WA warm feeling of peace fills your body.\n",
						gl->ch);
				}
			}
		}
	}
}

void spell_vigorize_critic(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			   P_obj /*obj*/)
{
	spell_invigorate(level, ch, 0, SPELL_TYPE_SPELL, victim, 0);
	return;
}

void spell_vigorize_serious(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			    P_obj /*obj*/)
{
	// For backwards compatibility for various potions and others.
	spell_invigorate((int)(level * 0.66), ch, 0, SPELL_TYPE_SPELL, victim, 0);
	return;
}

void spell_vigorize_light(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			  P_obj /*obj*/)
{
	// For backwards compatibility for various potions and others.
	spell_invigorate((int)(level * 0.33), ch, 0, SPELL_TYPE_SPELL, victim, 0);
	return;
}

// This single spell replaces all the old vigorize spells, which was not a real word.

void spell_invigorate(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		      P_obj /*obj*/)
{
	int movepoints;

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim) || ch->in_room != victim->in_room)
		return;

	movepoints = dice(3, level);

	if (GET_CLASS(ch, CLASS_CLERIC))
		movepoints = (int)(movepoints * 1.5);
	else if (GET_CLASS(ch, CLASS_RANGER))
		movepoints = (int)(movepoints * 1.15);
	else if (GET_CLASS(ch, CLASS_PALADIN | CLASS_ANTIPALADIN))
		movepoints = (int)(movepoints * .95);
	else
		movepoints = (int)(movepoints * 0.80);

	if ((movepoints + GET_VITALITY(victim)) > GET_MAX_VITALITY(victim))
	{
		/* Old debugging message:
		    if( GET_VITALITY(victim) != GET_MAX_VITALITY(victim) )
		    {
		      debug("INVIGORATE: Movement points (%d) %s to %s.",
		        GET_MAX_VITALITY(victim) - GET_VITALITY(victim), GET_NAME(ch), GET_NAME(victim));
		    }
		*/
		GET_VITALITY(victim) = GET_MAX_VITALITY(victim);
	}
	else
	{
		/* Old debugging message:
		    debug("INVIGORATE: Movement points (%d) %s to %s.", movepoints, GET_NAME(ch), GET_NAME(victim));
		*/
		GET_VITALITY(victim) += movepoints;
	}

	if (ch != victim)
	{
		act("You &+Winvigorate&n $N with renewed &+cenergy.&n", FALSE, ch, 0, victim,
		    TO_CHAR);
	}
	else
	{
		act("You &+Winvigorate&n yourself with renewed &+cenergy.&n", FALSE, ch, 0, victim,
		    TO_CHAR);
	}
	act("&+yFresh energy pours through your body. &+WYou are invigorated!&n", FALSE, NULL, 0,
	    victim, TO_VICT);
	act("$N &+Wappears invigorated.&n", FALSE, ch, 0, victim, TO_NOTVICT);

	update_pos(victim);
}

void spell_devitalize(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		      P_obj /*obj*/)
{
	int movepoints;

	movepoints = dice(3, (level / 3));

	if ((GET_VITALITY(victim) - movepoints) < 20)
	{
		GET_VITALITY(victim) = 20;
		return;
	}
	GET_VITALITY(victim) -= movepoints;

	act("&+L$n&+L drains $N's&+L stamina!", FALSE, ch, 0, victim, TO_NOTVICT);
	act("&+LYou drain $N's &+Lstamina!", FALSE, ch, 0, victim, TO_CHAR);
	act("&+L$n&+L drains your stamina!", FALSE, ch, 0, victim, TO_VICT);

	update_pos(victim);
}

void spell_vitality(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		    P_obj /*obj*/)
{
	struct affected_type af;
	int healpoints = 4 * level, duration = 1;

	if (!IS_ALIVE(ch))
		return;

	if (IS_NPC(victim) && GET_VNUM(victim) == IMAGE_REFLECTION_VNUM)
		return;

	if (affected_by_spell(victim, SPELL_MIELIKKI_VITALITY))
	{
		send_to_char(
			"&+GThe Goddess Mielikki is aiding your health, and prevents the vitality spell from functioning...\r\n",
			victim);
		return;
	}

	if (affected_by_spell(victim, SPELL_FALUZURES_VITALITY))
	{
		send_to_char(
			"&+LThe God &+yFa&+Lluz&+yure&+L is aiding your health, and prevents the vitality spell from functioning...\r\n",
			victim);
		return;
	}

	if (affected_by_spell(victim, SPELL_ESHABALAS_VITALITY))
	{
		send_to_char("&+rThe blessings of the vitality spell are denied by Eshabala!\r\n",
			     victim);
		return;
	}

	if (!IS_PC(ch) && !IS_PC(victim))
		duration = 30;
	else
		duration = (int)(MAX(10, GET_LEVEL(ch) / 4));

	if (!affected_by_spell(victim, SPELL_VITALITY))
	{
		send_to_char("&+BYou feel vitalized.\n", victim);
		act("$N looks vitalized.&n", TRUE, ch, 0, victim, TO_NOTVICT);

		if (ch != victim)
			act("You vitalize $N.&n", TRUE, ch, 0, victim, TO_CHAR);

		bzero(&af, sizeof(af));
		af.type = SPELL_VITALITY;
		af.duration = duration;
		af.modifier = healpoints;
		af.location = APPLY_HIT;

		affect_to_char(victim, &af);

		update_pos(victim);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_VITALITY)
			{
				send_to_char(
					"&+WYou feel a slight &+cmagical surge&+W that reinforces and refreshes your &+Bvitality.\r\n&n",
					victim);
				af1->duration = duration;
			}
	}
}

void spell_aid(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
	       P_obj obj)
{
	struct affected_type *af, *next_af_dude;
	int temp = (int)(level / 10);

	if (!ch)
		return;
	if (!victim)
	{
		return;
	}
	if (ch && victim) // Just making sure.
	{
		for (af = victim->affected; af; af = next_af_dude)
		{
			next_af_dude = af->next;

			if (af->flags & AFFTYPE_SHORT)
			{
				continue;
			}
			if ((af->type == SPELL_CURSE) || (af->type == SPELL_APOCALYPSE) ||
			    (af->type == SPELL_BLINDNESS) || (af->type == SPELL_DISEASE) ||
			    (af->type == SPELL_MINOR_PARALYSIS) ||
			    (af->type == SPELL_MAJOR_PARALYSIS) ||
			    (af->type == SPELL_RAY_OF_ENFEEBLEMENT))
			{
				af->duration = MAX(1, af->duration - (2 * (1 + temp)));
			}
		}
		// 1 in 6 chance to instantly cure the poison at level 50
		if (IS_SET(victim->specials.affected_by2, AFF2_POISONED) && !number(0, 5))
		{
			if (poison_common_remove(victim))
			{
				act("&+WYou neutralize the poison!", FALSE, ch, 0, 0, TO_CHAR);
				act("&+WThe poison in your bloodstream disappears!", FALSE, 0, 0,
				    victim, TO_VICT);
			}
		}
		if (GET_LEVEL(ch) > 16)
		{
			spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
			grapple_heal(victim);
		}
		if (!affected_by_spell(victim, SPELL_AID))
		{
			struct affected_type af;
			bzero(&af, sizeof(af));
			send_to_char(
				"&+WYou suddenly feel blessed by the protective spirits of the nature!\n",
				victim);
			af.type = SPELL_AID;
			af.duration = MAX(3, (level + 4 / 10));
			affect_to_char(victim, &af);
		}
		send_to_char("&+GYou feel the power of nature course through your veins.\n",
			     victim);
	}
}

void spell_regeneration(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	char Gbuf1[100];
	int skl_lvl;

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
		return;

	if (affected_by_spell(victim, SKILL_REGENERATE) ||
	    affected_by_spell(victim, SPELL_PACTUM_SERPENTIS))
	{
		send_to_char("You can't possibly heal any faster.\n", victim);
		return;
	}

	skl_lvl = MAX(4, (level / 10));

	for (struct affected_type *existing = victim->affected; existing; existing = existing->next)
	{
		if (existing->type == SPELL_REGENERATION &&
		    !IS_SET(existing->flags, AFFTYPE_ARAMUS_CROWN_REGENERATION))
		{
			existing->duration = skl_lvl;
			send_to_char("Your regeneration magic is refreshed!\r\n", victim);
			return;
		}
	}

	snprintf(Gbuf1, 100, "You begin to regenerate rapidly.\n");

	bzero(&af, sizeof(af));
	af.type = SPELL_REGENERATION;
	af.duration = skl_lvl;
	// af.location = APPLY_HIT_REG;
	// af.modifier = level * level * 2;
	af.bitvector4 = AFF4_REGENERATION;
	send_to_char(Gbuf1, victim);
	affect_to_char(victim, &af);
}

void spell_endurance(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		     P_obj /*obj*/)
{
	struct affected_type af;
	char Gbuf1[100];
	int skl_lvl;

	if (affected_by_spell(victim, SPELL_ENDURANCE))
	{
		send_to_char("You can't possibly regain movement any faster.\n", victim);
		return;
	}

	if (affected_by_spell(ch, SPELL_MIELIKKI_VITALITY) && !GET_CLASS(ch, CLASS_DRUID) &&
	    !GET_SPEC(ch, CLASS_RANGER, SPEC_HUNTSMAN))
	{
		send_to_char(
			"&+GThe Goddess Mielikki is aiding your health, and prevents the endurance spell from functioning...\r\n",
			victim);
		return;
	}

	skl_lvl =
		(int)(MAX(3, ((level / 4) - 1)) * get_property("spell.endurance.modifiers", 1.000));

	snprintf(Gbuf1, 100, "You feel energy begin to surge through your limbs.\n");

	bzero(&af, sizeof(af));
	af.type = SPELL_ENDURANCE;
	af.location = APPLY_MOVE_REG;
	af.duration = skl_lvl;
	af.modifier = skl_lvl;
	send_to_char(Gbuf1, victim);
	affect_to_char(victim, &af);
}

void spell_slow_poison(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
		       P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_SLOW_POISON))
	{
		act("$n looks slightly more healthy.", TRUE, victim, 0, 0, TO_ROOM);
		act("You feel slightly more healthy.", TRUE, victim, 0, 0, TO_CHAR);
		bzero(&af, sizeof(af));
		af.type = SPELL_SLOW_POISON;
		af.duration = level / 2;
		af.bitvector = AFF_SLOW_POISON;
		affect_to_char(victim, &af);
	}
}

void spell_vitalize_mana(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
			 P_obj /*obj*/)
{
	int trans;

	trans = MIN(GET_VITALITY(ch), number(33, GET_MAX_MANA(ch)));

	act("You feel more energized!", FALSE, ch, 0, 0, TO_CHAR);

	ch->points.mana += trans;
	ch->points.mana = MIN(GET_MANA(ch), GET_MAX_MANA(ch));
	ch->points.vitality -= trans;
}

void event_natures_touch(P_char ch, P_char vict, P_obj /*obj*/, void *data)
{
	int healpoints, wavevalue, x;

	healpoints = wavevalue = *((int *)data);

	switch (world[vict->in_room].sector_type)
	{
	case SECT_UNDRWLD_CITY:
	case SECT_CITY:
		healpoints = (healpoints * 2) / 3;
		break;
	case SECT_FIELD:
		healpoints = (healpoints * 4) / 3;
		break;
	case SECT_FOREST:
	case SECT_UNDRWLD_MUSHROOM:
		healpoints = (healpoints * 3) / 2;
		break;
	case SECT_HILLS:
	case SECT_UNDRWLD_WILD:
		healpoints = (healpoints * 5) / 4;
		break;
	case SECT_UNDERWATER_GR:
	case SECT_UNDRWLD_SLIME:
	case SECT_MOUNTAIN:
	case SECT_UNDRWLD_MOUNTAIN:
		healpoints = (healpoints * 6) / 5;
		break;
	case SECT_UNDRWLD_LOWCEIL:
	case SECT_UNDRWLD_LIQMITH:
		healpoints = (healpoints * 7) / 6;
		break;
	default:
		break;
	}

	x = vamp(vict, healpoints, GET_MAX_HIT(vict));
	update_pos(vict);

	if (x > 0 && IS_FIGHTING(vict))
		gain_exp(ch, vict, x, EXP_HEALING);

	wavevalue /= 2;
	if (wavevalue > 1)
		add_event(event_natures_touch, 2, ch, vict, 0, 0, &wavevalue, sizeof(wavevalue));
}

void spell_natures_touch(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			 P_obj obj)
{
	int healpoints;

	if (!IS_ALIVE(victim) || !IS_ALIVE(ch))
		return;

	// 32 at level 21 -> 63 hps healing, 50 at level 56 -> 97 hps healing, modified by terrain.
	healpoints = (level / 2) + 22;

	if (!GET_CLASS(ch, CLASS_DRUID))
	{
		if (!GET_CLASS(ch, CLASS_RANGER))
			healpoints /= 4;
		else
			healpoints /= 2;
	}

	if (GET_CLASS(ch, CLASS_DRUID) && IS_BLIND(victim))
		spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, victim, obj);

	grapple_heal(victim);

	if (healpoints < 8)
	{
		healpoints = 8;
	}

	add_event(event_natures_touch, 1, ch, victim, 0, 0, &healpoints, sizeof(healpoints));

	if (ch == victim)
		act("&+GThe warmth of nature fills your body.", FALSE, ch, 0, victim, TO_CHAR);
	else
	{
		act("&+GThe warmth of nature fills your body.", FALSE, ch, 0, victim, TO_VICT);
		act("&+GYou gently touch $N&+G's body, and $S wounds begin to heal!", FALSE, ch, 0,
		    victim, TO_CHAR);
	}
	act("&+G$n &+Ggently touches $N&+G's body, and $S wounds begin to heal!", FALSE, ch, 0,
	    victim, TO_NOTVICT);
}

void event_healing_salve(P_char ch, P_char vict, P_obj /*obj*/, void *data)
{
	int waves;

	waves = *((int *)data);

	if (GET_HIT(vict) < GET_MAX_HIT(vict))
		send_to_char("You feel a &+Wwarm wave&n going through your body.\n", vict);

	heal(vict, ch, number(GET_LEVEL(vict), (GET_LEVEL(vict) * 2)), GET_MAX_HIT(vict));

	update_pos(vict);

	if (waves-- == 0)
		return;

	add_event(event_healing_salve, (int)(PULSE_VIOLENCE * 0.8), ch, vict, 0, 0, &waves,
		  sizeof(waves));
}

void spell_healing_salve(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			 P_obj /*obj*/)
{
	int waves = MAX(3, level / 10);

	act("Upon $n's touch a &+Csoft glow&n flows from $s hands and surrounds $N.", FALSE, ch, 0,
	    victim, TO_NOTVICT);
	act("Upon $n's touch a &+Csoft glow&n flows from $s hands and surrounds you.", FALSE, ch, 0,
	    victim, TO_VICT);
	act("You touch $N invoking a &+Chealing energy&n to cure $S wounds.", FALSE, ch, 0, victim,
	    TO_CHAR);

	add_event(event_healing_salve, (int)(PULSE_VIOLENCE * 0.1), ch, victim, 0, 0, &waves,
		  sizeof(waves));

	// if(affected_by_spell(victim, SPELL_PLAGUE))
	//   affect_from_char(victim, SPELL_PLAGUE);

	if (IS_AFFECTED4(victim, AFF4_CARRY_PLAGUE))
		REMOVE_BIT(victim->specials.affected_by4, AFF4_CARRY_PLAGUE);

	// if(affected_by_spell(victim, SPELL_WITHER))
	//   affect_from_char(victim, SPELL_WITHER);
}

void spell_mass_heal(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		     P_obj obj)
{
	P_char tch;
	int healed;

	for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
	{
		spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, tch, obj);
		grapple_heal(tch);
		if (GET_HIT(tch) > GET_MAX_HIT(tch))
			continue;

		int maxhits = IS_HARDCORE(ch) ? hardcore_config_get()->bonus_mass_heal_base : 100;

		healed = vamp(tch, (int)maxhits + number(1, level / 3),
			      GET_MAX_HIT(tch) - number(1, 4));
		if (GET_OPPONENT(tch))
		{
			gain_exp(ch, tch, healed, EXP_HEALING);
		}
		update_pos(tch);
		if (IS_RACEWAR_UNDEAD(tch))
			send_to_char("&+WYou feel the powers of darkness strengthen you!\n", tch);
		else
			send_to_char("&+WA warm feeling fills your body.\n", tch);
	}
}

void spell_accel_healing(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			 P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	int skl_lvl = (int)(MAX(41, ((level / 2) - 1)));

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
		return;

	if (affected_by_spell(victim, SKILL_REGENERATE) ||
	    affected_by_spell(victim, SPELL_PACTUM_SERPENTIS))
	{
		act("$N can't possibly heal any faster.", TRUE, ch, 0, victim, TO_CHAR);
		return;
	}

	if (affected_by_spell(victim, SPELL_ACCEL_HEALING))
	{
		struct affected_type *af1;
		bool found = false;

		for (af1 = victim->affected; af1; af1 = af1->next)
		{
			if (af1->type == SPELL_ACCEL_HEALING)
			{
				af1->duration = skl_lvl + 1;
				found = true;
			}
		}

		if (found)
			send_to_char("Your &+caccelerated healing&n magic is &+Yrefreshed!\r\n",
				     victim);
		return;
	}

	bzero(&af, sizeof(af));
	af.type = SPELL_ACCEL_HEALING;
	af.duration = skl_lvl + 1;
	af.location = APPLY_HIT_REG;
	// 2x as effective as regen, but only works out of combat
	af.modifier = ((int)(get_property("hit.regen.Spell", 9.000) + 1) * 2) * level;
	affect_to_char(victim, &af);

	send_to_char("You begin to heal faster.\r\n", victim);
}

void spell_mielikki_vitality(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			     P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	bool message = false;
	int healpoints = (3 * level) + (level / 2);

	if (affected_by_spell(ch, SPELL_ESHABALAS_VITALITY) ||
	    affected_by_spell(ch, SPELL_FALUZURES_VITALITY))
	{
		send_to_char("&+GThe blessings of the Goddess Mielikki are denied!\r\n", victim);
		return;
	}

	if (affected_by_spell(ch, SPELL_VITALITY))
	{
		send_to_char("&+GThe Goddess Mielikki cannot further bless your vitality...\r\n",
			     victim);
		return;
	}

	if (affected_by_spell(ch, SPELL_MIELIKKI_VITALITY))
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_MIELIKKI_VITALITY)
			{
				af1->duration = 15;
				message = true;
			}

		if (message)
			send_to_char("&+GThe Goddess graces you.\r\n", victim);
		return;
	}

	bzero(&af, sizeof(af));
	af.type = SPELL_MIELIKKI_VITALITY;
	af.duration = 15;
	af.modifier = healpoints;
	af.location = APPLY_HIT;
	affect_to_char(victim, &af);

	if (GET_CLASS(ch, CLASS_DRUID))
	{
		af.modifier = level;
		af.location = APPLY_MOVE;
		affect_to_char(victim, &af);
	}

	if (GET_CLASS(ch, CLASS_RANGER) && !affected_by_spell(ch, SPELL_REGENERATION) &&
	    !affected_by_spell(ch, SPELL_ACCEL_HEALING))
	{
		af.modifier = number(20, 45);
		af.location = APPLY_HIT_REG;
		affect_to_char(victim, &af);
	}

	/*  // They get endurance, no need for this.
	if((GET_CLASS(ch, CLASS_RANGER) &&
	   !affected_by_spell(ch, SPELL_ENDURANCE)) ||
	   GET_SPEC(ch, CLASS_RANGER, SPEC_HUNTSMAN))
	{
	  af.modifier = number(25, 50);
	  af.location = APPLY_MOVE_REG;
	  affect_to_char(victim, &af);
	}
	*/

	send_to_char("&+GYou feel the &+ywarm &+Gbreath of the Goddess Mielikki.\r\n", ch);
}

void vital_intercession_heal(P_char ch, int dam, int spell)
{
	struct affected_type *paf = get_spell_from_char(ch, spell);

	int healpoints =
		number((int)(dam * get_property("spell.vitalIntercession.healScalarMin", 0.4)),
		       (int)(dam * get_property("spell.vitalIntercession.healScalarMax", 0.6)));
	if (healpoints > paf->modifier)
	{
		healpoints = paf->modifier;
	}

	vamp(ch, healpoints, GET_MAX_HIT(ch));

	send_to_char("&+WHealing energies surge through your body!\r\n&n", ch);

	paf->modifier -= healpoints;
	if (paf->modifier <= 0)
	{
		wear_off_message(ch, paf);
		affect_remove(ch, paf);
	}
}

void vital_intercession(int level, P_char ch, P_char victim, int spell)
{
	struct affected_type af;
	int maximumHitsHealed =
		(int)(level * get_property("spell.vitalIntercession.maxHitsHealedMultiplier", 6)) +
		number(-20, 20);

	if (!SanityCheck(ch, "spell_vital_intercession") ||
	    !SanityCheck(victim, "spell_vital_intercession"))
		return;

	if (!affected_by_spell(victim, spell))
	{
		bzero(&af, sizeof(af));
		af.type = spell;
		af.duration = 5;
		af.modifier = maximumHitsHealed;
		affect_to_char(victim, &af);
		update_pos(victim);
	}
	else
	{
		struct affected_type *af1;
		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == spell)
			{
				af1->duration = 5;
				af1->modifier = maximumHitsHealed;
			}
	}

	send_to_char("&+WYou feel healing energies surround you!\r\n&n", victim);
}

void spell_vital_intercession(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			      P_obj /*obj*/)
{
	vital_intercession(level, ch, victim, SPELL_VITAL_INTERCESSION);
}

void spell_holy_intercession(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
			     P_obj /*obj*/)
{
	struct group_list *gl;

	if (!SanityCheck(ch, "spell_holy_intercession"))
		return;

	if (ch->group)
	{
		gl = ch->group;
		/* leader first */
		if (gl->ch->in_room == ch->in_room)
			vital_intercession(level * 2 / 3, ch, gl->ch, SPELL_HOLY_INTERCESSION);
		/* followers */
		for (gl = gl->next; gl; gl = gl->next)
		{
			if (gl->ch->in_room == ch->in_room)
				vital_intercession(level * 2 / 3, ch, gl->ch,
						   SPELL_HOLY_INTERCESSION);
		}
	}
	else
	{
		vital_intercession(level * 2 / 3, ch, ch, SPELL_HOLY_INTERCESSION);
	}
}

void spell_heavens_aid(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		       P_obj /*obj*/)
{
	struct damage_messages messages = {
		"&+LYou direct the &+Wholy beam &+Ltowards $N&+L.",
		"$n&+L's holy light passes over you, burning you with holy power.",
		"$n &+Ldirects a &+Wholy beam&n &+Ltowards $N&+L, burning $m with its &+Wholy power&+L.",
		"$N &+Lconvulses and dies a quick and quiet &+rdeath.",
		"&+LYou feel the &+Wholy beam&n sap the last bit of &+clifeforce &+Wfrom you.",
		"$N quietly collapses and &+rdies!",
		0
	};

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
		return;

	if (IS_RACEWAR_GOOD(victim) || IS_ANGEL(victim))
	{
		act("&+WThe light from above passes over $N &+Wwithout harm.&n", FALSE, ch, 0,
		    victim, TO_CHAR);
		return;
	}

	if (GET_LEVEL(victim) < (level / 5) &&
	    spell_damage(ch, victim, 10000, SPLDAM_HOLY, SPLDAM_NODEFLECT, &messages) ==
		    DAM_NONEDEAD)
		return;

	int dam;
	dam = (int)number(level * 5, level * 7);

	if (IS_PC(ch) && IS_PC(victim))
		dam = dam * get_property("spell.area.damage.to.pc", 0.5);

	dam = dam * get_property("spell.area.damage.factor.aidOfTheHeavens", 1.000);

	if (spell_damage(ch, victim, dam, SPLDAM_HOLY, SPLDAM_NODEFLECT, &messages) == DAM_NONEDEAD)
	{
		if (level < (GET_LEVEL(victim) / 2))
			spell_minor_paralysis((int)(level / 2), ch, NULL, 0, victim, NULL);
	}
}

void event_aid_of_the_heavens(P_char ch, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	int room;
	room = *((int *)data);
	if (room != ch->in_room)
	{
		send_to_char("&+LThe light from above dissolves into nothing...\n", ch);
		act("&+LThe light from above fades out of existence...", TRUE, ch, 0, 0, TO_ROOM);
		return;
	}

	if (!number(0, 3))
	{
		act("$n&+W's light from above glides about the area.", FALSE, ch, 0, 0, TO_ROOM);
		add_event(event_aid_of_the_heavens, PULSE_VIOLENCE * 1, ch, 0, 0, 0, &room,
			  sizeof(room));
		return;
	}

	act("$n&+L's summoned illumination stretches out!", FALSE, ch, 0, 0, TO_ROOM);
	act("&+LYour summoned illumination stretches out!.", FALSE, ch, 0, 0, TO_CHAR);

	cast_as_damage_area(ch, spell_heavens_aid, GET_LEVEL(ch), NULL,
			    get_property("spell.area.minChance.aidOfTheHeavens", 90),
			    get_property("spell.area.chanceStep.aidOfTheHeavens", 10));
}

void spell_aid_of_the_heavens(int /*level*/, P_char ch, char * /*arg*/, int /*type*/,
			      P_char /*victim*/, P_obj /*obj*/)
{
	int room;
	room = ch->in_room;

	act("&+WA holy beam of light begins to &+Lco&+wa&+Wle&+ws&+Lce &+Waround $n.", FALSE, ch, 0,
	    0, TO_ROOM);

	act("&+WYou call down a holy beam of light from the heavens.", FALSE, ch, 0, 0, TO_CHAR);

	zone_spellmessage(
		ch->in_room, TRUE, "&+LYou see a bright light shining on the horizon.\n",
		"&+LThe air to the %s &+Rwarms &+Land the &+Yholy shine&n enlightens your senses.\n");

	add_event(event_aid_of_the_heavens, PULSE_VIOLENCE * 1, ch, 0, 0, 0, &room, sizeof(room));

	CharWait(ch, (int)1 * PULSE_VIOLENCE);
}

void spell_water_to_life(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			 P_obj obj)
{
	int healpoints;

	obj = world[ch->in_room].contents;

	if (!IS_WATER_ROOM(ch->in_room))
	{
		while (obj != NULL)
		{
			if (GET_ITEM_TYPE(obj) == ITEM_DRINKCON && (obj->wear_flags == 0) &&
			    (obj->value[2] == LIQ_WATER || obj->value[2] == LIQ_LOTSAWATER))
			{
				break;
			}
			else
			{
				obj = obj->next_content;
			}
		}

		if (obj == NULL)
		{
			send_to_char("There is not enough water around!\n", ch);
			return;
		}
	}

	if (ch != victim && GET_RACE(victim) != RACE_W_ELEMENTAL && !IS_WATERFORM(victim))
	{
		send_to_char("You will drown them instead!\n", ch);
		return;
	}

	healpoints = level * 2 + MAX(0, level - 50) * 20;

	heal(victim, ch, healpoints, GET_MAX_HIT(victim));
	update_pos(victim);

	if (ch == victim)
	{
		act("&+bThe purifying power of the &+Bwater&+b flows through your body.&n", FALSE,
		    ch, 0, victim, TO_CHAR);
	}
	else
	{
		act("&+bThe purifying power of the &+Bwater&+b flows through your body.&n", FALSE,
		    ch, 0, victim, TO_VICT);
		act("&+bWater absorbed from the surrounding area flows around $N.&n", FALSE, ch, 0,
		    victim, TO_CHAR);
	}
	act("&+bWater absorbed from the surrounding area flows around $N.&n", FALSE, ch, 0, victim,
	    TO_NOTVICT);
}
