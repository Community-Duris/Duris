/* Special procedures for shared teleporting pools. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "combat/ctf.h"
#include "world/db.h"
#include "world/events.h"
#include "world/specs.prototypes.h"

extern P_room world;
extern struct zone_data *zone_table;

/*
   This is for a random teleporting pool in a zone -DR
 */
int teleporting_pool(P_obj obj, P_char ch, int cmd, char *arg)
{
	int to_room;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	// Check to see if someone is trying to enter the pool
	if (cmd == CMD_ENTER)
	{
		return magic_pool(obj, ch, cmd, arg);
	}

	if (!ch)
	{
		/*
		   Random chance of 6%-first number that came to mind ;) of teleporting.
		   This might be a bit much, change if needed.
		 */
		if (number(1, 250) > 15)
		{
			return FALSE;
		}

		// Randomly pick a room in the zone
		do
		{
			to_room = (number(zone_table[world[obj->loc.room].zone].real_bottom,
					  zone_table[world[obj->loc.room].zone].real_top));
		} while (IS_ROOM(to_room, ROOM_PRIVATE | ROOM_NO_MOB | ROOM_NO_TRACK));

		act("$p &+Lslowly vanishes away into nothingness.&N", TRUE, NULL, obj, NULL,
		    TO_ROOM);
		obj_from_room(obj);
		obj_to_room(obj, to_room);
		act("$p &+Lslowly forms in front of you.&N", TRUE, NULL, obj, NULL, TO_ROOM);
		return TRUE;
	}
	return FALSE;
}

