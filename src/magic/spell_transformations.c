#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utils.h"
#include "core/defines.h"
#include "magic/spells.h"
#include <string.h>
#include <strings.h>

void BackToUsualForm(P_char ch)
{
	act("The mists in the room coalesce into $n's form...", TRUE, ch, 0, 0, TO_ROOM);
	act("You return to your ordinary form...", TRUE, ch, 0, 0, TO_CHAR);

	affect_from_char(ch, SPELL_WRAITHFORM);
}

void spell_elemental_form(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			  P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	int type_mental;

	if (IS_AFFECTED3(victim, AFF3_ENLARGE))
	{
		send_to_char("You're quite large already.\n", victim);
		return;
	}
	if (!IS_AFFECTED3(victim, AFF3_ELEMENTAL_FORM))
	{
		char buf1[500], buf2[500];

		type_mental = number(1, 4);

		switch (type_mental)
		{
		case 1: /* earth */
			strcpy(buf1,
			       "$n begins to grow, turning into a large &+yearth elemental form!&n");
			send_to_char(
				"You call upon the power of the &+yelemental plane of earth!&N\n",
				ch);
			strcpy(buf2,
			       "You grow, your body becoming more elemental than humanoid!&n");
			break;
		case 2: /* fire */
			strcpy(buf1,
			       "$n begins to grow, turning into a large &+rfire elemental form!&n");
			send_to_char(
				"You call upon the power of the &+relemental plane of fire!&N\n",
				ch);
			strcpy(buf2,
			       "You grow, your body becoming more elemental than humanoid!&n");
			break;
		case 3: /* water */
			strcpy(buf1,
			       "$n begins to grow, turning into a large &+bwater elemental form!&n");
			send_to_char(
				"You call upon the power of the &+belemental plane of water!&N\n",
				ch);
			strcpy(buf2,
			       "You grow, your body becoming more elemental than humanoid!&n");
			break;
		case 4: /* air */
			strcpy(buf1,
			       "$n begins to grow, turning into a large &+Cair elemental form!&n");
			send_to_char(
				"You call upon the power of the &+Celemental plane of air!&N\n",
				ch);
			strcpy(buf2,
			       "You grow, your body becoming more elemental than humanoid!&n");
			break;
		}

		act(buf1, TRUE, victim, 0, 0, TO_ROOM);
		act(buf2, TRUE, victim, 0, 0, TO_CHAR);

		switch (type_mental)
		{
		case 1: /* earth */
			spell_stone_skin(level, ch, 0, 0, ch, 0);
			bzero(&af, sizeof(af));
			af.type = SPELL_ARMOR;
			af.duration = 25;
			af.modifier = -30;
			af.location = APPLY_AC;
			affect_to_char(victim, &af);
			break;

		case 2: /* fire */
			if (!IS_AFFECTED2(victim, AFF2_FIRESHIELD))
				spell_fireshield(level, ch, NULL, 0, ch, 0);
			if (!IS_AFFECTED(victim, AFF_PROT_FIRE))
				spell_protection_from_fire(level, ch, 0, 0, ch, 0);
			break;

		case 3: /* water */
			if (!IS_AFFECTED(victim, AFF_WATERBREATH))
				spell_waterbreath(level, ch, NULL, 0, ch, 0);
			bzero(&af, sizeof(af));
			af.type = SPELL_ARMOR;
			af.duration = 25;
			af.modifier = -20;
			af.location = APPLY_AC;
			affect_to_char(victim, &af);
			af.modifier = 5;
			af.location = APPLY_DAMROLL;
			affect_to_char(victim, &af);
			break;

		case 4: /* air */
			if (!IS_AFFECTED(victim, AFF_FLY))
				spell_fly(level, ch, NULL, 0, ch, 0);
			if (!affected_by_spell(victim, SPELL_DETECT_INVISIBLE))
				spell_detect_invisibility(level, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
			if (!affected_by_spell(victim, SPELL_CONCEALMENT))
				spell_invisibility(level, ch, 0, 0, ch, 0);
			bzero(&af, sizeof(af));
			af.type = SPELL_ARMOR;
			af.duration = 25;
			af.modifier = -20;
			af.location = APPLY_AC;
			affect_to_char(victim, &af);

			break;
		}

		bzero(&af, sizeof(af));
		af.type = SPELL_ELEMENTAL_FORM;
		af.duration = 6;
		af.bitvector3 = AFF3_ELEMENTAL_FORM;
		affect_to_char(victim, &af);

		af.modifier = (victim->base_stats.Str / 5);
		af.location = APPLY_STR_MAX;
		affect_to_char(victim, &af);

		af.modifier = (victim->base_stats.Con / 2);
		af.location = APPLY_CON_MAX;
		affect_to_char(victim, &af);

		af.modifier = -(victim->base_stats.Agi / 2);
		af.location = APPLY_AGI;
		affect_to_char(victim, &af);

		af.modifier = -(victim->base_stats.Dex / 2);
		af.location = APPLY_DEX;
		affect_to_char(victim, &af);
	}
}

void spell_greater_wraithform(int /*level*/, P_char ch, P_char victim, char * /*arg*/)
{
	/*
	 * this is _THE_ king of all kludges.. for npcs, race (not so
	 * important) is saved in affect.. for pcs, its in arena_hits. the
	 * difference is mostly so pcs wouldn't screw themselves _too_ much in
	 * some cases. :-P Actual removal of wraithform is handled as event,
	 * surprise surprise.
	 */
	struct affected_type af;

	/*  P_obj o1, o2; */

	/*
	 * ok, now we _can_ use this beauty.. muhahahahahaa!
	 */
	if (GET_OPPONENT(ch))
		stop_fighting(ch);
	StopAllAttackers(ch);
	act("$n fades into thin air..", TRUE, ch, 0, 0, TO_ROOM);
	act("You feel your senses blur.. and then recover.", TRUE, ch, 0, 0, TO_CHAR);
	act("Suddenly you have no body anymore!", TRUE, ch, 0, 0, TO_CHAR);

	if (!affected_by_spell(victim, SPELL_WRAITHFORM))
	{
		bzero(&af, sizeof(af));
		af.duration = 10;
		af.type = SPELL_WRAITHFORM;
		af.bitvector = AFF_WRAITHFORM;
		af.flags = AFFTYPE_NODISPEL;
		affect_to_char(ch, &af);
	}
	CharWait(ch, 3 * PULSE_VIOLENCE);
	/*
	 * 12 minutes for a level 50 caster (12 hours)
	 */
	/*
	 * AddEvent(EVENT_CHAR_EXECUTE, GET_LEVEL(ch) * 200, TRUE, ch,
	 * BackToUsualForm);
	 */
}

void spell_wraithform(int /*level*/, P_char ch, P_char victim, char * /*arg*/)
{
	/*
	 * this is _THE_ king of all kludges.. for npcs, race (not so
	 * important) is saved in affect.. for pcs, its in arena_hits. the
	 * difference is mostly so pcs wouldn't screw themselves _too_ much in
	 * some cases. :-P Actual removal of wraithform is handled as event,
	 * surprise surprise.
	 */
	struct affected_type af;

	/*
	 * ok, now we _can_ use this beauty.. muhahahahahaa!
	 */
	send_to_char("Disabled!\n", ch);
	return;

	if (IS_RIDING(ch))
	{
		send_to_char("Get off your mount first.\n", ch);
		return;
	}
#if 0
  if( !IS_TRUSTED(ch) )
  {
    for (i = 0; i < CUR_MAX_WEAR; i++)
      if(ch->equipment[i])
      {
        obj_to_room(unequip_char(ch, i), ch->in_room);
        dr = TRUE;
      }
    for (o1 = ch->carrying; o1; o1 = o2)
    {
      o2 = o1->next_content;
      obj_from_char(o1);
      obj_to_room(o1, ch->in_room);
      dr = TRUE;
    }
    if(dr)
      send_to_char("Some of your equipment falls to the ground!\n", ch);
    act("As $n starts to fade, some equipment drops to the ground!", TRUE, ch,
        0, 0, TO_ROOM);
  }
#endif
	if (GET_OPPONENT(ch))
		stop_fighting(ch);
	StopAllAttackers(ch);
	act("$n fades into thin air..", TRUE, ch, 0, 0, TO_ROOM);
	act("You feel your senses blur.. and then recover.", TRUE, ch, 0, 0, TO_CHAR);
	act("Suddenly you have no body anymore!", TRUE, ch, 0, 0, TO_CHAR);
#if 0
  for (foll = ch->followers; foll; foll = next_foll)
  {
    next_foll = foll->followerfoll->next;

    if(IS_AFFECTED(foll->follower, AFF_CHARM))
      stop_follower(foll->follower);
  }
#endif

	if (!affected_by_spell(victim, SPELL_WRAITHFORM))
	{
		bzero(&af, sizeof(af));
		af.duration = 10;
		af.type = SPELL_WRAITHFORM;
		af.bitvector = AFF_WRAITHFORM;
		af.flags = AFFTYPE_NODISPEL;
		affect_to_char(ch, &af);
	}
	CharWait(ch, 3 * PULSE_VIOLENCE);
	/*
	 * 12 minutes for a level 50 caster (12 hours)
	 */
	/*
	 * AddEvent(EVENT_CHAR_EXECUTE, GET_LEVEL(ch) * 200, TRUE, ch,
	 * BackToUsualForm);
	 */
}

void spell_vampire(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		   P_char /*vict*/, P_obj /*obj*/)
{
	struct affected_type af, *afp;
	struct follow_type *foll, *next_foll;
	P_char tch;
	int circle;

	// Old pets no longer follow orders but still take same damage as PCs
	// and count towards pet limit to avoid cheesing: doesn't apply to mobs.
	if (!IS_ALIVE(ch))
	{
		return;
	}
	if (IS_PC(ch))
	{
		for (foll = ch->followers; foll; foll = next_foll)
		{
			next_foll = foll->next;
			tch = foll->follower;
			if (IS_PC_PET(tch))
			{
				stop_follower(tch);
				setup_pet(tch, ch, 2, PET_NOORDER);
				// force pets to die after 30 minutes when trance is cast, to remove the pet links
				add_event(event_pet_death, 1800, tch, NULL, NULL, 0, NULL, 0);
			}
		}
	}

	if (GET_CLASS(ch, CLASS_THEURGIST))
	{
		act("&+WF&+Yea&+Wth&+Ye&+Wr&+Ye&+Wd w&+Yi&+Wngs&n sprout from your back and your &+Ysk&+yi&+Yn&n and &+Ye&+yy&+Yes&n gain a &+Ygolden",
		    FALSE, ch, 0, 0, TO_CHAR);
		act("&+Yhu&+ye&n. You can feel the &+Wholy &+Rpower&n of &+Cr&+ci&+Cght&+ceou&+Csn&+ce&+Css&n flowing through you!",
		    FALSE, ch, 0, 0, TO_CHAR);
		act("&+WF&+Yea&+Wth&+Ye&+Wr&+Ye&+Wd w&+Yi&+Wngs&n sprout from $n's back and their &+Ysk&+yi&+Yn&n and &+Yey&+ye&+Ys&n gain a",
		    FALSE, ch, 0, 0, TO_ROOM);
		act("&+Ygolden hu&+ye&n. They are surrounded with a &+Wholy &+Yglow&n of &+Cr&+ci&+Cght&+ceou&+Cs &+Rpower&n!",
		    FALSE, ch, 0, 0, TO_ROOM);
	}
	else
	{
		send_to_char("You take on the shape of undead...\n", ch);
		act("A chill passes by as all the color drains from $n.", TRUE, ch, 0, 0, TO_ROOM);
	}

	if (affected_by_spell(ch, SPELL_VAMPIRE) ||
	    affected_by_spell(ch, SPELL_ANGELIC_COUNTENANCE))
	{
		for (afp = ch->affected; afp; afp = afp->next)
		{
			if (afp->type == SPELL_VAMPIRE || afp->type == SPELL_ANGELIC_COUNTENANCE)
			{
				afp->duration = 10;
			}
		}
		return;
	}

	bzero(&af, sizeof(af));
	af.type = (GET_CLASS(ch, CLASS_THEURGIST) ? SPELL_ANGELIC_COUNTENANCE : SPELL_VAMPIRE);
	af.duration = 10;
	af.modifier = (get_property("stats.str.Vampire", 100) - 100);
	if (af.modifier != 0)
	{
		af.location = APPLY_STR_MAX;
		affect_to_char(ch, &af);
	}
	af.modifier = (get_property("stats.con.Vampire", 100) - 100);
	if (af.modifier != 0)
	{
		af.location = APPLY_CON_MAX;
		affect_to_char(ch, &af);
	}
	af.location = APPLY_HITROLL;
	af.modifier = 15;
	affect_to_char(ch, &af);
	af.location = APPLY_DAMROLL;
	af.modifier = 10;
	af.bitvector2 = AFF2_VAMPIRIC_TOUCH;
	af.bitvector4 = AFF4_VAMPIRE_FORM;
	affect_to_char(ch, &af);

	if (!USES_SPELL_SLOTS(ch))
	{
		return;
	}

	// The code below makes sure necro gets only as much assim slots as he had memorized spells.
	// If we ever remove ability of assimilating from tranced necros - junk it.
	for (circle = 0; circle <= MAX_CIRCLE; circle++)
	{
		ch->specials.undead_spell_slots[circle] = 0;
	}

	for (afp = ch->affected; afp; afp = afp->next)
	{
		if (afp->type == TAG_MEMORIZE && (afp->flags & MEMTYPE_FULL))
		{
			ch->specials.undead_spell_slots[get_spell_circle(ch, afp->modifier)]++;
		}
	}
}
