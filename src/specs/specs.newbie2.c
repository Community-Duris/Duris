/* Special procedures for the Newbie Zone (vnums 228xx). */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include "world/vnum.obj.h"
#include <string.h>
#include <strings.h>

// From The Plains of Life - newbie zone.
int newbie_paladin(P_char ch, P_char pl, int cmd, char *arg)
{
	char arg1[MAX_STRING_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	P_obj sword;

	if (cmd != CMD_ASK || !IS_ALIVE(ch) || !IS_ALIVE(pl) || !arg)
	{
		return FALSE;
	}

	arg = one_argument(arg, arg1);
	arg = one_argument(arg, arg2);

	// If the first argument doesn't refer to the paladin, or the second arg isn't racewar and isn't racewars.
	if (!(get_char_room_vis(pl, arg1) == ch) ||
	    (strcmp(arg2, "racewar") && strcmp(arg2, "racewars")))
	{
		return FALSE;
	}

	if (!affected_by_spell(pl, TAG_LIFESTREAMNEWBIE))
	{
		mobsay(ch, "Go go in peace!");
		return TRUE;
	}
	affect_from_char(pl, TAG_LIFESTREAMNEWBIE);

	mobsay(ch, "&+bOgres&n, &+mDrow Elfs&n and &+gTrolls&n must die!&n");
	mobsay(ch, "Many evils died to this sword, it does me no good now, use it well.");
	mobsay(ch, "TYPE \"HELP RACEWAR\" for more information");

	sword = read_object(VOBJ_NEWBIE2_SWORD_BLESSED, VIRTUAL);
	act("$n gives $q to $N!", TRUE, ch, sword, pl, TO_NOTVICT);
	act("$n gives you $q!", TRUE, ch, sword, pl, TO_VICT);
	obj_to_char(sword, pl);
	return TRUE;
}

int newbie_sign1(P_obj /*obj*/, P_char ch, int cmd, char *arg)
{
	char Gbuf1[MAX_STRING_LENGTH];

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!ch || cmd == CMD_PERIODIC || !arg)
		return FALSE;
	one_argument(arg, Gbuf1);

	if (arg && (cmd == CMD_LOOK || cmd == CMD_EXAMINE || cmd == CMD_LOOK))
	{
		if (!isname(arg, "sign"))
			return FALSE;
		do_look(ch, writable_arg("sign"), -4);
		spell_armor(50, ch, 0, 0, ch, 0);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

// Second Sign
int newbie_sign2(P_obj /*obj*/, P_char ch, int cmd, char *arg)
{
	char Gbuf1[MAX_STRING_LENGTH];

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!ch || cmd == CMD_PERIODIC || !arg)
		return FALSE;
	one_argument(arg, Gbuf1);

	if (arg && (cmd == CMD_LOOK || cmd == CMD_EXAMINE || cmd == CMD_LOOK))
	{
		if (!isname(arg, "sign"))
			return FALSE;

		do_look(ch, writable_arg("sign"), -4);
		spell_bless(50, ch, 0, 0, ch, 0);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

int stream_of_life(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char dummy;
	P_obj dummyobj;
	int r_room;

	if (cmd != CMD_ENTER || !arg || !IS_ALIVE(ch) || !*arg)
		return FALSE;

	generic_find(arg, FIND_OBJ_ROOM, ch, &dummy, &dummyobj);
	if (obj == dummyobj)
	{
		if (affected_by_spell(ch, TAG_LIFESTREAMNEWBIE))
		{
			send_to_char("Your not ready yet, ask the paladin about racewar.\n", ch);
			return TRUE;
		}

		send_to_char("You feel refreshed as you enter the realm of life.\n", ch);
		GET_HIT(ch) = GET_MAX_HIT(ch);
		if ((r_room = real_room(29201)) > 0)
		{
			char_from_room(ch);
			char_to_room(ch, r_room, -1);
			act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
		}
		return TRUE;
	}

	return FALSE;
}
