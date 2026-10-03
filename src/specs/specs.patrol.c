/* Patrol leader special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "guild/assocs.h"
#include "combat/justice.h"
#include "combat/range.h"
#include "world/map.h"
#include "world/specs.prototypes.h"

extern P_room world;
extern struct zone_data *zone;
extern struct zone_data *zone_table;

/*
 *Patrol leader Mob Proc
 */

int patrol_leader(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	int door, direction;
	bool CombatInRoom;
	P_char tmp_ch;
	struct follow_type *k, *next_dude;
	char buf[256];

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd)
		return FALSE;

	if (GET_VITALITY(ch) < 10) /* ok dont get too tired */
		return TRUE;

	/* ok check to make sure followers are not out of move */

	if (ch->followers)
	{
		for (k = ch->followers; k; k = next_dude)
		{
			next_dude = k->next;
			if (IS_NPC(k->follower) && (GET_VITALITY(k->follower) < 10))
				return TRUE;
		}
	}

	CombatInRoom = FALSE;

	if (!ALONE(ch))
	{
		if (IS_FIGHTING(ch))
			CombatInRoom = TRUE;
		else
		{
			LOOP_THRU_PEOPLE(tmp_ch, ch)
				if (IS_FIGHTING(tmp_ch))
				{
					CombatInRoom = TRUE;
					break;
				}
		}
	}

	if (IS_FIGHTING(ch) && number(1, 3) == 1)
	{
		if (IS_PC(GET_OPPONENT(ch)) && IS_RACEWAR_EVIL(GET_OPPONENT(ch)))
		{
			strcpy(buf, "Die you evil scum!");
			do_yell(ch, buf, CMD_SHOUT);
		}
		else if (IS_PC(GET_OPPONENT(ch)) && IS_RACEWAR_GOOD(GET_OPPONENT(ch)))
		{
			strcpy(buf, "You moron I am here to protect you!");
			do_say(ch, buf, CMD_SAY);
		}
	}

	if (!CombatInRoom && (ch->in_room != NOWHERE) && !IS_ROOM(ch->in_room, ROOM_SILENT) &&
	    !IS_SET(zone_table[world[ch->in_room].zone].flags, ZONE_SILENT) &&
	    (MIN_POS(ch, POS_STANDING + STAT_NORMAL)))
	{
		/* ok we check if there is any evils near */

		if ((direction = range_scan(ch, NULL, 3, SCAN_EVILRACE)) >= 0)
		{
			if (EXIT(ch, direction))
			{
				if ((EXIT(ch, direction))->to_room &&
				    world[EXIT(ch, direction)->to_room].justice_area ==
					    world[ch->in_room].justice_area)
				{
					ch->only.npc->last_direction = direction;
					do_move(ch, 0, exitnumb_to_cmd(direction));
					return TRUE;
				}
			}
		}

		/* ok we check if there is combat near */

		if ((direction = range_scan(ch, NULL, 2, SCAN_COMBAT)) >= 0)
		{
			if (EXIT(ch, direction))
			{
				if ((EXIT(ch, direction))->to_room &&
				    world[EXIT(ch, direction)->to_room].justice_area ==
					    world[ch->in_room].justice_area)
				{
					ch->only.npc->last_direction = direction;
					do_move(ch, 0, exitnumb_to_cmd(direction));
					return TRUE;
				}
			}
		}
	}

	/* seem we can try to move, since nothing else to do */

	if (!CombatInRoom && !IS_AFFECTED(ch, AFF_CHARM))
	{
		if ((MIN_POS(ch, POS_STANDING + STAT_RESTING)) && ((door = number(0, 4)) < 4))
		{
			if (EXIT(ch, door))
			{
				if (CAN_GO(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
				    !IS_ROOM(EXIT(ch, door)->to_room, ROOM_NO_MOB) &&
				    !IS_ROOM(EXIT(ch, door)->to_room, ROOM_NO_TRACK) &&
				    world[EXIT(ch, door)->to_room].sector_type != SECT_NO_GROUND)
				{
					if (world[EXIT(ch, door)->to_room].justice_area ==
					    world[ch->in_room].justice_area)
					{
						ch->only.npc->last_direction = door;
						do_move(ch, 0, exitnumb_to_cmd(door));
						return TRUE;
					}
				}
			}
		}
	}

	return FALSE;
}

