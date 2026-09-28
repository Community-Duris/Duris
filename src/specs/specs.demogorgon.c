/* Special procedures for Demogorgon and his related combat actions. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "combat/damage.h"
#include "world/map.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"

extern P_room world;
int do_fetid_breath(P_char ch);
void hyena_bite(P_char ch, P_char victim);

int demogorgon_shout(P_char ch, P_char tch, int cmd, char * /*arg*/)
{
	int helpers[] = { 19830, 19850, 19860, 19880, 0 };
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (!tch && !number(0, 4))
		return shout_and_hunt(
			ch, 100,
			"&+GYou will pay for attacking me mortal worms!   Denizens of darkness, come and feast upon %s!",
			NULL, helpers, 0, 0);
	return FALSE;
}

void demogorgon_tail(P_char ch)
{
	int dam;
	P_char victim = pick_target(ch, PT_TOLERANT);

	if (!victim)
		return;

	act("$n &+Llashes out with $s &+Lpowerful &+rdemon spiked &+Ltail!&n", FALSE, ch, 0, 0,
	    TO_ROOM);

	int level = GET_LEVEL(ch);

	if (number(0, 2))
	{
		send_to_char("&+LYou are hit by &+GDemogorgon's &+Lmighty tail!\r\n", victim);
		act("$N is hit by &+GDemogorgon's &+Lmighty tail!", TRUE, ch, 0, victim,
		    TO_NOTVICT);
		dam = dice(level, 15);
		spell_dispel_magic(60, ch, 0, 0, victim, 0);
		melee_damage(ch, victim, dam, 0, 0);
		spell_energy_drain(level, ch, 0, 0, victim, 0);
	}
	else
	{
		dam = dice(level, 11);

		P_char tch, next_tch;

		for (tch = world[ch->in_room].people; tch; tch = next_tch)
		{
			next_tch = tch->next_in_room;

			if (should_area_hit(ch, tch) && number(0, 1))
			{
				send_to_char(
					"&+LYou are hit by &+GDemogorgon's &+Lthe sweeping tail!\r\n",
					tch);
				act("$N is hit by &+GDemogorgon's &+Lsweeping tail!", TRUE, ch, 0,
				    tch, TO_NOTVICT);
				melee_damage(ch, victim, dam, 0, 0);
				dam *= 10;
				dam /= 9;
			}
		}
	}
}

void demogorgon_second_head(P_char ch)
{
	act("&+GThe second head of&n $n &+Yglares &+Garound...", FALSE, ch, 0, 0, TO_ROOM);

	P_char victim = pick_target(ch, PT_WEAKEST | PT_TOLERANT);

	if (!victim)
		return;

	if (!number(0, 3))
		spell_negative_concussion_blast(GET_LEVEL(ch), ch, 0, 0, victim, 0);
	else if (!number(0, 2))
		do_fetid_breath(ch);
	else if (!number(0, 6))
		cast_as_damage_area(ch, spell_cdoom, GET_LEVEL(ch), victim, 50, 20);
	else
		hyena_bite(ch, victim);
}

int demogorgon(P_char ch, P_char /*tch*/, int cmd, char * /*arg*/)
{
	int helpers[] = { 19830, 19850, 19860, 19880, 19840, 19870, 19400, 19901, 19760, 0 };
	static bool stats_increased = FALSE;
	static bool demogorgon_shouted = FALSE;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!IS_ALIVE(ch) || !IS_AWAKE(ch) || cmd || IS_IMMOBILE(ch))
		return FALSE;

	/*
	  He's a demon prince, he should have proper stats.
	*/
	if (!stats_increased)
	{
		stats_increased = TRUE;
		give_proper_stat(ch);
		act("$n &+Lgrows more powerful!", FALSE, ch, 0, 0, TO_ROOM);
		return TRUE;
	}

	/*
	  Recharging the shout action, and preparing for the next battle
	  (a little surprise for groups that massed and/or had to retreat)
	*/
	if (demogorgon_shouted && !number(0, 10) && !IS_FIGHTING(ch))
	{
		demogorgon_shouted = FALSE; /* he's ready for shouting again */
		act("$n &+Lregains his posture, ready for any forthcoming battles.", FALSE, ch, 0,
		    0, TO_ROOM);
		wizlog(MINLVLIMMORTAL,
		       "Demogorgon has just regained the ability to call for help.");
		return TRUE;
	}

	if (!IS_FIGHTING(ch))
		return FALSE;

	/*
	  Note, the new "call friends in" radius is only TWO - this makes it
	  possible for groups to take chances with tackling the boss before
	  having actually fully cleared the grid. Radius 3 is still a good
	  chunk of the cubic grid. Most of it actually. To check if this shouldn't
	  get lowered down to 2.
	*/

	if (!demogorgon_shouted)
	{
		demogorgon_shouted = TRUE;
		shout_and_hunt(
			ch, 3,
			"&+GYou will pay for attacking me mortal worms!   Denizens of darkness, come and feast upon %s!",
			NULL, helpers, 0, 0);
		return TRUE;
	}

	/*
	  And now it's time for his second head to act.
	  Alternatively, both heads concentrate on using the tail.
	*/

	switch (number(0, 2))
	{
	case 0:
	case 1:
		demogorgon_second_head(ch);
		break;
	case 2:
		demogorgon_tail(ch);
		break;
	default:
		break;
	}
	return TRUE;
}
