/* Player guild-castle object procedures. */

#include <string.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"

extern P_room world;

int guildwindow(P_obj obj, P_char ch, int cmd, char *arg)
{
	int old_room;

	if (cmd == CMD_PERIODIC)
		return (FALSE);

	if (cmd != CMD_LOOK)
		return (FALSE);

	if (!arg || !*arg || str_cmp(arg, " window"))
		return (FALSE);

	if (cmd && (cmd == CMD_LOOK))
	{
		old_room = ch->in_room;
		char_from_room(ch);
		char_to_room(ch, real_room0(obj->value[0]), -1);
		char_from_room(ch);
		char_to_room(ch, old_room, -2);
	}
	return (TRUE);
}

int guildhome(P_obj obj, P_char ch, int cmd, char *argument)
{
	char *arg;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!obj)
		return FALSE;

	if (argument && (cmd == CMD_SAY))
	{
		arg = argument;

		while (*arg == ' ')
			arg++;

		if (!strcmp(arg, "home"))
		{
			if (!say(ch, arg))
				return TRUE;

			GET_BIRTHPLACE(ch) = world[ch->in_room].number;
			send_to_char("&+CYou feel a warmth which quickly fades away...\n", ch);
			return TRUE;
		}
	}
	return FALSE;
}
