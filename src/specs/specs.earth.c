/* Temple of Earth special procedures. */

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

#define ELIGOTH_RIFT_VNUM 43580
#define TOE_SWITCH_ROOM 43697

int eligoth_rift_spawn(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	if (cmd == CMD_DEATH)
	{
		P_obj obj1 = read_object(ELIGOTH_RIFT_VNUM, VIRTUAL);
		if (!obj1)
		{
			logit(LOG_DEBUG, "eligoth_rift_spawn: object failed to load.");
			debug("eligoth_rift_spawn: object failed to load.");
			return FALSE;
		}

		act("&+YAs the &+rlifeblood &+Yflows from the defeated &+LLord&+Y, his remains twist\n"
		    "&+Yand writhe, spasming violently.  A fell &+Lmixture &+Yof steam and smoke begins\n"
		    "&+Yto seep from the corpse as it slowly &+Lcollapses &+Yinto itself, forming a &+Ldark\n"
		    "&+Lrift.&n",
		    FALSE, ch, 0, 0, TO_ROOM);

		obj_to_room(obj1, ch->in_room);

		return (FALSE);
	}

	return FALSE;
}

#undef ELIGOTH_RIFT_VNUM

int toe_chamber_switch(P_obj obj, P_char ch, int cmd, char *arg)
{
	int correct_button = 0, button_guess = 0;
	P_char summoned;
	P_obj s_obj, t_obj;
	int mobiles[] = { 43540, 43539, 43538, 43550, 0 };

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!IS_ALIVE(ch) || !obj)
	{
		return FALSE;
	}

	for (s_obj = world[real_room0(TOE_SWITCH_ROOM)].contents; s_obj; s_obj = t_obj)
	{
		t_obj = s_obj->next_content;

		if (isname(s_obj->name, "limestone"))
		{
			correct_button = 1;
		}
		if (isname(s_obj->name, "obsidian"))
		{
			correct_button = 2;
		}
		if (isname(s_obj->name, "granite"))
		{
			correct_button = 3;
		}
		if (isname(s_obj->name, "adamantite"))
		{
			correct_button = 4;
		}
		if (isname(s_obj->name, "jade"))
		{
			correct_button = 5;
		}
	}

	if (arg && (cmd == CMD_PUSH))
	{
		if (isname(arg, "limestone"))
		{
			button_guess = 1;
		}
		if (isname(arg, "obsidian"))
		{
			button_guess = 2;
		}
		if (isname(arg, "granite"))
		{
			button_guess = 3;
		}
		if (isname(arg, "adamantite"))
		{
			button_guess = 4;
		}
		if (isname(arg, "jade"))
		{
			button_guess = 5;
		}

		if (correct_button > 0 && button_guess > 0 && correct_button == button_guess)
		{
			REMOVE_BIT(world[ch->in_room].dir_option[0]->exit_info, EX_BLOCKED);
			act("&+yA &+Lgrinding &+ysound of stone on stone reverberates about the chamber,\r\n"
			    "&+ybefore ending abruptly in an audible &+Lclick&+y.&n\r\n",
			    TRUE, ch, 0, 0, TO_CHAR);
			act("&+yA &+Lgrinding &+ysound of stone on stone reverberates about the chamber,\r\n"
			    "&+ybefore ending abruptly in an audible &+Lclick&+y.&n\r\n",
			    TRUE, ch, 0, 0, TO_ROOM);
			return TRUE;
		}
		else if (button_guess <= 5 && button_guess >= 1)
		{
			int i = number(0, 3);
			summoned = read_mobile(mobiles[i], VIRTUAL);

			if (!summoned)
			{
				logit(LOG_EXIT, "assert: error in toe_chamber_switch() proc");
				return FALSE;
			}
			act("&+yThere is a rumbling sound of movement from overhead.  A hidden trapdoor swings\r\n"
			    "&+yopen suddenly, releasing $N &+yback into the temple!&n\n"
			    "&+yJust as quickly, the trapdoor slams shut again, stirring up a &+Lcloud&+y of dust.&n\r\n",
			    FALSE, ch, 0, summoned, TO_CHAR);
			act("&+yThere is a rumbling sound of movement from overhead.  A hidden trapdoor swings\r\n"
			    "&+yopen suddenly, releasing $N &+yback into the temple!&n\n"
			    "&+yJust as quickly, the trapdoor slams shut again, stirring up a &+Lcloud&+y of dust.&n\r\n",
			    FALSE, ch, 0, summoned, TO_ROOM);
			char_to_room(summoned, ch->in_room, 0);
			return TRUE;
		}
	}
	return FALSE;
}