/*
 * Patrol leader (road) Mob Proc
 * this mob should stay on road, if he get away from road he gonna try to get back asap
 */

int patrol_leader_road(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	int door, i, direction;
	bool CombatInRoom;
	P_char tmp_ch;
	struct follow_type *k, *next_dude;
	char pos_exit[NUM_EXITS];
	int nb_exit = 0;
	char buf[256];

	/* check for periodic event calls */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd)
		return FALSE;

	if (GET_VITALITY(ch) < 10) /* ok dont get too tired */
		return TRUE;

	/* ok check to make sure followers are not out of move */

	if (ch->followers)
	{
		for (k = ch->followers; k; k = next_dude)
		{
			next_dude = k->next;
			if (IS_NPC(k->follower) && (GET_VITALITY(k->follower) < 10))
				return TRUE;
		}
	}

	CombatInRoom = FALSE;

	if (!ALONE(ch))
	{
		if (IS_FIGHTING(ch))
			CombatInRoom = TRUE;
		else
		{
			LOOP_THRU_PEOPLE(tmp_ch, ch)
				if (IS_FIGHTING(tmp_ch))
				{
					CombatInRoom = TRUE;
					break;
				}
		}
	}

	if (IS_FIGHTING(ch) && number(1, 3) == 1)
	{
		if (IS_PC(GET_OPPONENT(ch)) && IS_RACEWAR_EVIL(GET_OPPONENT(ch)))
		{
			strcpy(buf, "Die you evil scum!");
			do_yell(ch, buf, CMD_SHOUT);
		}
		else if (IS_PC(GET_OPPONENT(ch)) && IS_RACEWAR_GOOD(GET_OPPONENT(ch)))
		{
			strcpy(buf, "You moron I am here to protect you!");
			do_say(ch, buf, CMD_SAY);
		}
	}

	if (!CombatInRoom && (ch->in_room != NOWHERE) && !IS_ROOM(ch->in_room, ROOM_SILENT) &&
	    !IS_SET(zone_table[world[ch->in_room].zone].flags, ZONE_SILENT) &&
	    (MIN_POS(ch, POS_STANDING + STAT_NORMAL)))
	{
		/* ok we check if there is any evils near */

		if ((direction = range_scan(ch, NULL, 1, SCAN_EVILRACE)) >= 0)
		{
			if (EXIT(ch, direction))
			{
				if (EXIT(ch, direction)->to_room &&
				    world[EXIT(ch, direction)->to_room].justice_area ==
					    world[ch->in_room].justice_area)
				{
					ch->only.npc->last_direction = direction;
					do_move(ch, 0, exitnumb_to_cmd(direction));
					return TRUE;
				}
			}
		}

		/* ok we check if there is combat near */

		if ((direction = range_scan(ch, NULL, 1, SCAN_COMBAT)) >= 0)
		{
			if (EXIT(ch, direction))
			{
				if (EXIT(ch, direction)->to_room &&
				    world[EXIT(ch, direction)->to_room].justice_area ==
					    world[ch->in_room].justice_area)
				{
					ch->only.npc->last_direction = direction;
					do_move(ch, 0, exitnumb_to_cmd(direction));
					return TRUE;
				}
			}
		}
	}

	/* seem we can try to move, since nothing else to do */

	if (world[ch->in_room].sector_type != SECT_ROAD)
	{ /* gasp we left the road */

		if (!CombatInRoom && !IS_AFFECTED(ch, AFF_CHARM))
		{
			/* ok first lets see if we near a road? */
			for (door = 0; door < 4; door++)
			{
				if (EXIT(ch, door))
				{
					if ((MIN_POS(ch, POS_STANDING + STAT_RESTING)) &&
					    (EXIT(ch, door)->to_room) && CAN_GO(ch, door) &&
					    !IS_ROOM(EXIT(ch, door)->to_room, ROOM_NO_MOB) &&
					    !IS_ROOM(EXIT(ch, door)->to_room, ROOM_NO_TRACK) &&
					    world[EXIT(ch, door)->to_room].sector_type !=
						    SECT_NO_GROUND &&
					    world[EXIT(ch, door)->to_room].justice_area ==
						    world[ch->in_room].justice_area &&
					    world[EXIT(ch, door)->to_room].sector_type == SECT_ROAD)
					{
						ch->only.npc->last_direction = door;
						do_move(ch, 0, exitnumb_to_cmd(door));
						return TRUE;
					}
				}
			}

			/* ok seem we are not near a road, PANIC! we are lost */
			/* lets move and hope we hit a road somewhere */

			if ((MIN_POS(ch, POS_STANDING + STAT_RESTING)) &&
			    ((door = number(0, 4)) < 4))
			{
				if (EXIT(ch, door))
				{
					if (CAN_GO(ch, door) && (EXIT(ch, door)->to_room) &&
					    !IS_ROOM(EXIT(ch, door)->to_room, ROOM_NO_MOB) &&
					    !IS_ROOM(EXIT(ch, door)->to_room, ROOM_NO_TRACK) &&
					    world[EXIT(ch, door)->to_room].sector_type !=
						    SECT_NO_GROUND)
					{
						if (EXIT(ch, door)->to_room &&
						    world[EXIT(ch, door)->to_room].justice_area ==
							    world[ch->in_room].justice_area)
						{
							ch->only.npc->last_direction = door;
							do_move(ch, 0, exitnumb_to_cmd(door));
							return TRUE;
						}
					}
				}
			}
		}
	}
	else
	{ /* ok we are on the road so lets move */
		if (!CombatInRoom && !IS_AFFECTED(ch, AFF_CHARM))
		{
			/* first we check where we can go */
			for (door = 0; door < 4; door++)
			{
				if (EXIT(ch, door))
				{
					if ((MIN_POS(ch, POS_STANDING + STAT_RESTING)) &&
					    CAN_GO(ch, door) && (EXIT(ch, door)->to_room) &&
					    !IS_ROOM(EXIT(ch, door)->to_room, ROOM_NO_MOB) &&
					    !IS_ROOM(EXIT(ch, door)->to_room, ROOM_NO_TRACK) &&
					    world[EXIT(ch, door)->to_room].sector_type !=
						    SECT_NO_GROUND &&
					    world[EXIT(ch, door)->to_room].justice_area ==
						    world[ch->in_room].justice_area &&
					    world[EXIT(ch, door)->to_room].sector_type == SECT_ROAD)
					{
						pos_exit[door] = TRUE;
						nb_exit++;
					}
					else
					{
						pos_exit[door] = FALSE;
					}
				}
				else
					pos_exit[door] = FALSE;
			}

			/* now lets make sure we dont go backward if there is another possibility */

			if (nb_exit == 1)
			{
				if ((i = number(0, nb_exit)) < nb_exit)
				{
					for (door = 0; door < 4; door++)
					{
						if (pos_exit[door])
						{
							ch->only.npc->last_direction = door;
							do_move(ch, 0, exitnumb_to_cmd(door));
							return TRUE;
						}
					}
				}
			}
			else
			{
				pos_exit[(int)rev_dir[(int)ch->only.npc->last_direction]] = FALSE;
				if ((i = number(1, nb_exit)) < nb_exit)
				{
					for (door = 0; door < 4; door++)
					{
						if (pos_exit[door])
							i--;
						if (i <= 0)
						{
							ch->only.npc->last_direction = door;
							do_move(ch, 0, exitnumb_to_cmd(door));
							return TRUE;
						}
					}
				}
			}
		}
	}
	return FALSE;
}