int teleporting_map_pool(P_obj obj, P_char ch, int cmd, char *arg)
{
	int to_room;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd == CMD_ENTER)
	{
		return magic_map_pool(obj, ch, cmd, arg);
	}

	if (!ch)
	{
		/*
		   Random chance of 6%-first number that came to mind ;) of teleporting.
		   This might be a bit much, change if needed.
		 */
		if (number(1, 250) > 15)
		{
			return FALSE;
		}

		// Randomly pick a room in the zone
		do
		{
			to_room = (number(zone_table[world[obj->loc.room].zone].real_bottom,
					  zone_table[world[obj->loc.room].zone].real_top));
		} while (IS_ROOM(to_room, ROOM_PRIVATE | ROOM_NO_MOB | ROOM_NO_TRACK));

		act("$p &+Lslowly vanishes away into nothingness.&N", TRUE, NULL, obj, NULL,
		    TO_ROOM);
		obj_from_room(obj);
		obj_to_room(obj, to_room);
		act("$p &+Lslowly forms in front of you.&N", TRUE, NULL, obj, NULL, TO_ROOM);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

int magic_pool(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = obj->value[1];
	char Gbuf1[MAX_STRING_LENGTH];

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (cmd != CMD_ENTER || !OBJ_ROOM(obj) || !IS_ALIVE(ch) || !arg)
	{
		return FALSE;
	}

	one_argument(arg, Gbuf1);
	// If not the right portal..
	if (obj != get_obj_in_list(Gbuf1, world[ch->in_room].contents))
	{
		return FALSE;
	}

	if (real_room(obj->value[0]) == NOWHERE)
	{
		send_to_char("Hmm...  Looks like it's busted.  Might wanna notify a god.\n", ch);
		return (FALSE);
	}

#if defined(CTF_MUD) && (CTF_MUD == 1)
	if (ctf_carrying_flag(ch) == CTF_PRIMARY)
	{
		send_to_char("You can't carry that with you.\r\n", ch);
		drop_ctf_flag(ch);
	}
#endif

	act("As you step into the $o, there is a blinding flash of light!", FALSE, ch, obj, 0,
	    TO_CHAR);
	act("You are ripped through a dark and star-filled void, pain sears through", FALSE, ch,
	    obj, 0, TO_CHAR);
	act("your body!  When you again open your eyes, you are elsewhere...", FALSE, ch, obj, 0,
	    TO_CHAR);
	act("$n vanishes into the $o.", FALSE, ch, obj, 0, TO_ROOM);

	if (!IS_TRUSTED(ch))
	{
		if (GET_HIT(ch) > dam)
			GET_HIT(ch) -= dam;
		else
			GET_HIT(ch) = 1;
		StartRegen(ch, regen_resource::hit);
	}
	teleport_to(ch, real_room(obj->value[0]), 0);

	return (TRUE);
}

// For the gate to ardgral spell.
int magic_map_pool(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = obj->value[1];
	char Gbuf1[MAX_STRING_LENGTH];
	int target_room;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (cmd != CMD_ENTER || !IS_ALIVE(ch) || !arg || !OBJ_ROOM(obj))
	{
		return FALSE;
	}

	one_argument(arg, Gbuf1);
	// If not the right portal..
	if (obj != get_obj_in_list(Gbuf1, world[ch->in_room].contents))
	{
		return FALSE;
	}

	target_room = real_room(random_map_room());

	while (world[target_room].sector_type == SECT_MOUNTAIN ||
	       world[target_room].sector_type == SECT_INSIDE ||
	       world[target_room].sector_type == SECT_OCEAN || IS_ROOM(target_room, ROOM_NO_GATE))
	{
		target_room = real_room(random_map_room());
	}

	if (target_room == NOWHERE)
	{
		debug("magic_map_pool: Target room is NOWHERE for char '%s'.", J_NAME(ch));
		send_to_char("Hmm...  Looks like it's busted.  Might wanna notify a god.\n", ch);
		return FALSE;
	}

	act("As you step into the $o, there is a blinding flash of light!", FALSE, ch, obj, 0,
	    TO_CHAR);
	act("You are ripped through a dark and star-filled void, pain sears through", FALSE, ch,
	    obj, 0, TO_CHAR);
	act("your body!  When you again open your eyes, you are elsewhere...", FALSE, ch, obj, 0,
	    TO_CHAR);
	act("$n vanishes into the $o.", FALSE, ch, obj, 0, TO_ROOM);

	if (!IS_TRUSTED(ch))
	{
		// Tighter like this.
		GET_HIT(ch) = (GET_HIT(ch) > dam) ? GET_HIT(ch) - dam : 1;
		StartRegen(ch, regen_resource::hit);
	}
	teleport_to(ch, target_room, 0);

	return TRUE;
}

#define FLT_TOROOM(x, y) (world[(x)].dir_option[(y)]->to_room)

int floating_pool(P_obj obj, P_char ch, int cmd, char *arg)
{
	int num_choices = 0, i;
	int pos_dirs[NUM_EXITS];
	int my_room;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd)
		return magic_pool(obj, ch, cmd, arg);

	/*
	   okay... I will move with a 75% chance.
	 */
	if (number(1, 100) > 75)
		return FALSE;

	my_room = obj->loc.room;

	for (i = 0; i < NUM_EXITS; i++)
		if ((world[my_room].dir_option[i]) && (FLT_TOROOM(my_room, i) != NOWHERE) &&
		    (!IS_SET(world[(my_room)].dir_option[(i)]->exit_info, EX_CLOSED)) &&
		    (!IS_SET(world[(my_room)].dir_option[(i)]->exit_info, EX_SECRET)) &&
		    (!IS_SET(world[(my_room)].dir_option[(i)]->exit_info, EX_BLOCKED)) &&
		    (!IS_ROOM(FLT_TOROOM(my_room, i), ROOM_NO_MOB | ROOM_NO_TRACK)))
			pos_dirs[num_choices++] = i;

	if (!num_choices)
		return FALSE;

	/*
	   okay.. I'm going to move... lets do it
	 */

	act("$p floats away.", TRUE, NULL, obj, NULL, TO_ROOM);
	obj_from_room(obj);
	obj_to_room(obj, FLT_TOROOM(my_room, pos_dirs[number(0, num_choices - 1)]));
	act("$p floats in.", TRUE, NULL, obj, NULL, TO_ROOM);
	return TRUE;
}

#undef FLT_TOROOM
