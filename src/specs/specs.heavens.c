/* Heavens special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/map.h"
#include "world/vnum.room.h"
#include "world/weather.h"
#include "world/events.h"
#include "world/rested.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "core/utility.h"
#include "world/vnum.obj.h"
#include "economy/currency_transaction.h"
#include "economy/economic_gameplay_authority.h"
#include "combat/attack_continuation.h"
#include "combat/damage.h"
#include "item/forced_weapon_drop.h"
#include "item/native_artifact_actions.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include "classes/reavers.h"
#include "classes/necromancy.h"
#include "magic/blispells.h"
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <time.h>

extern P_char character_list;
extern char *coin_names[];
int isWieldingVnum(P_char, int);
int attemptToDisengage(P_char, int, char *);
extern bool has_skin_spell(P_char);
extern struct command_info cmd_info[MAX_CMD_LIST];
void reload_io_assistant(P_char, P_char, P_obj, void *);
extern struct time_info_data time_info;
extern void event_bleedproc(P_char ch, P_char victim, P_obj obj, void *data);
void bard_healing(int, P_char, P_char, int);
void bard_protection(int, P_char, P_char, int);
void bard_storms(int, P_char, P_char, int);
void bard_chaos(int, P_char, P_char, int);
void bard_harming(int, P_char, P_char, int);
void bard_cowardice(int, P_char, P_char, int);
void bard_calm(int, P_char, P_char, int);

int shadow_monster(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd == CMD_DEATH ||
	    (cmd == CMD_PERIODIC && (!number(0, GET_LEVEL(ch) / 3) || !GET_OPPONENT(ch))))
	{
		act("$n &+Lquickly fades into the thin air!", TRUE, ch, 0, 0, TO_ROOM);
		act("$n &+rdisappears as &+Lquickly&+r as it came!", TRUE, ch, 0, 0, TO_ROOM);
		extract_char(ch);
		return TRUE;
	}

	return FALSE;
}
int illus_dragon(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd == CMD_DEATH ||
	    (cmd == CMD_PERIODIC && (!number(0, GET_LEVEL(ch) / 3) || !GET_OPPONENT(ch))))
	{
		act("$n &+Lquickly fades into the thin air!", TRUE, ch, 0, 0, TO_ROOM);
		act("$n &+rdisappears as &+Lquickly&+r as it came!", TRUE, ch, 0, 0, TO_ROOM);
		extract_char(ch);
		return TRUE;
	}

	return FALSE;
}
int insects(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!IS_ALIVE(ch))
	{
		return FALSE;
	}

	if (cmd == CMD_DEATH ||
	    (cmd == CMD_PERIODIC && (!number(0, GET_LEVEL(ch) / 3) || !GET_OPPONENT(ch))))
	{
		act("$n &+Lquickly fades into the thin air!", TRUE, ch, 0, 0, TO_ROOM);
		act("$n &+rdisappears as &+Lquickly&+r as it came!", TRUE, ch, 0, 0, TO_ROOM);
		{
			extract_char(ch);
		}
		return TRUE;
	}

	// Let's junk it on anything if not fighting!
	if (!GET_OPPONENT(ch) || cmd != CMD_PERIODIC)
	{
		return FALSE;
	}

	if ((GET_OPPONENT(ch)->in_room == ch->in_room) && !number(0, (MAXLVL - GET_LEVEL(ch))))
	{
		/* This isn't used anywhere?
		int type;
		if (GET_LEVEL(ch) < 25)
		  type = 1;
		else if (GET_LEVEL(ch) < 50)
		  type = 3;
		else
		  type = 10;
		*/

		act("$n bites $N!", 1, ch, 0, GET_OPPONENT(ch), TO_NOTVICT);
		act("$n bites you!", 1, ch, 0, GET_OPPONENT(ch), TO_VICT);
		poison_neurotoxin(10, ch, 0, 0, GET_OPPONENT(ch), 0);
		return TRUE;
	}

	return FALSE;
}
int illus_titan(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd == CMD_DEATH ||
	    (cmd == CMD_PERIODIC && (!number(0, GET_LEVEL(ch) / 3) || !GET_OPPONENT(ch))))
	{
		act("$n &+Lquickly fades into the thin air!", TRUE, ch, 0, 0, TO_ROOM);
		act("$n &+rdisappears as &+Lquickly&+r as it came!", TRUE, ch, 0, 0, TO_ROOM);
		extract_char(ch);
		return TRUE;
	}

	return FALSE;
}
int living_stone(P_char ch, P_char /*pl*/, int /*cmd*/, char * /*arg*/)
{
	if (!ch)
		return FALSE;

	return FALSE;
}
int greater_living_stone(P_char ch, P_char /*pl*/, int /*cmd*/, char * /*arg*/)
{
	if (!ch)
		return FALSE;

	return FALSE;
}

int shadow_demon_of_torm(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || pl)
		return FALSE;

	if (cmd != -1)
	{
		if (affected_by_spell(ch, SPELL_SUMMON))
			return FALSE;
		act("Summoning magic dispersed, $n disappears into the shadows..", TRUE, ch, 0, 0,
		    TO_ROOM);
		act("Suddenly shadows seem to cover a lot more of the room than before..", TRUE, ch,
		    0, 0, TO_ROOM);
		spell_darkness(20, ch, 0, 0, 0, 0);
		extract_char(ch);
		ch = NULL;
		return TRUE;
	}
	act("As $n dies, it melts into the shadows of the room.", TRUE, ch, 0, 0, TO_ROOM);
	act("Suddenly shadows seem to cover a lot more of the room than before..", TRUE, 0, 0, 0,
	    TO_ROOM);
	spell_darkness(20, ch, 0, 0, 0, 0);
	return TRUE;
}

int billthecat(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd != 0) /*
	               * mobact.c
	               */
		return (FALSE);

	switch (number(0, 10))
	{
	case 0:
		mobsay(ch, "pffffpht!");
		return (TRUE);

	case 1:
		do_action(ch, 0, CMD_TRIP);
		return (TRUE);

	case 2:
		mobsay(ch, "ACK!");
		return (TRUE);

	case 3:
		do_action(ch, 0, CMD_BANG);
		return (TRUE);

	case 4:
		do_action(ch, 0, CMD_MOSH);
		return (TRUE);

	case 5:
		act("$n hocks up a furball.", TRUE, ch, 0, 0, TO_ROOM);
		return (TRUE);

	case 6:
		do_action(ch, 0, CMD_MOAN);
		return (TRUE);

	case 7:
		do_action(ch, 0, CMD_SLOBBER);
		return (TRUE);

	default:
		return (FALSE);
	}
}

int beavis(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	char buf[255];

	/* check for periodic event calls  */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd != 0) /* mobact.c  */
		return (FALSE);

	strcpy(buf, "butthead");

	switch (number(0, 40))
	{
	case 0:
		mobsay(ch, "Heh hehe that sucks dude!");
		return (TRUE);
	case 1:
		mobsay(ch, "Dude, this is like cool!");
		return (TRUE);
	case 2:
		mobsay(ch, "Heh, whoa dude that was cool!");
		return (TRUE);
	case 3:
		mobsay(ch, "I think this is cool or something.");
		return (TRUE);
	case 4:
		mobsay(ch, "Look at those chicks, huh huh huh");
		return (TRUE);
	case 5:
		mobsay(ch, "huh huh huh huh huh huh huh huh huh");
		return (TRUE);
	case 6:
		mobsay(ch, "We're there dude!");
		return (TRUE);
	case 7:
		mobsay(ch, "This video SUCKS!");
		return (TRUE);
	case 8:
		mobsay(ch, "Shutup asswipe!");
		return (TRUE);
	case 9:
		mobsay(ch, "Metallica kicks ass!");
		return (TRUE);
	case 10:
		mobsay(ch, "White Zombie RULES!");
		return (TRUE);
	case 11:
		mobsay(ch, "Mmmmm tastes like chicken.");
		return (TRUE);
	case 12:
		mobsay(ch, "Fire fire fire fire fire!");
		return (TRUE);
	case 13:
		mobsay(ch, "Shutup ButtHead, I'll kick your ass!");
		return (TRUE);
	case 15:
		do_action(ch, 0, CMD_BANG);
		return (TRUE);
	case 16:
		do_action(ch, 0, CMD_MOSH);
		return (TRUE);
	case 17:
		do_action(ch, buf, CMD_BANG);
		return (TRUE);
	case 18:
		mobsay(ch, "Change it or kill me, Butthead!");
		return (TRUE);
	case 19:
		mobsay(ch, "Isn't this new band, Schlong?");
		return (TRUE);
	case 20:
		mobsay(ch, "Nachos rule!  They rule!");
		return (TRUE);
	default:
		return (FALSE);
	}
}

int butthead(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	char buf[20];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd != 0) /*
	               * mobact.c
	               */
		return (FALSE);

	switch (number(0, 40))
	{
	case 0:
		mobsay(ch, "Heh hehe that sucks dude!");
		return (TRUE);
	case 1:
		mobsay(ch, "Dude, this is like cool!");
		return (TRUE);
	case 2:
		mobsay(ch, "Heh, whoa dude that was cool!");
		return (TRUE);
	case 3:
		mobsay(ch, "I think this is cool or something.");
		return (TRUE);
	case 4:
		mobsay(ch, "Look at those chicks, huh huh huh");
		return (TRUE);
	case 5:
		mobsay(ch, "huh huh huh huh huh huh huh huh huh");
		return (TRUE);
	case 6:
		mobsay(ch, "We're there dude!");
		return (TRUE);
	case 7:
		mobsay(ch, "This video SUCKS!");
		return (TRUE);
	case 8:
		mobsay(ch, "Shutup asswipe!");
		return (TRUE);
	case 9:
		mobsay(ch, "Metallica kicks ass!");
		return (TRUE);
	case 10:
		mobsay(ch, "White Zombie RULES!");
		return (TRUE);
	case 13:
		mobsay(ch, "Settle down Beavis.");
		return (TRUE);
	case 14:
		mobsay(ch, "I'll kick your ass!");
		return (TRUE);
	case 15:
		do_action(ch, 0, CMD_BANG);
		return (TRUE);
	case 16:
		do_action(ch, 0, CMD_MOSH);
		return (TRUE);
	case 17:
		strcpy(buf, "beavis");
		do_action(ch, buf, CMD_BANG);
		return (TRUE);
	case 18:
		mobsay(ch, "Shutup butt munch!");
		return (TRUE);
	case 19:
		mobsay(ch, "Shutup dillhole!");
		return (TRUE);
	case 20:
		mobsay(ch, "Don't bogart my log Beavis!");
		return (TRUE);
	case 21:
		mobsay(ch, "No, thats Prong");
		return (TRUE);
	case 22:
		mobsay(ch, "Hey Beavis, we're cool huh.");
		return (TRUE);
	case 23:
		mobsay(ch, "Nachos rule!  They rule!");
		return (TRUE);
	case 24:
		mobsay(ch, "Nudi... n u i d i s... heh nude people.");
		return (TRUE);

	default:
		return (FALSE);
	}
}

int imageproc(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd == CMD_DEATH)
		return TRUE;

	if (!ch || IS_FIGHTING(ch) || cmd)
		return FALSE;

	if (!affected_by_spell(ch, SPELL_CHARM_PERSON))
	{
		act("&+L$n&+L quickly fades into the thin air!", TRUE, ch, 0, 0, TO_ROOM);
		char_from_room(ch);
		extract_char(ch);
		return TRUE;
	}

	return FALSE;
}

int cow_talk(P_char ch, P_char tch, int cmd, char * /*arg*/)
{
	if (cmd == CMD_MOUNT)
	{
		if (GET_LEVEL(tch) < MINLVLIMMORTAL)
		{
			act("$n growls as you try to mount $m.  You rethink the idea.", FALSE, ch,
			    0, tch, TO_VICT);
			act("$n growls as $N tries to mount $m.  $N rethinks the idea.", TRUE, ch,
			    0, tch, TO_NOTVICT);
			return (TRUE);
		}
		return (FALSE);
	}

	/*
	 * check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || cmd)
		return FALSE;

	switch (number(0, 40))
	{
	case 0:
		mobsay(ch, "Moooooooooooooooooo");
		return TRUE;
	case 1:
		mobsay(ch, "MooooooooooooooooooOOOOOOOOOoooooooooooooOOOOOOOOOooooooooO!");
		return TRUE;
	case 2:
		do_action(ch, 0, CMD_MOON);
		mobsay(ch, "Hmm! That's not moo.");
		return TRUE;
	case 3:
		do_action(ch, 0, CMD_COW);
		do_action(ch, 0, CMD_COW);
		do_action(ch, 0, CMD_COW);
		mobsay(ch, "Muhahahaha");
		do_action(ch, 0, CMD_COW);
		return TRUE;
	case 4:
		act("$n looks at you.", TRUE, ch, 0, 0, TO_ROOM);
		act("$n sizes you up with a quick glance.", TRUE, ch, 0, 0, TO_ROOM);
		act("$n whispers to you 'Moo'", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	case 5:
		mobsay(ch, "I wasn't always a cow you know.");
		do_action(ch, 0, CMD_HICCUP);
		mobsay(ch, "I was a champion boxer, now look at me.");
		do_action(ch, 0, CMD_HICCUP);
		return TRUE;
	}
	return FALSE;
}

int annoying_mob(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char temp;
	static int songcounter = 0;

	/*
	 * check for periodic event calls
	 */

	if (cmd == CMD_DEATH)
	{
		temp = read_mobile(GET_RNUM(ch), REAL);
		if (temp)
		{
			char_to_room(temp, ch->in_room, 0);
		}
		act("An aura of intensely bright light surrounds &+Lan &+runk&+Rilla&+rble &+Lbastard&N for a moment.",
		    TRUE, ch, 0, 0, TO_ROOM);
		act("&+LAn &+runk&+Rilla&+rble &+Lbastard&N comes to life again! Taking a deep breath, &+Lan &+runk&+Rilla&+rble &+Lbastard&N opens its eyes!",
		    TRUE, ch, 0, 0, TO_ROOM);
	}

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || cmd)
		return FALSE;
	if (cmd == 0)
	{
		switch (songcounter)
		{
		case 0:
			act("$n sings 'This is the song that never ends...'", FALSE, ch, 0, 0,
			    TO_ROOM);
			songcounter++;
			break;
		case 1:
			act("$n sings 'Yes it goes on and on my friend...'", FALSE, ch, 0, 0,
			    TO_ROOM);
			songcounter++;
			break;
		case 2:
			act("$n sings 'Some people started singing it, not knowing what it was...'",
			    FALSE, ch, 0, 0, TO_ROOM);
			songcounter++;
			break;
		case 3:
			act("$n sings 'And they'll continue singing it forever just because...'",
			    FALSE, ch, 0, 0, TO_ROOM);
			songcounter = 0;
			break;
		}
	}
	return (FALSE);
}

