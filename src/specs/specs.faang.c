/* Special procedures for Faang. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "world/difficulty.h"
#include "world/db.h"
#include "world/events.h"
#include "world/specs.prototypes.h"
#include "combat/damage.h"

extern P_room world;
extern struct zone_data *zone_table;

/*
 * Boulder pushers will check to see if there are non-evil races in the rooms
 * * they're surveying, then possibly throw a "boulder" at them as a warning,
 * * inflicting minor damage.
 * *
 * * Designed for the ogre boulder pushers of Faang by Oghma
 * * -- DTS 2/8/95
 */

int boulder_pusher(P_char ch, P_char t_ch, int cmd, char * /*arg*/)
{
	int to_room = NOWHERE, dam = 0;
	P_char victim;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
	{
		return (TRUE);
	}
	if (!ch || t_ch || cmd)
	{
		return (FALSE);
	}
	if (IS_FIGHTING(ch) || !IS_AWAKE(ch))
		return (FALSE);

	switch (world[ch->in_room].number)
	{
	case 15354:
	case 15355:
	case 15356:
		to_room = 15302;
		break;
	case 15299:
	case 15300:
	case 15301:
	case 15302:
		to_room = 15253;
		break;
	default:
		return (FALSE);
	}

	if (world[real_room(to_room)].people)
	{
		for (victim = world[real_room(to_room)].people; victim && EVIL_RACE(victim);
		     victim = victim->next_in_room)
			;
		if (victim)
		{ /*
		   * not an evil race:  drow/duergar/ogre/troll
		   */
			if ((CAN_SEE(ch, victim)) && (GET_LEVEL(victim) < MINLVLIMMORTAL))
			{
				if (number(1, 100) < 20)
				{
					act("$n suddenly pushes a nearby boulder off the ledge with a roar!",
					    FALSE, ch, 0, 0, TO_ROOM);
					act("$n peers downward to see the results of $s toss.",
					    FALSE, ch, 0, 0, TO_ROOM);
					if (!IS_SET(zone_table[world[victim->in_room].zone].flags,
						    ZONE_SILENT) &&
					    !IS_ROOM(victim->in_room, ROOM_SILENT))
					{
						act("You hear a mighty roar from overhead, and feel a sudden chill!",
						    TRUE, victim, 0, 0, TO_CHAR);
						act("You suddenly hear a mighty roar from overhead!",
						    TRUE, victim, 0, 0, TO_ROOM);
					}
					else
					{
						act("You feel a sudden chill, as of impending doom.",
						    TRUE, victim, 0, 0, TO_CHAR);
					}

					/*
					 * does it hit? partly based on chance, with slight bonus for dex
					 */
					if ((number(1, 100) + STAT_INDEX(GET_C_DEX(victim))) < 50)
					{ /*
					   * hit
					   *
					   */
						act("A distant thud and a cry of pain can be heard from below.",
						    FALSE, ch, 0, 0, TO_ROOM);

						act("A boulder hurtles down from overhead, striking you soundly!",
						    FALSE, victim, 0, 0, TO_CHAR);
						act("A boulder hurtles down from overhead, striking $n soundly!",
						    TRUE, victim, 0, 0, TO_ROOM);

						/*
						 * damage is based partly on strength of ogre, partly on chance
						 */
						dam = number(1, 50) + STAT_INDEX(GET_C_STR(ch));
						GET_HIT(victim) -= dam;
						update_pos(victim);
						send_to_char("&+ROWWW!!&n That really hurt!\r\n",
							     victim);
						send_to_char(
							"It would probably be a good idea to GET OUT OF HERE!\r\n",
							victim);

						/*
						 * does it kill victim?
						 */
						if (GET_HIT(victim) < -10)
						{
							send_to_char(
								"Alas, your wounds prove too much for you...\r\n",
								victim);
							die(victim, ch);
							return (TRUE);
						}
						StartRegen(victim, regen_resource::hit);

						/*
						 * low possibility of stunnage or even KO
						 */
						if (number(1, 100) < 10)
						{
							if (number(1, 100) < 40)
							{ /*
							   * KO
							   */
								KnockOut(
									victim,
									number(2,
									       25 - STAT_INDEX(GET_C_CON(
											    victim))));
							}
							else
							{ /*
							   * stun
							   */
								Stun(victim, ch,
								     (number(2, 10) *
								      PULSE_VIOLENCE),
								     TRUE);
							}
						}
						return (TRUE);
					}
					else
					{ /*
					   * miss
					   */
						act("A distant thud can be heard.  $n curses and kicks at the ground.",
						    TRUE, ch, 0, 0, TO_ROOM);
						act("A boulder from overhead narrowly misses you and bounces off to one side.",
						    FALSE, victim, 0, 0, TO_CHAR);
						act("Perhaps would be a good time to LEAVE this area!",
						    FALSE, victim, 0, 0, TO_CHAR);
						act("A boulder from overhead narrowly misses $n and bounces off to one side.",
						    TRUE, victim, 0, 0, TO_ROOM);
						act("You count your lucky stars that it didn't strike you!",
						    TRUE, victim, 0, 0, TO_ROOM);
						act("It would probably be a good idea to leave before more rocks come!",
						    TRUE, victim, 0, 0, TO_ROOM);
						return (TRUE);
					}
				}
			}
		}
	}
	/*
	 * random actions if no target or unsatisfactory target
	 */
	switch (number(1, 30))
	{
	case 1:
		do_action(ch, 0, CMD_MUTTER);
		return (TRUE);
	case 2:
		do_action(ch, 0, CMD_GRUMBLE);
		return (TRUE);
	case 3:
		do_action(ch, 0, CMD_TARZAN);
		return (TRUE);
	case 4:
		do_action(ch, 0, CMD_STOMP);
		return (TRUE);
	case 5:
		do_action(ch, 0, CMD_GRUNT);
		return (TRUE);
	case 6:
		do_action(ch, 0, CMD_ROAR);
		return (TRUE);
	case 7:
		do_action(ch, 0, CMD_FART);
		return (TRUE);
	case 8:
		do_action(ch, 0, CMD_STARE);
		return (TRUE);
	case 9:
		do_action(ch, 0, CMD_DROOL);
		return (TRUE);
	default:
		return (FALSE);
	}
}
