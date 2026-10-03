/* Special procedures for Menden-on-the-Deep. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "guild/assocs.h"
#include "world/specs.prototypes.h"
#include <string.h>

int menden_figurine_die(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd == CMD_DEATH)
	{
		act("$n dies, crumbling into powder which is quickly swept away by a sudden breeze.",
		    TRUE, ch, 0, 0, TO_ROOM);
	}
	return (FALSE);
}

int menden_inv_serv_die(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd == CMD_DEATH)
	{
		act("$n, vanquished, dissolves into ethereal vapors and disappears.", FALSE, ch, 0,
		    0, TO_ROOM);
	}
	return (FALSE);
}

/*
 * This is for the drunken magus in Menden-on-the-Deep, but its style is
 * * lifted from jester(), with my own additions, of course. >8^)
 * *
 * * -- Damon Silver, aka Fleven 1994/07/02
 */

int menden_magus(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd || !IS_AWAKE(ch) || cmd)
		return FALSE;

	switch (number(0, 50))
	{
	case 0:
		mobsay(ch, "What the hell wassh that shtupid word again?");
		return TRUE;
	case 1:
		mobsay(ch, "Shnake? No.");
		return TRUE;
	case 2:
		mobsay(ch, "Shlitherer? Nope nope nope.");
		return TRUE;
	case 3:
		mobsay(ch, "Definitely shomething to do with a shea monshter...");
		return TRUE;
	case 4:
		mobsay(ch, "Wench! Bring me another tankard of ale!");
		return TRUE;
	case 5:
		mobsay(ch, "Time to be getting home, maybe.");
		return TRUE;
	case 6:
		act("$n hiccups, and a little bolt of lightning shoots from his fingers, scorching the floor.",
		    TRUE, ch, 0, 0, TO_ROOM);
		mobsay(ch, "'Shcuse me.");
		act("$n grins sheepishly.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 7:
		act("$n teeters for a moment, but regains his balance.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 8:
		do_action(ch, 0, CMD_MUTTER);
		return TRUE;
	default:
		return FALSE;
	}
}

int menden_fisherman(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	char buf[MAX_INPUT_LENGTH];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || cmd)
		return FALSE;

	switch (number(1, 80))
	{
	case 1:
	{
		do_action(ch, 0, CMD_BURP);
		return TRUE;
	}
	case 2:
	{
		mobsay(ch, "'Course, Kilten's been sayin'...'scuse me.");
		do_action(ch, 0, CMD_COUGH);
		mobsay(ch, "Where was I? Oh.");
		strcpy(buf, "carafe");
		do_action(ch, buf, CMD_SIP);
		mobsay(ch, "His damned ship can make it to Verzanan in under two days.");
		do_action(ch, 0, CMD_ROLL);
		return TRUE;
	}
	case 3:
	{
		do_action(ch, 0, CMD_SMIRK);
		return TRUE;
	}
	case 4:
	{
		strcpy(buf, "wench");
		do_action(ch, buf, CMD_PINCH);
		return TRUE;
	}
	case 5:
	{
		mobsay(ch, "I once caught a dragonfish _this_ big!");
		act("$n stretches $s arms wide.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	}
	case 6:
	{
		mobsay(ch, "Those damned harpies must have eaten half me crew.");
		do_action(ch, 0, CMD_SHIVER);
		return TRUE;
	}
	case 7:
	{
		mobsay(ch, "I was stranded in the Moonshaes for put near a month.");
		return TRUE;
	}
	case 8:
	{
		mobsay(ch, "Yeah, me wife's been nagging at me to get her some pearls.");
		return TRUE;
	}
	case 9:
	{
		do_action(ch, 0, CMD_SCRATCH);
		return TRUE;
	}
	case 10:
	{
		mobsay(ch, "I've got a couple of brats down south, but I hardly ever see 'em.");
		do_action(ch, 0, CMD_CRY);
		return TRUE;
	}
	case 11:
	{
		mobsay(ch,
		       "So I roll a five, another five, and suddenly I'm ahead forty fire-eyes!");
		return TRUE;
	}
	case 12:
	{
		mobsay(ch, "Aaaaah, feels good to be on dry land for a stretch.");
		do_action(ch, 0, CMD_STRETCH);
		do_action(ch, 0, CMD_WINK);
		return TRUE;
	}
	case 13:
	{
		do_action(ch, 0, CMD_HICCUP);
		return TRUE;
	}
	case 14:
	{
		mobsay(ch, "Murkas Magintii?  Yeah, I done heared of him.");
		mobsay(ch, "Wasn't he that guy that got swallowed by a whale?");
		return TRUE;
	}
	case 15:
	{
		strcpy(buf, "wench");
		do_action(ch, buf, CMD_OGLE);
		return TRUE;
	}
	case 16:
	{
		mobsay(ch, "Calim harem girls know tricks that'll blow yer jerkin off!");
		do_action(ch, 0, CMD_WINK);
		return TRUE;
	}
	case 17:
	{
		mobsay(ch, "I heard there be a spell lets ye ken any language.");
		do_action(ch, 0, CMD_SHRUG);
		do_action(ch, 0, CMD_PONDER);
		mobsay(ch, "Valkur knows that'd be helpful when I go tradin'!");
		do_action(ch, 0, CMD_CACKLE);
		return TRUE;
	}
	case 18:
	{
		mobsay(ch, "Y'know, me pa was a fisherman too.");
		return TRUE;
	}
	case 19:
	{
		mobsay(ch,
		       "Ye'll never be as strong fighting wimpy dragons as if ye work the sea.");
		do_action(ch, 0, CMD_FLEX);
		return TRUE;
	}
	case 20:
	{
		mobsay(ch,
		       "Wench, remind me to bring you back some turtle legs from Nhavan Island next trip.");
		strcpy(buf, "wench");
		do_action(ch, buf, CMD_SMILE);
		return TRUE;
	}
	case 21:
	{
		strcpy(buf, "me");
		do_action(ch, buf, CMD_SCRATCH);
		mobsay(ch, "What're ye after, anyway, ye crazy old coot?");
		strcpy(buf, "magus");
		do_action(ch, buf, CMD_POKE);
		return TRUE;
	}
	default:
		return FALSE;
	}
}

int menden_figurine(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char tempchar = NULL;
	int pos = -1;
	char Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!OBJ_WORN(obj) || !IS_ALIVE(ch) || !IS_AWAKE(ch))
	{
		return FALSE;
	}
	if (obj->loc.wearing != ch)
	{
		logit(LOG_DEBUG, "Buggy equip in figurine(): obj->loc.wearing != ch.");
		return FALSE;
	}

	// Item must be held to work.
	if (!OBJ_WORN_POS(obj, HOLD) && !OBJ_WORN_POS(obj, WIELD))
	{
		return FALSE;
	}

	if (cmd == CMD_FLEX)
	{
		argument_interpreter(arg, Gbuf1, Gbuf2);

		if (isname(Gbuf1, obj->name))
		{
			// Load depicted mob
			tempchar = read_mobile(obj->value[0], VIRTUAL);

			if (!tempchar)
			{
				logit(LOG_DEBUG, "menden_figurine(): mob %d not loadable",
				      obj->value[0]);
				logit(LOG_SYS, "read_mobile(): failed to load");
				return FALSE;
			}
			char_to_room(tempchar, ch->in_room, -1);
			unequip_char(obj->loc.wearing, pos);

			act("You flex $p several times, until it finally snaps!", TRUE, ch, obj, 0,
			    TO_CHAR);
			act("$n flexes $p several times, until it finally snaps!", TRUE, ch, obj, 0,
			    TO_NOTVICT);
			act("From the $o come swirling vapors, which solidify to form $n.", TRUE,
			    tempchar, obj, 0, TO_ROOM);
			act("The $o rapidly disintegrates to powder, only to be born away by a sudden wind.",
			    TRUE, tempchar, obj, 0, TO_ROOM);

			add_follower(tempchar, ch);
			setup_pet(tempchar, ch, 10, PET_NOCASH);

			extract_obj(obj, TRUE); // Not an arti, but 'in game.'
			return TRUE;
		}
	}
	return FALSE;
}

/*
 * spec death proc for hippogriff, mob 88815
 */

int hippogriff_die(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd == CMD_DEATH)
		act("As $n dies, it disintegrates in a flash of bright light!", TRUE, ch, 0, 0,
		    TO_ROOM);
	return (FALSE);
}

/*
 * special death proc for crystal golem, mob 88814
 */

int crystal_golem_die(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd == CMD_DEATH)
		act("As $n dies, it shatters into crystal dust which quickly dissipates.", TRUE, ch,
		    0, 0, TO_ROOM);
	return (FALSE);
}