int raoul(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	char buf[20];
	static int songcounter = 0;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || cmd)
		return FALSE;
	strcpy(buf, "christine");

	if (cmd == 0)
	{
		switch (songcounter)
		{
		case 0:
			do_action(ch, buf, CMD_CALM);
			break;
		case 1:
			do_action(ch, buf, CMD_STARE);
			break;
		case 2:
			act("$n sings 'No more talk of darkness, '", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 3:
			act("$n sings 'Forget these wide-eyed fears.'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 4:
			act("$n sings 'I'm here, '", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 5:
			act("$n sings 'nothing can harm you --'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 6:
			act("$n sings 'my words will warm and calm you.'", FALSE, ch, 0, 0,
			    TO_ROOM);
			break;
		case 7:
			act("$n sings 'Let me be your freedom, '", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 8:
			act("$n sings 'let daylight dry your tears.'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 9:
			act("$n sings 'I'm here, '", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 10:
			act("$n sings 'with you, beside you, '", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 11:
			act("$n sings 'to guard you and to guide you...'", FALSE, ch, 0, 0,
			    TO_ROOM);
			break;
		case 17:
			act("$n sings 'Let me be your shelter, '", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 18:
			act("$n sings 'let me be your light.'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 19:
			act("$n sings 'You're safe:'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 20:
			act("$n sings 'No-one will find you --'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 21:
			act("$n sings 'your fears are far behind you...'", FALSE, ch, 0, 0,
			    TO_ROOM);
			break;
		case 27:
			act("$n sings 'Then say you'll share with me one love, one lifetime...'",
			    FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 28:
			act("$n sings 'let me lead you from your solitude...'", FALSE, ch, 0, 0,
			    TO_ROOM);
			break;
		case 29:
			act("$n sings 'Say you need me with you here, beside you...'", FALSE, ch, 0,
			    0, TO_ROOM);
			break;
		case 30:
			act("$n sings 'anywhere you go, let me go too --'", FALSE, ch, 0, 0,
			    TO_ROOM);
			break;
		case 31:
			act("$n sings 'Christine, '", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 32:
			act("$n sings 'that's all I ask of you...'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 35:
			act("$n sings 'Share each day with me, each night, each morning...'", FALSE,
			    ch, 0, 0, TO_ROOM);
			break;
		case 37:
			act("$n sings 'You know I do...'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 38:
			act("$n sings 'Love me --'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 39:
			act("$n sings 'that's all I ask of you...'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 40:
			do_action(ch, buf, CMD_SMILE);
			break;
		case 41:
			act("$n sings 'Anywhere you go let me go too...'", FALSE, ch, 0, 0,
			    TO_ROOM);
			break;
		case 42:
			act("$n sings 'Love me --'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 43:
			act("$n sings 'that's all I ask of you...'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 45:
			do_action(ch, 0, CMD_BOW);
			break;
		default:
			break;
		}
		songcounter++;
		if (songcounter >= 50)
			songcounter = 0;
	}
	return (FALSE);
}

int christine(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	char buf[20];

	static int songcounter = 0;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || cmd)
		return FALSE;
	strcpy(buf, "raoul");
	if (cmd == 0)
	{
		switch (songcounter)
		{
		case 1:
			do_action(ch, buf, CMD_STARE);
			break;
		case 12:
			act("$n sings 'Say you love me every waking moment, '", FALSE, ch, 0, 0,
			    TO_ROOM);
			break;
		case 13:
			act("$n sings 'turn my head with talk of summertime...'", FALSE, ch, 0, 0,
			    TO_ROOM);
			break;
		case 14:
			act("$n sings 'Say you need me with you, now and always...'", FALSE, ch, 0,
			    0, TO_ROOM);
			break;
		case 15:
			act("$n sings 'promise me that all you say is true --'", FALSE, ch, 0, 0,
			    TO_ROOM);
			break;
		case 16:
			act("$n sings 'that's all I ask of you...'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 22:
			act("$n sings 'All I want is freedom, '", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 23:
			act("$n sings 'a world with no more night...'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 24:
			act("$n sings 'and you, '", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 25:
			act("$n sings 'always beside me, '", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 26:
			act("$n sings 'to hold me and to hide me...'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 33:
			act("$n sings 'Say you'll share with me one love, one lifetime...'", FALSE,
			    ch, 0, 0, TO_ROOM);
			break;
		case 34:
			act("$n sings 'say the word and I will follow you...'", FALSE, ch, 0, 0,
			    TO_ROOM);
			break;
		case 35:
			act("$n sings 'Share each day with me, each night, each morning...'", FALSE,
			    ch, 0, 0, TO_ROOM);
			break;
		case 36:
			act("$n sings 'Say you love me...'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 38:
			act("$n sings 'Love me --'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 39:
			act("$n sings 'that's all I ask of you...'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 40:
			do_action(ch, buf, CMD_SMILE);
			break;
		case 41:
			act("$n sings 'Anywhere you go let me go too...'", FALSE, ch, 0, 0,
			    TO_ROOM);
			break;
		case 42:
			act("$n sings 'Love me --'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 43:
			act("$n sings 'that's all I ask of you...'", FALSE, ch, 0, 0, TO_ROOM);
			break;
		case 45:
			do_action(ch, 0, CMD_CURTSEY);
			break;
		default:
			break;
		}
		songcounter++;
		if (songcounter >= 50)
			songcounter = 0;
	}
	return (FALSE);
}

int cookie_monster(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	static int songcounter = 0;

	/*
	 * check for periodic event calls
	 */

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || cmd)
		return FALSE;
	if (cmd == 0)
	{
		switch (songcounter)
		{
		case 0:
			act("$n sings 'C is for cookie, that's good enough for me'", FALSE, ch, 0,
			    0, TO_ROOM);
			songcounter++;
			break;
		case 1:
			act("$n sings 'C is for cookie, that's good enough for me'", FALSE, ch, 0,
			    0, TO_ROOM);
			songcounter++;
			break;
		case 2:
			act("$n sings 'C is for cookie, that's good enough for me'", FALSE, ch, 0,
			    0, TO_ROOM);
			songcounter++;
			break;
		case 3:
			act("$n sings 'C is for cookie, that's good enough for me'", FALSE, ch, 0,
			    0, TO_ROOM);
			songcounter++;
			break;
		case 4:
			act("$n sings 'C is for cookie, that's good enough for me'", FALSE, ch, 0,
			    0, TO_ROOM);
			songcounter++;
			break;
		case 5:
			act("$n sings 'Oh, cookie, cookie, cookie starts with C", FALSE, ch, 0, 0,
			    TO_ROOM);
			songcounter = 0;
			break;
		}
	}
	return (FALSE);
}

/* stat pools, level 51+, drink mods stats by -3 to +5 (up to 100)
   (once a week), sip mods stats by -1 to +2 (up to 100..), twice a
   week (RL) */

#define STAT_POOL_SIP_MIN 1
#define STAT_POOL_SIP_MAX 3

#define STAT_POOL_DRINK_MIN 1
#define STAT_POOL_DRINK_MAX 3

static int stat_pool_common(P_obj obj, P_char ch, int cmd, sh_int *statPtr, const char *minusMsgCh,
			    const char *minusMsgRoom, const char *plusMsgCh,
			    const char *plusMsgRoom)
{
	int numb, oldStat;
	struct affected_type *af2;

	if (!ch || !IS_PC(ch))
		return FALSE;

	switch (cmd)
	{
	/*case CMD_SIP:
		  if (GET_LEVEL(ch) < 51 || affected_by_spell(ch, TAG_POOL) )
		  {
			send_to_char("The liquid burns your throat!  Ouch!\n", ch);
			act("$n reels in pain as $e takes a sip from $p!", FALSE,
			    ch, obj, 0, TO_ROOM);

			damage(ch, ch, TYPE_UNDEFINED, 25);

			return TRUE;
		  }

		  numb = number(STAT_POOL_SIP_MIN, STAT_POOL_SIP_MAX);
		  break;
		*/
	case CMD_DRINK:
		if (GET_LEVEL(ch) < 51)
		{
			send_to_char("You are much to lowly to even dream of drinking from that!\n",
				     ch);
			return TRUE;
		}

		if ((af2 = get_spell_from_char(ch, TAG_POOL)) != NULL)
		{
			if ((af2->modifier + (60 * 60 * 24 * 2)) > time(NULL))
			{
				send_to_char("The liquid burns your throat!  Ouch!\n", ch);
				act("$n reels in pain as $e takes a drink from $p!", FALSE, ch, obj,
				    0, TO_ROOM);

				damage(ch, ch, TYPE_UNDEFINED, 50);

				return TRUE;
			}
		}

		numb = number(STAT_POOL_DRINK_MIN, STAT_POOL_DRINK_MAX);
		break;

	default:
		return FALSE;
	}

	affect_from_char(ch, TAG_POOL);

	struct affected_type af;
	bzero(&af, sizeof(af));
	af.type = TAG_POOL;
	af.flags = AFFTYPE_STORE | AFFTYPE_PERM;
	// af.duration = (int) (get_property("timer.mins.statPool", SECS_PER_REAL_HOUR * 24));
	af.duration = -1;
	af.modifier = time(NULL);
	affect_to_char(ch, &af);

	act("$n takes a drink from $p.", FALSE, ch, obj, 0, TO_ROOM);
	send_to_char("You take a drink...\n", ch);

	// heal em!

	if (GET_HIT(ch) < GET_MAX_HIT(ch))
	{
		send_to_char("The cool waters of the pool heal your wounds!\n", ch);
		act("$n's wounds appear to have been healed!", FALSE, ch, 0, 0, TO_ROOM);

		GET_HIT(ch) = GET_MAX_HIT(ch);
	}

	// modify stat!

	oldStat = *statPtr;
	*statPtr = BOUNDED(1, (*statPtr) + numb, 100);
	numb = (*statPtr) - oldStat;

	if (numb == 0)
	{
		send_to_char("Strange..  You don't feel any different..\n", ch);
	}
	else if (numb < 0)
	{
		send_to_char(minusMsgCh, ch);
		act(minusMsgRoom, FALSE, ch, 0, 0, TO_ROOM);

		affect_total(ch, TRUE);
	}
	else
	{
		send_to_char(plusMsgCh, ch);
		act(plusMsgRoom, FALSE, ch, 0, 0, TO_ROOM);

		affect_total(ch, TRUE);
	}

	return TRUE;
}

int spell_pool(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	int curr_time, rannum;
	typedef void (*spell_func_ptr)(int, P_char, char *, int, P_char, P_obj);
	spell_func_ptr spells[9] = { spell_lifelust,
				     spell_fly,
				     spell_lionrage,
				     spell_hawkvision,
				     spell_armor,
				     spell_detect_invisibility,
				     spell_biofeedback,
				     spell_greater_spirit_ward,
				     spell_baladors_protection };

	void (*spell_func)(int, P_char, char *, int, P_char, P_obj);

	rannum = number(0, 8);

	if (cmd == CMD_SET_PERIODIC)
	{
		curr_time = time(NULL);

		obj->value[0] = rannum;
		obj->timer[0] = curr_time;

		return TRUE;
	}

	spell_func = spells[obj->value[0]];

	curr_time = time(NULL);

	if (obj->timer[0] + (60 * 40) <= curr_time)
	{
		spell_func = spells[rannum];

		obj->value[0] = rannum;
		obj->timer[0] = curr_time;
	}

	if (cmd != CMD_DRINK)
		return FALSE;

	spell_func(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);

	return TRUE;
}

int stat_pool_str(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	return stat_pool_common(obj, ch, cmd, &(ch->base_stats.Str), "&+LYou feel weaker.\n",
				"$n's body momentarily sags, as though overburdened.",
				"&+WYou feel stronger!\n",
				"$n's muscles seem to grow for a brief instant.");
}

int stat_pool_dex(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	return stat_pool_common(
		obj, ch, cmd, &(ch->base_stats.Dex), "&+LYou feel less dextrous.\n",
		"You somehow get the sense that $n won't be so good at piano-playing anymore.",
		"&+WYou feel more dextrous!\n",
		"$n nimbly moves $s fingers with newfound dexterity.");
}

int stat_pool_agi(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	return stat_pool_common(
		obj, ch, cmd, &(ch->base_stats.Agi), "&+LYou feel less agile.\n",
		"$n takes on a more clumsy appearance than before.", "&+WYou feel more agile!\n",
		"$n's body suddenly appears more flexible than ever - $e does 23 backflips in a row!");
}

int stat_pool_con(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	return stat_pool_common(obj, ch, cmd, &(ch->base_stats.Con),
				"&+LYou suddenly feel less healthy.\n",
				"$n's countenance takes on a more unhealthy appearance.",
				"&+WYou feel ten years younger!\n",
				"$n's countenance takes on a more healthy appearance.");
}

int stat_pool_pow(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	return stat_pool_common(
		obj, ch, cmd, &(ch->base_stats.Pow),
		"&+LYou suddenly feel less able to use the power of your mind.\n",
		"$n sags noticably under the weight of the world.",
		"&+WYour mind suddenly feels ten times as powerful!\n",
		"$n's eyes take on a glimmering sheen as $e looks up from the pool.");
}

int stat_pool_int(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	return stat_pool_common(
		obj, ch, cmd, &(ch->base_stats.Int),
		"&+LYou suddenly feel stupider...  You think.\n",
		"$n suddenly looks utterly lost and confused.",
		"&+WYou feel smarter!  Man, you were a real dumbass before.\n",
		"$n smiles and recites a poem in a language you do not even recognize!");
}

int stat_pool_wis(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	return stat_pool_common(
		obj, ch, cmd, &(ch->base_stats.Wis),
		"&+LYou feel as though some of your hard-won knowledge has slipped away..\n",
		"$n has a momentary look of utter confusion.", "&+WYou feel wiser!\n",
		"$n begins to admonish younger people around $m.");
}

int stat_pool_cha(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	return stat_pool_common(obj, ch, cmd, &(ch->base_stats.Cha),
				"&+LYou don't feel any different, really..\n",
				"You thought $n looked ugly before, but now $e looks even uglier.",
				"&+WYou don't feel any different, really..\n",
				"Wow, you never noticed before, but $n is kinda sexy.");
}

int stat_pool_luc(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	return stat_pool_common(obj, ch, cmd, &(ch->base_stats.Luk),
				"&+LYour outlook on life is somewaht grimmer..\n",
				"$n doesn't look so confident in believing in his lucky stars.\n",
				"&+WYou feel as if you could roll Triple Tiamat's at the slots..\n",
				"$n looks to have the confidence that life is going his way.\n");
}

int druid_spring(P_obj obj, P_char ch, int cmd, char *arg)
{
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd == CMD_DRINK)
	{
		if (!arg || !*arg)
			return FALSE;
		arg = skip_spaces(arg);
		if (*arg && !strcmp(arg, "spring"))
		{
			act("You drink from $p.", FALSE, ch, obj, 0, TO_CHAR);
			act("$n drinks from $p.", FALSE, ch, obj, 0, TO_ROOM);

			if (obj->value[0] >= 51)
				spell_regeneration(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
			//      if (obj->value[0] >= 41)
			//        spell_endurance(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
			if (obj->value[0] >= 36)
				spell_aid(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);

			if (obj->value[0] >= 31)
			{
				spell_natures_touch(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
			}
			else
			{
				spell_cure_serious(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
			}
			spell_invigorate(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
			CharWait(ch, PULSE_VIOLENCE);
			return TRUE;
		}
	}

	if (cmd == CMD_DECAY)
	{
		if (world[obj->loc.room].people)
		{
			act("$p rapidly shrinks in size until finally it disappears entirely.", 0,
			    world[obj->loc.room].people, obj, 0, TO_ROOM);
			act("$p rapidly shrinks in size until finally it disappears entirely.", 0,
			    world[obj->loc.room].people, obj, 0, TO_CHAR);
		}
		return TRUE;
	}

	return FALSE;
}

int divine_font(P_obj obj, P_char ch, int cmd, char *arg)
{
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd == CMD_DRINK)
	{
		if (!arg || !*arg)
			return FALSE;
		arg = skip_spaces(arg);
		if (*arg && (!strcmp(arg, "font") || !strcmp(arg, "divine")))
		{
			act("You drink from $p.", FALSE, ch, obj, 0, TO_CHAR);
			act("$n drinks from $p.", FALSE, ch, obj, 0, TO_ROOM);

			spell_full_heal(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
			spell_vitality(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
			spell_invigorate(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);

			CharWait(ch, PULSE_VIOLENCE);
			return TRUE;
		}
	}

	if (cmd == CMD_DECAY)
	{
		if (world[obj->loc.room].people)
		{
			act("$p rapidly shrinks in size until finally it disappears entirely.", 0,
			    world[obj->loc.room].people, obj, 0, TO_ROOM);
			act("$p rapidly shrinks in size until finally it disappears entirely.", 0,
			    world[obj->loc.room].people, obj, 0, TO_CHAR);
		}
		return TRUE;
	}

	return FALSE;
}

int blighter_pond(P_obj obj, P_char ch, int cmd, char *arg)
{
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd == CMD_DRINK)
	{
		if (!arg || !*arg)
			return FALSE;
		arg = skip_spaces(arg);
		if (*arg && !strcmp(arg, "pond"))
		{
			if (obj->value[0] < 31)
			{
				CharWait(ch, WAIT_SEC);
				return FALSE;
			}

			act("You drink from $p.", FALSE, ch, obj, 0, TO_CHAR);
			act("$n drinks from $p.", FALSE, ch, obj, 0, TO_ROOM);

			if (obj->value[0] >= 31)
			{
				spell_drain_nature(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
			}
			//      if (obj->value[0] >= 41)
			//        spell_sap_nature(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
			if (obj->value[0] >= 51)
			{
				spell_regeneration(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
			}
			if (GET_VITALITY(ch) < GET_MAX_VITALITY(ch))
			{
				spell_invigorate(obj->value[0], ch, 0, SPELL_TYPE_SPELL, ch, 0);
			}
			CharWait(ch, PULSE_VIOLENCE);
			return TRUE;
		}
	}

	if (cmd == CMD_DECAY)
	{
		if (world[obj->loc.room].people)
		{
			act("$p rapidly shrinks in size until finally it disappears entirely.", 0,
			    world[obj->loc.room].people, obj, 0, TO_ROOM);
			act("$p rapidly shrinks in size until finally it disappears entirely.", 0,
			    world[obj->loc.room].people, obj, 0, TO_CHAR);
		}
		return TRUE;
	}

	return FALSE;
}

int refreshing_fountain(P_obj obj, P_char ch, int cmd, char *arg)
{
	struct affected_type af;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd == CMD_DRINK)
	{
		if (!arg || !*arg)
			return FALSE;
		arg = skip_spaces(arg);
		if (*arg && !strcmp(arg, "spring"))
		{
			act("You drink from $p.", FALSE, ch, obj, 0, TO_CHAR);
			act("$n drinks from $p.", FALSE, ch, obj, 0, TO_ROOM);
			if (!affected_by_spell(ch, SPELL_REFRESHING_FOUNTAIN))
			{
				act("$p&+W's &+Cwater&+W touches your soul!&n", TRUE, ch, obj, 0,
				    TO_CHAR);
				bzero(&af, sizeof(af));
				af.type = SPELL_REFRESHING_FOUNTAIN;
				af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW;
				af.duration = 1 * PULSE_VIOLENCE;
				af.location = APPLY_HIT_REG;
				af.modifier = 1000;
				affect_to_char(ch, &af);
				af.location = APPLY_MOVE_REG;
				af.modifier = 40;
				affect_to_char(ch, &af);
			}
			else
				act("$p&+W's &+Cwater&+W touches your soul!&n", TRUE, ch, obj, 0,
				    TO_CHAR);

			CharWait(ch, PULSE_VIOLENCE);
			return TRUE;
		}
	}

	return FALSE;
}

int magical_fountain(P_obj /*obj*/, P_char /*ch*/, int /*cmd*/, char * /*arg*/)
{
	return FALSE;
}

static int random_gc_room()
{
	int rroom;

	// Pick a random map room
	while ((rroom = real_room0(number(SURFACE_MAP_START, SURFACE_MAP_END))))
	{
		// If it's on gc and not on mountains, return it.
		if (IS_CONTINENT(rroom, CONT_GC) && world[rroom].sector_type != SECT_MOUNTAIN)
		{
			return rroom;
		}
	}
	return 0;
}

int dragoon_blade(P_obj /*obj*/, P_char /*ch*/, int /*cmd*/, char * /*argument*/)
{
	return 0;
}

int dragoon_lance(P_obj /*obj*/, P_char /*ch*/, int /*cmd*/, char * /*argument*/)
{
	return 0;
}

int dragoon_totem(P_obj /*obj*/, P_char /*ch*/, int /*cmd*/, char * /*argument*/)
{
	return 0;
}

int gc_portal(P_obj obj, P_char ch, int cmd, char *argument)
{
	char buf[MAX_INPUT_LENGTH];

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!IS_ALIVE(ch))
		return FALSE;

	if (cmd != CMD_ENTER)
		return FALSE;

	one_argument(argument, buf);
	// If not the right portal..
	if (obj != get_obj_in_list(buf, world[ch->in_room].contents))
	{
		return FALSE;
	}

	if ((GET_LEVEL(ch) < 10 || GET_LEVEL(ch) > 35) && !IS_TRUSTED(ch))
	{
		act("&+LA strong force pushes you away from $p&+L.", FALSE, ch, obj, 0, TO_CHAR);
		return TRUE;
	}

	// Add ch enters portal message here.
	act("&+L$n &+Lenters $p &+Land quickly fades to nothing!", FALSE, ch, obj, 0, TO_ROOM);
	act("&+LAs you enter $p&+L, you feel your body begin to be torn apart!", FALSE, ch, obj, 0,
	    TO_CHAR);

	// Move ch to random room on gc.
	char_from_room(ch);
	char_to_room(ch, random_gc_room(), -2);

	// Destroy eq here.

	// Add ch appears here.
	// Add message to ch here.
	act("&+LSuddenly, you feel your body reform...", FALSE, ch, obj, 0, TO_CHAR);
	act("&+LSuddenly, $n &+Lforms out of nothing...", FALSE, ch, obj, 0, TO_ROOM);
	do_look(ch, NULL, -4);

	return TRUE;
}

static int random_ec_room()
{
	int rroom;

	// Pick a random map room
	while ((rroom = real_room0(number(SURFACE_MAP_START, SURFACE_MAP_END))))
	{
		// If it's on gc and not on mountains, return it.
		if (IS_CONTINENT(rroom, CONT_EC) && world[rroom].sector_type != SECT_MOUNTAIN)
		{
			return rroom;
		}
	}
	return 0;
}

static int random_ud_room()
{
	int rroom;

	// Pick a random map room
	while (TRUE)
	{
		if ((rroom = real_room0(number(VROOM_UD_PORTAL_START, VROOM_UD_PORTAL_END))) == 0)
		{
			continue;
		}
		// If it's on gc and not on mountains, return it.
		if ((rroom > 0) && (world[rroom].sector_type != SECT_UNDRWLD_MOUNTAIN) &&
		    (world[rroom].sector_type != SECT_OCEAN))
		{
			return rroom;
		}
	}
}

int ec_portal(P_obj obj, P_char ch, int cmd, char *argument)
{
	char buf[MAX_INPUT_LENGTH];

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!IS_ALIVE(ch))
		return FALSE;

	if (cmd != CMD_ENTER)
		return FALSE;

	one_argument(argument, buf);
	// If not the right portal..
	if (obj != get_obj_in_list(buf, world[ch->in_room].contents))
	{
		return FALSE;
	}

	if ((GET_LEVEL(ch) < 10 || GET_LEVEL(ch) > 35) && !IS_TRUSTED(ch))
	{
		act("&+LA strong force pushes you away from $p&+L.", FALSE, ch, obj, 0, TO_CHAR);
		return TRUE;
	}

	// Add ch enters portal message here.
	act("&+L$n &+Lenters $p &+Land quickly fades to nothing!", FALSE, ch, obj, 0, TO_ROOM);
	act("&+LAs you enter $p&+L, you feel your body begin to be torn apart!", FALSE, ch, obj, 0,
	    TO_CHAR);

	// Move ch to random room on gc.
	char_from_room(ch);
	char_to_room(ch, random_ec_room(), -2);

	// Destroy eq here.

	// Add ch appears here.
	// Add message to ch here.
	act("&+LSuddenly, you feel your body reform...", FALSE, ch, obj, 0, TO_CHAR);
	act("&+LSuddenly, $n &+Lforms out of nothing...", FALSE, ch, obj, 0, TO_ROOM);
	do_look(ch, NULL, -4);

	return TRUE;
}

int ud_portal(P_obj obj, P_char ch, int cmd, char *argument)
{
	char buf[MAX_INPUT_LENGTH];

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!IS_ALIVE(ch))
		return FALSE;

	if (cmd != CMD_ENTER)
		return FALSE;

	one_argument(argument, buf);
	// If not the right portal..
	if (obj != get_obj_in_list(buf, world[ch->in_room].contents))
	{
		return FALSE;
	}

	if ((GET_LEVEL(ch) < 10 || GET_LEVEL(ch) > 35) && !IS_TRUSTED(ch))
	{
		act("&+LA strong force pushes you away from $p&+L.", FALSE, ch, obj, 0, TO_CHAR);
		return TRUE;
	}

	// Add ch enters portal message here.
	act("&+L$n &+Lenters $p &+Land quickly fades to nothing!", FALSE, ch, obj, 0, TO_ROOM);
	act("&+LAs you enter $p&+L, you feel your body begin to be torn apart!", FALSE, ch, obj, 0,
	    TO_CHAR);

	// Move ch to random room on gc.
	char_from_room(ch);
	char_to_room(ch, random_ud_room(), -2);

	// Destroy eq here.

	// Add ch appears here.
	// Add message to ch here.
	act("&+LSuddenly, you feel your body reform...", FALSE, ch, obj, 0, TO_CHAR);
	act("&+LSuddenly, $n &+Lforms out of nothing...", FALSE, ch, obj, 0, TO_ROOM);
	do_look(ch, NULL, -4);

	return TRUE;
}

int uc_nexus_portal(P_obj obj, P_char ch, int cmd, char *argument)
{
	char buf[MAX_INPUT_LENGTH];

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!IS_ALIVE(ch))
		return FALSE;

	if (cmd != CMD_ENTER)
		return FALSE;

	one_argument(argument, buf);
	// If not the right portal..
	if (obj != get_obj_in_list(buf, world[ch->in_room].contents))
	{
		return FALSE;
	}

	if ((GET_LEVEL(ch) < 10 || GET_LEVEL(ch) > 30) && !IS_TRUSTED(ch))
	{
		act("&+LA strong force pushes you away from $p&+L.", FALSE, ch, obj, 0, TO_CHAR);
		return TRUE;
	}

	// Add ch enters portal message here.
	act("&+L$n &+Lenters $p &+Land quickly fades to nothing!", FALSE, ch, obj, 0, TO_ROOM);
	act("&+LAs you enter $p&+L, you feel your body begin to be torn apart!", FALSE, ch, obj, 0,
	    TO_CHAR);

	// Move ch to random room on gc.
	char_from_room(ch);
	char_to_room(ch, real_room(130200), -2);

	// Destroy eq here.

	// Add ch appears here.
	// Add message to ch here.
	act("&+LSuddenly, you feel your body reform...", FALSE, ch, obj, 0, TO_CHAR);
	act("&+LSuddenly, $n &+Lforms out of nothing...", FALSE, ch, obj, 0, TO_ROOM);
	do_look(ch, NULL, -4);

	return TRUE;
}

int xmas_cap(P_obj obj, P_char ch, int cmd, char *arg)
{
	struct proc_data *data;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	// 1/20 chance.
	if (cmd != CMD_GOTHIT || number(0, 19))
	{
		return FALSE;
	}

	// important! can do this cast (next line) ONLY if cmd was CMD_GOTHIT or CMD_GOTNUKED
	if (!(data = legacy_proc_arg<struct proc_data *>(arg)))
	{
		return FALSE;
	}
	victim = data->victim;
	if (!IS_ALIVE(victim))
	{
		return FALSE;
	}

	/*  if(curr_time > ( 1135362289 + 60 * 60* 24 * 7))
	   {
	    act("&+L$p &+Lhums with a &+GCRA&+YC&+GKED &+Lsounds.&n", FALSE, ch, obj, 0, TO_CHAR);
	    act("&+L$p &+Lcrumble to dust.&n", FALSE, ch, obj, 0, TO_CHAR);
	    extract_obj(obj, TRUE); // Not an arti, but 'in game.'
	    return FALSE;
	  } */

	if (IS_PC(ch) && obj->value[6] != GET_PID(ch))
	{
		send_to_char("Stealing Presents is a BAD idea!\r\n", ch);
		act("&+L$p &+Lhums with a &+GCRA&+YC&+GKED &+Lsounds.&n", FALSE, ch, obj, 0,
		    TO_CHAR);
		act("&+L$p &+Lcrumble to dust.&n", FALSE, ch, obj, 0, TO_CHAR);
		spell_ice_missile(30, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
		extract_obj(obj, TRUE); // Not an arti, but 'in game.'
		return FALSE;
	}
	if (IS_PC(victim))
	{
		return FALSE;
	}

	act("&+W$n's&N $q &+Wspins in a fury&n, bringing the &+Rpower&n of &+WSanta!&N", TRUE, ch,
	    obj, victim, TO_NOTVICT);
	act("&+WYour&N $q &+Wspins in a fury&n, bringing the &+Rpower&n of &+WSanta!&N", TRUE, ch,
	    obj, victim, TO_CHAR);
	act("&+W$n's&N $q &+Wspins in a fury&n, bringing the &+Rpower&n of &+WSanta!&N", TRUE, ch,
	    obj, victim, TO_VICT);

	if (GET_LEVEL(ch) < 10)
	{
		spell_ice_missile(30, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
		return TRUE;
	}
	if (GET_LEVEL(ch) < 25)
	{
		spell_chill_touch(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, victim, 0);
		return TRUE;
	}
	if (GET_LEVEL(ch) < 38)
	{
		spell_ice_storm(GET_LEVEL(ch), ch, NULL, 0, victim, 0);
		return TRUE;
	}
	if (GET_LEVEL(ch) < 45)
	{
		spell_frostbite(GET_LEVEL(ch), ch, 0, SPELL_TYPE_SPELL, victim, 0);
		;
		return TRUE;
	}
	if (number(0, 1))
	{
		spell_arieks_shattering_iceball(30, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
	}
	else
	{
		spell_cone_of_cold(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, victim, 0);
	}
	return TRUE;
}

int tripboots(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	P_char vict = NULL;
	int rand;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WEAR_FEET) || cmd != CMD_PERIODIC ||
	    ch != obj->loc.wearing)
	{
		return FALSE;
	}

	// 1/10 chance.
	if (IS_FIGHTING(ch) && !number(0, 9))
	{
		rand = number(1, 10);
		// 70% chance.
		if (rand < 8)
		{
			vict = GET_OPPONENT(ch);
			act("&N$n sweep sends you crashing to the ground!&N", TRUE, ch, obj, vict,
			    TO_VICT);
			act("&N$n sweep sends $N crashing to the ground!!&N", TRUE, ch, obj, vict,
			    TO_NOTVICT);
			act("&NYour sweep sends $N crashing to the ground!&N", TRUE, ch, obj, vict,
			    TO_CHAR);
			SET_POS(vict, POS_SITTING + GET_STAT(vict));
			CharWait(vict, PULSE_VIOLENCE * 2);
			return TRUE;
		}
		else
		{
			vict = GET_OPPONENT(ch);
			act("&N$n falls like a drunk turkey to the ground!&N", TRUE, ch, obj, vict,
			    TO_VICT);
			act("&N$n falls like a drunk turkey to the ground!&N", TRUE, ch, obj, vict,
			    TO_NOTVICT);
			act("&NIn your haste to sweep $N off the ground, you fall to the ground in embarrassment! &N",
			    TRUE, ch, obj, vict, TO_CHAR);
			SET_POS(ch, POS_SITTING + GET_STAT(ch));
			CharWait(ch, PULSE_VIOLENCE * 2);
			return (TRUE);
		}
	}
	return (FALSE);
}

int blindbadge(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	P_char vict = NULL;
	struct affected_type af;
	int rand;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!OBJ_WORN_POS(obj, GUILD_INSIGNIA) || !IS_ALIVE(ch) || ch != obj->loc.wearing ||
	    cmd != CMD_PERIODIC)
	{
		return FALSE;
	}

	// 1/6 chance.
	if (IS_FIGHTING(ch) && !number(0, 5))
	{
		vict = GET_OPPONENT(ch);
		if (!IS_ALIVE(vict))
		{
			return FALSE;
		}
		rand = number(1, 10);
		// 70% chance.
		if (rand < 8)
		{
			act("&N$n's $q &+ysends out a stream of &+Ylight&+y towards you!&N", TRUE,
			    ch, obj, vict, TO_VICT);
			act("&N$n's $q &+ysends out a stream of &+Ylight&+y towards $N!", TRUE, ch,
			    obj, vict, TO_NOTVICT);
			act("&NYour $q &+ysends out a stream of &+Ylight&+y towards $N!&N", TRUE,
			    ch, obj, vict, TO_CHAR);
			send_to_char("&+LYou have been blinded!\n", vict);
			act("&+L$N seems to been blinded!.&N", TRUE, ch, obj, vict, TO_NOTVICT);
			act("&+L$N seems to been blinded!.&N", TRUE, ch, obj, vict, TO_CHAR);
			bzero(&af, sizeof(af));
			af.type = SPELL_BLINDNESS;
			af.bitvector = AFF_BLIND;
			af.duration = 0;
			affect_to_char(vict, &af);
			return TRUE;
		}
		else
		{
			vict = GET_OPPONENT(ch);
			act("&+y$n &+m's $q &+yfalls down over his eyes!&n", TRUE, ch, obj, vict,
			    TO_VICT);
			act("&+y$n &+m's $q &+yfalls down over his eyes!&n", TRUE, ch, obj, vict,
			    TO_NOTVICT);
			act("&+yYour $q &+yfalls down over your eyes!&n", TRUE, ch, obj, vict,
			    TO_CHAR);
			send_to_char("&+LYou have been blinded!\n", ch);
			act("&+L$N seems to been blinded!.&N", TRUE, vict, obj, ch, TO_NOTVICT);
			act("&+L$N seems to been blinded!.&N", TRUE, vict, obj, ch, TO_CHAR);
			bzero(&af, sizeof(af));
			af.type = SPELL_BLINDNESS;
			af.bitvector = AFF_BLIND;
			af.duration = 0;
			affect_to_char(ch, &af);
			return TRUE;
		}
	}

	return FALSE;
}

int confusionsword(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char victim;
	int rand;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch))
	{
		return FALSE;
	}
	victim = legacy_proc_arg<P_char>(arg);
	// 1/30 chance.
	if (!IS_ALIVE(victim) || number(0, 29))
	{
		return FALSE;
	}

	rand = number(1, 10);
	// 70% chance.
	if (rand < 8)
	{
		act("&n$n's&N $q &n&+Mcreates a strange sound...&N", TRUE, ch, obj, victim,
		    TO_NOTVICT);
		act("&nYour&N $q &n&+Mcreates a strange sound...&N", TRUE, ch, obj, victim,
		    TO_CHAR);
		act("&n$n's&N $q &n&+Mcreates a strange sound... &N", TRUE, ch, obj, victim,
		    TO_VICT);
		spell_inflict_pain(40, ch, 0, 0, victim, obj);
	}
	else
	{
		act("&n$n's&N $q &n&+Mcreates a &+YCRACKED&+M sound...&N", TRUE, ch, obj, victim,
		    TO_NOTVICT);
		act("&nYour&N $q &n&+Mcreates a &+YCRACKED&+M sound...&N", TRUE, ch, obj, victim,
		    TO_CHAR);
		act("&n$n's&N $q &n&+Mcreates a &+YCRACKED&+M sound... &N", TRUE, ch, obj, victim,
		    TO_VICT);
		spell_ego_blast(30, ch, 0, 0, ch, obj);
	}
	return TRUE;
}

int fumblegaunts(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	P_char vict = NULL;
	int rand;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!OBJ_WORN_POS(obj, WEAR_HANDS) || !char_in_list(ch) || !IS_ALIVE(ch) ||
	    ch != obj->loc.wearing || cmd != CMD_PERIODIC)
	{
		return FALSE;
	}

	if (GET_CLASS(ch, CLASS_MONK))
	{
		return FALSE;
	}

	// 1/15 chance.
	if (IS_FIGHTING(ch) && !number(0, 14))
	{
		vict = GET_OPPONENT(ch);
		if (!vict || !char_in_list(vict) || !IS_ALIVE(vict))
		{
			return FALSE;
		}
		rand = number(1, 10);
		if (rand < 8)
		{
			act("&n$n's&N $q &+Rflares red!&N", TRUE, ch, obj, vict, TO_NOTVICT);
			act("&nYour&N $q &+Rflares red!&N", TRUE, ch, obj, vict, TO_CHAR);
			act("&n$n's&N $q &+Rflares red!&N", TRUE, ch, obj, vict, TO_VICT);
#ifndef REALTIME_COMBAT
			act("&+LYour $q blurs as it strikes&N $N.", FALSE, ch, obj, vict, TO_CHAR);
			act("&+L$n's $q blurs as it strikes you.", FALSE, ch, obj, vict, TO_VICT);
			act("&+L$n's $q blurs as it strikes&N $N.", FALSE, ch, obj, vict,
			    TO_NOTVICT);
#endif
			for (int strike = 0; strike < 5; ++strike)
			{
				if (!char_in_list(ch) || !IS_ALIVE(ch))
					return FALSE;

				vict = GET_OPPONENT(ch);
				if (!vict || !char_in_list(vict) || !IS_ALIVE(vict))
					break;

				const attack_continuation continuation =
					begin_attack_continuation(ch, vict);
				hit(ch, vict, ch->equipment[PRIMARY_WEAPON]);

				const attack_continuation_result after_hit =
					check_attack_continuation(continuation);
				if (!after_hit.can_continue())
					return FALSE;

				ch = after_hit.actor;
				vict = after_hit.target;
			}
		}
		else
		{
			act("&n$n's&N $q &+Yflares yellow!&N", TRUE, ch, obj, vict, TO_NOTVICT);
			act("&nYour&N $q &+Yflares yellow!&N", TRUE, ch, obj, vict, TO_CHAR);
			act("&n$n's&N $q &+Yflares yellow!&N", TRUE, ch, obj, vict, TO_VICT);

			if (ch->equipment[WIELD] && (GET_LEVEL(ch) > 1) &&
			    !IS_SET(ch->equipment[WIELD]->extra_flags, ITEM_NODROP) &&
			    (ch->equipment[WIELD]->type == ITEM_WEAPON))
			{
				forced_weapon_drop(ch, ch->equipment[WIELD],
						   forced_weapon_drop_cause::combat_fumble);
			}
			else
				send_to_char("You stumble, but recover in time!\n", ch);
		}
	}
	return FALSE;
}

int fun_dagger(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch))
	{
		return FALSE;
	}

	if (!(victim = legacy_proc_arg<P_char>(arg)) || number(0, 1))
	{
		return FALSE;
	}

	act("&+w$n's&N $q &N strikes out in a &+rfre&+Rnzy&N of &+cs&+Cp&+We&+Ce&+cd&N", TRUE, ch,
	    obj, victim, TO_NOTVICT);
	act("&+wYour&N $q &N strikes out in a &+rfre&+Rnzy&N of &+cs&+Cp&+We&+Ce&+cd&N", TRUE, ch,
	    obj, victim, TO_CHAR);
	act("&+w$n's&N $q &N strikes out in a &+rfre&+Rnzy&N of &+cs&+Cp&+We&+Ce&+cd&N", TRUE, ch,
	    obj, victim, TO_VICT);
	spell_magic_missile(30, ch, NULL, 0, victim, 0);
	if (IS_ALIVE(ch) && IS_ALIVE(victim))
	{
		spell_ice_missile(30, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
	}
	return TRUE;
}

int rax_red_dagger(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch) ||
	    !(victim = legacy_proc_arg<P_char>(arg)))
	{
		return FALSE;
	}
	// 1/2 chance.
	if (!IS_ALIVE(victim) || number(0, 1))
	{
		return FALSE;
	}

	act("&+L$n's&N $q &+r st&+Rrik&+res &+Lout in a &+RF&+rUR&+RY &+Lof &+rde&+Rst&+rruc&+Rt&+rio&+Rn&+L!&N",
	    TRUE, ch, obj, victim, TO_NOTVICT);
	act("&+LYour&N $q &+r st&+Rrik&+res &+Lout in a &+RF&+rUR&+RY &+Lof &+rde&+Rst&+rruc&+Rt&+rio&+Rn&+L!&N",
	    TRUE, ch, obj, victim, TO_CHAR);
	act("&+L$n's&N $q &+r st&+Rrik&+res &+Lout in a &+RF&+rUR&+RY &+Lof &+rde&+Rst&+rruc&+Rt&+rio&+Rn&+L!&N",
	    TRUE, ch, obj, victim, TO_VICT);
	spell_bigbys_crushing_hand(60, ch, NULL, SPELL_TYPE_SPELL, victim, 0);

	return TRUE;
}

int cold_hammer(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char vict;
	struct proc_data *data;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (OBJ_ROOM(obj) && cmd == CMD_PERIODIC)
	{
		hummer(obj);
		return TRUE;
	}

	if (!IS_ALIVE(ch))
	{
		return FALSE;
	}

	if (cmd == CMD_MELEE_HIT)
	{
		if (!OBJ_WORN_POS(obj, WIELD))
		{
			return FALSE;
		}
		vict = legacy_proc_arg<P_char>(arg);

		if (OBJ_WORN_BY(obj, ch) && IS_ALIVE(vict))
		{
			// 1/15 chance.
			if (!number(0, 14))
			{
				act("&+yYour $q &+ysummons a wind full with &+Ysharp&+y stones down on $N!",
				    FALSE, obj->loc.wearing, obj, vict, TO_CHAR);
				act("$n's $q &+ysummons a wind full with &+Ysharp&+y stones down on you!",
				    FALSE, obj->loc.wearing, obj, vict, TO_VICT);
				act("$n's $q &+ysummons a wind full with &+Ysharp&+y stones down on $N!",
				    FALSE, obj->loc.wearing, obj, vict, TO_NOTVICT);
				spell_greater_living_stone(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);

				if (char_in_list(vict))
				{
					spell_living_stone(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
				}
			}
		}
		return FALSE;
	}
	else if (cmd == CMD_GOTNUKED && !number(0, 3))
	{
		if (!(data = legacy_proc_arg<struct proc_data *>(arg)))
		{
			return FALSE;
		}
		vict = data->victim;
		if (!IS_ALIVE(vict))
		{
			return FALSE;
		}
		act("$n's $p calls down the power of heavens into the room!", FALSE, ch, obj, vict,
		    TO_NOTVICT);
		act("$n's $p calls down the power of heavens into the room!", FALSE, ch, obj, vict,
		    TO_VICT);
		act("Your $p calls down the power of heavens into the room!", FALSE, ch, obj, vict,
		    TO_CHAR);

		spell_napalm(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
		if (!number(0, 2) && IS_ALIVE(vict) && IS_ALIVE(ch))
		{
			spell_napalm(40, ch, 0, SPELL_TYPE_SPELL, vict, 0);
		}
		if (!number(0, 19) && IS_ALIVE(ch))
		{
			spell_meteorswarm(40, ch, 0, SPELL_TYPE_SPELL, NULL, 0);
		}
		if (!number(0, 19) && IS_ALIVE(ch))
		{
			spell_earthen_rain(40, ch, 0, SPELL_TYPE_SPELL, NULL, 0);
		}
		return FALSE;
	}
	else if (cmd == CMD_GOTHIT && !number(0, 5))
	{
		if (!(data = legacy_proc_arg<struct proc_data *>(arg)))
		{
			return FALSE;
		}
		vict = data->victim;
		if (!IS_ALIVE(vict))
		{
			return FALSE;
		}
		act("Your hammer parries $N's vicious attack.", FALSE, ch, 0, vict,
		    TO_CHAR | ACT_NOTTERSE);
		act("$n's hammer parries your futile attack.", FALSE, ch, 0, vict,
		    TO_VICT | ACT_NOTTERSE);
		act("$n's hammer parries $N's attack.", FALSE, ch, 0, vict,
		    TO_NOTVICT | ACT_NOTTERSE);
		return TRUE;
	}
	return FALSE;
}

int khaziddea_blade(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;
	bool fired;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	// 1/50 chance.
	if (cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !(victim = legacy_proc_arg<P_char>(arg)) ||
	    number(0, 49))
	{
		return FALSE;
	}
	if (!IS_ALIVE(victim))
	{
		return FALSE;
	}

	fired = FALSE;
	if (affected_by_spell(victim, SPELL_STONE_SKIN))
	{
		affect_from_char(victim, SPELL_STONE_SKIN);
		fired = TRUE;
	}
	else if (affected_by_spell(victim, SPELL_BIOFEEDBACK))
	{
		affect_from_char(victim, SPELL_BIOFEEDBACK);
		fired = TRUE;
	}
	else if (affected_by_spell(victim, SPELL_SHADOW_SHIELD))
	{
		affect_from_char(victim, SPELL_SHADOW_SHIELD);
		fired = TRUE;
	}
	else if (affected_by_spell(victim, SPELL_DRAKESCALE_AEGIS))
	{
		affect_from_char(victim, SPELL_DRAKESCALE_AEGIS);
		fired = TRUE;
	}

	if (fired)
	{
		act("$q draws $n's hand down swiftly and strikes through $N's defenses!", FALSE, ch,
		    obj, victim, TO_ROOM);
		act("$q draws $n's hand down swiftly and strikes through your defenses!", FALSE, ch,
		    obj, victim, TO_VICT);
		act("$q draws your hand down swiftly and strikes through $N's defenses!", FALSE, ch,
		    obj, victim, TO_CHAR);
		return TRUE;
	}

	return FALSE;
}

int flame_blade(P_obj obj, P_char ch, int cmd, char * /*argument*/)
{
	if (!ch || !obj)
		return FALSE;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	P_char tch;

	if (OBJ_WORN(obj))
		tch = obj->loc.wearing;
	else if (OBJ_CARRIED(obj))
		tch = obj->loc.carrying;
	else
		return FALSE;

	if (!tch || IS_NPC(tch))
		return FALSE;

	if (GET_PID(tch) != obj->timer[1])
	{
		// Lamify it!
		obj->value[5] = 0;
		obj->value[6] = 0;
		obj->value[7] = 0;
		obj->bitvector = 0;
		obj->bitvector2 = 0;

		// Lets go ahead and kill the timer
		obj->timer[0] = 1;
		return FALSE;
	}

	return FALSE;
}

int artifact_biofeedback(P_obj obj, P_char ch, int cmd, char * /*argument*/)
{
	int curr_time;
	P_char temp_ch;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!obj)
	{
		return FALSE;
	}

	temp_ch = ch;

	if (!ch)
	{
		if (OBJ_WORN(obj) && obj->loc.wearing)
		{
			temp_ch = obj->loc.wearing;
		}
		else
		{
			return FALSE;
		}
	}

	if (!OBJ_WORN(obj))
	{
		return FALSE;
	}

	if (cmd != CMD_PERIODIC && !(cmd / 1000))
	{
		return FALSE;
	}

	curr_time = time(NULL);

	if (obj->timer[0] + get_property("timer.bioIoun", 80) <= curr_time &&
	    !has_skin_spell(temp_ch))
	{
		spell_biofeedback(25, temp_ch, 0, SPELL_TYPE_SPELL, temp_ch, 0);
		obj->timer[0] = curr_time;
	}
	return FALSE;
}

// Allows holder to use decline/accept/ptell.
int trustee_artifact(P_obj obj, P_char ch, int cmd, char *arg)
{
	int cmd_list[] = { CMD_DECLINE, CMD_APPROVE, CMD_PTELL, 0 }, i;

	if (!IS_ALIVE(ch) || IS_NPC(ch) || !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}

	// Ok, chars trustee.
	for (i = 0; cmd_list[i]; i++)
	{
		if (cmd_list[i] == cmd)
		{
			break;
		}
	}
	if (!cmd_list[i])
		return FALSE;

	// Ok, we be cooking with gas now. We have proper cmd..
	(*cmd_info[cmd].command_pointer)(ch, arg, cmd);
	return TRUE;
}

/* This object randomly moves around the map world, and allows entrance to
   the flying citadel floating above */
int flying_citadel(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	int door;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	// If not in a room.. (i.e. some God picked it up or something).
	if (!OBJ_ROOM(obj))
	{
		return FALSE;
	}

	door = number(0, 11);

	if ((world[obj->loc.room].number >= 110000) && (world[obj->loc.room].number <= 199999) &&
	    door < 4)
	{
		debug("flying_citadel: Passed check one.");
		// This crap crashes the MUD atm.
		return FALSE;
		if (VIRTUAL_EXIT(OBJ_ROOM(obj), door)->to_room &&
		    VIRTUAL_EXIT(OBJ_ROOM(obj), door)->to_room != NOWHERE)
		{
			debug("flying_citadel: Passed check two.");
			send_to_room("The dark clouds overhead move onward.\n", OBJ_ROOM(obj));
			obj_from_room(obj);
			obj_to_room(obj, real_room(door));
			send_to_room("An extremely large shadow floats overhead.\n", OBJ_ROOM(obj));
			return TRUE;
		}
	}
	// Dunno about this code either.. As we don't fly up these days..
	if (IS_ALIVE(ch) && ch->specials.z_cord == 4)
	{
		char_from_room(ch);
		char_to_room(ch, real_room0(obj_index[obj->R_num].number), 0);
		ch->specials.z_cord = 0;
		return TRUE;
	}

	return FALSE;
}

int changelog(P_obj obj, P_char ch, int cmd, char *args)
{
	FILE *f = NULL;
	char o_buf[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	char *ret;

	if (cmd != CMD_READ || GET_LEVEL(ch) < MINLVLIMMORTAL || !obj)
	{
		return FALSE;
	}

	if (!args)
	{
		send_to_char(
			"The book of files contain: src, areas, bugs, typos, ideas, cheats, cheaters, donations.\n",
			ch);
		return TRUE;
	}

	o_buf[0] = '\0';

	half_chop(args, arg, arg2);
	if (!str_cmp(arg, "src"))
	{
		f = fopen("lib/information/changelog.src", "r");
	}
	else if (!str_cmp(arg, "bugs"))
	{
		f = fopen("lib/reports/bugs", "r");
	}
	else if (!str_cmp(arg, "typos"))
	{
		f = fopen("lib/reports/typos", "r");
	}
	else if (!str_cmp(arg, "ideas"))
	{
		f = fopen("lib/reports/ideas", "r");
	}
	else if (!str_cmp(arg, "cheats"))
	{
		f = fopen("lib/reports/cheats", "r");
	}
	else if (!str_cmp(arg, "cheaters"))
	{
		f = fopen("lib/reports/cheaters", "r");
	}
	else if (!str_cmp(arg, "areas"))
	{
		f = fopen("lib/information/changelog.areas", "r");
	}
	else if (!str_cmp(arg, "donations"))
	{
		f = fopen("logs/log/donation", "r");
	}
	else
	{
		return FALSE;
	}

	if (!f)
	{
		send_to_char(
			"Could not open file.\nThe book of files contain: src, areas, bugs, typos, ideas, cheats, cheaters, donations.\n",
			ch);
		return TRUE;
	}

	do
	{
		ret = fgets(buf, MAX_STRING_LENGTH, f);
		if (ret)
		{
			if ((strlen(o_buf) + strlen(buf)) >= (MAX_STRING_LENGTH - 10))
			{
				ret = NULL;
			}
			else
			{
				if (isalpha(buf[0]))
					strcat(o_buf, "&+B");
				else
					strcat(o_buf, "&+c");
				strcat(o_buf, buf);
			}
		}
	} while (ret);
	strcat(o_buf, "\n");

	fclose(f);
	page_string(ch->desc, o_buf, 1);
	return TRUE;
}

int ice_shattered_bits(P_obj obj, P_char /*ch*/, int cmd, char * /*argument*/)
{
	if (cmd == CMD_DECAY)
	{
		if (world[obj->loc.room].people)
		{
			act("$p melt into nothingness.", TRUE, world[obj->loc.room].people, obj, 0,
			    TO_ROOM);
			act("$p melt into nothingness.", TRUE, world[obj->loc.room].people, obj, 0,
			    TO_CHAR);
		}
		return TRUE;
	}
	return FALSE;
}

int ice_block(P_obj /*obj*/, P_char /*ch*/, int cmd, char * /*argument*/)
{
	if (cmd == CMD_DECAY)
		return TRUE;

	return FALSE;
}

int frost_beacon(P_obj obj, P_char ch, int cmd, char * /*argument*/)
{
	P_char tch;
	char buf[1024];

	memset(buf, 0, sizeof(buf));

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd == CMD_DECAY)
	{
		// Give a message to the caster if they're not in the room.
		for (tch = character_list; tch; tch = tch->next)
			if (IS_PC(tch) && GET_PID(tch) == obj->value[0])
			{
				if (tch->in_room != obj->loc.room)
					act("$p melts into nothingness.", TRUE, tch, obj, 0,
					    TO_CHAR);
				break;
			}

		// Give a message to all in the room.
		if (world[obj->loc.room].people)
		{
			act("$p melts into nothingness.", TRUE, world[obj->loc.room].people, obj, 0,
			    TO_ROOM);
			act("$p melts into nothingness.", TRUE, world[obj->loc.room].people, obj, 0,
			    TO_CHAR);
		}
		return TRUE;
	}

	if (!ch || (ch->specials.z_cord != 0))
		return FALSE;

	if (number(0, 2) && IS_SET(obj->extra_flags, ITEM_SECRET) && cmd_to_exitnumb(cmd) != -1 &&
	    obj->value[0] && obj->loc_p == LOC_ROOM &&
	    (IS_PC(ch) && obj->value[0] != GET_PID(ch)) && !IS_TRUSTED(ch))
	{
		for (tch = character_list; tch; tch = tch->next)
			if (IS_PC(tch) && GET_PID(tch) == obj->value[0])
				break;

		if (tch != NULL && ch->in_room != tch->in_room && !grouped(ch, tch))
		{
			snprintf(buf, 1024, "$N has set off your frost beacon at %s!",
				 world[ch->in_room].name);
			act(buf, FALSE, tch, 0, ch, TO_CHAR);
		}
	}

	return FALSE;
}

int vareena_statue(P_obj obj, P_char ch, int cmd, char * /*argument*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd && !(cmd / 1000))
		return FALSE;

	if (!obj || ch || !OBJ_ROOM(obj))
		return FALSE;

	// see if vareena is in the room
	for (ch = world[obj->loc.room].people; ch; ch = ch->next_in_room)
	{
		if (isname("vareena", GET_NAME(ch)))
			break;
	}
	if (!ch)
		return FALSE;

	// chance of statue acting is VERY slight

	switch (number(0, 250))
	{
	case 0:
		act("&+BA wing feath&+ber from $p &+bgently caress&+Bes your cheek.&n", TRUE, ch,
		    obj, NULL, TO_CHAR);
		act("&+BA soft wing fea&+bther from $p&n &+bgently caress&+Bes $n&+B's cheek.&n",
		    TRUE, ch, obj, 0, TO_ROOM);
		return TRUE;
	case 100:
		act("&+L$a $q &+bsurrounds you with &+Ba loving embrace.&n", TRUE, ch, obj, NULL,
		    TO_CHAR);
		act("&+L$a $q &+bsorrounds $n with &+Ba loving embrace.&n", TRUE, ch, obj, 0,
		    TO_ROOM);
		return TRUE;
	}
	return FALSE;
}

int disarm_pick_gloves(P_obj obj, P_char ch, int cmd, char *arg)
{
	struct proc_data *data;
	P_char vict;
	P_obj weap = NULL;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!OBJ_WORN_POS(obj, WEAR_HANDS))
	{
		return FALSE;
	}

	// 1/10 chance.
	if (cmd == CMD_GOTHIT && !number(0, 9))
	{
		if (!(data = legacy_proc_arg<struct proc_data *>(arg)))
		{
			return FALSE;
		}
		vict = data->victim;
		if (!IS_ALIVE(vict))
		{
			return FALSE;
		}

		if (vict->equipment[WIELD] &&
		    !IS_SET(vict->equipment[WIELD]->extra_flags, ITEM_NODROP) &&
		    vict->equipment[WIELD]->type == ITEM_WEAPON)
		{
			weap = unequip_char(vict, WIELD);
		}

		if (weap)
		{
			obj_to_char(weap, vict);
			strip_holy_sword(vict);

			act("You hear a soft tingling laughter as your gloves lash onto", FALSE, ch,
			    obj, vict, TO_CHAR);
			act("$N's arm and start to bite him !! $N yelps in pain and drops his $q",
			    FALSE, ch, weap, vict, TO_CHAR);

			act("OUCH OUCH!! You fumble your weapon as $n's", FALSE, ch, obj, vict,
			    TO_VICT);
			act("$q bite you!", FALSE, ch, obj, vict, TO_VICT);
			set_short_affected_by(vict, SKILL_DISARM, 3 * PULSE_VIOLENCE);
			return TRUE;
		}
	}

	return FALSE;
}

int doom_blade_Proc(P_obj obj, P_char ch, int cmd, char *arg)
{
	int rand, dam = cmd / 1000, lvl = 1;
	P_char vict;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	vict = legacy_proc_arg<P_char>(arg);
	if (!(obj) || !IS_ALIVE(ch) || !dam || !IS_ALIVE(vict) || ch->in_room != vict->in_room)
	{
		return FALSE;
	}

	lvl = MIN(40, GET_LEVEL(ch));

	// Max 1/9 chance.
	if (number(0, MAX(8, 56 - GET_LEVEL(ch))))
	{
		return FALSE;
	}

	act("$n's $p releases a &+yhazy&n cloud of &+LDeath&n that surrounds $N!&n", FALSE, ch, obj,
	    vict, TO_NOTVICT);
	act("$n's $p releases a &+LDEATH CLOUD&n that surrounds you entirely!", FALSE, ch, obj,
	    vict, TO_VICT);
	act("Your $p releases a &+LDEATH CLOUD&n that surrounds $N!", FALSE, ch, obj, vict,
	    TO_CHAR);

	if (IS_UNDEADRACE(vict) && (GET_SPEC(ch, CLASS_NECROMANCER, SPEC_REAPER) ||
				    GET_SPEC(ch, CLASS_THEURGIST, SPEC_THAUMATURGE)))
	{
		spell_undead_to_death(lvl, ch, NULL, 0, vict, obj);
	}
	else
	{
		rand = number(0, 4);
		switch (rand)
		{
		case 0:
			spell_wither(lvl, ch, NULL, 0, vict, obj);
			break;
		case 1:
			spell_blindness(lvl, ch, NULL, 0, vict, obj);
			break;
		case 2:
			spell_disease(lvl, ch, NULL, 0, vict, obj);
			break;
		case 3:
			spell_curse(lvl, ch, NULL, 0, vict, NULL);
			break;
		case 4:
			spell_poison(lvl, ch, NULL, 0, vict, obj);
			break;
		default:
			break;
		}
	}
	return TRUE;
}

int orcus_wand(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim, tmp_ch1, tmp_ch2;
	int temproom;
	char Gbuf1[MAX_STRING_LENGTH], Gbuf4[MAX_STRING_LENGTH];

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch) || !arg ||
	    (cmd != CMD_SACRIFICE && cmd != CMD_TERMINATE && cmd != CMD_CD) ||
	    !OBJ_WORN_BY(obj, ch) || obj->loc.wearing->equipment[HOLD] != obj)
	{
		return FALSE;
	}

	if (GET_LEVEL(ch) < 59 && cmd != CMD_CD)
	{
		return FALSE;
	}

	one_argument(arg, Gbuf1);

	if (!*Gbuf1)
	{
		send_to_char("Who?\n", ch);
		return TRUE;
	}

	if (!(victim = get_char_room_vis(ch, Gbuf1)))
	{
		if (!(victim = get_char_vis(ch, Gbuf1)))
		{
			send_to_char("Who?\n", ch);
			return (FALSE);
		}
		cmd = CMD_CD;
	}
	if (GET_LEVEL(ch) < 56)
	{
		act("&+LThe Wand of Orcus glows briefly with a sickly black light.", FALSE, ch, 0,
		    victim, TO_CHAR);
		act("&+LThe Wand wrenches free of your grip, diving into your chest!", FALSE, ch, 0,
		    victim, TO_CHAR);
		act("&+r$n&+L attempts to kill&N&+r $N&N&+L with the Wand of Orcus!", TRUE, ch, 0,
		    victim, TO_NOTVICT);
		act("&+LThe Wand of Orcus wrenches free from&N&+r $n&N, and carves its way into $n's chest!",
		    TRUE, ch, 0, victim, TO_NOTVICT);
		act("&+r$n&N&+L attempts to kill you with the Wand of Orcus!", FALSE, ch, 0, victim,
		    TO_VICT);
		act("&+LThe Wand of Orcus glows black, and carves into&N&+r $n's&N&+L chest!",
		    FALSE, ch, 0, victim, TO_VICT);
		die(ch, ch);
		return TRUE;
	}
	if ((cmd == CMD_CD) && isname("Orcus", ch->player.name))
	{
		temproom = victim->in_room;
		if ((temproom < 0) || (temproom > 999999))
		{
			return FALSE;
		}

		strcpy(Gbuf4,
		       "&+LOrcus&N&+b whispers 'Shift' to his Wand, which starts to glow black.\n"
		       "&+bThe Wand opens a portal to the Abyss, which &+LOrcus&N&+b steps through.\n"
		       "&+bA beam of black light hits the portal, and it snaps shut.&N\n");
		send_to_room(Gbuf4, ch->in_room);
		char_from_room(ch);
		char_to_room(ch, temproom, -1);

		strcpy(Gbuf4,
		       "&+bThe sky turns black, as a great shadow looms over all you see.\n"
		       "&+bThe shadow slowly shrinks down in size until the ghastly form of\n"
		       "&+LOrcus, Demon Prince of The Undead&N&+b, appears infront of you.\n");
		send_to_room(Gbuf4, ch->in_room);
		for (tmp_ch1 = world[ch->in_room].people; tmp_ch1; tmp_ch1 = tmp_ch2)
		{
			tmp_ch2 = tmp_ch1->next_in_room;
			if ((tmp_ch1 != ch) && (GET_LEVEL(tmp_ch1) < 51))
			{
				do_flee(tmp_ch1, 0, 2);
			}
		}
		return (TRUE);
	}
	if (ch == victim)
	{
		send_to_char("You cannot kill yourself, DUH!! Go pick your nose!..\n", ch);
		return (FALSE);
	}
	if (IS_PC(victim) && !str_cmp(victim->player.name, "Orcus"))
	{
		act("The Wand glows with a sickly black light, and flies out of your hand\n"
		    "and shoots out a beam of deadly black light into your chest!\n"
		    "You fall victim to the very god you sought to destroy...",
		    FALSE, ch, 0, victim, TO_CHAR);
		act("$n attempts to kill Orcus with his own Wand!\n"
		    "The Wand glows black, and flies out in the air and dives into $n's chest!",
		    TRUE, ch, 0, victim, TO_NOTVICT);
		act("$n attempts to kill you with your own wand! What a dolt!\n"
		    "The Wand glows black, and flies into the air and dives into $n's chest!",
		    FALSE, ch, 0, victim, TO_VICT);
		die(ch, ch);
		return TRUE;
	}
	if (ch != victim)
	{
		act("&+bYou point your Wand at $N, and call down your power at $N.\n"
		    "&+bThe Wand comes to life, and starts to glow with the power of the Abyss, \n"
		    "&+bas if it had a hunger of its own for $N's heart!\n"
		    "&+bThe Wand rips out $N's heart, stealing $S life life away...",
		    FALSE, ch, 0, victim, TO_CHAR);

		act("&+b$n points at you, and whispers 'Die' to $s Wand.\n"
		    "&+b$n points $s Wand at you and calls down $s power on you!\n"
		    "&+bThe Wand comes to life, and starts to glow with the power of the Abyss!  It\n"
		    "&+bThe Wand shoots a black beam of light at your head. Searing pain shoots\n"
		    "&+bthrough your head for a split second, and then all goes black.\n",
		    FALSE, ch, 0, victim, TO_VICT);

		act("&+b$n points at $N, and whispers 'Die' to $s Wand.\n"
		    "&+bThe Wand shoots forth a beam of black light towords $N!\n"
		    "&+b$N shudders slightly, and then falls to the ground, a withered\n"
		    "&+bpile of flesh. $N is sacrificed to the Demon Prince, Orcus.\n",
		    TRUE, ch, 0, victim, TO_NOTVICT);

		if (cmd == CMD_SACRIFICE)
		{
			die(victim, ch);
			if ((GET_LEVEL(ch) >= 57) && (GET_LEVEL(ch) >= GET_LEVEL(victim)))
			{
				statuslog(ch->player.level, "%s was destroyed by Orcus.",
					  GET_NAME(victim));
			}
		}
		else if ((cmd == CMD_TERMINATE) && (IS_PC(victim)))
		{
			if ((GET_LEVEL(ch) >= MINLVLIMMORTAL) &&
			    (GET_LEVEL(ch) >= GET_LEVEL(victim)))
			{
				act(".", FALSE, ch, 0, victim, TO_CHAR);
				act(".", FALSE, ch, 0, victim, TO_CHAR);
				act("You call on the pure might of the Forgers down upon $N", FALSE,
				    ch, 0, victim, TO_CHAR);
				act("With great magic, you devour $N's soul and utterly, ", FALSE,
				    ch, 0, victim, TO_CHAR);
				act("obliterating $M from this world of Duris, forever...", FALSE,
				    ch, 0, victim, TO_CHAR);
				act("The world stands in awe of your awesome power... ;)", FALSE,
				    ch, 0, victim, TO_CHAR);
				act(".", FALSE, ch, 0, victim, TO_NOTVICT);
				act(".", FALSE, ch, 0, victim, TO_NOTVICT);
				act("$n points at $N, and whispers 'Destroy' to $s Wand.", FALSE,
				    ch, 0, victim, TO_NOTVICT);
				act("$n raises $s hand, and calls down the might of the Forgers on $N",
				    FALSE, ch, 0, victim, TO_NOTVICT);
				act("$n slowly devours $N's soul, utterly destroying $S from this world forever.",
				    FALSE, ch, 0, victim, TO_NOTVICT);
				act("You can hear $N's soul scream in agony one last time, then fade on the winds....",
				    FALSE, ch, 0, victim, TO_NOTVICT);
				act(".", FALSE, ch, 0, victim, TO_VICT);
				act(".", FALSE, ch, 0, victim, TO_VICT);
				act("$n points at you, and whispers 'Destroy' to $s Wand.", FALSE,
				    ch, 0, victim, TO_VICT);
				act("$n raises his hand, and calls down the might of the Forgers upon you!",
				    FALSE, ch, 0, victim, TO_VICT);
				act("$n slowly devours your soul as it rises out of your dead body.",
				    FALSE, ch, 0, victim, TO_VICT);
				act("AAAAAAAAAAHHHHHHHHHHHHH!!!! The pain is agonizing, though there is no escape..",
				    FALSE, ch, 0, victim, TO_VICT);
				act("Your soul is destroyed utterly, forever obliterated from this world...",
				    FALSE, ch, 0, victim, TO_VICT);
				statuslog(
					ch->player.level,
					"%s's soul was just utterly devoured by the power of Orcus. Boo Hiss! What a PUD!",
					GET_NAME(victim));
				if (victim->desc)
				{
					victim->desc->connected = CON_DELETE;
				}
				// If it's not an immortal.
				if (IS_PC(ch) && (GET_LEVEL(ch) < MINLVLIMMORTAL))
				{
					update_ingame_racewar(-GET_RACEWAR(ch));
				}
				extract_char(victim);
			}
		}
		else if ((cmd == CMD_TERMINATE) && (IS_NPC(victim)))
		{
			die(ch, ch);
			statuslog(
				ch->player.level,
				"%s's soul was just utterly devoured by the power of Orucs.. whee",
				GET_NAME(victim));
		}
		return TRUE;
	}
	return FALSE;
}

/* Guild badges Kvark 2002-02-04
 * This item is a bonus for those with artifact's and those that have frags, item changes name tho soo
 *   it's also fun for lowbies and others. This item might need tweaking in long wipes since the frag
 *   amount might be pretty high and frags above 100 let ya get pretty nice stat's.
 */
int guild_badge(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	int affects_bonus;
	int i, count, weekday;
	char buf1[MAX_STRING_LENGTH];
	int curr_time;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd != CMD_PERIODIC || !OBJ_WORN(obj))
	{
		return FALSE;
	}
	ch = WEARER(obj);
	if (!IS_ALIVE(ch) || IS_NPC(ch))
	{
		return FALSE;
	}

	curr_time = time(NULL);
	if (obj->timer[0] <= curr_time)
	{
		// Things breaks sometimes...
		//   be scared all the time - on regular proc this object has a chance to break.
		if (!number(0, 90))
		{
			act("&+L$p &+Lhums with a &+GCRA&+YC&+GKING &+Lsound.&n", FALSE, ch, obj, 0,
			    TO_CHAR);
			act("&+L$p &+Lcrumble to dust.&n", FALSE, ch, obj, 0, TO_CHAR);
			extract_obj(obj, TRUE); // Not an arti, but 'in game.'
			return TRUE;
		}

		// Let's only proc this function once every 12 min.
		obj->timer[0] = curr_time + 12 * SECS_PER_REAL_MIN;

		// Didnt break it? ok.... lets give them some fun then (if they're a positive fragger).
		// Things that modifes the badge.
		// If you have alot of frags, that's good for you!
		if (ch->only.pc->frags > 0)
		{
			// 20.00 frags gives 400 points.
			affects_bonus = (int)(ch->only.pc->frags / 5);

			// Artis are worth some also!
			for (i = count = 0; i < MAX_WEAR; i++)
			{
				if (ch->equipment[i])
				{
					if (IS_ARTIFACT(ch->equipment[i]))
					{
						count++;
					}
				}
			}
			// 80 points per artifact.
			affects_bonus += 80 * count;

			// Level! Of course lvl changes is!
			// 4 points per level: up to 224 points at 56.
			affects_bonus += 4 * GET_LEVEL(ch);

			// Goodies have it a bit easier get good stat's
			if (IS_RACEWAR_GOOD(ch))
			{
				affects_bonus = (affects_bonus * 13) / 10;
			}

			// Add some little randomness
			affects_bonus += number(0, 100);

			// affects_bonus will always be at least 0, since we start with a non-negative and
			//   only add more non-negatives or multiply/divide by a positive.
			// We cap it at 1000 to make the below modifiers cap properly.
			affects_bonus = MIN(affects_bonus, 1000);

			// Ok lets switch affects depending on weekday.
			weekday = ((35 * time_info.month) + time_info.day + 1) % 7;
			switch (weekday)
			{
			case 0:
				obj->affected[0].location = APPLY_HIT;
				// Max 35 hps.
				obj->affected[0].modifier = affects_bonus / 28;
				// Remove the affect if there's no modifier.
				if (obj->affected[0].modifier == 0)
				{
					obj->affected[0].location = APPLY_NONE;
				}
				obj->affected[1].location = APPLY_STR_MAX;
				// Max 5 maxstat.
				obj->affected[1].modifier = affects_bonus / 200;
				break;
			case 1:
				obj->affected[0].location = APPLY_HIT;
				obj->affected[0].modifier = affects_bonus / 28;
				if (obj->affected[0].modifier == 0)
				{
					obj->affected[0].location = APPLY_NONE;
				}
				// Max 5 damroll.
				obj->affected[1].location = APPLY_DAMROLL;
				obj->affected[1].modifier = affects_bonus / 200;
				if (obj->affected[1].modifier == 0)
				{
					obj->affected[1].location = APPLY_NONE;
				}
				break;
			case 2:
				obj->affected[0].location = APPLY_AC;
				// Best negative 50 ac (Remember negative ac is good, positive is bad).
				obj->affected[0].modifier = -(affects_bonus / 20);
				if (obj->affected[0].modifier == 0)
				{
					obj->affected[0].location = APPLY_NONE;
				}
				obj->affected[1].location = APPLY_CON_MAX;
				obj->affected[1].modifier = affects_bonus / 200;
				if (obj->affected[1].modifier == 0)
				{
					obj->affected[1].location = APPLY_NONE;
				}
				break;
			case 3:
				obj->affected[0].location = APPLY_AC;
				obj->affected[0].modifier = -(affects_bonus / 20);
				if (obj->affected[0].modifier == 0)
				{
					obj->affected[0].location = APPLY_NONE;
				}
				obj->affected[1].location = APPLY_SAVING_SPELL;
				// Best negative 5 save spell (Remember negative save spell is good).
				obj->affected[1].modifier = -(affects_bonus / 200);
				if (obj->affected[1].modifier == 0)
				{
					obj->affected[1].location = APPLY_NONE;
				}
				break;
			case 4:
				obj->affected[0].location = APPLY_AC;
				obj->affected[0].modifier = -(affects_bonus / 20);
				if (obj->affected[0].modifier == 0)
				{
					obj->affected[0].location = APPLY_NONE;
				}
				// Max 8 regular stat.
				obj->affected[1].location = APPLY_DEX;
				obj->affected[1].modifier = affects_bonus / 125;
				if (obj->affected[1].modifier == 0)
				{
					obj->affected[1].location = APPLY_NONE;
				}
				break;
			case 5:
				obj->affected[0].location = APPLY_HIT;
				obj->affected[0].modifier = affects_bonus / 28;
				if (obj->affected[0].modifier == 0)
				{
					obj->affected[0].location = APPLY_NONE;
				}
				obj->affected[1].location = APPLY_SAVING_PARA;
				// Best negative 5 save para (Remember negative save spell is good).
				obj->affected[1].modifier = -(affects_bonus / 200);
				if (obj->affected[1].modifier == 0)
				{
					obj->affected[1].location = APPLY_NONE;
				}
				break;
			case 6:
				obj->affected[0].location = APPLY_HIT;
				obj->affected[0].modifier = affects_bonus / 28;
				if (obj->affected[0].modifier == 0)
				{
					obj->affected[0].location = APPLY_NONE;
				}
				obj->affected[1].location = APPLY_CON;
				obj->affected[1].modifier = affects_bonus / 125;
				if (obj->affected[1].modifier == 0)
				{
					obj->affected[1].location = APPLY_NONE;
				}
				break;
			default:
				break;
			}

			balance_affects(ch);
			// ok calculations are made, lets HUMMMM
			act("&+L$p &+Lhums briefly.&n", FALSE, ch, obj, 0, TO_CHAR);
			if (GET_TITLE(ch))
			{
				// The long desc is, "A magical symbol floats in the air.", so keywords magical and symbol are important.
				// The short desc is "The symbol of ...", so keywords symbol and ... are important.
				snprintf(buf1, MAX_STRING_LENGTH, "badge symbol magical %s",
					 strip_ansi(GET_TITLE(ch)).c_str());
				if ((obj->str_mask & STRUNG_DESC1) && obj->name)
				{
					FREE(obj->name);
				}
				obj->name = NULL;
				obj->str_mask |= STRUNG_DESC1;
				obj->name = str_dup(buf1);

				snprintf(buf1, MAX_STRING_LENGTH, "&+LThe symbol of %s&N",
					 GET_TITLE(ch));
				if ((obj->str_mask & STRUNG_DESC2) && obj->short_description)
				{
					FREE(obj->short_description);
				}
				obj->short_description = NULL;
				obj->str_mask |= STRUNG_DESC2;
				obj->short_description = str_dup(buf1);
			}
			else
			{
				// The long desc is, "A magical symbol floats in the air.", so keywords magical and symbol are important.
				// The short desc is "The symbol of <ch's name>", so keywords symbol and <ch's name> are important.
				snprintf(buf1, MAX_STRING_LENGTH, "badge symbol magical %s",
					 GET_NAME(ch));
				if ((obj->str_mask & STRUNG_DESC1) && obj->name)
				{
					FREE(obj->name);
				}
				obj->name = NULL;
				obj->str_mask |= STRUNG_DESC1;
				obj->name = str_dup(buf1);

				snprintf(buf1, MAX_STRING_LENGTH, "&+LThe symbol of %s&N",
					 GET_NAME(ch));
				if ((obj->str_mask & STRUNG_DESC2) && obj->short_description)
				{
					FREE(obj->short_description);
				}
				obj->short_description = NULL;
				obj->str_mask |= STRUNG_DESC2;
				obj->short_description = str_dup(buf1);
			}
			return TRUE;
		}
	}
	return FALSE;
}

int obj_imprison(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	int victim_in_room;
	P_char tch;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	victim_in_room = FALSE;
	if (OBJ_ROOM(obj))
	{
		for (tch = world[obj->loc.room].people; tch; tch = tch->next_in_room)
		{
			if (IS_PC(tch) && GET_PID(tch) == obj->value[0])
			{
				victim_in_room = TRUE;
				break;
			}
		}
	}

	// If it's been picked up or victim escaped the room somehow, then purge object and remove bit.
	if (!victim_in_room)
	{
		for (tch = character_list; tch; tch = tch->next)
		{
			if (IS_PC(tch) && GET_PID(tch) == obj->value[0])
			{
				if (IS_AFFECTED5(tch, AFF5_IMPRISON))
				{
					REMOVE_BIT(tch->specials.affected_by5, AFF5_IMPRISON);
					break;
				}
			}
		}
		// obj_from_room(obj);
		extract_obj(obj, TRUE); // Not an arti, but 'in game.'
		return FALSE;
	}

	// If ch isn't the right ch.
	if (!IS_ALIVE(ch) || IS_TRUSTED(ch) || IS_NPC(ch) || obj->value[0] != GET_PID(ch))
	{
		return FALSE;
	}

	if (cmd == CMD_LOOK || cmd == CMD_SAY || cmd == CMD_PETITION)
	{
		return FALSE;
	}

	obj->value[1] -= GET_LEVEL(ch) + 66;

	// Struggled enough or 5% 'lucky' chance.
	if (obj->value[1] > 0 && number(1, 100) <= 95)
	{
		act("But you are totally encased by the $q!", FALSE, ch, obj, NULL, TO_CHAR);
		act("$N struggles to break out of the $q but fails.", TRUE, ch, obj, ch,
		    TO_NOTVICT);
		CharWait(ch, PULSE_VIOLENCE / 2);
		return TRUE;
	}

	act("Aha! The $q was nothing but an illusion, you are free!", FALSE, ch, obj, ch, TO_CHAR);
	act("An indescribable relief emanates from $N as $E realises the $q was just an illusion!",
	    TRUE, NULL, obj, ch, TO_NOTVICT);
	REMOVE_BIT(ch->specials.affected_by5, AFF5_IMPRISON);
	extract_obj(obj, TRUE); // Not an arti, but 'in game.'

	return FALSE;
}

/* this proc is the staff of blue flames proc modified, Raxxel did it, and it probably doesn't work! */
int totem_of_mastery(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char vict;
	char e_pos;

	vict = legacy_proc_arg<P_char>(arg);

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (obj && !ch && cmd == CMD_PERIODIC)
	{
		hummer(obj);
		return TRUE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN(obj) || !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}

	// If it must be wielded, use this
	e_pos = ((obj->loc.wearing->equipment[HOLD] == obj)		? HOLD :
		 (obj->loc.wearing->equipment[WIELD] == obj)		? WIELD :
		 (obj->loc.wearing->equipment[SECONDARY_WEAPON] == obj) ? SECONDARY_WEAPON :
		 (obj->loc.wearing->equipment[THIRD_WEAPON] == obj)	? THIRD_WEAPON :
		 (obj->loc.wearing->equipment[FOURTH_WEAPON] == obj)	? FOURTH_WEAPON :
									  0);

	if (!e_pos)
	{
		return FALSE;
	}

	if (arg && (cmd == CMD_SAY))
	{
		if (isname(arg, "spirit"))
		{
			act("You whisper 'spirit' to your $q", FALSE, ch, 0, 0, TO_CHAR);
			act("&+mA spirit fog streams out of your $q &+mand surrounds you.&N", FALSE,
			    ch, obj, obj, TO_CHAR);
			act("$n whispers 'spirit' to $q", TRUE, ch, obj, NULL, TO_ROOM);
			act("&+mA spirit fog streams out of &+M$n&+m's $q&+m.&N", TRUE, ch, obj,
			    NULL, TO_ROOM);

			if (ch->group)
			{
				cast_as_area(ch, SPELL_SPIRIT_ARMOR, 60, 0);
			}
			else
			{
				spell_spirit_armor(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
			}
			return TRUE;
		}
		else if (isname(arg, "hawk"))
		{
			act("You whisper 'hawk' to your $q", FALSE, ch, 0, vict, TO_CHAR);
			act("&+wA vaporous &+Whawk&+w appears briefly from your&N $q.&N", FALSE, ch,
			    obj, obj, TO_CHAR);
			act("$n whispers 'hawk' to $q.", FALSE, ch, obj, obj, TO_ROOM);
			act("&+wA vaporous &+Whawk&+w appears briefly from &+W$n&+w's&N $q.", TRUE,
			    ch, obj, vict, TO_ROOM);
			if (ch->group)
			{
				cast_as_area(ch, SPELL_HAWKVISION, 60, 0);
			}
			else
			{
				spell_hawkvision(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
			}
			return TRUE;
		}
		else if (isname(arg, "panther"))
		{
			act("You whisper 'panther' to your $q", FALSE, ch, 0, vict, TO_CHAR);
			act("&+LThe spirit of a jet black &+wpanther&+L flows from your &N$q&+w.&N",
			    FALSE, ch, obj, obj, TO_CHAR);
			act("$n whispers 'panther' to $q.", FALSE, ch, obj, obj, TO_ROOM);
			act("&+LThe spirit of a jet black &+wpanther&+L flows from &+W$n&+w's&N $q&+w.",
			    TRUE, ch, obj, vict, TO_ROOM);
			if (ch->group)
			{
				cast_as_area(ch, SPELL_PANTHERSPEED, 60, 0);
			}
			else
			{
				spell_pantherspeed(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
			}
			return TRUE;
		}
		else if (isname(arg, "sense"))
		{
			act("You whisper 'sense' to your $q", FALSE, ch, 0, vict, TO_CHAR);
			act("&+cA rush of air from your &N $q&+w.&N&+c engulfs you.", FALSE, ch,
			    obj, obj, TO_CHAR);
			act("$n whispers 'sense' to $q.", FALSE, ch, obj, obj, TO_ROOM);
			act("&+cA rush of air from &+W$n&+w's&N $q&+c.", TRUE, ch, obj, vict,
			    TO_ROOM);
			if (ch->group)
			{
				cast_as_area(ch, SPELL_SENSE_SPIRIT, 60, 0);
			}
			else
			{
				spell_sense_spirit(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
			}
			return TRUE;
		}
		else if (isname(arg, "raven"))
		{
			act("You whisper 'raven' to your $q", FALSE, ch, 0, vict, TO_CHAR);
			act("&+ySmoke billows from your&N $q &+yin the form of a &+WRaven&+y.&N",
			    FALSE, ch, obj, obj, TO_CHAR);
			act("$n whispers 'raven' to $p.", FALSE, ch, obj, obj, TO_ROOM);
			act("&+ySmoke billows from &+Y$n&+y's&N $q &+yin the form of &+WRaven&+y.",
			    TRUE, ch, obj, vict, TO_ROOM);
			if (ch->group)
			{
				cast_as_area(ch, SPELL_RAVENFLIGHT, 60, 0);
			}
			else
			{
				spell_ravenflight(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
			}
			return TRUE;
		}
		else if (isname(arg, "ward"))
		{
			act("You whisper 'ward' to your $q", FALSE, ch, 0, vict, TO_CHAR);
			act("&+wA &+Wspirit &+wappears from your &N$q &+wand engulfs you.&N", FALSE,
			    ch, obj, obj, TO_CHAR);
			act("$n whispers 'ward' to $q.", FALSE, ch, obj, obj, TO_ROOM);
			act("&+wA &+Wspirit&+w appears from &+W$n&+w's&N $q&+w.&N", TRUE, ch, obj,
			    vict, TO_ROOM);
			if (ch->group)
			{
				cast_as_area(ch, SPELL_GREATER_SPIRIT_WARD, 60, 0);
			}
			else
			{
				spell_greater_spirit_ward(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
			}
			return TRUE;
		}
		else if (isname(arg, "lion"))
		{
			act("&+yA ghostly &+YLion &+RROARS&+y from your $q.&N", FALSE, ch, obj, obj,
			    TO_CHAR);
			act("$n whispers 'lion' to $q.", FALSE, ch, obj, obj, TO_ROOM);
			act("&+yA ghostly &+YLion &+RROARS&+y from &+Y$n&+y's&N $q&+y.&N", TRUE, ch,
			    obj, vict, TO_ROOM);
			if (ch->group)
			{
				cast_as_area(ch, SPELL_LIONRAGE, 60, 0);
			}
			else
			{
				spell_lionrage(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
			}
			return TRUE;
		}
		else if (isname(arg, "elephant"))
		{
			act("You whisper 'elephant' to your $q", FALSE, ch, 0, vict, TO_CHAR);
			act("&+LThe sounds of &+welephants &+Ltrumpeting erupt from your&N $q&+L.&N",
			    FALSE, ch, obj, obj, TO_CHAR);
			act("$n whispers 'elephant' to $q.", FALSE, ch, obj, obj, TO_ROOM);
			act("&+LThe sounds of &+Welephants &+Ltrumpeting erupt from &+w$n&+L's&N $q&+L.",
			    TRUE, ch, obj, vict, TO_ROOM);
			if (ch->group)
			{
				cast_as_area(ch, SPELL_ELEPHANTSTRENGTH, 60, 0);
			}
			else
			{
				spell_elephantstrength(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
			}
			return TRUE;
		}
		/* EVERYTHING HERE IS JUST FUNNY SILLY STUFF */
		else if (isname(arg, "mystra"))
		{
			act("You whisper 'mystra' to your $q", FALSE, ch, 0, vict, TO_CHAR);
			act("&+LThe sky darkens and fills with clouds.  You hear a deep rumbling&N",
			    FALSE, ch, obj, obj, TO_CHAR);
			act("&+Lin the distance.  The rumbling gets louder and louder.  God himself&N",
			    FALSE, ch, obj, obj, TO_CHAR);
			act("&+Lappears infront of you. &+WHOW DARE YOU SPEAK THAT VILE NAME?!&N",
			    FALSE, ch, obj, obj, TO_CHAR);
			act("$n whispers 'mystra' to $q.", FALSE, ch, obj, obj, TO_ROOM);
			act("&+LThe sky darkens and fills with clouds.  You hear a deep rumbling&N",
			    TRUE, ch, obj, vict, TO_ROOM);
			act("&+Lin the distance.  The rumbling gets louder and louder.  God himself&N",
			    TRUE, ch, obj, vict, TO_ROOM);
			act("&+Lappears infront of you. &+WHOW DARE YOU SPEAK THAT VILE NAME?!&N",
			    TRUE, ch, obj, vict, TO_ROOM);
			spell_lightning_bolt(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
			return TRUE;
		}
	}

	if (cmd != CMD_MELEE_HIT)
	{
		return FALSE;
	}

	// 1/16 chance
	if (vict && !number(0, 15))
	{
		act("&+BYour $q flashes with lightning as a bolt fires out at $N.", FALSE,
		    obj->loc.wearing, obj, vict, TO_CHAR);
		act("$n's $q &+Bradiates a bolt of lightning out at you.", FALSE, obj->loc.wearing,
		    obj, vict, TO_VICT);
		act("$n's $q &+Bradiates a blot of lightning at $N.", FALSE, obj->loc.wearing, obj,
		    vict, TO_NOTVICT);
		spell_lightning_bolt(61, ch, 0, SPELL_TYPE_SPELL, vict, 0);
		// Stop attack if one dies.
		if (!IS_ALIVE(ch) || !IS_ALIVE(vict))
		{
			return TRUE;
		}
	}
	return FALSE;
}

int lightning_armor(P_obj obj, P_char ch, int cmd, char *arg)
{
	struct proc_data *data;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	// 1/30 chance on a hit.
	if (cmd != CMD_GOTHIT || number(0, 29) ||
	    !(data = legacy_proc_arg<struct proc_data *>(arg)))
	{
		return FALSE;
	}

	victim = data->victim;
	if (!IS_ALIVE(victim))
	{
		return FALSE;
	}
	act("&+L$n's $q &+bfl&+Bar&+Wes &+Bup &+bat &+L$N's &+chit &+Land &=LBZAPPPPS &N&+W$M&+L!&N",
	    TRUE, ch, obj, victim, TO_NOTVICT);
	act("&+L$n's $q &+bfl&+Bar&+Wes &+Bup &+bat &+Lyour &+chit &+Land &=LBZAPPPPS &N&+WYOU&+L!&N",
	    TRUE, ch, obj, victim, TO_VICT);
	act("&+LYour $q &+bfl&+Bar&+Wes &+Bup &+bat &+L$N's &+chit &+Land &=LBZAPPPPS &N&+W$M&+L!&N",
	    TRUE, ch, obj, victim, TO_CHAR);
	spell_lightning_bolt(60, ch, NULL, 0, victim, 0);
	if (IS_ALIVE(victim) && IS_ALIVE(ch))
	{
		spell_call_lightning(60, ch, victim, 0);
	}
	return TRUE;
}

int imprison_armor(P_obj obj, P_char ch, int cmd, char *arg)
{
	struct proc_data *data;
	struct affected_type af;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (cmd != CMD_GOTHIT) // || number(0,30) )
	{
		return FALSE;
	}
	if (!(data = legacy_proc_arg<struct proc_data *>(arg)))
	{
		return FALSE;
	}
	if (!(victim = data->victim))
	{
		return FALSE;
	}

	if (!check_freedom_of_movement(victim, number(0, 1)))
	{
		act("&+L$n's $q &+bfl&+Bar&+Wes &+Bup &+bat &+L$N's &+chit &+Land &=LBZAPPPPS &N&+W$M&+L!&N",
		    TRUE, ch, obj, victim, TO_NOTVICT);
		act("&+L$n's $q &+bfl&+Bar&+Wes &+Bup &+bat &+Lyour &+chit &+Land &=LBZAPPPPS &N&+WYOU&+L!&N",
		    TRUE, ch, obj, victim, TO_VICT);
		act("&+LYour $q &+bfl&+Bar&+Wes &+Bup &+bat &+L$N's &+chit &+Land &=LBZAPPPPS &N&+W$M&+L!&N",
		    TRUE, ch, obj, victim, TO_CHAR);
		af.type = SPELL_MAJOR_PARALYSIS;
		af.duration = 5;
		af.bitvector2 = AFF2_MAJOR_PARALYSIS;
		affect_to_char(victim, &af);

		if (IS_FIGHTING(victim))
		{
			stop_fighting(victim);
		}
	}

	return TRUE;
}

int god_bp(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char tch;
	char Gbuf1[MAX_STRING_LENGTH];
	char getname[MAX_STRING_LENGTH];
	int rr;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd != CMD_GET)
		return FALSE;

	arg = one_argument(arg, Gbuf1); // multicoin
	if (!*Gbuf1)
		return FALSE;

	if (!IS_TRUSTED(ch))
	{
		wizlog(MINLVLIMMORTAL, "%s has a god backpack in [%d]", GET_NAME(ch),
		       world[ch->in_room].number);
		return FALSE;
	}

	// check if gbuf2 is a playername in the room
	for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
	{
		snprintf(getname, MAX_STRING_LENGTH, "%s", GET_NAME(tch));
		for (rr = 0; *(getname + rr) != '\0'; rr++)
			getname[rr] = LOWER(*(getname + rr));

		//    if (!isname(Gbuf1, getname))
		//          return FALSE;

		if (isname(Gbuf1, getname))
		{ // we have a winner!
			if ((GET_RACE(tch) == RACE_GOBLIN) || (GET_RACE(tch) == RACE_GNOME))
			{
				act("$n picks up $N.", FALSE, ch, obj, tch, TO_NOTVICT);
				act("$n picks you up.", FALSE, ch, obj, tch, TO_VICT);
				act("You pick up $N.", FALSE, ch, obj, tch, TO_CHAR);
				act("$n opens $s &+ya large leather backpack&N.", FALSE, ch, obj,
				    tch, TO_NOTVICT);
				act("$n opens $s &+ya large leather backpack&N.", FALSE, ch, obj,
				    tch, TO_VICT);
				act("You open &+ya large leather backpack&N.", FALSE, ch, obj, tch,
				    TO_CHAR);
				act("$n throws $N into $s &+ya large leather backpack&N.", FALSE,
				    ch, obj, tch, TO_NOTVICT);
				act("$n throws YOU into $s &+ya large leather backpack&N.", FALSE,
				    ch, obj, tch, TO_VICT);
				act("You throw $N into &+ya large leather backpack&N.", FALSE, ch,
				    obj, tch, TO_CHAR);
				char_from_room(tch);
				char_to_room(tch, real_room(501), -1);
				//         act("The flap is opened up.", FALSE, ch, obj, 0, TO_ROOM);
				//         act("$N is thrown in by a huge hand.", FALSE, ch, obj, tch, TO_ROOM);
				//         act("The flap closes quickly.", FALSE, ch, obj, 0, TO_ROOM);
				return TRUE;
			}
		}
	}
	return 0;
}

int out_of_god_bp(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char tch = NULL;
	char Gbuf1[MAX_STRING_LENGTH];
	char whee[MAX_STRING_LENGTH];
	int rr, target;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd != CMD_TUG)
		return FALSE;

	arg = one_argument(arg, Gbuf1);
	if (!*Gbuf1)
		return FALSE;

	// check if gbuf1 is flap
	checked_snprintf(whee, MAX_STRING_LENGTH, "%s", Gbuf1);
	for (rr = 0; *(whee + rr) != '\0'; rr++)
		whee[rr] = LOWER(*(whee + rr));

	if (!isname(whee, "flap"))
		return FALSE;

	target = real_room(world[ch->specials.was_in_room].number);

	if (isname(whee, "flap"))
	{ // we have a winner!
		if (number(0, 4) == 4)
		{ // let em out heh
			act("$n opens the leather flap with a heroic show of might.", FALSE, ch,
			    obj, tch, TO_NOTVICT);
			act("$n climbs out and quickly closes the flap behind $m.", FALSE, ch, obj,
			    tch, TO_NOTVICT);
			act("You summon up all the strength within you and open the &+yflap&N.",
			    FALSE, ch, obj, tch, TO_CHAR);
			act("You quickly climb out and close the &+yflap&N behind you.", FALSE, ch,
			    obj, tch, TO_CHAR);

			char_from_room(ch);
			char_to_room(ch, real_room(target), -1);

			act("$n quickly climbs out of &+ya large leather backpack&N, closing the flap behind $m.",
			    FALSE, ch, obj, 0, TO_ROOM);
			return TRUE;
		}
		else
		{ // they failed haha
			act("$n attempts to open &+ya large leather backpack&N's huge flap but fails miserably, falling on $m ass.",
			    FALSE, ch, obj, tch, TO_NOTVICT);
			act("You attempt open &+ya large leather backpack&N but fail miserably, falling on your ass.",
			    FALSE, ch, obj, tch, TO_CHAR);
			Stun(ch, ch, (dice(1, 3) * PULSE_VIOLENCE), FALSE);
			SET_POS(ch, POS_PRONE + GET_STAT(ch));
			CharWait(ch, PULSE_VIOLENCE * 1);
		}
	}

	return 0;
}

int lyrical_instrument_of_time(P_obj obj, P_char ch, int cmd, char * /*argument*/)
{
	int rand;
	int curr_time;
	P_char temp_ch;
	P_char vict;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;
	if (cmd != 0)
		return FALSE;

	if (!obj)
		return FALSE;

	temp_ch = ch;

	if (!OBJ_WORN(obj))
		return FALSE;

	if (!ch)
	{
		if (OBJ_WORN(obj) && obj->loc.wearing)
			temp_ch = obj->loc.wearing;
		else
			return FALSE;
	}

	if (!temp_ch || !SINGING(temp_ch))
		return FALSE;

	if (!(obj == temp_ch->equipment[HOLD]))
		return FALSE;

	curr_time = time(NULL);

	if (!IS_ROOM(temp_ch->in_room, ROOM_NO_MAGIC))
	{
		if (obj->timer[0] + 30 <= curr_time)
		{
			obj->timer[0] = curr_time;
			if (GET_HIT(temp_ch) < GET_MAX_HIT(temp_ch))
			{
				act("&+L$n&+L's $q &+Yglows&+L and $n &+Lis bathed in a healing aura.&N",
				    FALSE, temp_ch, obj, 0, TO_ROOM);
				act("&+LYour $q &+Yglows&+L and bathes you in a healing aura.&N",
				    FALSE, temp_ch, obj, 0, TO_CHAR);

				bard_healing(60, temp_ch, temp_ch, SONG_HEALING);

				return (FALSE);
			}
			else
			{
				bard_protection(60, temp_ch, temp_ch, SONG_PROTECTION);

				return (FALSE);
			}
		}
	}

	if (IS_FIGHTING(temp_ch) && !number(0, 2))
	{
		vict = GET_OPPONENT(temp_ch);

		if (!vict)
			return FALSE;

		rand = number(0, 5);

		act("&+LYou point your $p &+Lat &+W$N&+L.&N", TRUE, temp_ch, obj, vict, TO_CHAR);
		act("&+L$n points $p &+Lat &+W$N&+L.&N", TRUE, temp_ch, obj, vict, TO_NOTVICT);
		act("&+L$n points $p &+Lat &+Wyou&+L!&N", TRUE, temp_ch, obj, vict, TO_VICT);

		switch (rand)
		{
		case 0:
			bard_storms(60, temp_ch, vict, SONG_STORMS);
			break;
		case 1:
			bard_chaos(60, temp_ch, vict, SONG_CHAOS);

			break;
		case 2:
			bard_harming(60, temp_ch, vict, SONG_HARMING);

			break;
		case 3:
			bard_harming(60, temp_ch, vict, SONG_HARMING);
			break;
		case 4:
			bard_cowardice(60, temp_ch, vict, SONG_COWARDICE);
			break;
		case 5:
			bard_calm(60, temp_ch, vict, SONG_CALMING);
			break;
		}
		return FALSE;
	}
	return FALSE;
}

int huntsman_ward(P_obj obj, P_char ch, int cmd, char *argument)
{
	P_char tch;
	int dam, item;
	char buf[256];

	item = OBJ_VNUM(obj);

	if (cmd == CMD_HIDE)
	{
		one_argument(argument, buf);

		if (GET_CHAR_SKILL(ch, SKILL_TRAP) && OBJ_ROOM(obj) &&
		    obj == get_obj_in_list_vis(ch, buf, world[ch->in_room].contents))
		{
			act("You arm $p and hide it from the eyes of any trespassers.", FALSE, ch,
			    obj, NULL, TO_CHAR);
			act("$n arms $p and hides it from the eyes of any trespassers.", FALSE, ch,
			    obj, NULL, TO_ROOM);
			SET_BIT(obj->extra_flags, ITEM_SECRET);
			set_obj_affected(obj, 1800 * WAIT_SEC, TAG_OBJ_DECAY, 0);
			CharWait(ch, PULSE_VIOLENCE);
			if (IS_PC(ch))
			{
				obj->value[0] = GET_PID(ch);
			}
			else
			{
				obj->value[0] = GET_RNUM(ch);
			}
			return TRUE;
		}
	}

	if (cmd == CMD_FOUND)
	{
		act("You succesfully disarmed $p hidden here.", FALSE, ch, obj, 0, TO_CHAR);
		disarm_obj_nevents(obj, NULL);
		obj->value[0] = 0;
		return TRUE;
	}

	if (number(0, 2) && IS_SET(obj->extra_flags, ITEM_SECRET) && cmd_to_exitnumb(cmd) != -1 &&
	    obj->value[0] && OBJ_ROOM(obj) && (IS_PC(ch) && obj->value[0] != GET_PID(ch)) &&
	    !IS_TRUSTED(ch))
	{
		for (tch = character_list; tch; tch = tch->next)
		{
			if (IS_PC(tch) && GET_PID(tch) == obj->value[0])
			{
				break;
			}
		}

		if (item == 54)
		{
			if (tch != NULL && ch->in_room != tch->in_room && !grouped(ch, tch))
			{
				snprintf(buf, 256, "$N has trespassed in %s!",
					 world[ch->in_room].name);
				act(buf, FALSE, tch, 0, ch, TO_CHAR);
				REMOVE_BIT(obj->extra_flags, ITEM_SECRET);
				obj->value[0] = 0;
				disarm_obj_nevents(obj, NULL);

				if (number(0, 120) < GET_C_INT(ch))
				{
					act("You notice you just broke a &+Wthin string&n attached to $p!",
					    FALSE, ch, obj, 0, TO_CHAR);
				}
				extract_obj(obj, TRUE); // Not an arti, but 'in game.'
			}

			return FALSE;
		}

		if (item == 77)
		{
			if (tch != NULL && ch->in_room != tch->in_room && !grouped(ch, tch))
			{
				REMOVE_BIT(obj->extra_flags, ITEM_SECRET);
				obj->value[0] = 0;
				disarm_obj_nevents(obj, NULL);

				// PHSDAM_NOREDUCE -> 5d5 damage total.
				dam = dice(5, 5);

				act("Without warning, a hidden trap sends a flurry of tiny &+Lblack&n darts piercing $n's &+rflesh&n.",
				    FALSE, ch, obj, 0, TO_NOTVICT);
				act("Without warning, a hidden trap sends a flurry of tiny &+Lblack&n darts piercing your &+rflesh&n.",
				    FALSE, ch, obj, 0, TO_CHAR);

				melee_damage(tch, ch, dam,
					     PHSDAM_NOREDUCE | PHSDAM_NOSHIELDS |
						     PHSDAM_NOPOSITION | PHSDAM_NOENGAGE,
					     0);

				extract_obj(obj, TRUE); // Not an arti, but 'in game.'
			}
			return FALSE;
		}

		if (item == 400229)
		{
			struct affected_type af;
			if (tch != NULL && ch->in_room != tch->in_room && !grouped(ch, tch))
			{
				snprintf(buf, 256,
					 "$N &+yhas sprung your &+rcrippling &+ytrap at&n %s!",
					 world[ch->in_room].name);
				act(buf, FALSE, tch, 0, ch, TO_CHAR);
				REMOVE_BIT(obj->extra_flags, ITEM_SECRET);
				obj->value[0] = 0;
				disarm_obj_nevents(obj, NULL);

				if (number(0, 120) < GET_C_INT(ch))
				{
					act("You notice you just broke a &+Wthin string&n attached to $p!",
					    FALSE, ch, obj, 0, TO_CHAR);
				}

				memset(&af, 0, sizeof(af));

				af.type = TAG_CRIPPLED;
				af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL;
				af.duration = 40;
				affect_to_char(ch, &af);
				act("&+ROUCH!!&+y Without warning, a &+rrusty &+yclamp suddenly tears at your legs!&n",
				    FALSE, ch, 0, 0, TO_CHAR);
				act("$n &+ywinces in &+ragony &+yas a &+rrusty &+yclamp suddenly tears at their legs!&n",
				    FALSE, ch, 0, 0, TO_ROOM);

				// 3-7 * ~5 damage = 15-35 damage total.
				int numb = number(3, 7);
				add_event(event_bleedproc, PULSE_VIOLENCE, tch, ch, 0, 0, &numb,
					  sizeof(numb));
				extract_obj(obj, TRUE); // Not an arti, but 'in game.'
			}
			return FALSE;
		}
		if (item == 73)
		{
			if (tch != NULL && ch->in_room != tch->in_room && !grouped(ch, tch))
			{
				REMOVE_BIT(obj->extra_flags, ITEM_SECRET);
				obj->value[0] = 0;
				disarm_obj_nevents(obj, NULL);

				act("Without warning, a hidden trap injects a large dose of &+gpoison&n into $n's &+rflesh&n.",
				    FALSE, ch, obj, 0, TO_NOTVICT);
				act("Without warning, a hidden trap injects a large dose of &+gpoison&n into your &+rflesh&n.",
				    FALSE, ch, obj, 0, TO_CHAR);

				spell_poison(56, ch, 0, 0, ch, NULL);

				extract_obj(obj, TRUE); // Not an arti, but 'in game.'
			}
			return FALSE;
		}
	}
	return FALSE;
}

int necro_specpet_bone(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict;
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 15) == 1))
	{
		vict = GET_OPPONENT(ch);

		act("$n&+L cackles with delight!", FALSE, ch, 0, vict, TO_ROOM);
		spell_energy_drain(GET_LEVEL(ch), ch, NULL, SPELL_TYPE_SPELL, vict, 0);
	}
	return FALSE;
}

int necro_specpet_flesh(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict;

	struct damage_messages acid_blood = {
		"&+L$N &+Lwrithes in agony the black blood greedily eats into $S &+rflesh.&n",
		"&+LThe world &+rexp&+Rlo&+rdes in p&+Ra&+rin &+Las the black blood greedily eats into your &+rflesh.",
		"&+L$N &+Lwrithes in agony the black blood greedily eats into $S &+rflesh.&n",
		"What was once $N, but now only a mass of burnt flesh, crumbles in a heap on the ground.",
		"A pain beyond imagination overwhelms you as the black blood eats its ways into your heart.",
		"What was once $N, but now only a mass of burnt flesh, crumbles in a heap on the ground."
	};

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (IS_FIGHTING(ch) && (cmd == 0) && !number(0, 19))
	{
		vict = GET_OPPONENT(ch);

		act("&+LBlack blood &+wspurts from your wound as $N&+w's weapon &+wrips your &+rflesh.&n",
		    FALSE, ch, 0, vict, TO_CHAR);
		act("&+LBlack blood &+wspurts from $n &+was your weapon &+wrips $s &+rflesh.&n",
		    FALSE, ch, 0, vict, TO_VICT);
		act("&+LBlack blood &+wspurts from $n &+was $N&+w's weapon &+wrips $s &+rflesh.&n",
		    FALSE, ch, 0, vict, TO_NOTVICT);

		if ((15 + GET_C_AGI(vict) / 6) > number(0, 100))
		{
			act("&+w$N &+wjumps out of the way barely avoiding the &+rsp&+Ru&+rr&+Rt&+r of b&+Rlo&+rod.",
			    FALSE, vict, 0, ch, TO_CHAR);

			act("&+wYou jump out of the way barely avoiding the &+rsp&+Ru&+rr&+Rt&+r of b&+Rlo&+rod.",
			    FALSE, vict, 0, ch, TO_VICT);

			act("&+w$N &+wjumps out of the way barely avoiding the &+rsp&+Ru&+rr&+Rt&+r of b&+Rlo&+rod.",
			    FALSE, vict, 0, ch, TO_NOTVICT);
		}
		else
		{
			spell_damage(ch, vict, 40 + number(1, 40), SPLDAM_ACID,
				     SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &acid_blood);
		}
	}

	return FALSE;
}

int conj_specpet_xorn(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 10) == 1))
	{
		vict = GET_OPPONENT(ch);
		if ((GET_SIZE(vict) >= SIZE_TINY) && (GET_SIZE(vict) <= SIZE_GIANT))
		{
			if ((GET_POS(vict) == POS_PRONE) || (GET_POS(vict) == POS_SITTING) ||
			    (GET_POS(vict) == POS_KNEELING))
			{
				act("$n&+y rears up and dives into the ground sending fragments flying.",
				    FALSE, ch, 0, vict, TO_NOTVICT);
				act("&+yMoments later it bursts forth charging into $N.", FALSE, ch,
				    0, vict, TO_NOTVICT);
				act("$N &+ycannot be knocked down any further!", FALSE, ch, 0, vict,
				    TO_NOTVICT);

				act("$n&+y rears up and dives into the ground sending fragments flying.",
				    FALSE, ch, 0, vict, TO_VICT);
				act("&+yMoments later it bursts forth charging into you!", FALSE,
				    ch, 0, vict, TO_VICT);
				act("&+yYou cannot be knocked down any further!", FALSE, ch, 0,
				    vict, TO_VICT);
			}
			else
			{
				act("$n&+y rears up and dives into the ground sending fragments flying.",
				    FALSE, ch, 0, vict, TO_NOTVICT);
				act("&+yMoments later it bursts forth charging into an $N.", FALSE,
				    ch, 0, vict, TO_NOTVICT);
				act("$N &+yis flung to the ground by $n!", FALSE, ch, 0, vict,
				    TO_NOTVICT);

				act("$n&+y rears up and dives into the ground sending fragments flying.",
				    FALSE, ch, 0, vict, TO_VICT);
				act("&+yMoments later it bursts forth charging into you!", FALSE,
				    ch, 0, vict, TO_VICT);
				act("&+yYou are flung to the ground by $n!", FALSE, ch, 0, vict,
				    TO_VICT);
				SET_POS(vict, POS_SITTING + GET_STAT(vict));
				CharWait(vict, PULSE_VIOLENCE * 1);
			}
		}
		else
		{
			act("$n&+y rears up and dives into the ground sending fragments flying.",
			    FALSE, ch, 0, vict, TO_NOTVICT);
			act("&+yMoments later it bursts forth charging into an $N.", FALSE, ch, 0,
			    vict, TO_NOTVICT);
			act("$n&+y rears up and dives into the ground sending fragments flying.",
			    FALSE, ch, 0, vict, TO_VICT);
			act("&+yMoments later it bursts forth charging into you!.", FALSE, ch, 0,
			    vict, TO_VICT);

			if (GET_SIZE(vict) > SIZE_GIANT)
			{
				act("$N &+yis simply too large to be knocked down by $n!", FALSE,
				    ch, 0, vict, TO_NOTVICT);
				act("&+yYou are simply too large to be knocked down by $n!", FALSE,
				    ch, 0, vict, TO_VICT);
			}
			if (GET_SIZE(vict) < SIZE_TINY)
			{
				act("$N &+yis simply too small to be knocked down by $n!", FALSE,
				    ch, 0, vict, TO_NOTVICT);
				act("&+yYou are simply too small to be knocked down by $n!", FALSE,
				    ch, 0, vict, TO_VICT);
			}
		}
		return FALSE;
	}
	return FALSE;
}

int conj_specpet_golem(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict;
	int temp = 0;
	int healpoints, door, target_room;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 10) == 1))
	{
		vict = GET_OPPONENT(ch);
		// whee bearhug!

		if (vict->in_room != ch->in_room)
			return false;

		if (GET_SIZE(vict) > SIZE_GIANT)
			return FALSE;

		if (GET_SIZE(vict) <= SIZE_SMALL)
		{
			act("&+y$n &+ygrabs &+Y$N&+y by the &+Yhead &+yand &+Wthrows &+y$M&+y!",
			    FALSE, ch, 0, vict, TO_NOTVICT);
			act("&+y$n &+ygrabs &+Yyou&+y by the &+Yhead &+yand &+Wthrows &+yyou!",
			    FALSE, ch, 0, vict, TO_VICT);

			door = number(0, 9);

			if ((door == DIR_UP) || (door == DIR_DOWN))
				door = number(0, 3);

			if ((CAN_GO(vict, door)) && (!check_wall(vict->in_room, door)))
			{
				act("$n &+yflings &+Yyou &+yout of the room!", FALSE, ch, 0, vict,
				    TO_VICT);
				act("$n &+yflings &+Y$N &+yout of the room!", FALSE, ch, 0, vict,
				    TO_NOTVICT);
				target_room = world[vict->in_room].dir_option[door]->to_room;
				char_from_room(vict);
				if (char_to_room(vict, target_room, -1))
				{
					act("$n &+Yflies in &+Yface first&+y, crashing on the floor!",
					    TRUE, vict, 0, 0, TO_ROOM);
					stop_fighting(vict);
					SET_POS(vict, POS_SITTING + GET_STAT(vict));
					CharWait(vict, PULSE_VIOLENCE * 1);
				}
			}
			else
			{
				act("&+Y$N &+yflies into the &+Ywall &+ybreaking &+wbones &+yand landing with a stunning &+Yforce&+y.",
				    FALSE, ch, 0, vict, TO_NOTVICT);
				act("&+YYou &+yfly into the &+Ywall &+ybreaking &+wbones &+yand land with a stunning &+Yforce&+y.",
				    FALSE, ch, 0, vict, TO_VICT);
				SET_POS(vict, POS_SITTING + GET_STAT(vict));
				healpoints = (number(1, 30));
				CharWait(vict, PULSE_VIOLENCE * 1);
				GET_HIT(vict) -= healpoints;
				update_pos(vict);
			}
		}

		if (!CanDoFightMove(ch, vict))
			return FALSE;

		if (GET_ALT_SIZE(vict) > GET_ALT_SIZE(ch))
		{
			act("$n tries to squeeze the life out of $N, but can't get a grip.", FALSE,
			    ch, 0, vict, TO_NOTVICT);
			act("You might as well try to hug a mountain!", FALSE, ch, 0, vict,
			    TO_CHAR);
			act("$n tried to wrap his arms around you, but of course failed.", FALSE,
			    ch, 0, vict, TO_VICT);
			CharWait(ch, PULSE_VIOLENCE * 2);
			return FALSE;
		}

		if (GET_ALT_SIZE(vict) < GET_ALT_SIZE(ch) - 2)
		{
			send_to_char("Don't hug that - simply swat it!\r\n", ch);
			return FALSE;
		}

		if (IS_SLIME(vict) || GET_RACE(vict) == RACE_AQUATIC_ANIMAL ||
		    GET_RACE(vict) == RACE_SNAKE /*|| GET_RACE(vict) == RACE_FLYING_ANIMAL */)
		{
			send_to_char("You just can't get a good grip on that.\r\n", ch);
			return FALSE;
		}

		if (GET_POS(vict) != POS_STANDING)
		{
			send_to_char("Your enemy is not standing, you can't grab him right.\r\n",
				     ch);
			return FALSE;
		}

		temp = (GET_LEVEL(vict) - GET_LEVEL(ch)) / 10 +
		       (GET_C_AGI(vict) - GET_C_AGI(ch)) / 15 +
		       (GET_C_STR(vict) - GET_C_STR(ch)) / 15;

		temp -= GET_AC(vict) / 20;

		if (IS_AFFECTED(vict, AFF_BLIND) || IS_AFFECTED(vict, AFF_KNOCKED_OUT) ||
		    IS_AFFECTED(vict, AFF_SLEEP) || IS_AFFECTED(vict, AFF_MEDITATE) ||
		    IS_AFFECTED2(vict, AFF2_SLOW) || IS_AFFECTED2(vict, AFF2_STUNNED))
			temp -= 5;
		if (IS_AFFECTED(ch, AFF_HASTE))
			temp -= 5;

		temp = BOUNDED(1, number(1, 101) + temp, 101);

		if (temp == 1)
		{
			act("$n&+y squeezes $N &+yso hard, you can hear $N&+y's bones creaking!",
			    FALSE, ch, 0, vict, TO_NOTVICT);
			act("You listen with satisfaction to the sound of $N's bones snapping.",
			    FALSE, ch, 0, vict, TO_CHAR);
			act("$n &+yis breaking your bones with $s deadly hug!", FALSE, ch, 0, vict,
			    TO_VICT);
			/* this really, really hurts... always better than usual bearhug */
			damage(ch, vict, 50 + GET_LEVEL(ch), SKILL_BEARHUG);
			return FALSE;
		}

		if (temp < 80)
		{
			act("$n &+ywraps $s &+YHUGE &+yarms around $N&+y, squeezing $M powerfully.",
			    FALSE, ch, 0, vict, TO_NOTVICT);
			act("You squeeze the living daylights out of $N.", FALSE, ch, 0, vict,
			    TO_CHAR);
			act("$n&+y's &+YHUGE &+yarms lock around you in a painful grip.", FALSE, ch,
			    0, vict, TO_VICT);
			damage(ch, vict, 25 + GET_LEVEL(ch), SKILL_BEARHUG);
			return FALSE;
		}
		else
		{
			act("$n &+ytries to wrap $s &+YHUGE &+yarms around $N&+y, but fails.",
			    FALSE, ch, 0, vict, TO_VICT);
			act("$N escaped right out of your arms. You must be getting slow.", FALSE,
			    ch, 0, vict, TO_CHAR);
			act("$n &+ytries to lock $s &+YHUGE &+yarms around you, but you escape easily.",
			    FALSE, ch, 0, vict, TO_VICT);
			CharWait(ch, PULSE_VIOLENCE * 2);
			return FALSE;
		}
		return FALSE;
	}
	return FALSE;
}

int conj_specpet_djinni(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char tch;
	P_char vict;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd == CMD_PERIODIC && (tch = get_linked_char(ch, LNK_PET)) != NULL && IS_PC(tch) &&
	    !strcmp(tch->player.name, "Orthyn") && !number(0, 500))
	{
		do_say(ch, writable_arg("AIDS!"), CMD_SAY);
	}

	if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 10) == 1))
	{
		vict = GET_OPPONENT(ch);
		if (vict->in_room != ch->in_room)
			return false;
		act("$n&+C suddenly picks up speed and &+Wtears &+Cthrough the room!", FALSE, ch, 0,
		    vict, TO_ROOM);
		for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
		{
			if ((!ch->group || ch->group != tch->group) && !number(0, 10))
			{
				if ((GET_SIZE(tch) > SIZE_GIANT))
				{
					act("$n&+C tries to &+Wthrow &+C$N &+Cbut they are simply too large to lift into the &+Wair&+C!",
					    FALSE, ch, 0, tch, TO_NOTVICT);
					act("$n&+C tries to &+Wthrow &+Cyou &+Cbut you are simply too large to lift into the &+Wair&+C!",
					    FALSE, ch, 0, tch, TO_VICT);
				}
				else
				{
					act("$n&+C &+Wthrows &+C$N in mid &+Wair &+Cand sends them flying &+Wupwards&+C!",
					    FALSE, ch, 0, tch, TO_NOTVICT);
					act("$N &+ccr&+Cash&+ces &+Cto the ground with a bone crunching &+Wthud&+C!",
					    FALSE, ch, 0, tch, TO_NOTVICT);

					act("$n&+C &+Wthrows &+Cyou in mid &+Wair &+Csending you flying &+Wupwards&+C!",
					    FALSE, ch, 0, tch, TO_VICT);
					act("&+CYou &+ccr&+Cas&+ch &+Cto the ground with a bone crunching &+Wthud&+C!",
					    FALSE, ch, 0, tch, TO_VICT);
					SET_POS(tch, POS_SITTING + GET_STAT(tch));
					CharWait(tch, PULSE_VIOLENCE * 1);
				}
			}
		}
	}
	return FALSE;
}

int conj_specpet_slyph(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict;
	int room;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (IS_ALIVE(ch) && IS_FIGHTING(ch) && cmd == CMD_MOB_MUNDANE && number(1, 10) == 1)
	{
		room = ch->in_room;
		vict = GET_OPPONENT(ch);
		if (vict->in_room != room)
		{
			return FALSE;
		}
		act("$n&+C gains a burst of &+Wenergy&+C!", FALSE, ch, 0, vict, TO_ROOM);
		spell_cyclone(45, ch, NULL, SPELL_TYPE_SPELL, vict, 0);
		// if (is_char_in_room(ch, room) && is_char_in_room(vict, room))
		// spell_cyclone(45, ch, NULL, SPELL_TYPE_SPELL, vict, 0);
	}
	return FALSE;
}

int conj_specpet_triton(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict = NULL;
	int healpoints = 432;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 15) == 1) &&
	    world[ch->in_room].sector_type != SECT_FIREPLANE &&
	    world[ch->in_room].sector_type != SECT_LAVA)
	{
		if (GET_HIT(ch) < GET_MAX_HIT(ch))
		{
			if ((healpoints + GET_HIT(ch)) >= GET_MAX_HIT(ch))
				healpoints = MAX(0, GET_MAX_HIT(ch) - GET_HIT(ch) - dice(1, 4));
			GET_HIT(ch) += healpoints;
			update_pos(ch);
			act("$n&+B stretches &+bout and lets loose a &+Btriumphant &+Whowl&+b!",
			    FALSE, ch, 0, vict, TO_ROOM);
			act("&+bAbsorbing &+Bmoisture &+bfrom its surroundings $n &+brebuilds its &+Bbody.&+b!",
			    FALSE, ch, 0, vict, TO_ROOM);
		}
	}
	return FALSE;
}

int conj_specpet_undine(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict;
	int healpoints = 50;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 10) == 1))
	{
		vict = GET_OPPONENT(ch);

		if (vict->in_room != ch->in_room)
			return false;

		if ((GET_HIT(vict) - healpoints) <= 0)
			healpoints = (GET_HIT(vict) - dice(1, 4));
		GET_HIT(vict) -= healpoints;
		update_pos(vict);
		act("&+bWith &=LBlightning&N&+B speed $n&+b flows forward, choking &+B$N&+b!",
		    FALSE, ch, 0, vict, TO_NOTVICT);
		act("&+bWith &=LBlightning&N&+B speed $n&+b flows forward, choking &+BYOU&+b!",
		    FALSE, ch, 0, vict, TO_VICT);
		if (number(1, 10) == 1)
		{
			spell_dread_wave(45, ch, NULL, SPELL_TYPE_SPELL, vict, NULL);
		}
	}
	return FALSE;
}

int conj_specpet_serpent(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (IS_FIGHTING(ch) && (cmd == 0) && (number(1, 10) == 1) &&
	    world[ch->in_room].sector_type != SECT_WATER_PLANE)
	{
		vict = GET_OPPONENT(ch);
		if (vict->in_room != ch->in_room)
			return false;

		act("$n &+ropens its &+Rjaws &+rsucking in a deep &+Rbreath&+r!", FALSE, ch, 0,
		    vict, TO_NOTVICT);
		act("$n &+ropens its &+Rjaws &+rsucking in a deep &+Rbreath&+r!", FALSE, ch, 0,
		    vict, TO_VICT);
		act("&+rA &+WGIGANTIC &+Rfireball &+rshoots forward totally &+Renveloping &+R$N&+r!",
		    FALSE, ch, 0, vict, TO_NOTVICT);
		act("&+rA &+WGIGANTIC &+Rfireball &+rshoots forward totally &+Renveloping &+Ryou&+r!",
		    FALSE, ch, 0, vict, TO_VICT);
		spell_solar_flare(60, ch, NULL, 0, vict, NULL);
	}
	return FALSE;
}

int necro_dracolich(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_obj t_obj, next;
	int i;

	if (!cmd && !GET_MASTER(ch))
	{
		if (!number(0, 2))
		{
			act("As the magic binding $n vanished, $e returns to unlife.", FALSE, ch, 0,
			    0, TO_ROOM);
			die(ch, ch);
		}
		return TRUE;
	}

	if (cmd == CMD_DEATH)
	{
		check_saved_corpse(ch);
		for (t_obj = ch->carrying; t_obj; t_obj = next)
		{
			next = t_obj->next_content;
			obj_from_char(t_obj);
			obj_to_room(t_obj, ch->in_room);
		}
		for (i = 0; i < MAX_WEAR; i++)
			if (ch->equipment[i])
				obj_to_room(unequip_char(ch, i), ch->in_room);

		if (GET_VNUM(ch) != 1201)
			act("$n collapses into a pile of &+Wbones&n which crumble to dust.", FALSE,
			    ch, 0, 0, TO_ROOM);
		else
			act("$n crumbles to dust.", FALSE, ch, 0, 0, TO_ROOM);
		return TRUE;
	}

	return FALSE;
}

/*
 * The salesman, always after a tidy profit and the satisfaction of a sale, is
 * a social individual...Maybe overly so... <grin>
 */

int sales_spec(P_char ch, P_char pl, int cmd, char *arg)
{
	int i_val = 0;
	P_obj selling = 0, s_item = 0, obj = 0;
	P_char c_obj = 0, k, dummy_char, old_follow = 0;
	char Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];
	char Gbuf4[MAX_STRING_LENGTH];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	/*
	 * Find out if we have anything to sell, either in first equipment slot
	 */
	/*
	 * or something being carried
	 */

	if ((selling = ch->equipment[WIELD]))
		i_val = ch->equipment[WIELD]->cost * 2 + 3;
	else if ((selling = ch->carrying))
		i_val = ch->carrying->cost * 2 + 3;
	/*
	 * Handle cases where we might care what a player does: buy, steal, etc.
	 */
	if (cmd != 0)
	{
		argument_interpreter(arg, Gbuf1, Gbuf2);
		if (*Gbuf1)
			c_obj = get_char_room(Gbuf1, ch->in_room);
		switch (cmd)
		{
		case CMD_KISS:
		case CMD_FONDLE:
		case CMD_GROPE:
		case CMD_LICK:
		case CMD_LOVE:
		case CMD_NIBBLE:
		case CMD_SQUEEZE:
		case CMD_FRENCH:
			/*
				 * Actions of questionable intent
				 */
			if ((c_obj != ch) || (!IS_AWAKE(ch)))
				return FALSE;
			if (pl->player.sex != 2)
			{
				act("$N slaps you before you can even begin.", FALSE, pl, 0, ch,
				    TO_CHAR);
				act("$N slaps $n for his naughty intentions.", FALSE, pl, 0, ch,
				    TO_ROOM);
			}
			return TRUE;
			break;
		case CMD_TELL:
			if ((c_obj != ch) || (!IS_AWAKE(ch)))
				return FALSE;
			mobsay(ch, "I can talk all day, but only you can buy...");
			return TRUE;
			break;
		case CMD_SMILE:
			/*
				 * The salesman suffers from acceptance anxiety :-)
				 */
			if (!IS_AWAKE(ch))
				return FALSE;
			do_action(pl, Gbuf1, cmd);
			act("$n smiles too, trying to join in on the fun.", TRUE, ch, 0, 0,
			    TO_ROOM);
			return TRUE;
			break;
		case CMD_INSULT:
			if ((c_obj != ch) || (!IS_AWAKE(ch)))
				return FALSE;
			do_insult(pl, arg, 0);
			act("$N humbles you with a greater insult.", FALSE, pl, 0, ch, TO_CHAR);
			act("$N snaps back with a greater insult.", FALSE, pl, 0, ch, TO_ROOM);
			return TRUE;
			break;
		case CMD_WAKE:
			if (c_obj != ch)
				return FALSE;
			if (GET_STAT(ch) != STAT_SLEEPING)
				act("$N is not sleeping.", FALSE, pl, 0, ch, TO_CHAR);
			else
			{
				act("Your gentle nudging awakens $N, who yawns and clears his throat.",
				    FALSE, pl, 0, ch, TO_CHAR);
				act("$n nudges $N awake.", FALSE, pl, 0, ch, TO_ROOM);
				SET_POS(ch, POS_STANDING + STAT_NORMAL);
			}
			return TRUE;
			break;
		case CMD_POKE:
			/*
				 * An exchange of poking
				 */
			if ((c_obj != ch) || (!IS_AWAKE(ch)))
				return FALSE;
			do_action(pl, arg, CMD_POKE);
			act("$N pokes you back.", FALSE, pl, 0, ch, TO_CHAR);
			act("$N pokes $n back.", TRUE, pl, 0, ch, TO_ROOM);
			return TRUE;
			break;
		case CMD_BACKSTAB:
			if ((c_obj != ch) || (!IS_AWAKE(ch)))
				return FALSE;
			act("Oof! $N knocks you down.", FALSE, pl, 0, ch, TO_CHAR);
			act("$n tries to backstab $N who, more alert than $E appears, slams\r\n"
			    "$m down on the ground instead!",
			    FALSE, pl, 0, ch, TO_ROOM);
			mobsay(ch, "I've dealt with your kind before!");
			SET_POS(pl, POS_SITTING + GET_STAT(pl));
			return TRUE;
			break;
		case CMD_BUY:
			/*
				 * "To buy is to be" is the salesman's motto...
				 */
			if ((strlen(Gbuf1) == 0) || (!IS_AWAKE(ch)) || !selling)
				return FALSE;
			if (c_obj == ch)
			{
				mobsay(ch, "I'm not for sale, idiot!");
				return TRUE;
			}
			if (!generic_find(Gbuf1, FIND_OBJ_EQUIP, ch, &dummy_char, &s_item))
				s_item = get_obj_in_list(Gbuf1, selling);
			if (strlen(Gbuf2) == 0)
			{
				if (!get_char_room_vis(pl, "2.salesman"))
				{
					if (s_item == selling)
					{
						if (transact(ch, s_item, pl, i_val))
						{
							if (ch->following)
								stop_follower(ch);
							REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
						}
					}
					else if (s_item)
					{
						snprintf(
							Gbuf4, MAX_STRING_LENGTH,
							"$n grins and says, 'Buy the %s first, and then I'll\r\n"
							"consider selling you the %s.",
							selling->short_description,
							s_item->short_description);
						act(Gbuf4, FALSE, ch, 0, 0, TO_ROOM);
					}
					else
						mobsay(ch, "Sell you what?");
					return (TRUE);
				}
				else if (ch == get_char_room_vis(ch, "salesman"))
				{
					snprintf(
						Gbuf4, MAX_STRING_LENGTH,
						"$n arches an eyebrow and says, 'Who 'ya talkin' to, %s?'\r\n"
						"Use: BUY <OBJ> [FROM] <SELLER>.",
						pl->player.name);
					act(Gbuf4, FALSE, ch, 0, 0, TO_ROOM);
				}
				else
					mobsay(ch, "Yeah, we can't ALL oblige you, ya know?");
			}
			else
			{
				if (ch == get_char_room_vis(pl, Gbuf2))
				{
					if (s_item == selling)
					{
						if (transact(ch, s_item, pl, i_val))
						{
							if (ch->following)
								stop_follower(ch);
							REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
						}
					}
					else
						mobsay(ch, "I don't have that.");
					return (TRUE);
				}
			}
			break;
		default:
			return FALSE;
			break;
		}
	}
	else if (IS_LIGHT(ch->in_room))
	{
		if (GET_STAT(ch) == STAT_SLEEPING)
		{
			SET_POS(ch, POS_STANDING + STAT_NORMAL);
			act("$n becomes alert, ready to assault any unsuspecting "
			    "persons with $s salesmanship.",
			    TRUE, ch, 0, 0, TO_ROOM);
		}
		if (ch->following)
		{
			if (!selling)
			{
				/*
				 * No items to sell, must have had item stolen
				 */
				act("$n looks puzzled.", TRUE, ch, 0, 0, TO_ROOM);
				stop_follower(ch);
				REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
				return (TRUE);
			}
			i_val = selling->cost * 2 + 4;
			if (ch->following->in_room == ch->in_room)
			{
				if (ch->only.npc->spec[0] > 108)
				{ /*
				   * Number is not fixed in stone
				   *
				   */
					act("$n throws $s hands up in disgust.", TRUE, ch, 0, 0,
					    TO_ROOM);
					old_follow = ch->following;
					stop_follower(ch);
					REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
					return TRUE;
				}
				else
				{
					if (!IS_AWAKE(ch->following))
						return (0);
					Gbuf4[0] = 0;
					switch (number(0, 14))
					{
					case 0:
						if (!(selling))
						{
							act("$n looks puzzled.", TRUE, ch, 0, 0,
							    TO_ROOM);
							if (ch->following)
							{
								stop_follower(ch);
							}
							REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
							return TRUE;
						}
						act("$n says 'I tell you this $o is of the finest quality.'",
						    FALSE, ch, selling, 0, TO_ROOM);
						break;
					case 1:
						snprintf(Gbuf4, MAX_STRING_LENGTH,
							 "Only %d coppers - a bargain!", i_val);
						do_say(ch, Gbuf4, 0);
						break;
					case 2:
						Gbuf4[0] = 'a';
						act("$N splutters 'You know, I have six hungry wives\r\n"
						    "and a child to feed...'",
						    FALSE, pl, 0, ch, TO_CHAR);
						break;
					case 3:
						act("$N waves the $p in your face.", FALSE,
						    ch->following, selling, ch, TO_CHAR);
						act("$N waves the $p in $n's face.", FALSE,
						    ch->following, selling, ch, TO_ROOM);
						break;
					case 4:
						if (!(selling))
						{
							act("$n looks puzzled.", TRUE, ch, 0, 0,
							    TO_ROOM);
							if (ch->following)
							{
								stop_follower(ch);
							}
							REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
							return TRUE;
						}
						act("The salesman demonstrates the unique usefulness of the $o.",
						    TRUE, ch, selling, 0, TO_ROOM);
						break;
					case 5:
						do_action(ch, 0, CMD_CHUCKLE);
						break;
					default:
						break;
					}
					if (Gbuf4[0])
					{
						if (!(selling))
						{
							act("$n looks puzzled.", TRUE, ch, 0, 0,
							    TO_ROOM);
							if (ch->following)
							{
								stop_follower(ch);
							}
							REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
							return TRUE;
						}
						act("$n tries to sell the $o to $N.", FALSE, ch,
						    selling, ch->following, TO_NOTVICT);
						ch->only.npc->spec[0] += 1;
					}
				}
				return TRUE;
			}
			else
			{
				stop_follower(ch);
				REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
				return TRUE;
			}
		}
		if (!ch->following && !selling)
		{
			SET_BIT(ch->specials.act, ACT_SCAVENGER);
			if (number(0, 5) == 0)
			{
				act("$n scans the floor.", TRUE, ch, 0, 0, TO_ROOM);
				return TRUE;
			}
		}
		else if (!ch->following)
		{
			REMOVE_BIT(ch->specials.act, ACT_SCAVENGER);
			for (k = world[ch->in_room].people; (pl = k); k = k->next_in_room)
			{
				if ((number(0, 4) < 3) && CAN_SEE(ch, pl) &&
				    !circle_follow(ch, pl) && (old_follow != pl))
				{
					add_follower(ch, pl);
					SET_BIT(ch->specials.act, ACT_SENTINEL);
					snprintf(
						Gbuf4, MAX_STRING_LENGTH,
						"The salesman saunters up to you and says, 'Hey %s!  Have I got a\r\n"
						"deal for you! Take a look at this magnificent $o.\r\n"
						"Isn't it just a dream?  And it can be yours for just %d coins!'",
						pl->player.name, i_val);
					if (!(selling))
					{
						act("$n looks puzzled.", TRUE, ch, 0, 0, TO_ROOM);
						if (ch->following)
						{
							stop_follower(ch);
						}
						REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
						return TRUE;
					}
					act(Gbuf4, FALSE, pl, selling, 0, TO_CHAR);
					act("$N makes a sales pitch to $n.", FALSE, pl, 0, ch,
					    TO_ROOM);
					ch->only.npc->spec[0] = 100;
					return TRUE;
					break;
				}
			}
		}
	}
	else if (IS_AWAKE(ch))
	{
		if (!ch->light)
		{ /*
		   * See if we have a light source
		   */
			for (obj = ch->carrying; obj; obj = obj->next_content)
			{
				if (obj->type == ITEM_LIGHT)
				{
					strcpy(Gbuf1, obj->name);
					do_grab(ch, Gbuf1, 0);
					return (TRUE);
				}
			}
		}
		else
		{
			obj = ch->equipment[HOLD];
			strcpy(Gbuf1, obj->name);
			do_remove(ch, Gbuf1, 0);
			strcpy(Gbuf1, obj->name);
			do_drop(ch, Gbuf1, 0);
			return (TRUE);
		}
		if (number(0, 5) == 0)
		{
			mobsay(ch, "Ah, must be time to go to bed!");
			act("The sounds of violent snoring filter through the area.", FALSE, ch, 0,
			    0, TO_ROOM);
			do_sleep(ch, 0, 0);
			if (ch->following)
			{
				stop_follower(ch);
				REMOVE_BIT(ch->specials.act, ACT_SENTINEL);
			}
			return TRUE;
		}
	}
	return (FALSE);
}

int witch_doctor(P_char witch, P_char customer, int cmd, char *arg)
{
	struct affected_type af;
	char buf[256];
	int i, room, tries, code;
	struct item
	{
		const char *keyword;
		const char *desc;
		int price;
		::byte affect_vector;
		uint affect_flag;
	} elixir_list[] = {
		{ "accelerate haste", "a &+ymagical elixir&n labeled \"&+YAcce&+yler&+Yate&n\"",
		  3000, 1, AFF_HASTE },
		{ "incendio fire", "a &+cwarm bottle&n labeled \"&+rProtectum &+RIncendio&n\"", 400,
		  1, AFF_PROT_FIRE },
		{ "frigeo cold", "a &+ccold bottle&n labeled \"&+bProtectum &+BFrigeo&n\"", 400, 2,
		  AFF2_PROT_COLD },
		{ "aviate fly", "a &+clight flask&n labeled \"&+CAv&+Wia&+Cte&n\"", 1000, 1,
		  AFF_FLY },
		{ "indigetis epic epics", "a &+Wglowing elixir&n labeled \"&+WIn&+wdiget&+Wis&n\"",
		  10000, 4, AFF4_EPIC_INCREASE },
		{ "gnowsis experience exp",
		  "a &+mrapidly vibrating vial&n labeled \"&+MGnowsis Ektaktos&n\"",
		  (100 * GET_LEVEL(customer)), 0, TAG_RESTED },
		{}
	};

	if (cmd == CMD_SET_PERIODIC)
		return TRUE; // change to TRUE to enable wandering

	if (cmd == CMD_LIST)
	{
		act("$n whispers, '&+WI can offer you the following arcane elixirs:&n'\r\n", FALSE,
		    witch, 0, customer, TO_VICT);
		for (i = 0; elixir_list[i].keyword; i++)
		{
			if (elixir_list[i].affect_vector == 0 &&
			    elixir_list[i].affect_flag == TAG_RESTED && !rested_bonus_enabled())
				continue;
			snprintf(buf, 256, "&+W%d)&n %s  - %d &+Wplatinum&n\r\n", i + 1,
				 elixir_list[i].desc, elixir_list[i].price);
			send_to_char(buf, customer);
		}
		return TRUE;
	}

	if (cmd == CMD_BUY)
	{
		if (economic_gameplay_authority::active())
		{
			send_to_char(
				"The witch doctor's elixirs are unavailable while active accounting is enabled.\r\n",
				customer);
			return TRUE;
		}
		for (i = 0; elixir_list[i].keyword; i++)
		{
			if (((code = atoi(arg)) > 0 && code == i + 1) ||
			    isname(arg, elixir_list[i].keyword))
			{
				memset(&af, 0, sizeof(struct affected_type));
				// Static for TAG_RESTED potion for exp
				if (i == 5)
				{
					if (!rested_bonus_enabled())
					{
						send_to_char(
							"The rested bonus feature is currently disabled.\r\n",
							customer);
						return TRUE;
					}
					if (affected_by_spell(customer, TAG_RESTED) ||
					    affected_by_spell(customer, TAG_WELLRESTED))
					{
						send_to_char(
							"The &+Gwi&+gtc&+Gh &+gdoctor&n says to you 'You already have an &+mexperience bonus&n currently active, come find me again when that one has expired!'\n\n",
							customer);
						return TRUE;
					}
					else
					{
						// 150 ticks about 2 1/2 hrs.
						af.duration = 150;
						af.type = TAG_RESTED;
						// Note: This one is visible on score while the rest of the witch spells aren't.
						//   This is justifiable since it has a much shorter timer.
						af.flags = AFFTYPE_PERM | AFFTYPE_NODISPEL |
							   AFFTYPE_OFFLINE;
						debug("'%s' getting rested bonus from WITCH!",
						      J_NAME(customer));
					}
				}
				else
				{
					// 1 rl week = 7 * 24 * 60.
					af.duration = 10080;
					af.type = TAG_WITCHSPELL;
					af.flags = AFFTYPE_NOSHOW | AFFTYPE_PERM |
						   AFFTYPE_NODISPEL | AFFTYPE_OFFLINE;
				}

				// Prices are in platinum -> 1000 * ...
				if (transact(customer, NULL, witch, 1000 * elixir_list[i].price))
				{
					if (elixir_list[i].affect_vector == 1)
						af.bitvector = elixir_list[i].affect_flag;
					else if (elixir_list[i].affect_vector == 2)
						af.bitvector2 = elixir_list[i].affect_flag;
					else if (elixir_list[i].affect_vector == 3)
						af.bitvector3 = elixir_list[i].affect_flag;
					else if (elixir_list[i].affect_vector == 4)
						af.bitvector4 = elixir_list[i].affect_flag;
					else if (elixir_list[i].affect_vector == 5)
						af.bitvector5 = elixir_list[i].affect_flag;
					affect_to_char(customer, &af);
					snprintf(
						buf, 256,
						"$n reaches down to $s sack and hands to you %s.\n"
						"As you quaff %s you feel the &+Ymagical powers&n surge through your body"
						" transforming you and making more &+Wpowerful&n than before.",
						elixir_list[i].desc, elixir_list[i].desc);
					act(buf, FALSE, witch, 0, customer, TO_VICT);
					GET_PLATINUM(witch) = 0;
				}
				return TRUE;
			}
		}
		act("$n says, 'I dont sell that.'", FALSE, witch, 0, customer, TO_VICT);
		return TRUE;
	}

	if (!cmd && !number(0, 150))
	{
		tries = 0;
		room = real_room0(500001);
		do
		{
			room = number(zone_table[world[room].zone].real_bottom,
				      zone_table[world[room].zone].real_top);
		} while (tries++ < 500 && (PRIVATE_ZONE(room) || IS_ROOM(room, ROOM_NO_TELEPORT) ||
					   world[room].sector_type == SECT_OCEAN ||
					   world[room].sector_type == SECT_WATER_SWIM ||
					   world[room].sector_type == SECT_MOUNTAIN ||
					   world[room].sector_type == SECT_WATER_NOSWIM));
		if (tries >= 500)
		{
			statuslog(0, "Witch Doctor cannot find a spot to land, check his proc plz");
		}
		else
		{
			act("$n looks around and says, '&+WNice doing trade with you but I need to serve other customers as well.&n'",
			    FALSE, witch, 0, 0, TO_ROOM);
			act("$n utters a few words, $s form blurs and shifts and $e's gone!", FALSE,
			    witch, 0, 0, TO_ROOM);
			char_from_room(witch);
			char_to_room(witch, room, -1);
		}
	}

	return FALSE;
}

int io_assistant(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}
	// anti-kill...
	GET_HIT(ch) = GET_MAX_HIT(ch);

	if (cmd == CMD_PURGE)
	{
		// set up an event to reload if needed..
		add_event(reload_io_assistant, 1, NULL, NULL, NULL, 0, NULL, 0);
		// act like we didn't handle it
		return FALSE;
	}

	if (!ch || !IS_AWAKE(ch) || cmd)
		return FALSE;

	int realroom44 = real_room0(44);
	// if NOT in the proper room, move to proper room
	if (ch->in_room != realroom44)
	{
		mobsay(ch, "I really need to get back to Io's room...");
		act("$n disappears in a puff of smoke.", FALSE, ch, 0, 0, TO_ROOM);
		char_from_room(ch);
		char_to_room(ch, realroom44, -1);
		act("$n arrives in a puff of smoke.", FALSE, ch, 0, 0, TO_ROOM);
	}

	// if in the proper room, and others of me in the room, purge self
	for (pl = world[realroom44].people; pl; pl = pl->next_in_room)
	{
		if ((ch != pl) && IS_NPC(ch) && IS_NPC(pl) && (GET_RNUM(pl) == GET_RNUM(ch)))
		{
			mobsay(ch, "Oh no!  Too many assistants will drive Io crazy!  Bye!");
			act("$n disappears in a puff of smoke.", FALSE, ch, 0, 0, TO_ROOM);
			extract_char(ch);
			return TRUE;
		}
	}

	P_obj obj;
	char buf[500];
	P_char chVar = get_char_room_vis(ch, "Vareena");

	static bool bVarHere = false;
	static time_t lastWhisper = 0;

	if (!bVarHere && chVar)
	{ // vareena entered the room...
		bVarHere = true;
		strcpy(buf, "vareena");
		do_action(ch, buf, CMD_HAND);
		// load a rose...
		obj = read_object(1277, VIRTUAL);
		if (obj)
		{
			obj_to_char(obj, ch);
			do_give(ch, writable_arg(" rose vareena"), CMD_GIVE);
		}
		return TRUE;
	}
	else if (!chVar)
	{
		bVarHere = false;
	}

	if (!number(0, 200))
	{
		do_action(ch, NULL, CMD_PONDER);
		;
		return TRUE;
	}

	// something is whacked with the random number generator, so adding some code to ensure a quiet
	// time...  don't spam poor vareena with whispers!  Max of 1 per 15 minutes
	if (chVar && (time(0) > (lastWhisper + (15 * 60))))
	{
		switch (number(1, 100))
		{
		case 50:
		case 51:
			act("$n whispers to you, 'Do you know that Io really loves you?  He's ALWAYS talking about you...'",
			    FALSE, ch, 0, chVar, TO_VICT);
			break;

		case 52:
		case 53:
		case 54:
			act("$n whispers to you, 'Io asked me to remind you to email him info on what special procs you need in your glacier zone'",
			    FALSE, ch, 0, chVar, TO_VICT);
			break;

		case 55:
		case 56:
			act("$n whispers to you, 'Smile, Vareena, someone loves you!'", FALSE, ch,
			    0, chVar, TO_VICT);
			break;

		case 57:
		case 58:
			act("$n whispers to you, 'What have you done to Io?  He's completely taken by you!'",
			    FALSE, ch, 0, chVar, TO_VICT);
			break;

		default:
			return FALSE;
		}
		act("$n whispers something to $N.", FALSE, ch, 0, chVar, TO_NOTVICT);
		lastWhisper = time(0);
		return TRUE;
		;
	}

	return FALSE;
}

void reload_io_assistant(P_char, P_char, P_obj, void * /*data*/)
{
	P_char ch;
	int realroom44 = real_room0(44);
	int realmob444 = real_mobile0(444);

	for (ch = world[realroom44].people; ch; ch = ch->next_in_room)
	{
		// mob is already there.. don't load new
		if (GET_RNUM(ch) == realmob444)
			return;
	}
	ch = read_mobile(444, VIRTUAL);
	char_to_room(ch, realroom44, -1);
	act("$n arrives in a puff of smoke.", FALSE, ch, 0, 0, TO_ROOM);
}

//  updates the extra description "symbols" on the obj
// to reflect how many charges are left
static void artifact_monolith_update(P_obj obj)
{
	char buf[500];
	if (!obj)
		return;
	if (!(obj->value[0]))
		strcpy(buf,
		       "&+LThe once glowing symbols are dull and faded, making them impossible to read.&n\n");
	else if (1 == obj->value[0])
		strcpy(buf,
		       "&+LThere is a single &n&+rsymbol&+L, written in the ancient language of dragons.&n\n");
	else
		snprintf(
			buf, 500,
			"&+LThere appear to be %d different &n&+rsymbols&+L, written in the ancient language\n&+Lof the dragons.&n\n",
			obj->value[0]);

	// now, find the proper extra description, remove the old one (if any) and
	// replace with the new one
	for (struct extra_descr_data *ed = obj->ex_description; ed; ed = ed->next)
		if (!str_cmp(ed->keyword, "symbols"))
		{
			if (ed->description)
				FREE(ed->description);
			ed->description = str_dup(buf);
			obj->str_mask |= STRUNG_EDESC;
		}
}

int artifact_monolith(P_obj monolith, P_char ch, int cmd, char *arg)
{
	P_obj obj, next_obj;
	const int HOURS_PER_CHARGE = 2;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	// If its not in room, return
	if (!monolith || !OBJ_ROOM(monolith))
		return FALSE;

	// periodics.. do something fancy
	if (cmd == CMD_PERIODIC)
	{
		// Check for the existence of other of this item in the room.  If they exist, absorb them.
		bool bUpdateDesc = FALSE;

		for (obj = world[monolith->loc.room].contents; obj; obj = next_obj)
		{
			next_obj = obj->next_content;

			if (obj == monolith)
				continue;
			if (obj->R_num == monolith->R_num)
			{
				// Okay, each 'obj' has a charge count stored in val0.  add
				// obj's charges to monolith's charges, and then destroy obj.
				monolith->value[0] += MAX(1, obj->value[0]);
				extract_obj(obj, TRUE); // Not an arti, but 'in game.'
				bUpdateDesc = TRUE;
			}
		}
		if (bUpdateDesc || !(monolith->value[1]))
		{
			artifact_monolith_update(monolith);
			monolith->value[1] = CMD_SCRATCH;
			return TRUE;
		}
		// Now hum, or crackle or something - only if there are charges left
		if (!number(0, 12) && (monolith->value[0]))
		{
			if (number(0, 1))
				act("&+YA bolt of energy&n &+Rcrackles &+Yalong the length of $p&n.",
				    TRUE, NULL, monolith, NULL, TO_ROOM);
			else
				act("The &+rsymbols&n on $p &+Wglow&n with &+Rpower&n.", TRUE, NULL,
				    monolith, NULL, TO_ROOM);
			// the symbols on xxx glow with power
		}
		return TRUE;
	}

	if (ch && (cmd == monolith->value[1]))
	{
		char buf[MAX_STRING_LENGTH];
		one_argument(arg, buf);

		if (monolith == get_obj_in_list_vis(ch, buf, world[ch->in_room].contents))
		{
			// check if the person even has any arti's.
			int i, nArtiCount = 0;
			for (i = 0; i < MAX_WEAR; i++)
			{
				if (ch->equipment[i] && IS_ARTIFACT(ch->equipment[i]))
					nArtiCount++;
			}
			// if no arti's, or no charges in the monolith, do nothing
			if (!nArtiCount || !(monolith->value[0]))
			{
				send_to_char("Nothing seems to happen.\n", ch);
			}
			else
			{
				// okay, each "charge" is worth X hours (or X * 3600 seconds).  Divide
				// that between all artifacts EQUALLY (regardless of how full they may
				// already be)
				int nFeedSeconds = (HOURS_PER_CHARGE * 3600) / nArtiCount;
				// now feed up them arti's!
				for (i = 0; i < MAX_WEAR; i++)
				{
					if (ch->equipment[i] && IS_ARTIFACT(ch->equipment[i]))
					{
						obj = ch->equipment[i];
						artifact_feed_sql(ch, ch->equipment[i],
								  nFeedSeconds);
					}
				}
				// update the feeder object to have one less charge..
				monolith->value[0]--;
				artifact_monolith_update(monolith);
			}
			return TRUE;
		}
	}
	return FALSE;
}

/*
   Use ITEM_OTHER:
   value[0] = payback - #/100
   value[1] = amount of money this machine has made from player
   value[2] = number of times machine is played
   value[3] = amount of money this machine has payed off
   value[4] = # Same Goodie Payoffs
   value[5] = # Same Evil Payoffs
   value[6] = # Same Undead Payoffs
   value[7] = # Tiamat Payoffs

   this is now based on real slots,  payoffs are as follows

   Odds = (#of things on wheel/# of occurances)^3 (for all same)
    i.e. 100 objects on wheel/1 tiamat on wheel = 100^3 or 1:1,000,000 odds of 3 tiamats
   Odds = (#of things on wheel1/# of occurances)*(same for wheel 2, and wheel3) (for all same)
   Chance = 100/odds (gives a %)
   Payback = (Payoff * Chance) / 100
   Payoff = (Payback * 100) / Chance

   9 Goodies (4 each)
   7 Evils (3 each)
   6 Undead (2 each)

                   %Chance        payoff    odds        payback
   Any Goodie     36.0%                 x        1:      2.7    0.
   Any Evil       21.25%                x        1:      4.76   0.
   Any Undead     12.0%                 x        1:      8.33   0.
   Any Illithid   10.0%           x    1:     10      0.

   Any 3 Goodies   4.6656%          x2   1:     21.433  0.093
   Any 3 Evils     0.9261%    x5     1:    107.979  0.092
   All Blank       0.8%                 x10      1:    125      0.120
   Any 3 Undead    0.1728%          x25    1:    578.705  0.086
   All Illithid    0.1%                 x50      1:   1000      0.10
   All Same Goodie 0.0064%        x250     1:  15625      0.016
   All Same Evil   0.0027%    x500     1:  37037.037  0.013
   All Same Undead 0.0008%            x2500    1: 125000      0.02
   All Tiamat      0.0001%      x25000   1:1000000      0.025
    Totals         6.6745%                                      0.528

   If you don't understand this table, what it boils down to is approx 7% of all pulls
   will win something.  Over time the machine eats 52.8% on average of what is put into it.

 */

int slot_machine(P_obj obj, P_char ch, int cmd, char *arg)
{
	const char *name[] = {
		"[&+L     TIAMAT    &N]", // 0
		"[&+M    ILLITHID   &N]", // 1
		"[&+M    ILLITHID   &N]", // 2
		"[&+M    ILLITHID   &N]", // 3
		"[&+M    ILLITHID   &N]", // 4
		"[&+M    ILLITHID   &N]", // 5
		"[&+M    ILLITHID   &N]", // 6
		"[&+M    ILLITHID   &N]", // 7
		"[&+M    ILLITHID   &N]", // 8
		"[&+M    ILLITHID   &N]", // 9
		"[&+M    ILLITHID   &N]", // 10
		"[&+C     HUMAN     &N]", // 11
		"[&+C     HUMAN     &N]", // 12
		"[&+C     HUMAN     &N]", // 13
		"[&+C     HUMAN     &N]", // 14
		"[&+R     GNOME     &N]", // 15
		"[&+R     GNOME     &N]", // 16
		"[&+R     GNOME     &N]", // 17
		"[&+R     GNOME     &N]", // 18
		"[&+B   BARBARIAN   &N]", // 19
		"[&+B   BARBARIAN   &N]", // 20
		"[&+B   BARBARIAN   &N]", // 21
		"[&+B   BARBARIAN   &N]", // 22
		"[&+Y     DWARF     &N]", // 23
		"[&+Y     DWARF     &N]", // 24
		"[&+Y     DWARF     &N]", // 25
		"[&+Y     DWARF     &N]", // 26
		"[&+y    HALFLING   &N]", // 27
		"[&+y    HALFLING   &N]", // 28
		"[&+y    HALFLING   &N]", // 29
		"[&+y    HALFLING   &N]", // 30
		"[&+W  S&+wT&+WO&+wR&+WM &+wG&+WI&+wA&+WNT  &N]", // 31
		"[&+W  S&+wT&+WO&+wR&+WM &+wG&+WI&+wA&+WNT  &N]", // 32
		"[&+W  S&+wT&+WO&+wR&+WM &+wG&+WI&+wA&+WNT  &N]", // 33
		"[&+W  S&+wT&+WO&+wR&+WM &+wG&+WI&+wA&+WNT  &N]", // 34
		"[&+g    CEN&+LTAUR    &N]", // 35
		"[&+g    CEN&+LTAUR    &N]", // 36
		"[&+g    CEN&+LTAUR    &N]", // 37
		"[&+g    CEN&+LTAUR    &N]", // 38
		"[&+C    HALF-&+cELF   &N]", // 39
		"[&+C    HALF-&+cELF   &N]", // 40
		"[&+C    HALF-&+cELF   &N]", // 41
		"[&+C    HALF-&+cELF   &N]", // 42
		"[&+c    GREY ELF   &N]", // 43
		"[&+c    GREY ELF   &N]", // 44
		"[&+c    GREY ELF   &N]", // 45
		"[&+c    GREY ELF   &N]", // 46
		"[&+m    DROW ELF   &N]", // 47
		"[&+m    DROW ELF   &N]", // 48
		"[&+m    DROW ELF   &N]", // 49
		"[&+G   GITH&+WYANKI   &N]", // 50
		"[&+G   GITH&+WYANKI   &N]", // 51
		"[&+G   GITH&+WYANKI   &N]", // 52
		"[&+g     TROLL     &N]", // 53
		"[&+g     TROLL     &N]", // 54
		"[&+g     TROLL     &N]", // 55
		"[&+L      ORC      &N]", // 56
		"[&+L      ORC      &N]", // 57
		"[&+L      ORC      &N]", // 58
		"[&+r    DUERGAR    &N]", // 59
		"[&+r    DUERGAR    &N]", // 60
		"[&+r    DUERGAR    &N]", // 51
		"[&+G     GOBLIN    &N]", // 52
		"[&+G     GOBLIN    &N]", // 63
		"[&+G     GOBLIN    &N]", // 64
		"[&+b      OGRE     &N]", // 65
		"[&+b      OGRE     &N]", // 66
		"[&+b      OGRE     &N]", // 67
		"[&+L      L&+mIC&+LH     &N]", // 68
		"[&+L      L&+mIC&+LH     &N]", // 69
		"[&+R    VAM&+rPI&+RRE    &N]", // 70
		"[&+R    VAM&+rPI&+RRE    &N]", // 71
		"[&+R     W&+rI&+RG&+rH&+RT     &N]", // 72
		"[&+R     W&+rI&+RG&+rH&+RT     &N]", // 73
		"[&+L  DEATH &+bKNIGHT &N]", // 74
		"[&+L  DEATH &+bKNIGHT &N]", // 75
		"[&+W    PHA&+LNTOM    &N]", // 76
		"[&+W    PHA&+LNTOM    &N]", // 77
		"[&+L SHADOW &+rBEAST  &N]", // 78
		"[&+L SHADOW &+rBEAST  &N]", // 79
		"[&+L     BLANK     &N]", // 80
		"[&+L     BLANK     &N]",
		"[&+L     BLANK     &N]",
		"[&+L     BLANK     &N]",
		"[&+L     BLANK     &N]",
		"[&+L     BLANK     &N]", // 85
		"[&+L     BLANK     &N]",
		"[&+L     BLANK     &N]",
		"[&+L     BLANK     &N]",
		"[&+L     BLANK     &N]",
		"[&+L     BLANK     &N]", // 90
		"[&+L     BLANK     &N]",
		"[&+L     BLANK     &N]",
		"[&+L     BLANK     &N]",
		"[&+L     BLANK     &N]",
		"[&+L     BLANK     &N]", // 95
		"[&+L     BLANK     &N]",
		"[&+L     BLANK     &N]",
		"[&+L     BLANK     &N]",
		"[&+L     BLANK     &N]", // 99
		"[&+L     BLANK     &N]", // 100
		"[&+L     BLANK     &N]", //
		"[&+L     BLANK     &N]", //
		"[&+L     BLANK     &N]", //
		"[&+L     BLANK     &N]", //
		"[&+L     BLANK     &N]", // 105
		"[&+L     BLANK     &N]" //
		"[&+L     BLANK     &N]" //
		"[&+L     BLANK     &N]" //
		"[&+L     BLANK     &N]" // 109
	};

	int coins, type, wheela, wheelb, wheelc, greywheel, count;
	P_char dummy;
	P_obj object;
	char Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH];
	int gpayoff, epayoff, upayoff, tpayoff, ipayoff, gggpayoff, eeepayoff, uuupayoff,
		blankpayoff = 0;
	int coinamt;
	int64_t payout_value = 0;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd != CMD_PUT)
		return FALSE;

	// disabled until winning millions of plats is removed
	// send_to_char("Slot machines machines have been disabled due to excessive taxes.\r\n", ch);
	// return FALSE;

	arg = one_argument(arg, Gbuf1); // multicoin
	arg = one_argument(arg, Gbuf2); // multicoin
	arg = one_argument(arg, Gbuf3); // multicoin
	//  argument_interpreter(arg, Gbuf1, Gbuf2);
	if (!*Gbuf1 || !*Gbuf2 || !*Gbuf3)
		return FALSE;

	type = coin_type(Gbuf2);
	coinamt = atoi(Gbuf1);
	// Don't include tracks with slot machine.
	generic_find(Gbuf3, FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &dummy, &object);

	//  wizlog(MINLVLIMMORTAL, "%s played %s %s on %s in [%d]", GET_NAME(ch),
	//                              Gbuf1, Gbuf2, Gbuf3, world[ch->in_room].number);
	// wizlog(MINLVLIMMORTAL, "%s played %d %s on %s in [%d]", GET_NAME(ch),
	//                              coinamt, Gbuf2, Gbuf3,world[ch->in_room].number);

	if ((type < 0) || (type > 3))
		return FALSE;

	if (coinamt < 0)
	{
		wizlog(MINLVLIMMORTAL,
		       "&=LR%s just tried to play %d %s on a slot machine in [%d]&N", GET_NAME(ch),
		       coinamt, Gbuf2, world[ch->in_room].number);
		return FALSE;
	}
	if (coinamt == 0)
		return FALSE;

	if ((coinamt > 5) && !IS_TRUSTED(ch))
	{
		act("&NYou cannot put that much in!&N", FALSE, ch, 0, 0, TO_CHAR);
		return TRUE;
	}

	//  bits = generic_find(Gbuf2, FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &dummy, &object);

	if (obj != object)
		return FALSE;

	//  if (((type == 0) && (GET_COPPER(ch) == 0)) ||
	//      ((type == 1) && (GET_SILVER(ch) == 0)) ||
	//      ((type == 2) && (GET_GOLD(ch) == 0)) ||
	//      ((type == 3) && (GET_PLATINUM(ch) == 0)))
	if (((type == 0) && (GET_COPPER(ch) < coinamt)) ||
	    ((type == 1) && (GET_SILVER(ch) < coinamt)) ||
	    ((type == 2) && (GET_GOLD(ch) < coinamt)) ||
	    ((type == 3) && (GET_PLATINUM(ch) < coinamt)))
	{
		send_to_char("You don't have enough of that coin type.\n", ch);
		return TRUE;
	}
	else
	{
		if (type < 0 || type > 3 || !currency_transaction_can_submit_nonrebasable(ch))
		{
			send_to_char("The slot machine could not accept that wager.\n", ch);
			return TRUE;
		}
		act("You insert your coin(s) into $p.", FALSE, ch, obj, 0, TO_CHAR);
		switch (type)
		{
		case 0:
			obj->value[1] += coinamt;
			break;
		case 1:
			obj->value[1] += (10 * coinamt);
			break;
		case 2:
			obj->value[1] += (100 * coinamt);
			break;
		case 3:
			obj->value[1] += (1000 * coinamt);
			break;
		}
		if ((obj->value[2]) == 0)
			(obj->value[0] = 0);
		obj->value[2]++;
	}
	wheela = number(0, 79);
	wheelb = number(0, 79);
	wheelc = number(0, 79);
	//    wheela = 39;  //these force elftime :P
	//    wheelb = 43;
	//    wheelc = 48;

	snprintf(Gbuf1, MAX_STRING_LENGTH, "You pull the lever and see: %s %s %s\n", name[wheela],
		 name[wheelb], name[wheelc]);
	send_to_char(Gbuf1, ch);
	coins = 0;
	gggpayoff = 2 * coinamt;
	eeepayoff = 5 * coinamt;
	//  blankpayoff = 11*coinamt;
	uuupayoff = 10 * coinamt;
	ipayoff = 15 * coinamt;
	gpayoff = 100 * coinamt;
	epayoff = 250 * coinamt;
	upayoff = 1250 * coinamt;
	tpayoff = 25000 * coinamt;
	greywheel = 1313;

	if ((wheela == 0) && (wheelb == 0) && (wheelc == 0))
		coins = tpayoff; /* tiamat tiamat tiamat */

	if (((wheela >= 15) && (wheela <= 18)) || ((wheelb >= 15) && (wheelb <= 18)) ||
	    ((wheelc >= 15) && (wheelc <= 18)))
		coins = coinamt; /* any gnome */

	if (((wheela >= 62) && (wheela <= 64)) || ((wheelb >= 62) && (wheelb <= 64)) ||
	    ((wheelc >= 62) && (wheelc <= 64)))
		coins = coinamt; /* any goblin */

	if (((wheela >= 11) && (wheela <= 46)) && ((wheelb >= 11) && (wheelb <= 46)) &&
	    ((wheelc >= 11) && (wheelc <= 46)))
		coins = gggpayoff; /* 3 goodies */

	if (((wheela >= 47) && (wheela <= 67)) && ((wheelb >= 47) && (wheelb <= 67)) &&
	    ((wheelc >= 47) && (wheelc <= 67)))
		coins = eeepayoff; /* 3 evils */

	if (((wheela >= 68) && (wheela <= 79)) && ((wheelb >= 68) && (wheelb <= 79)) &&
	    ((wheelc >= 68) && (wheelc <= 79)))
		coins = uuupayoff; /* 3 undead */

	if (((wheela >= 1) && (wheela <= 10)) && ((wheelb >= 1) && (wheelb <= 10)) &&
	    ((wheelc >= 1) && (wheelc <= 10)))
		coins = ipayoff; /* any illithid */

	if (((wheela >= 11) && (wheela <= 14)) && ((wheelb >= 11) && (wheelb <= 14)) &&
	    ((wheelc >= 11) && (wheelc <= 14)))
		coins = gpayoff; /* 3 same goodies */

	if (((wheela >= 15) && (wheela <= 18)) && ((wheelb >= 15) && (wheelb <= 18)) &&
	    ((wheelc >= 15) && (wheelc <= 18)))
		coins = gpayoff; /* 3 same goodies */

	if (((wheela >= 19) && (wheela <= 22)) && ((wheelb >= 19) && (wheelb <= 22)) &&
	    ((wheelc >= 19) && (wheelc <= 22)))
		coins = gpayoff; /* 3 same goodies */

	if (((wheela >= 23) && (wheela <= 26)) && ((wheelb >= 23) && (wheelb <= 26)) &&
	    ((wheelc >= 23) && (wheelc <= 26)))
		coins = gpayoff; /* 3 same goodies */

	if (((wheela >= 27) && (wheela <= 30)) && ((wheelb >= 27) && (wheelb <= 30)) &&
	    ((wheelc >= 27) && (wheelc <= 30)))
		coins = gpayoff; /* 3 same goodies */

	if (((wheela >= 31) && (wheela <= 34)) && ((wheelb >= 31) && (wheelb <= 34)) &&
	    ((wheelc >= 31) && (wheelc <= 34)))
		coins = gpayoff; /* 3 same goodies */

	if (((wheela >= 35) && (wheela <= 38)) && ((wheelb >= 35) && (wheelb <= 38)) &&
	    ((wheelc >= 35) && (wheelc <= 38)))
		coins = gpayoff; /* 3 same goodies */

	if (((wheela >= 39) && (wheela <= 42)) && ((wheelb >= 39) && (wheelb <= 42)) &&
	    ((wheelc >= 39) && (wheelc <= 42)))
		coins = gpayoff; /* 3 same goodies */

	if (((wheela >= 43) && (wheela <= 46)) && ((wheelb >= 43) && (wheelb <= 46)) &&
	    ((wheelc >= 43) && (wheelc <= 46)))
		coins = gpayoff; /* 3 same goodies */

	if (((wheela >= 47) && (wheela <= 49)) && ((wheelb >= 47) && (wheelb <= 49)) &&
	    ((wheelc >= 47) && (wheelc <= 49)))
		coins = epayoff; /* 3 same evils */

	if (((wheela >= 50) && (wheela <= 52)) && ((wheelb >= 50) && (wheelb <= 52)) &&
	    ((wheelc >= 50) && (wheelc <= 52)))
		coins = epayoff; /* 3 same evils */

	if (((wheela >= 53) && (wheela <= 55)) && ((wheelb >= 53) && (wheelb <= 55)) &&
	    ((wheelc >= 53) && (wheelc <= 55)))
		coins = epayoff; /* 3 same evils */

	if (((wheela >= 56) && (wheela <= 58)) && ((wheelb >= 56) && (wheelb <= 58)) &&
	    ((wheelc >= 56) && (wheelc <= 58)))
		coins = epayoff; /* 3 same evils */

	if (((wheela >= 59) && (wheela <= 61)) && ((wheelb >= 59) && (wheelb <= 61)) &&
	    ((wheelc >= 59) && (wheelc <= 61)))
		coins = epayoff; /* 3 same evils */

	if (((wheela >= 62) && (wheela <= 64)) && ((wheelb >= 62) && (wheelb <= 64)) &&
	    ((wheelc >= 62) && (wheelc <= 64)))
		coins = epayoff; /* 3 same evils */

	if (((wheela >= 65) && (wheela <= 67)) && ((wheelb >= 65) && (wheelb <= 67)) &&
	    ((wheelc >= 65) && (wheelc <= 67)))
		coins = epayoff; /* 3 same evils */

	if (((wheela >= 68) && (wheela <= 69)) && ((wheelb >= 68) && (wheelb <= 69)) &&
	    ((wheelc >= 68) && (wheelc <= 69)))
		coins = upayoff; /* 3 same undead */

	if (((wheela >= 70) && (wheela <= 71)) && ((wheelb >= 70) && (wheelb <= 71)) &&
	    ((wheelc >= 70) && (wheelc <= 71)))
		coins = upayoff; /* 3 same undead */

	if (((wheela >= 72) && (wheela <= 73)) && ((wheelb >= 72) && (wheelb <= 73)) &&
	    ((wheelc >= 72) && (wheelc <= 73)))
		coins = upayoff; /* 3 same undead */

	if (((wheela >= 74) && (wheela <= 75)) && ((wheelb >= 74) && (wheelb <= 75)) &&
	    ((wheelc >= 74) && (wheelc <= 75)))
		coins = upayoff; /* 3 same undead */

	if (((wheela >= 76) && (wheela <= 77)) && ((wheelb >= 76) && (wheelb <= 77)) &&
	    ((wheelc >= 76) && (wheelc <= 77)))
		coins = upayoff; /* 3 same undead */

	if (((wheela >= 78) && (wheela <= 79)) && ((wheelb >= 78) && (wheelb <= 79)) &&
	    ((wheelc >= 78) && (wheelc <= 79)))
		coins = upayoff; /* 3 same undead */

	if (((wheela >= 80) && (wheela <= 109)) && ((wheelb >= 80) && (wheelb <= 109)) &&
	    ((wheelc >= 80) && (wheelc <= 109)))
		coins = blankpayoff; /* 3 blanks */

	if (((wheela >= 39) && (wheela <= 42)) && ((wheelb >= 43) && (wheelb <= 46)) &&
	    ((wheelc >= 47) && (wheelc <= 49)))
		coins = greywheel; /* GREY WHEEL! */

	if (coins)
	{
		if (coins > (5 * coinamt))
			act("$n &+winserts a coin into $p&+w, pulls the lever, and &+Bwon&+w!&N",
			    FALSE, ch, obj, 0, TO_ROOM);
		if (coins == gpayoff)
		{
			act("&+wThe &=LRsiren&N&+w goes off&+B!&+w  &+B3 Same Goodies&N!", FALSE,
			    ch, 0, 0, TO_ROOM);
			act("&+wThe &=LRsiren&N&+w goes off&+B!&+w  &+B3 Same Goodies!&N", FALSE,
			    ch, 0, 0, TO_CHAR);
			obj->value[4]++;
		}
		if (coins == ipayoff)
		{
			act("&+wThe &=LRsiren&N&+w goes off&+B!&+w  &+B3 Illithids&N!", FALSE, ch,
			    0, 0, TO_ROOM);
			act("&+wThe &=LRsiren&N&+w goes off&+B!&+w  &+B3 Illithids!&N", FALSE, ch,
			    0, 0, TO_CHAR);
			//          obj->value[0]++;
		}

		if (coins == epayoff)
		{
			act("&+wThe &=LRsiren&N&+w goes off&+B!&+w  &+B3 Same Evils&N!", FALSE, ch,
			    0, 0, TO_ROOM);
			act("&+wThe &=LRsiren&N&+w goes off&+B!&+w  &+B3 Same Evils!&N", FALSE, ch,
			    0, 0, TO_CHAR);
			obj->value[5]++;
		}

		if (coins == upayoff)
		{
			act("&+wThe &=LRsiren&N&+w goes off&+B!&+w  &+B3 Same Undeads&N!", FALSE,
			    ch, 0, 0, TO_ROOM);
			act("&+wThe &=LRsiren&N&+w goes off&+B!&+w  &+B3 Same Undeads!&N", FALSE,
			    ch, 0, 0, TO_CHAR);
			obj->value[6]++;
		}

		if (coins == tpayoff)
		{
			act("&+wThe &=LRsiren&N&+w goes off&+B!&+w  Looks like someone hits the &+Rjackpot&N!",
			    FALSE, ch, 0, 0, TO_ROOM);
			act("&+wThe &=LRsiren&N&+w goes off&+B!&+w  Looks like you hit the &+Rjackpot!&N",
			    FALSE, ch, 0, 0, TO_CHAR);
			obj->value[7]++;
			wizlog(MINLVLIMMORTAL,
			       "%s has just won the Tiamat jackpot at 1:512000 odds  on a slot machine in room [%d]",
			       GET_NAME(ch), world[ch->in_room].number);
		}

		if (coins == greywheel)
		{
			// odds of even getting here are...        1:10,666
			// odds of getting all all 3 drow 3 times are    1:119,796
			// total odds of winning full jackpot of 100k are  1:1,277,825,638
			obj->value[0]++;
			coins = 100 * coinamt;
			act("&+wThe &=LBsiren&N&+w goes off&+B!&+c Its ELF TIME!&N!", FALSE, ch, 0,
			    0, TO_ROOM);
			act("&+wThe &=LBsiren&N&+w goes off&+B!&+c Its ELF TIME!&N!", FALSE, ch, 0,
			    0, TO_CHAR);

			for (count = 0; count < 3; count++)
			{
				wheela = number(39, 49);
				wheelb = number(39, 49);
				wheelc = number(39, 49);
				snprintf(Gbuf1, MAX_STRING_LENGTH,
					 "The machine shakes and you see: %s %s %s\n", name[wheela],
					 name[wheelb], name[wheelc]);
				send_to_char(Gbuf1, ch);

				if (((wheela >= 39) && (wheela <= 42)) && // half elf
				    ((wheelb >= 39) && (wheelb <= 42)) && // odds 1 20.796875
				    ((wheelc >= 39) && (wheelc <= 42)))
				{
					coins *= 5;
					act("&+wThe &=LBsiren&N&+w goes off&+B!&+c Three &+CHalf-&+cElf&+ws! Multiplier x5!&N!",
					    FALSE, ch, 0, 0, TO_ROOM);
					act("&+wThe &=LBsiren&N&+w goes off&+B!&+c Three &+CHalf-&+cElf&+ws! Multiplier x5!&N",
					    FALSE, ch, 0, 0, TO_CHAR);
				}

				if (((wheela >= 43) && (wheela <= 46)) && // grey elf
				    ((wheelb >= 43) && (wheelb <= 46)) && // odds 1 20.796875
				    ((wheelc >= 43) && (wheelc <= 46)))
				{
					coins *= 5;
					act("&+wThe &=LBsiren&N&+w goes off&+B!&+c Three &+cGrey-Elf&+ws! Multiplier x5!&N!",
					    FALSE, ch, 0, 0, TO_ROOM);
					act("&+wThe &=LBsiren&N&+w goes off&+B!&+c Three &+cGrey-Elf&+ws! Multiplier x5!&N",
					    FALSE, ch, 0, 0, TO_CHAR);
				}

				if (((wheela >= 47) && (wheela <= 49)) && // drow elf
				    ((wheelb >= 47) &&
				     (wheelb <= 49)) && // odds 1 49.2962962962962962962962962962784
				    ((wheelc >= 47) && (wheelc <= 49)))
				{
					coins *= 10;
					act("&+wThe &=LBsiren&N&+w goes off&+B!&+c Three &+mDrow Elf&+ws! Multiplier x10!&N!",
					    FALSE, ch, 0, 0, TO_ROOM);
					act("&+wThe &=LBsiren&N&+w goes off&+B!&+c Three &+mDrow Elf&+ws! Multiplier x10!&N",
					    FALSE, ch, 0, 0, TO_CHAR);
				}
			}
		}
		if (coins == 100000 * coinamt)
		{
			wizlog(MINLVLIMMORTAL,
			       "%s got the ELF jackpot at 1 to 1,277,825,638 odds on a slot machine in room [%d]",
			       GET_NAME(ch), world[ch->in_room].number);
			act("&+wThe &=LBsiren&N&+w goes off&+B!&+c %s has just won &=LWTHE JACKPOT!&N!",
			    FALSE, ch, 0, 0, TO_ROOM);
			act("&+wThe &=LRsiren&N&+w goes off&+B!&+c %s has just won &=LWTHE JACKPOT!&N!",
			    FALSE, ch, 0, 0, TO_ROOM);
			act("&+wThe &=LRsiren&N&+w goes off&+B!&+c you have just won &=LWTHE JACKPOT!&N",
			    FALSE, ch, 0, 0, TO_CHAR);
			act("&+wThe &=LRsiren&N&+w goes off&+B!&+c you have just won &=LWTHE JACKPOT!&N",
			    FALSE, ch, 0, 0, TO_CHAR);
			if (type == 3)
			{
				coins = 50000 * coinamt;
				act("&+wYou receive &+Ba restring coupon&N.&N", FALSE, ch, 0, 0,
				    TO_CHAR);
				act("&+wYou receive &+Ra restring coupon&N.&N", FALSE, ch, 0, 0,
				    TO_CHAR);
				obj_to_char(read_object(44, VIRTUAL), ch);
				obj_to_char(read_object(43, VIRTUAL), ch);
			}
		}

		snprintf(Gbuf1, MAX_STRING_LENGTH, "You win %d %s coin(s)!", coins,
			 coin_names[type]);
		static const int coin_values[] = { 1, 10, 100, 1000 };
		payout_value = (int64_t)coins * coin_values[type];
		switch (type)
		{
		case 0:
			break;
		case 1:
			coins *= 10;
			break;
		case 2:
			coins *= 100;
			break;
		case 3:
			coins *= 1000;
			break;
		}
		act(Gbuf1, FALSE, ch, 0, 0, TO_CHAR);
		obj->value[3] += coins;
	}
	static const int coin_values[] = { 1, 10, 100, 1000 };
	const int64_t net_value = payout_value - (int64_t)coinamt * coin_values[type];
	if (net_value != 0 && !currency_transaction_submit_wallet_value(
				      ch, net_value,
				      net_value > 0 ? currency_reason_type::wallet_reward :
						      currency_reason_type::wallet_spend,
				      OBJ_VNUM(obj), critical_source_site::command,
				      critical_deadline_class::interactive, nullptr, nullptr, 0))
	{
		logit(LOG_WIZ, "slot_machine: net wallet submission failed for pid %d",
		      GET_PID(ch));
		send_to_char("The slot result could not be recorded. Please contact staff.\n", ch);
	}
	return TRUE;
}

// for 51 potions

int treasure_chest(P_obj obj, P_char ch, int cmd, char * /*argument*/)
{
	int found, chance1, chance2;
	P_obj potion;

	chance1 = 10;
	chance2 = 1;
	found = 0;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (IS_TRUSTED(ch) || !CAN_SEE_OBJ(ch, obj))
	{
		return FALSE;
	}
	if (!ch || !obj)
	{
		return FALSE;
	}

	if (number(0, 100) < chance1)
	{
		found++;

		if (number(0, 100) < chance2)
		{
			found++;
		}
	}

	/*
	   Any powers activated by keywords? Right here, bud.
	 */
	if (cmd == CMD_OPEN)
	{
		if (number(0, 100) < chance1)
		{
			found++;

			if (number(0, 100) < chance2)
			{
				found++;
			}
		}

		if (found)
		{
			act("$n opens the &+Ytreasure &+ychest&n and finds something!&n", FALSE, ch,
			    obj, NULL, TO_NOTVICTROOM);
			act("Your open the &+Ytreasure &+ychest&n and find something...&n", FALSE,
			    ch, obj, NULL, TO_CHAR);

			while (found > 0)
			{
				potion = read_object(51004, VIRTUAL);

				obj_to_char(potion, ch);
				act("... &+Wa glowing potion!&n", FALSE, ch, obj, NULL, TO_CHAR);
				found--;
			}
		}

		act("&+yA &+Ytreasure &+ychest crumbles into &ndust.", FALSE, ch, obj, NULL,
		    TO_CHAR);
		act("&+yA &+Ytreasure &+ychest crumbles into &ndust.", FALSE, ch, obj, NULL,
		    TO_NOTVICTROOM);
		obj_from_room(obj);
		return TRUE;
	}

	return FALSE;
}

// Chaos/Ambran/Death Rider
int holy_weapon(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char tch, vict, attacker;
	struct affected_type aff, *af;
	bool should_jump;
	int pcnt, tmpper, alignment; // 0 == good, 1 == evil, -1 == neutral
	struct damage_messages goodmessages = {
		"&+wThe power of your &+WGod&+w rains down pain and suffering upon $N&+w!&n",
		"&+wPain unlike you have ever felt before permeates your body.&n",
		"&+w$N &+wscreams in utter terror as he is judged before $n&+w's &+WGod&+w.&n",
		"&+w$N &+wfalls to the ground, their soul a mere shell of what it once was.&n",
		"&+WJudgement &+wis rendered, as you feel your soul being shattered to pieces.&n",
		"&+w$N &+wfalls to the ground, their soul a mere shell of what it once was.&n.",
		0
	};
	struct damage_messages evilmessages = {
		"&+LThe power of your &+RGod&+L rains down pain and suffering upon $N&+L!&n",
		"&+LPain unlike you have ever felt before permeates your body.&n",
		"&+L$N &+Lscreams in utter terror as he is damned by $n&+L's &+RGod&+L.&n",
		"&+L$N &+Lfalls to the ground, their soul a mere shell of what it once was.&n",
		"&+RDamnation &+Lis rendered, as you feel your soul being shattered to pieces.&n",
		"&+L$N &+Lfalls to the ground, their soul a mere shell of what it once was.&n.",
		0
	};

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!obj)
	{
		return FALSE;
	}

	switch (obj_index[obj->R_num].virtual_number)
	{
	case VOBJ_HOLYSWORD_AMBRAN:
		alignment = RACEWAR_GOOD;
		break;
	case VOBJ_HOLYSWORD_DEATHRIDER:
		alignment = RACEWAR_EVIL;
		break;
	case VOBJ_HOLYSWORD_CHAOS:
		alignment = RACEWAR_UNDEAD;
		break;
	default:
		alignment = RACEWAR_NONE;
		break;
	}

	if ((cmd == CMD_REMOVE) && arg)
	{
		if (isname(arg, obj->name) || isname(arg, "all"))
		{
			if (affected_by_spell(ch, TAG_HOLY_OFFENSE))
				affect_from_char(ch, TAG_HOLY_OFFENSE);
			if (affected_by_spell(ch, TAG_HOLY_DEFENSE))
				affect_from_char(ch, TAG_HOLY_DEFENSE);
		}
	}

	if (OBJ_WORN(obj))
	{
		if (cmd == CMD_SAY && arg)
		{
			if (isname(arg, "attack"))
			{
				af = get_spell_from_char(ch, TAG_HOLY_OFFENSE);
				if (!af)
				{
					memset(&aff, 0, sizeof(aff));
					aff.type = TAG_HOLY_OFFENSE;
					aff.flags = AFFTYPE_NODISPEL;
					aff.modifier = 12;
					aff.location = APPLY_STR_MAX;
					aff.duration = -1;
					affect_to_char(ch, &aff);
					aff.modifier = 12;
					aff.location = APPLY_DEX_MAX;
					aff.duration = -1;
					affect_to_char(ch, &aff);
					aff.modifier = -1;
					aff.location = APPLY_COMBAT_PULSE;
					aff.duration = -1;
					affect_to_char(ch, &aff);
					act("&+LAs you utter the word, a chill runs through your body as power takes hold...",
					    FALSE, ch, obj, 0, TO_CHAR);
					act("$n &+wwhispers something to $s $q&+w, and &+Lshudders &+wwith new power...",
					    FALSE, ch, obj, 0, TO_ROOM);
					if ((af = get_spell_from_char(ch, TAG_HOLY_DEFENSE)))
						affect_from_char(ch, TAG_HOLY_DEFENSE);
					CharWait(ch, 2 * PULSE_VIOLENCE);
					return TRUE;
				}
			}
			else if (isname(arg, "defend"))
			{
				if (!(af = get_spell_from_char(ch, TAG_HOLY_DEFENSE)))
				{
					memset(&aff, 0, sizeof(aff));
					aff.type = TAG_HOLY_DEFENSE;
					aff.flags = AFFTYPE_NODISPEL;
					aff.modifier = 15;
					aff.location = APPLY_AGI_MAX;
					aff.duration = -1;
					affect_to_char(ch, &aff);
					aff.modifier = -8;
					aff.location = APPLY_SAVING_PARA;
					aff.duration = -1;
					affect_to_char(ch, &aff);
					aff.modifier = -8;
					aff.location = APPLY_SAVING_SPELL;
					aff.duration = -1;
					affect_to_char(ch, &aff);
					aff.modifier = -8;
					aff.location = APPLY_SAVING_BREATH;
					aff.duration = -1;
					affect_to_char(ch, &aff);
					aff.modifier = 1;
					aff.location = APPLY_COMBAT_PULSE;
					aff.duration = -1;
					affect_to_char(ch, &aff);
					aff.modifier = -30;
					aff.location = APPLY_AC;
					aff.duration = -1;
					affect_to_char(ch, &aff);
					act("&+WAs you utter the word, a chill runs through your body as power takes hold...",
					    FALSE, ch, obj, 0, TO_CHAR);
					act("$n &+wwhispers something to $s $q&+w, and &+Wshudders &+wwith new power...",
					    FALSE, ch, obj, 0, TO_ROOM);
					if ((af = get_spell_from_char(ch, TAG_HOLY_OFFENSE)))
						affect_from_char(ch, TAG_HOLY_OFFENSE);
					CharWait(ch, 2 * PULSE_VIOLENCE);
					return TRUE;
				}
			}
			else if (isname(arg, "protect"))
			{
				for (struct group_list *tgl = ch->group; tgl && tgl->ch;
				     tgl = tgl->next)
				{
					if (tgl->ch->in_room != ch->in_room)
						continue;

					if (!affected_by_spell(tgl->ch, SPELL_ARMOR))
						spell_armor(60, ch, 0, 0, tgl->ch, 0);
					if (!affected_by_spell(tgl->ch, SPELL_BLESS))
						spell_bless(60, ch, 0, 0, tgl->ch, 0);
				}
			}
		}
	}

	if (cmd == CMD_MELEE_HIT && !number(0, 25) && CheckMultiProcTiming(ch))
	{
		vict = legacy_proc_arg<P_char>(arg);

		if (!IS_ALIVE(vict))
		{
			return FALSE;
		}

		if (GET_RACEWAR(ch) == RACEWAR_GOOD)
		{
			act("$n's $q &+Wflares with pure light, unleashing the virtue of the gods at $N!&n",
			    TRUE, ch, obj, vict, TO_NOTVICT);
			act("Your $q &+Wflares with pure light, unleashing the virtue of the gods at $N!&n",
			    TRUE, ch, obj, vict, TO_CHAR);
			act("$n's $q &+Wflares with pure light, unleashing the virtue of the gods at _YOU_!&n",
			    TRUE, ch, obj, vict, TO_VICT);
			spell_damage(ch, vict, 360, SPLDAM_HOLY, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT,
				     &goodmessages);
			if (GET_C_LUK(ch) > number(0, 500))
			{
				spell_mending(51, ch, 0, 0, ch, 0);
			}
		}
		else if (GET_RACEWAR(ch) == RACEWAR_EVIL)
		{
			act("$n's $q &+Lflares with darkness, unleashing the wrath of the underworld upon $N!&n",
			    TRUE, ch, obj, vict, TO_NOTVICT);
			act("Your $q &+Lflares with darkness, unleashing the wrath of the underworld upon $N!&n",
			    TRUE, ch, obj, vict, TO_CHAR);
			act("$n's $q &+Lflares with darkness, unleashing the wrath of the underworld upon _YOU_!&n",
			    TRUE, ch, obj, vict, TO_VICT);
			spell_damage(ch, vict, 360, SPLDAM_NEGATIVE,
				     SPLDAM_NOSHRUG | SPLDAM_NODEFLECT, &evilmessages);
			if (GET_C_LUK(ch) > number(0, 500))
			{
				spell_malison(56, ch, 0, 0, vict, 0);
			}
		}
		return TRUE;
	}

	if (cmd == CMD_PERIODIC)
	{
		// This periodically checks to see if item is in the proper racewar hands
		if (OBJ_WORN(obj))
			ch = obj->loc.wearing;
		else if (OBJ_CARRIED(obj))
			ch = obj->loc.carrying;
		else
			return FALSE;

		should_jump = GET_RACEWAR(ch) != alignment;

		if (IS_TRUSTED(ch) || IS_NPC(ch))
		{
			should_jump = FALSE;
		}

		if (should_jump)
		{
			if (OBJ_WORN(obj))
			{
				obj_to_char(unequip_char(ch,
							 (ch->equipment[WIELD] == obj) ?
								 WIELD :
							 (ch->equipment[SECONDARY_WEAPON] == obj) ?
								 SECONDARY_WEAPON :
							 (ch->equipment[HOLD] == obj) ? HOLD :
											WEAR_NONE),
					    ch);
			}
			obj_from_char(obj);

			for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
			{
				if (IS_PC(tch) && GET_RACEWAR(tch) == alignment)
				{
					act("$p screams in outrage at your touch!", FALSE, ch, obj,
					    0, TO_CHAR);
					act("$p screams in outrage at $n's touch!", FALSE, ch, obj,
					    0, TO_ROOM);
					act("$p &=LCshimmers&n, blasts you with power and leaps to $N!",
					    FALSE, ch, obj, tch, TO_CHAR);
					act("$p &=LCshimmers&n and blasts $n as it leaps to $N!",
					    FALSE, ch, obj, tch, TO_NOTVICT);
					act("$p &=LCshimmers&n and blasts $n as it leaps to you!",
					    FALSE, ch, obj, tch, TO_VICT);
					// Rehome before the bolt: it can kill ch, and a death
					// with the sword detached from every list leaves it
					// stranded in limbo across make_corpse().
					obj_to_char(obj, tch);
					spell_lightning_bolt(61, ch, 0, SPELL_TYPE_SPELL, ch, 0);
					break;
				}
			}
			if (!tch)
			{
				act("$p &=LCshimmers&n, blasts you with power and vanishes from your hand!",
				    FALSE, ch, obj, 0, TO_CHAR);
				act("$p &=LCshimmers&n and blasts $n before vanishing!", FALSE, ch,
				    obj, 0, TO_ROOM);
				extract_obj(obj, TRUE); // Bye arti sword.
				spell_lightning_bolt(61, ch, 0, SPELL_TYPE_SPELL, ch, 0);
			}
			return TRUE;
		}

		if (OBJ_WORN(obj) && !number(0, 10))
		{
			hummer(obj);
			return TRUE;
		}

		// Following proc finds the most hurt casting ally that is tanking and rescues them - Jexni 12/2/10
		// Re-wrote this to make more sense.
		if (IS_FIGHTING(ch) && OBJ_WORN(obj))
		{
			pcnt = 100;
			vict = attacker = NULL;
			// Look through the room to see who's fighting.
			for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
			{
				// Skip the people just standing there (and attacking ch).
				if (!IS_FIGHTING(tch) || GET_OPPONENT(tch) == ch)
				{
					continue;
				}
				// If tch is attacking someone in ch's group.
				if (grouped(ch, GET_OPPONENT(tch)) && IS_CASTER(GET_OPPONENT(tch)))
				{
					tmpper = (GET_HIT(GET_OPPONENT(tch)) /
						  GET_MAX_HIT(GET_OPPONENT(tch))) *
						 100;
					if (tmpper < pcnt)
					{
						attacker = tch;
						vict = GET_OPPONENT(tch);
						pcnt = tmpper;
					}
				}
			}
			// If we didn't find anyone to rescue.
			if (!vict)
			{
				return FALSE;
			}
			if (CanDoFightMove(ch, attacker))
			{
				if (alignment == RACEWAR_GOOD)
				{
					act("Your $q &+Wglows &+was you slam the pommel into $N&+w, &+wknocking $M away from your ally!",
					    FALSE, ch, obj, attacker, TO_CHAR);
					act("$p &+Wglows &+was its pommel is slammed into $N&+w, &+wknocking $M away from YOU!",
					    FALSE, vict, obj, attacker, TO_CHAR);
					act("$n&+w's $q &+Wglows &+was its pommel smashes into you, &+wknocking you off-balance and back several steps!",
					    FALSE, ch, obj, attacker, TO_VICT);
				}
				else
				{
					act("&+LYour $q &+Rglows &+Las you slam the pommel into $N&+L, &+Lknocking $M away from your ally!",
					    FALSE, ch, obj, attacker, TO_CHAR);
					act("$p &+Rglows &+Las its pommel is slammed into $N&+L, &+Lknocking $M away from YOU!",
					    FALSE, vict, obj, attacker, TO_CHAR);
					act("$n&+L's $q &+Rglows &+Las its pommel smashes into you, &+Lknocking you off-balance and back several steps!",
					    FALSE, ch, obj, attacker, TO_VICT);
				}
				if (GET_OPPONENT(vict) == attacker)
					stop_fighting(vict);
				stop_fighting(attacker);
				set_fighting(attacker, ch);
				return TRUE;
			}
		}
	}
	return FALSE;

	/* Rewrote holy_weapon proc to be defensive or offensive by choice of wielder - Jexni 11/30/10
	  if(cmd == CMD_MELEE_HIT && !number(0, 24) && CheckMultiProcTiming(ch))
	  {
	    vict = legacy_proc_arg<P_char>(arg);

	    if( !vict )
	      return FALSE;

	    if( alignment == 0 )
	    {
	      act("Your $p&n fills you with &+WHOLY&n power!", FALSE, ch, obj, 0, TO_CHAR);
	      act("$n's $p&n fills $m with &+WHOLY&n power!", FALSE, ch, obj, 0, TO_ROOM);
	    }
	    else
	    {
	      act("Your $p&n fills you with &+LUNHOLY&n power!", FALSE, ch, obj, 0, TO_CHAR);
	      act("$n's $p&n fills $m with &+LUNHOLY&n power!", FALSE, ch, obj, 0, TO_ROOM);
	    }

	    for( int i = 0; (i < 2) && (GET_STAT(vict) != STAT_DEAD); i++ )
	      spell_full_harm(60, ch, 0, 0, vict, obj);

	    return TRUE;
	  }

	  if(cmd == CMD_GOTNUKED &&
	    !number(0, 9))
	  {

	    data = legacy_proc_arg<struct proc_data *>(arg);
	    vict = data->victim;

	    if( !vict )
	      return FALSE;

	    act("&+L$N&+L's $q &+Labsorbs $n&+L's spell!", FALSE, vict, obj, ch, TO_NOTVICT);
	    act("&+LYour spell is absorbed by $N&+L's $q&+L!", FALSE, vict, obj, ch, TO_CHAR);
	    act("&+L$n&+L's spell is absorbed by your $q&+L!", FALSE, vict, obj, ch, TO_VICT);

	    if( alignment == 0 && ( IS_EVIL(vict) || IS_RACEWAR_EVIL(vict) ) )
	    {
	      act("You are filled with &+WHOLY&n power!", FALSE, ch, obj, 0, TO_CHAR);
	      act("$n is filled with &+WHOLY&n power!", FALSE, ch, obj, 0, TO_ROOM);
	      spell_holy_word(60, ch, NULL, 0, vict, 0);
	    }
	    else if( alignment == 1 && ( IS_GOOD(vict) || IS_RACEWAR_GOOD(vict) ) )
	    {
	      act("You are filled with &+LUNHOLY&n power!", FALSE, ch, obj, 0, TO_CHAR);
	      act("$n is filled with &+LUNHOLY&n power!", FALSE, ch, obj, 0, TO_ROOM);
	      spell_unholy_word(60, ch, NULL, 0, vict, 0);
	    }

	    return TRUE;
	  }

	*/
}

/*
   Pooks/Erevan, added because Erevan is the God of Mischief. Sep 7 1994
 */
int banana(P_obj obj, P_char ch, int cmd, char *arg)
{
	int rand;

	/*
	   P_char t_ch;
	 */
	P_obj new_obj;
	char Gbuf1[MAX_STRING_LENGTH];

	/*
	 * peel:   v-num 1234
	 * banana: v-num 1235
	 */

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!obj || !ch || cmd == CMD_PERIODIC || !IS_AWAKE(ch))
		return (FALSE);

	if ((cmd < CMD_NORTH) || ((cmd > CMD_DOWN) && (cmd != CMD_EAT)))
		return (FALSE);

	/*
	   Eat for banana
	 */
	if (cmd == CMD_EAT)
	{
		one_argument(arg, Gbuf1);
		if (str_cmp(Gbuf1, "banana"))
			return (FALSE);
		if (obj->R_num == real_object(1235))
		{
			if (GET_COND(ch, FULL) > 20)
			{
				act("No thanks, I'm absolutely stuffed, couldn't eat another bite.",
				    FALSE, ch, 0, 0, TO_CHAR);
				return (TRUE);
			}
			else
			{
				act("$n eats $p. After $e finishes it, $e arrogantly tosses "
				    "the peel on the ground to rot.",
				    TRUE, ch, obj, 0, TO_ROOM);
				act("You eat the $o. After you're done, you arrogantly toss the "
				    "peel on the ground to rot.",
				    FALSE, ch, obj, 0, TO_CHAR);
				gain_condition(ch, FULL, obj->value[0]);
				CharWait(ch, PULSE_VIOLENCE);
				if (GET_COND(ch, FULL) > 20)
					act("You feel comfortably sated.", FALSE, ch, 0, 0,
					    TO_CHAR);
				extract_obj(obj, TRUE); // Not an arti, but 'in game.'
				obj = NULL;
				new_obj = read_object(1234, VIRTUAL);
				set_obj_affected(new_obj, 2400, TAG_OBJ_DECAY, 0);
				obj_to_room(new_obj, ch->in_room);
				return (TRUE);
			}
			return (TRUE);
		}
		return (FALSE);
	}
	/*
	   It's a peel and the player is moving...
	 */
	if (obj->R_num == real_object(1235))
		return (FALSE);

	if (IS_TRUSTED(ch))
		return (FALSE);

	if (IS_RIDING(ch)) /*
	                      Mounts are sure-footed :)
	                    */
		return (FALSE);

	if (IS_AFFECTED(ch, AFF_FLY) || IS_AFFECTED(ch, AFF_LEVITATE))
		return (FALSE);

	rand = number(1, STAT_INDEX(GET_C_INT(ch)));
	if (rand > 4)
	{ /*
		 Int check to be smart and not step on it
	   */
		return (FALSE);
	}
	else
	{ /*
		 They stepped on it... random dex roll
		 determines how bad...
	   */
		rand = number(1, STAT_INDEX(GET_C_DEX(ch)));
		switch (rand)
		{
		case 1:
			act("You slip on a banana peel, fall, and pass out when your head smacks the ground!",
			    FALSE, ch, 0, 0, TO_CHAR);
			act("$n slips on a banana peel, falls over with a surprised look on $s "
			    "face, and passes out when $s head smacks the ground!",
			    TRUE, ch, 0, 0, TO_ROOM);
			if (GET_OPPONENT(ch))
				stop_fighting(ch);
			KnockOut(ch, 6);
			SET_POS(ch, GET_STAT(ch) + POS_PRONE);
			/*
				      bzero(&af, sizeof(af));
				      af.type = SPELL_SLEEP;
				      af.duration = number(4, 6);
				      af.bitvector = AFF_SLEEP;
				      SET_POS(ch, GET_POS(ch) + STAT_SLEEPING);
				      affect_join(ch, &af, FALSE, FALSE);
				*/

			StopMercifulAttackers(ch);
			GET_HIT(ch) = (GET_HIT(ch) > 15) ? GET_HIT(ch) - 15 : 1;
			return (TRUE);
		case 2:
		case 3:
		case 4:
		case 5:
			/*
				   Might as well use rand again... good as anything
				 */
			GET_HIT(ch) = (GET_HIT(ch) > rand) ? GET_HIT(ch) - rand : 1;
			act("You slip on a banana peel and fall over with a shriek and a thump!\n"
			    "\nYou hurt yourself in the fall!",
			    TRUE, ch, 0, 0, TO_CHAR);
			act("$n slips on a banana peel, shrieks, and falls over!", TRUE, ch, 0, 0,
			    TO_ROOM);
			SET_POS(ch, POS_SITTING + GET_STAT(ch));
			CharWait(ch, PULSE_VIOLENCE);
			return (TRUE);
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			act("You step on a banana peel!", TRUE, ch, 0, 0, TO_CHAR);
			act("Arms flailing, you manage you maintain your balance and end up looking only like a small fool.",
			    TRUE, ch, 0, 0, TO_CHAR);
			act("$n steps on a banana peel and, arms flailing wildly, barely maintains $s balance.",
			    TRUE, ch, 0, 0, TO_ROOM);
			CharWait(ch, PULSE_VIOLENCE);
			return (TRUE);
		default:
			act("You step on a banana peel, but manage to dance your way out of the area.",
			    TRUE, ch, 0, 0, TO_CHAR);
			act("$n steps on a banana peel, but with a quick smirk resumes $s travel.",
			    TRUE, ch, 0, 0, TO_ROOM);
			return (FALSE);
		}
	}
	return (FALSE);
}

void good_evil_procDrain(P_char ch, P_obj obj, P_char opponent, int *swordMana)
{
	act("Your $p &+Rglows&N as it &+rbites deeply&n into $N&n", FALSE, ch, obj, opponent,
	    TO_CHAR);
	act("$n's $p &+rbites deeply&n into you, draining your &+Wlife force&n.", FALSE, ch, obj,
	    opponent, TO_VICT);
	act("$N &+Lpales&n as $S &+Wlife force&n is drained significantly.", FALSE, ch, obj,
	    opponent, TO_NOTVICTROOM);
	GET_HIT(opponent) -= 50;
	GET_HIT(ch) += 50;
	*swordMana += 100;
}

int good_evil_attemptFightProc(P_char ch, P_obj obj, P_char opponent, int procMana, int *swordMana)
{
	act("$p &+Wglows brightly&N for a moment.", FALSE, ch, obj, 0, TO_CHAR);
	act("$p &+Wglows brightly&N for a moment.", FALSE, ch, obj, 0, TO_ROOM);

	if (procMana < *swordMana)
	{
		*swordMana -= procMana;
		obj->value[6] = (obj->value[6] + 1) % 15;
		return TRUE;
	}
	good_evil_procDrain(ch, obj, opponent, swordMana);
	return FALSE;
}

int good_evil_attemptDefenseProc(P_char ch, P_obj obj, int procMana, int *swordMana)
{
	if (procMana < *swordMana)
	{
		act("$p &+Wglows brightly for a moment.&N", FALSE, ch, obj, 0, TO_CHAR);
		act("$p &+Wglows brightly for a moment.&N", FALSE, ch, obj, 0, TO_ROOM);
		*swordMana -= procMana;
		obj->value[6] = (obj->value[6] + 1) % 5;
		return TRUE;
	}

	/* Maybe we should punish the wielder here */

	return FALSE;
}

void good_evil_spellUp(P_char ch)
{
	/* Put in some cool spells here... deflect and blur for example */
	spell_blur(60, ch, 0, 0, ch, 0);
	spell_deflect(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
}

void good_evil_startBigFight(P_char attacker, P_char defender, int goodieStarted, P_obj obj)
{
	statuslog(AVATAR, "Mayhem and Symmetry are facing off at [%d]!",
		  world[attacker->in_room].number);

	/* USE act(...) HERE A FEW TIMES AND PUT IN SOME COOL MESSAGES */
	act("&+WUnimaginable &+WP&+CO&+BW&+bER flows through you as $p &+Rcompels&+L you to strike down its nemesis!&N",
	    FALSE, attacker, obj, 0, TO_CHAR);
	send_to_char("&+YYou are compelled to fight without any self control!&N\n", attacker);
	if (goodieStarted)
	{
		act("$p &+Wenvelops $n &+Win a magnificent aura as the battle begins!&N", FALSE,
		    attacker, obj, 0, TO_ROOM);
		act("$n charges forth and strikes at $N!", FALSE, attacker, obj, defender, TO_ROOM);
	}
	else
	{
		act("$p &+renshrouds $n &+rin a menacing aura as the battle begins!&N", FALSE,
		    attacker, obj, 0, TO_ROOM);
		act("$n charges forth and strikes at $N!", FALSE, attacker, obj, defender, TO_ROOM);
	}

	good_evil_spellUp(attacker);
	good_evil_spellUp(defender);
	obj->value[5] = TRUE;
	if (GET_OPPONENT(attacker) != defender)
		stop_fighting(attacker);
	if (GET_OPPONENT(defender) != attacker)
		stop_fighting(defender);

	// Original crashes - Clav
	// set_fighting(attacker, defender);
	// set_fighting(defender, attacker);

	attack(attacker, defender);
	attack(defender, attacker);
}

void good_evil_coolDown(P_obj obj, P_char ch)
{
	/* PUT IN SOME COOL DOWN MESSAGE HERE */
	act("&+bColdness seeps into you as $p &+btakes it's toll on you.&N", FALSE, ch, obj, 0,
	    TO_CHAR);
	act("$p &+Ldarkens and the &+Waura&+L surrounding $n dies down.&N", FALSE, ch, obj, 0,
	    TO_ROOM);
	spell_dispel_magic(60, ch, NULL, SPELL_TYPE_SPELL, ch, NULL);
	obj->value[7] = -10000;
	obj->value[5] = FALSE;
}

int good_evil_stoneOrSoulshield(P_obj obj)
{
	int curr_time;
	P_char temp_ch;

	temp_ch = obj->loc.wearing;

	if ((GET_ALIGNMENT(temp_ch) < 900) && (GET_ALIGNMENT(temp_ch) > -900))
	{
		if (!FIRESHIELDED(temp_ch))
			spell_fireshield(55, temp_ch, 0, SPELL_TYPE_SPELL, temp_ch, 0);
	}
	else
	{
		if (!IS_AFFECTED2(temp_ch, AFF2_SOULSHIELD))
			spell_soulshield(55, temp_ch, 0, SPELL_TYPE_SPELL, temp_ch, 0);
	}
	curr_time = time(NULL);
	if (obj->timer[0] + 10 <= curr_time && !has_skin_spell(temp_ch))
	{
		spell_stone_skin(55, temp_ch, 0, SPELL_TYPE_SPELL, temp_ch, 0);
		obj->timer[0] = curr_time;
		return TRUE;
	}

	return FALSE;
}

int good_evil_fightingProc(P_char ch, P_obj obj, int isGood, int mana)
{
	/* PUT IN USEFUL VALUES HERE */
	int manaCosts[] = {
		50, /* 0  dazzle */
		50, /* 1  blind */
		50, /* 2  curse */
		100, /* 3  bigby's hand */
		0, /* 4  keep at zero for mana draining */
		100, /* 5  heal */
		100, /* 6  fist */
		50, /* 7  immolate */
		100, /* 8  earthquake */
		0, /* 9  keep at zero for mana draining */
		100, /* 10 gStornog */
		50, /* 11 poison */
		100, /* 12 (un)holy word */
		0, /* 13 keep at zero for mana draining */
		200 /* 14 apocalypse / judgement */
	};

	int rand, save;
	P_char tar_ch, next;

	P_char opponent;

	mana += 100;
	if (mana > GET_HIT(ch))
		mana = GET_HIT(ch);

	if (isGood)
		rand = obj->value[6];
	else
		rand = number(0, 14);

	opponent = GET_OPPONENT(ch);
	if (opponent == NULL)
		return mana;

	if (good_evil_attemptFightProc(ch, obj, opponent, manaCosts[rand], &mana))
	{
		switch (rand)
		{
		case 0:
			break;
		case 1:
			/* blind */
			act("&+LA dark cloud shoots forth toward you!&N", FALSE, ch, 0, opponent,
			    TO_VICT);
			act("&+LA dark cloud shoots forth toward $n!&N", TRUE, opponent, 0, 0,
			    TO_ROOM);
			save = opponent->specials.apply_saving_throw[SAVING_SPELL];
			opponent->specials.apply_saving_throw[SAVING_SPELL] += 15;
			spell_blindness(60, ch, 0, SPELL_TYPE_SPELL, opponent, 0);
			opponent->specials.apply_saving_throw[SAVING_SPELL] = save;
			break;
		case 2:
			/* curse */
			act("&+rA red cloud shoots forth toward you!&N", FALSE, ch, 0, opponent,
			    TO_VICT);
			act("&+rA red cloud shoots forth toward $n!&N", TRUE, opponent, 0, 0,
			    TO_ROOM);
			save = opponent->specials.apply_saving_throw[SAVING_SPELL];
			opponent->specials.apply_saving_throw[SAVING_SPELL] += 15;
			spell_curse(60, ch, 0, SPELL_TYPE_SPELL, opponent, 0);
			opponent->specials.apply_saving_throw[SAVING_SPELL] = save;
			break;
		case 11:
			/* poison */
			act("&+GA green cloud shoots forth toward you!&N", FALSE, ch, 0, opponent,
			    TO_VICT);
			act("&+GA green cloud shoots forth toward $n!&N", TRUE, opponent, 0, 0,
			    TO_ROOM);
			save = opponent->specials.apply_saving_throw[SAVING_SPELL];
			opponent->specials.apply_saving_throw[SAVING_SPELL] += 15;
			spell_poison(30, ch, 0, SPELL_TYPE_SPELL, opponent, 0);
			opponent->specials.apply_saving_throw[SAVING_SPELL] = save;
			break;
		case 4:
		case 9:
		case 13:
			/* drain mana */
			good_evil_procDrain(ch, obj, opponent, &mana);
			break;
		case 5:
			/* group heal */
			for (tar_ch = world[ch->in_room].people; tar_ch; tar_ch = next)
			{
				next = tar_ch->next_in_room;
				if (tar_ch->group == ch->group)
				{
					spell_heal(55, ch, 0, 0, tar_ch, 0);
				}
			}
			spell_heal(20, ch, 0, 0, ch, 0);
			break;
		case 6:
			/* fist */
			spell_bigbys_clenched_fist(60, ch, NULL, SPELL_TYPE_SPELL, opponent, 0);
			break;
		case 7:
			/* immolate */
			spell_immolate(60, ch, NULL, 0, opponent, 0);
			break;
		case 8:
			/* earthquake */
			spell_earthquake(60, ch, NULL, SPELL_TYPE_SPELL, NULL, 0);
			break;
		case 10:
			/* group stornog */
			spell_group_stornog(56, ch, 0, 0, ch, NULL);
			break;
		case 3:
			/* hand */
			spell_bigbys_crushing_hand(60, ch, NULL, SPELL_TYPE_SPELL, opponent, 0);
			break;
		case 12:
			/* holy word / unholy word */
			if (isGood)
			{
				if (GET_ALIGNMENT(ch) > -350)
					spell_holy_word(60, ch, NULL, 0, opponent, 0);
			}
			else
			{
				if (GET_ALIGNMENT(ch) < 350)
					spell_unholy_word(60, ch, NULL, 0, opponent, 0);
			}
			break;
		case 14:
			spell_nova(60, ch, 0, 0, NULL, 0);
			break;
		}
	}

	return mana;
}

int good_evil_defenseProc(P_char ch, P_obj obj, int isGood, int mana)
{
	int rand;

	/* PUT IN USEFUL VALUES HERE */
	int manaCosts[] = {
		30, /* gStone / gStornog */
		10, /* gHeal */
		10, /* gVigCrit */
		10, /* gProt cold+fire+acid+gas+lightning */
		10 /* group armor+bless */
	};

	P_char tar_ch, next;

	if (number(0, 14))
		return mana;

	if (isGood)
		rand = obj->value[6];
	else
		rand = number(0, 4);

	if (good_evil_attemptDefenseProc(ch, obj, manaCosts[rand], &mana))
	{
		switch (rand)
		{
		case 0:
			/* gStone / gStornog */
			if (!IS_AFFECTED4(ch, AFF4_STORNOGS_SPHERES) || !number(0, 3))
			{
				spell_group_stornog(60, ch, 0, 0, ch, NULL);
				break;
			}
			else
			{
				spell_group_stone_skin(45, ch, 0, 0, ch, NULL);
				break;
			}

		case 1:
			/* gHeal */

			// this only "counts" if someone in the group actually needs to be healed!
			for (tar_ch = world[ch->in_room].people; tar_ch; tar_ch = next)
			{
				next = tar_ch->next_in_room;
				if ((tar_ch->group == ch->group) &&
				    (GET_HIT(tar_ch) < GET_MAX_HIT(tar_ch)))
					break;
			}
			if (tar_ch)
			{
				spell_group_heal(50, ch, 0, 0, ch, 0);
				break;
			}
			[[fallthrough]];

		case 2:
			/* gVigCrit */
			for (tar_ch = world[ch->in_room].people; tar_ch; tar_ch = next)
			{
				next = tar_ch->next_in_room;
				if (tar_ch->group == ch->group)
				{
					spell_vigorize_critic(50, ch, 0, 0, tar_ch, 0);
				}
			}
			break;
		case 3:
			/* gProt cold+fire+acid+gas+lightning */
			for (tar_ch = world[ch->in_room].people; tar_ch; tar_ch = next)
			{
				next = tar_ch->next_in_room;
				if (tar_ch->group == ch->group)
				{
					spell_protection_from_cold(50, ch, 0, 0, tar_ch, 0);
					spell_protection_from_fire(50, ch, 0, 0, tar_ch, 0);
					spell_protection_from_acid(50, ch, 0, 0, tar_ch, 0);
					spell_protection_from_gas(50, ch, 0, 0, tar_ch, 0);
					spell_protection_from_lightning(50, ch, 0, 0, tar_ch, 0);
				}
			}
			break;
		case 4:
			/* group armor+bless */
			for (tar_ch = world[ch->in_room].people; tar_ch; tar_ch = next)
			{
				next = tar_ch->next_in_room;
				if (tar_ch->group == ch->group)
				{
					spell_armor(50, ch, 0, 0, ch, 0);
					spell_bless(50, ch, 0, 0, ch, 0);
				}
			}
			break;
		}
	}

	return mana;
}

int good_evil_checkHunger(P_char ch, P_obj obj, int mana)
{
	if (mana < 0 && !IS_TRUSTED(ch)) // punishment
	{
		act("$p &+rbecomes hungry and saps some of your strength.&N", FALSE, ch, obj, 0,
		    TO_CHAR);
		act("$p &+rbecomes hungry and saps some of $n's strength.&N", FALSE, ch, obj, 0,
		    TO_ROOM);
		if (GET_CLASS(ch, CLASS_WARRIOR))
		{
			GET_HIT(ch) >>= 1;
		}
		else
		{
			spell_dispel_magic(60, ch, NULL, SPELL_TYPE_SPELL, ch, NULL);
			GET_HIT(ch) -= GET_HIT(ch) / 4;
		}

		mana = GET_MAX_HIT(ch) / 2;
	}

	return mana;
}

int killOtherSword(P_obj obj, P_char ch, int isGood)
{
	P_char opponent;
	int enemySwordVNum;

	if (!OBJ_WORN(obj))
		return FALSE;

	enemySwordVNum = isGood ? 21 : 22;

	if (IS_FIGHTING(ch) && isWieldingVnum(GET_OPPONENT(ch), enemySwordVNum))
	{ /* already fighting the other sword */
		obj->value[5] = TRUE;
		obj->value[7] = -10000;
		return FALSE;
	}

	LOOP_THRU_PEOPLE(opponent, ch)
		if ((opponent != ch) && (!IS_TRUSTED(opponent)))
			if (isWieldingVnum(opponent, enemySwordVNum))
			{
				good_evil_startBigFight(ch, opponent, isGood, obj);
				return TRUE;
			}

	if (obj->value[5] == TRUE)
		// I WAS in a big fight, but not now.. cool down!
		good_evil_coolDown(obj, ch);
	return FALSE;
}

void good_evil_poofSword(P_char ch, P_obj obj)
{
	/* Zap the char and poof */
	act("$p &+Wflares up&n, burns your hands, and vaporizes!&N", FALSE, ch, obj, 0, TO_CHAR);
	act("$p &+Wflares up&n, burns $n's hands, and vaporizes!&N", FALSE, ch, obj, 0, TO_ROOM);
	GET_HIT(ch) -= 100;

	// Don't need to remove from ch anymore, extract_obj handles it.
	extract_obj(obj, TRUE); // Bye arti sword.

	/* Old code for putting it in a new room. (tweaked)
	  if( OBJ_WORN(obj) )
	  {
	    for( int i = 0; i < MAX_WEAR; i++ )
	    {
	      if( ch->equipment[i] == obj )
	      {
	        unequip_char(ch, i);
	        break;
	      }
	    }
	  }
	  else if( OBJ_CARRIED(obj) )
	  {
	    obj_from_char(obj);
	  }
	  // OBJ_ROOM or OBJ_INSIDE
	  else if( !OBJ_NOWHERE(obj) )
	  {
	    return;
	  }
	  // Go to any random room in the game not in heaven.
	  // Should tweak this to not land on mountains or other !accessible areas.
	  obj_to_room(obj, number(zone_table[0].real_top + 1, top_of_world));
	*/
}

void good_evil_configSword(P_char ch, P_obj obj)
{
	// configure the sword as 1h or 2h depending on the wielder, and set the dice
	// as follows:
	//  1h 5d5 5/5
	//  2h 6d6 6/6

	// classes which use as a 2h:  paladin, anti-paladin,

	// if the object is already worn, or there's no ch, then don't do anything
	if (OBJ_WORN(obj) || !ch)
		return;

	// The new cycle belongs to the physical item across custody changes.
	if (native_artifact_owns(OBJ_VNUM(obj)))
		obj->value[6] = BOUNDED(0, obj->value[6], 14);
	else
		obj->value[6] = 0; // which "random" effect
	obj->value[5] = FALSE; // currently in noflee fight

	obj->affected[0].location = APPLY_HITROLL;
	obj->affected[1].location = APPLY_DAMROLL;

	if (GET_CLASS(ch, CLASS_PALADIN) || GET_CLASS(ch, CLASS_ANTIPALADIN) ||
	    GET_CLASS(ch, CLASS_AVENGER) || GET_CLASS(ch, CLASS_DREADLORD) ||
	    (GET_RACE(ch) == RACE_OGRE) || (GET_RACE(ch) == RACE_MINOTAUR))
	{
		SET_BIT(obj->extra_flags, ITEM_TWOHANDS);
		obj->value[0] = 13;
		obj->value[1] = obj->value[2] = 6; // 6d6
		obj->affected[0].modifier = obj->affected[1].modifier = 6;
		obj->weight = 15;
	}
	else
	{
		REMOVE_BIT(obj->extra_flags, ITEM_TWOHANDS);
		obj->value[0] = 5;
		obj->value[1] = obj->value[2] = 5; // 5d5
		obj->affected[0].modifier = obj->affected[1].modifier = 5;
		obj->weight = 7;
	}
}

/* 'Symmetry', the uniform sword of light */
int good_evil_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
	bool bIsEvil = FALSE, bIsGood = FALSE;
	bool bBumpedOthers = FALSE;
	int slot, mana, i;
	char tmp_buf[300];
	char curWhisper[300];
	static int num_attacks = 0;
	const char **whisperings;
#define SWORD_WHISPERINGS 7
	const char *e_whispers[SWORD_WHISPERINGS] = {
		"I crave blood!",
		"Serve me well, and you shall be given unimaginable power!",
		"This world has not yet seen our true power, show them!",
		"Satisfy my thirst for blood, my minion.",
		"Bring me to Symmetry so that I can absorb its power.", // 5
		"All your base are belong to us!",
		"Slice your enemies apart with me, so that I may absorb their souls."
	};
	const char *g_whispers[SWORD_WHISPERINGS] = {
		"Persue the path of goodness always.",
		"Study the way of 'pleasantry' that you may better love the gods.",
		"You must destroy the non-believers.",
		"This world shall be cleansed of evil by our power.",
		"You must vanquish more evil for me to aid you more.", // 5
		"All your base are belong to us!",
		"Do not be swayed to the dark side."
	};

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!obj)
	{
		return FALSE;
	}

	if (21 == obj_index[obj->R_num].virtual_number)
		bIsEvil = TRUE;
	else if (22 == obj_index[obj->R_num].virtual_number)
		bIsGood = TRUE;
	else
		return FALSE;
	const bool modern = native_artifact_owns(OBJ_VNUM(obj));
	const bool combat_event = cmd == CMD_MELEE_HIT || cmd == CMD_GOTHIT || cmd == CMD_GOTNUKED;

	// wield - if we might be wielding the sword, configure it
	// as required and then return FALSE (acting like we didn't do anything)
	if (cmd == CMD_WIELD && *arg)
	{
		if (!OBJ_WORN(obj))
		{
			good_evil_configSword(ch, obj);
		}
		// Let the normal wield code deal with it now.
		return FALSE;
	}

	if (OBJ_WORN(obj) && (ch = obj->loc.wearing))
	{
		if (!IS_SET(obj->extra_flags, ITEM_NODROP))
		{
			SET_BIT(obj->extra_flags, ITEM_NODROP);
		}
		if (!char_in_list(ch) || !IS_ALIVE(ch))
		{
			return FALSE;
		}
		good_evil_configSword(ch, obj);
		// if worn, but not as primary weapon, unequip
		if (ch->equipment[PRIMARY_WEAPON] != obj)
		{
			// It ONLY wants to be worn as primary slot!
			bBumpedOthers = TRUE;
			// Figure out what slot they have it in, and remove it!
			for (slot = 0; slot < MAX_WEAR; slot++)
			{
				if (ch->equipment[slot] == obj)
				{
					break;
				}
			}
			if (slot < MAX_WEAR)
			{
				obj_to_char(unequip_char(ch, slot), ch);
			}
		}
	}
	if (OBJ_CARRIED(obj) && char_in_list(obj->loc.carrying) && IS_ALIVE(obj->loc.carrying) &&
	    !IS_TRUSTED(obj->loc.carrying))
	{
		ch = obj->loc.carrying;
		good_evil_configSword(ch, obj);
		if (ch->equipment[PRIMARY_WEAPON])
		{
			obj_to_char(unequip_char(ch, PRIMARY_WEAPON), ch);
			bBumpedOthers = TRUE;
		}
		if (ch->equipment[HOLD])
		{
			obj_to_char(unequip_char(ch, HOLD), ch);
			bBumpedOthers = TRUE;
		}
		if (ch->equipment[WEAR_SHIELD])
		{
			obj_to_char(unequip_char(ch, WEAR_SHIELD), ch);
			bBumpedOthers = TRUE;
		}
		if (IS_SET(obj->extra_flags, ITEM_TWOHANDS) && ch->equipment[SECONDARY_WEAPON])
		{
			obj_to_char(unequip_char(ch, SECONDARY_WEAPON), ch);
			bBumpedOthers = TRUE;
		}
		if (bBumpedOthers)
		{
			// send a message :)
			act("Forcing aside your other equipment, $p shoves itself into your hands, ready for battle!",
			    TRUE, ch, obj, NULL, TO_CHAR);
			act("Forcing aside $s other equipment, $p shoves itself $n's hands, ready for battle!",
			    TRUE, ch, obj, NULL, TO_ROOM);
		}
		else
		{
			act("$p shoves itself into your hands, ready for battle!", TRUE, ch, obj,
			    NULL, TO_CHAR);
			act("$p shoves itself $n's hands, ready for battle!", TRUE, ch, obj, NULL,
			    TO_ROOM);
		}
		const uint64_t actor_id = ch->runtime_id, source_uid = obj->obj_uid;
		obj_from_char(obj);
		equip_char(ch, obj, PRIMARY_WEAPON, 0);
		if (modern)
		{
			ch = find_character_by_runtime_id(actor_id);
			if (!IS_ALIVE(ch) || !(obj = ch->equipment[PRIMARY_WEAPON]) ||
			    obj->obj_uid != source_uid)
				return combat_event ? FALSE : TRUE;
		}
	}

	if (!char_in_list(ch) || !IS_ALIVE(ch))
	{
		return FALSE;
	}
	if (modern)
	{
		const int result = advance_sword_artifact(obj, ch, cmd, arg);
		return combat_event ? FALSE : result;
	}
	if (cmd == CMD_LOOK)
	{
		if (isname(arg, obj->name) && OBJ_WORN(obj))
		{
			snprintf(tmp_buf, sizeof tmp_buf, "&+LHealth remaining:&+r %d&N\n",
				 -(obj->value[7]));
			send_to_char(tmp_buf, ch);
			return TRUE;
		}
	}
	if (!IS_TRUSTED(ch))
	{
		if (!IS_NPC(ch) &&
		    ((bIsEvil && IS_RACEWAR_GOOD(ch)) || (bIsGood && IS_RACEWAR_EVIL(ch))))
		{
			good_evil_poofSword(ch, obj);
			return combat_event ? FALSE : TRUE; // The source was extracted.
		}

		if (killOtherSword(obj, ch, bIsGood))
		{
			return TRUE;
		}

		// if in "the big fight", prevent attempts to disengage
		if (obj->value[5] && attemptToDisengage(ch, cmd, arg))
		{
			return TRUE;
		}
	}
	if (cmd == CMD_PERIODIC)
	{
		whisperings = bIsEvil ? e_whispers : g_whispers;
		if (!OBJ_WORN_BY(obj, ch))
		{
			return FALSE;
		}
		// 1/30 chance.
		if (!number(0, 29))
		{
			snprintf(curWhisper, sizeof curWhisper, "$p whispers into your mind '%s&n'",
				 whisperings[number(0, SWORD_WHISPERINGS - 1)]);
			act(curWhisper, FALSE, ch, obj, 0, TO_CHAR);
		}
		good_evil_stoneOrSoulshield(obj);

		if (IS_TRUSTED(ch))
		{
			mana = 10000;
		}
		else
		{
			mana = -(obj->value[7]);
		}

		if (IS_FIGHTING(ch))
		{
			mana = good_evil_fightingProc(ch, obj, bIsGood, mana);
		}
		else
		{
			mana = good_evil_defenseProc(ch, obj, bIsGood, mana);
		}

		mana = good_evil_checkHunger(ch, obj, mana);
		if (!IS_TRUSTED(ch))
		{
			mana -= 3;
		}
		if (mana <= 30)
		{
			send_to_char(
				"&+LYou feel a strong urge to kill something emanating from your weapon!&N\n",
				ch);
		}
		obj->value[7] = -mana;
		return TRUE;
	}
	else if (obj->value[5] && (cmd / 1000))
	{
		obj->value[7] = -10000;
		mana = 10000;
		if (!number(0, 3) && !IS_AFFECTED4(ch, AFF4_DEFLECT))
		{
			spell_deflect(60, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
		}
		else if (!number(0, 2))
		{
			obj->value[6] = (obj->value[6] + 1) % 15;
			good_evil_fightingProc(ch, obj, bIsGood, mana);
		}
		else if (!num_attacks && !number(0, 3))
		{
			num_attacks = number(3, 5);
			P_char victim = GET_OPPONENT(ch);
			if (!victim || !char_in_list(victim) || !IS_ALIVE(victim))
				return FALSE;

			act("$p &+Wflares up, slashing your opponent with incredible speed!&n",
			    TRUE, ch, obj, victim, TO_CHAR);
			act("&+W$n's&N $p&+W flares up, slashing $N with incredible speed!&n", TRUE,
			    ch, obj, victim, TO_NOTVICT);
			act("&+W$n's&N $p&+W flares up, slashing YOU with incredible speed!&n",
			    TRUE, ch, obj, victim, TO_VICT);
			for (i = 0; i < num_attacks; i++)
			{
				const attack_continuation continuation =
					begin_attack_continuation(ch, victim, obj);
				hit(ch, victim, obj);

				const attack_continuation_result after_hit =
					check_attack_continuation(continuation);
				if (!after_hit.can_continue())
					break;

				ch = after_hit.actor;
				victim = after_hit.target;
				obj = after_hit.weapon;
			}
		}
	}
	return FALSE;
}

void dispel_portal(P_char ch, P_obj obj)
{
	P_obj obj2;

	if (GET_LEVEL(ch) < 46)
	{
		act("$p easily resists your assault!", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}
	else if ((GET_LEVEL(ch) < 50) && !number(0, 1))
	{
		act("$p resists your assault!", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}

	// success, search for portal on other side

	obj2 = world[real_room(obj->value[0])].contents;

	while (obj2)
	{
		if (obj2->R_num == obj->R_num && (obj2->value[7] == obj->value[7]) && (obj2 != obj))
			break;

		obj2 = obj2->next_content;
	}

	if (!obj2)
	{ // means we scrolled whole content of another side and cannot find portal
		send_to_char("bug in dispelling portals, notify a god.\n", ch);
		return;
	}

	// decay them both
	Decay(obj);
	Decay(obj2);
}

//---------------------------------------------------------
// general portal actions: dispel,look in, enter
// (msg comes from portal hooks)
//---------------------------------------------------------
int portal_general_internal(P_obj obj, P_char ch, int cmd, char *arg,
			    struct portal_action_messages *msg)
{
	int to_room;
	P_char dummy;
	P_obj obj2 = NULL;

	if (cmd == CMD_DISPEL)
	{
		dispel_portal(ch, obj);
		return TRUE;
	}

	if (cmd == CMD_DECAY)
	{
		// if some decay message is not set, then we will use generic obj decay
		if (!msg->decay_to_room || !msg->decay_to_char)
			return FALSE;

		if (world[obj->loc.room].people)
		{
			act(msg->decay_to_room, FALSE, world[obj->loc.room].people, obj, 0,
			    TO_ROOM);
			act(msg->decay_to_char, FALSE, world[obj->loc.room].people, obj, 0,
			    TO_CHAR);
		}
		return TRUE;
	}

	// parse thru "in"
	if (cmd == CMD_LOOK)
	{
		while (*arg == ' ')
			arg++;
		if (!*arg)
			return FALSE;
		if (strn_cmp(arg, "in ", 3))
			return FALSE;
		arg += 3;
	}

	// Get the portal object, since there may be more than one (skipping tracks)
	generic_find(arg, FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &dummy, &obj2);

	// If the object is not the one we seek then return false
	if (obj2 != obj)
		return FALSE;

	to_room = real_room(obj->value[0]);
	if (to_room == NOWHERE)
	{
		send_to_char(msg->bug_to_char, ch);
		return (TRUE);
	}
	if (cmd == CMD_LOOK)
	{
		act("You peer into $p and see...", 0, ch, obj, 0, TO_CHAR);
		if (0)
			send_to_char("It is pitch black over there...\n", ch);
		else
		{
			send_to_char(world[to_room].name, ch);
			send_to_char("\n", ch);
		}
		return TRUE;
	}

	// somehow other command passed, NO WAY! squash it! (why? because only enter processing follows)
	if (cmd != CMD_ENTER)
		return FALSE;

	/* otherwise cmd == enter */

	//--------------------------------
	// check timers
	// 1. initial stabilization
	if ((obj->value[4] > 0) && (time(0) - obj->timer[0]) < obj->value[4])
	{
		send_to_char(msg->wait_init_to_char, ch);
		return TRUE;
	}

	// 2. post enter stabilization
	if (obj->timer[1] > 0 && obj->value[5] > 0 && (time(0) - obj->timer[1]) < obj->value[5])
	{
		send_to_char(msg->wait_to_char, ch);
		return TRUE;
	}
	//--------------------------------

	if (!can_enter_room(ch, to_room, FALSE) ||
	    ((obj->value[1] == RACE_ILLITHID) && (!IS_ILLITHID(ch))) ||
	    (IS_ROOM(ch->in_room, ROOM_ARENA) != IS_ROOM(to_room, ROOM_ARENA)))
	{
		send_to_char("A strong force pushes you back!\n", ch);
		return TRUE;
	}

	/*
	act("&+W$p suddenly glows brightly!", FALSE, ch, obj, 0, TO_ROOM);
  */

#if defined(CTF_MUD) && (CTF_MUD == 1)
	if (ctf_carrying_flag(ch) == CTF_PRIMARY)
	{
		send_to_char("You can't carry that with you.\r\n", ch);
		drop_ctf_flag(ch);
	}
#endif

	act(msg->step_in_to_room, FALSE, ch, obj, 0, TO_ROOM);
	char_from_room(ch);
	act(msg->step_in_to_char, FALSE, ch, obj, 0, TO_CHAR);

	/* Probability of in transit instability - SKB 13 Feb 1998 */
	/*  if (!number(0, 99))
	        to_room = real_room(number(99900,99999));*/

	char_to_room(ch, to_room, -1);
	act(msg->step_out_to_room, FALSE, ch, obj, 0, TO_ROOM);

	//------------------------
	// reset enter stabilization as someone entered
	obj->timer[1] = time(0);
	//------------------------

	// add lag after step out from portal
	if (obj->value[6] > 0)
		// value is set in seconds, convert into pulses
		CharWait(ch, obj->value[6] * WAIT_SEC);

	/* if obj->value[2] > 0, don't drop below 1, or else portal won't disappear properly */
	// decay when limit enters
	if (obj->value[2] > 0)
	{
		obj->value[2] = BOUNDED(1, obj->value[2], 9999);
		obj->value[2]--;
		if (obj->value[2] == 0)
			Decay(obj); // ??? dont decay another portal side?
	}

	return TRUE;
}

// common portals hook used for some portals/gates
// when we lazy enough to write new messages and dont need special checks
int portal_door(P_obj obj, P_char ch, int cmd, char *arg)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	// what commands invokes portal actions
	if ((cmd == CMD_DECAY) || (cmd == CMD_DISPEL) ||
	    (((cmd == CMD_ENTER) || (cmd == CMD_LOOK)) && ch))
	{
		/*
		 if(ch && !is_Raidable(ch, 0, 0))
		 {
		   send_to_char("&=LWYou are not raidable! You shall not pass!\r\n", ch);
		   return false;
		 }
	   */

		struct portal_action_messages msg = {
			/*in ch    */ "You enter $p and reappear elsewhere...",
			/*in ch r  */
			"&+W$p suddenly glows brightly!\n"
			"$n enters $p and disappears among the mist.",
			/*out ch   */ 0,
			/*out ch r */ "$n steps out of $p.",
			/*wait init*/ "It hasn't solidified yet...\n",
			/*wait stb */ "It hasn't solidified again yet...\n",
			/*decay ch */ "$p dissolves in a swirl of colors and is gone.",
			/*decay r  */ "$p dissolves in a swirl of colors and is gone.",
			/*bug      */ "Bug with portal!!! Tell a god.\n"
		};

		return portal_general_internal(obj, ch, cmd, arg, &msg);
	}

	return FALSE;
}

int portal_wormhole(P_obj obj, P_char ch, int cmd, char *arg)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if ((cmd == CMD_DECAY) || (cmd == CMD_DISPEL) ||
	    (((cmd == CMD_ENTER) || (cmd == CMD_LOOK)) && ch))
	{
		if (ch && !is_Raidable(ch, 0, 0))
		{
			send_to_char("&=LWYou are not raidable! You shall not pass!\r\n", ch);
			return false;
		}

		struct portal_action_messages msg = {
			/*in ch    */
			"&+LAs you enter $p&+L, you feel yourself being torn into a thousand pieces,\n"
			"&+Lscattered over the entirety of reality.  Bits of your shattered\n"
			"&+Lconsciousness float randomly about the universe with no overall\n"
			"&+Ldirection or purpose.  Suddenly, you find yourself elsewhere..",
			/*in ch r  */ "$n steps into $p and disappears among the darkness.",
			/*out ch   */ 0,
			/*out ch r */ "$n stumbles out of $p.",
			/*wait init*/ "The rift isn't quite stable enough yet...\n",
			/*wait stb */ "The rift isn't quite stable enough again yet...\n",
			/*decay ch */ 0,
			/*decay r  */ 0,
			/*bug      */ "Bug with wormhole!!! Tell a god.\n"
		};

		/*  if (GET_RACE(ch) != RACE_ILLITHID) {
		     send_to_char("&+WYour mind is too puny to survive the trip.\n", ch);
		     return TRUE;
		    }
		    if (GET_LEVEL(ch) < 41) {
		     send_to_char("&+WYour mind is not strong enough to survive the trip.\n", ch);
		     return TRUE;
		    }
		*/

		return portal_general_internal(obj, ch, cmd, arg, &msg);
	}

	return FALSE;
}

int portal_etherportal(P_obj obj, P_char ch, int cmd, char *arg)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	// what commands invokes portal actions
	if ((cmd == CMD_DECAY) || (cmd == CMD_DISPEL) ||
	    (((cmd == CMD_ENTER) || (cmd == CMD_LOOK)) && ch))
	{
		if (ch && !is_Raidable(ch, 0, 0))
		{
			send_to_char("&=LWYou are not raidable! You shall not pass!\r\n", ch);
			return false;
		}

		struct portal_action_messages msg = {
			/*in ch    */
			"&+YAs you enter $p&+Y, you feel yourself being torn into a thousand\n"
			"&+Ypieces, scattered over the entirety of reality. Bits of your shattered\n"
			"&+Yconsciousness float randomly about the universe with no overall\n"
			"&+Ydirection or purpose.  Suddenly, you find yourself elsewhere..",
			/*in ch r  */ "$n steps into $p and disappears among the light.",
			/*out ch   */ 0,
			/*out ch r */ "$n steps out of $p.",
			/*wait init*/ "The portal hasn't stabilized yet...\n",
			/*wait stb */ "The portal hasn't re-stabilized yet...\n",
			/*decay ch */ 0,
			/*decay r  */ 0,
			/*bug      */ "Bug with etherprotal!! Tell a god.\n"
		};

		return portal_general_internal(obj, ch, cmd, arg, &msg);
	}

	return FALSE;
}

int moonstone(P_obj obj, P_char ch, int cmd, char * /*argument*/)
{
	char *name;
	struct obj_affect *aff;
	P_nevent e;

	// If not moonstone or bloodstone.
	if (!obj || (obj_index[obj->R_num].virtual_number != 419 &&
		     obj_index[obj->R_num].virtual_number != 433))
	{
		logit(LOG_DEBUG,
		      "moonstone: obj proc called with no obj or non-moonstone obj: '%s' %d.",
		      !obj ? "Null" : obj->short_description, !obj ? -1 : OBJ_VNUM(obj));
	}

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (cmd == CMD_DECAY)
	{
		// Notify the caster then let it decay.
		name = get_player_name_from_pid(obj->value[0]);
		if (name)
		{
			ch = get_char_online(name);
			if (ch)
			{
				if (obj_index[obj->R_num].virtual_number == 419)
				{
					send_to_char("&+WYour moonstone fades to nothingness.&n\n",
						     ch);
					affect_from_char(ch, SPELL_MOONSTONE);
				}
				else
				{
					send_to_char(
						"&+YYour &+rblood&+ystone&+Y fades to nothingness.&n\n",
						ch);
					affect_from_char(ch, SPELL_BLOODSTONE);
				}
			}
		}
		return FALSE;
	}

	// Upon dispel, make object decay soon and let it be dispelled.
	if (cmd == CMD_DISPEL)
	{
		// Get obj affect.
		aff = get_obj_affect(obj, TAG_OBJ_DECAY);
		if (aff)
		{
			// Find decay event.
			LOOP_EVENTS_OBJ(e, obj->nevents)
			{
				if (e->func != event_obj_affect)
				{
					continue;
				}
				// If found, move decay one full scheduler-wheel interval from now.
				if (*((struct obj_affect **)e->data) == aff)
				{
					if (!nevent_reschedule_after(nevent_handle_from_event(e),
								     PULSES_IN_TICK))
						logit(LOG_EXIT,
						      "moonstone: failed to reschedule decay event");
				}
			}
		}
		else
		{
			// Set timer to 1 min
			logit(LOG_DEBUG, "moonstone: obj has no decay timer! (creating one)");
			set_obj_affected(obj, 1, TAG_OBJ_DECAY, 0);
		}
		return FALSE;
	}

	return FALSE;
}
