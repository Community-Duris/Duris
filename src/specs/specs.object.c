/*
 ***************************************************************************
 *  File: specs.object.c                                     Part of Duris *
 *  Usage: special procedures for objects                                    *
 *  Copyright  1990, 1991 - see 'license.doc' for complete information.      *
 *  Copyright 1994 - 2008 - Duris Systems Ltd.                             *
 ***************************************************************************
 */

#include <ctype.h>
#include <list>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
;

#include "core/prototypes.h"

#include "item/forced_weapon_drop.h"
#include "item/native_artifact_actions.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "guild/assocs.h"
#include "magic/blispells.h"
#include "combat/ctf.h"
#include "economy/currency_transaction.h"
#include "economy/economic_gameplay_authority.h"
#include "combat/damage.h"
#include "world/graph.h"
#include "world/handler.h"
#include "combat/justice.h"
#include "world/map.h"
#include "classes/reavers.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include "world/vnum.obj.h"
#include "world/vnum.room.h"
#include "world/weather.h"

/*
   external variables
 */
extern P_char character_list;
extern P_desc descriptor_list;
extern P_index mob_index;
extern P_index obj_index;
extern P_room world;
extern char *coin_names[];
extern char *command[];
extern const char *dirs[];
extern const char *race_types[];
// extern const char rev_dir[];
extern const struct stat_data stat_factor[];
extern int innate_abilities[];
extern int planes_room_num[];
extern const int top_of_world;
extern int top_of_zone_table;
extern struct command_info cmd_info[MAX_CMD_LIST];
extern struct dex_app_type dex_app[52];
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern void event_bleedproc(P_char ch, P_char victim, P_obj obj, void *data);
extern const struct racial_data_type racial_data[];
int do_simple_move_skipping_procs(P_char, int, unsigned int);
extern Skill skills[];

void bard_healing(int, P_char, P_char, int);
void bard_protection(int, P_char, P_char, int);
void bard_storms(int, P_char, P_char, int);
void bard_chaos(int, P_char, P_char, int);
void bard_harming(int, P_char, P_char, int);
void bard_cowardice(int, P_char, P_char, int);
void bard_calm(int, P_char, P_char, int);
void bard_dragons(int, P_char, P_char, int);

void event_balance_affects(P_char, P_char, P_obj, void *);
void event_object_proc(P_char, P_char, P_obj, void *);
extern bool has_skin_spell(P_char);

/*static void hummer(P_obj);*/

int illithid_sack(P_obj obj, P_char ch, int cmd, char *argument)
{
	P_obj s_obj = NULL;
	char GBuf1[MAX_STRING_LENGTH], GBuf2[MAX_STRING_LENGTH];

	*GBuf1 = '\0';
	*GBuf2 = '\0';

	if (cmd == CMD_SET_PERIODIC) /*
	                   Events have priority
	                 */
		return FALSE;

	if (!ch || !obj) /*
	                    If the player ain't here, why are we?
	                  */
		return FALSE;

	if (argument && cmd == CMD_PUT)
	{
		argument_interpreter(argument, GBuf1, GBuf2);
		if (!*GBuf2)
			return FALSE;
		s_obj = get_obj_in_list_vis(ch, GBuf2, ch->carrying);
		if (!s_obj)
			s_obj = get_obj_in_list_vis(ch, GBuf2, world[ch->in_room].contents);
		if (s_obj != obj)
			return FALSE;

		/* ok they are attempting to get something from this chest */
		if (!IS_ILLITHID(ch) && !IS_PILLITHID(ch) && !IS_TRUSTED(ch))
		{
			act("&+L$n &+Lis &+Rzapped&+L as $e tries to put something into $p!", FALSE,
			    ch, obj, 0, TO_ROOM);
			act("&+LYou are &+Rzapped&+L as you try to put something into $p!", FALSE,
			    ch, obj, 0, TO_CHAR);
			return TRUE;
		}
	}
	return FALSE;
}

int death_proc(P_obj obj, P_char ch, int cmd, char * /*argument*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;
	if (!obj || !ch)
		return FALSE;
	if (IS_PC(ch) && GET_LEVEL(ch) < 57)
	{
		statuslog(AVATAR, "%s being destroyed for possessing a god object [%d] %s.",
			  GET_NAME(ch), obj_index[obj->R_num].virtual_number,
			  obj->short_description);
		act("$p &n&+Lbegins to glow &n&+wbrighter&+L and &+Wbrighter&+L until you are disolved by its divine power!",
		    FALSE, ch, obj, 0, TO_CHAR);
		act("$p &n&+Lbegins to glow &n&+wbrighter&+L and &+Wbrighter&+L until $n is disolved by its divine power!",
		    FALSE, ch, obj, 0, TO_ROOM);
		die(ch, ch);
		return TRUE;
	}
	return FALSE;
}

// pathfinder from KT

/*void hummer (P_obj obj)
{
P_char t_ch;
if (!obj || number(0,9))
   return;

//Kvark if (IS_AFFECTED(ch, AFF_HIDE)
 if(OBJ_WORN(obj)){
      t_ch = obj->loc.wearing;
      if(t_ch)
         if (IS_AFFECTED(t_ch, AFF_HIDE)){
        return;
         }
  }
  if (OBJ_WORN(obj) || OBJ_CARRIED(obj)) {
    act("&+LA faint &n&+rhum&N&+L can be heard from&N $p &+Lcarried by $n&N.",
       FALSE, obj->loc.wearing, obj, 0 ,TO_ROOM);
    act("&+LA faint &n&+rhum&N&+L can be heard from&N $p&+L you are carrying.",
       FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
  }
}*/

int magic_mouth(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	P_desc i;
	char buff[MAX_STRING_LENGTH];

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!ch)
		return FALSE;

	if (IS_TRUSTED(ch))
		return FALSE;

	if ((static_cast<unsigned int>(obj->value[0]) !=
	     GET_ASSOC(ch)->get_id()) && /* not in guild */
	    (!number(0, 4))) /* do only occasionally */
	{
		snprintf(
			buff, MAX_STRING_LENGTH,
			"&+cA magic mouth tells your guild 'Alert!  $N&n&+c has trespassed into %s&n&+c!'&N",
			world[ch->in_room].name);
		for (i = descriptor_list; i; i = i->next)
			if (!i->connected && !is_silent(i->character, TRUE) &&
			    IS_SET(i->character->specials.act, PLR_GCC) &&
			    IS_MEMBER(GET_A_BITS(i->character)) &&
			    (GET_ASSOC(i->character)->get_id() ==
			     static_cast<unsigned int>(obj->value[0])) &&
			    !IS_TRUSTED(i->character))
				act(buff, FALSE, i->character, 0, ch, TO_CHAR);
	}
	return FALSE;
}

int creeping_doom(P_obj /*obj*/, P_char ch, int cmd, char *arg)
{
	P_char t;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	/*
	   if argument is prefixed with spaces, skip over them
	 */
	while (*arg == ' ')
		++arg;

	switch (cmd)
	{
	case CMD_KILL:
	case CMD_HIT:
		if (!str_cmp("creeping doom", arg) || !str_cmp("creeping", arg) ||
		    !str_cmp("doom", arg))
		{
			send_to_char("Your swing seems to have no affect on the shapeless mass.\n",
				     ch);
			act("$n attacks the creeping doom, but it's everywhere and apparently is unaffected.",
			    FALSE, ch, 0, 0, TO_ROOM);
			return TRUE;
		}
		break;
	case CMD_CAST:
		/*
			   skip over "'<spell name>' "
			 */
		if (rindex(arg, '\'') == NULL)
			return FALSE;

		/*
			   skip over spaces between spell incantation and target name
			 */
		for (++arg; *arg && (*arg == ' '); arg++)
			;

		if (!str_cmp("creeping doom", arg) || !str_cmp("creeping", arg) ||
		    !str_cmp("doom", arg))
		{
			/*
				   use magic resistance of 100% when imp'd
				 */
			send_to_char("You can't.\n", ch);
			return TRUE;
		}
		break;
	case CMD_BACKSTAB:
		if (!str_cmp("creeping doom", arg) || !str_cmp("creeping", arg) ||
		    !str_cmp("doom", arg))
		{
			send_to_char(
				"You hopelessly attempt to backstab the shapeless creeping doom.\n",
				ch);
			act("$n hopelessly tries to backstab the creeping doom.", FALSE, ch, 0, 0,
			    TO_ROOM);
			return TRUE;
		}
		break;
	case CMD_KICK:
		if (!str_cmp("creeping doom", arg) || !str_cmp("creeping", arg) ||
		    !str_cmp("doom", arg))
		{
			send_to_char("You kick the creeping doom, but with no apparent affect.\n",
				     ch);
			act("$n kicks the creeping doom, but it has no affect.", FALSE, ch, 0, 0,
			    TO_ROOM);
			return TRUE;
		}
		break;
	case CMD_HITALL:
		LOOP_THRU_PEOPLE(t, ch)
			if (IS_NPC(t) && (GET_RNUM(t) == 9))
			{ /*
				 creeping dooms virtual #
			   */
			}
		break;
	default:
		return FALSE;
		break;
	} /*
	     switch
	   */

	return FALSE;
}

/*
   use object with ITEM_SWITCH type:
   value[0] = command to trigger
   value[1] = room containing the blocked exit
   value[2] = direction of the exit in the room
   value[3] = 0:wall moves, 1:switch item moves
 */

int item_switch(P_obj obj, P_char ch, int cmd, char *arg)
{
	int door, in_room, back;
	P_char dummy;
	P_obj object;
	char buf[MAX_STRING_LENGTH];

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!IS_ALIVE(ch) || !arg || !obj)
	{
		return FALSE;
	}

	// Tracks are never a switch.
	generic_find(arg, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM | FIND_NO_TRACKS, ch,
		     &dummy, &object);
	if (obj != object)
	{
		return FALSE;
	}

	if (obj->type != ITEM_SWITCH || obj->value[0] != cmd)
	{
		return FALSE;
	}

	in_room = real_room(obj->value[1]);
	if (in_room < 0)
	{
		send_to_char("This item is broken.  Talk to a god!\n", ch);
		wizlog(MINLVLIMMORTAL, "item_switch: The switch '%s' (%d) is broken!",
		       obj->short_description, OBJ_VNUM(obj));
		return TRUE;
	}
	door = obj->value[2];
	if (door < 0 || door >= NUM_EXITS)
	{
		send_to_char("This item is broken (exit # out of range).  Talk to a god!\n", ch);
		wizlog(MINLVLIMMORTAL,
		       "item_switch: The switch '%s' (%d) has out of range exit %d!",
		       obj->short_description, OBJ_VNUM(obj), door);
		return TRUE;
	}
	if (!world[in_room].dir_option[door])
	{
		send_to_char("This item is broken (exit doesn't exist).  Talk to a god!\n", ch);
		wizlog(MINLVLIMMORTAL,
		       "item_switch: The switch '%s' (%d) has exit %d which doesn't exit!",
		       obj->short_description, OBJ_VNUM(obj), door);
		return TRUE;
	}
	if (!IS_SET(world[in_room].dir_option[door]->exit_info, EX_BLOCKED))
	{
		send_to_char("Nothing happens.\n", ch);
		return TRUE;
	}
	if (OBJ_ROOM(obj))
	{
		if (obj->loc.room != in_room)
		{
			send_to_char("You hear a rumbling sound in the distance.\n", ch);
			act("You hear a rumbling sound in the distance.", FALSE, ch, 0, 0, TO_ROOM);
		}
	}
	else if (OBJ_CARRIED_BY(obj, ch) || OBJ_WORN_BY(obj, ch))
	{
		if (ch->in_room != in_room)
		{
			send_to_char("Nothing happens.\n", ch);
			return TRUE;
		}
	}
	else
	{
		return FALSE;
	}
	REMOVE_BIT(world[in_room].dir_option[door]->exit_info, EX_BLOCKED);
	back = world[in_room].dir_option[door]->to_room;

	if (IS_SET(world[in_room].dir_option[door]->exit_info, EX_SECRET))
	{
		if (obj->value[3] == 1)
		{
			snprintf(buf, MAX_STRING_LENGTH,
				 "%s moves aside, revealing a wall behind.\n",
				 obj->short_description);
			CAP(buf);
			send_to_room(buf, in_room);
		}
		else
		{
			if (door == DIR_DOWN)
			{
				strcpy(buf, "Part of the floor seems to be moving.\n");
			}
			else if (door == DIR_UP)
			{
				strcpy(buf, "Part of the ceiling seems to be moving.\n");
			}
			else
			{
				snprintf(buf, MAX_STRING_LENGTH,
					 "The %s wall seems to be moving.\n", dirs[door]);
			}
			send_to_room(buf, in_room);
		}
	}
	else
	{
		REMOVE_BIT(world[back].dir_option[(int)rev_dir[door]]->exit_info, EX_BLOCKED);

		if (obj->value[3] == 1)
		{
			snprintf(buf, MAX_STRING_LENGTH,
				 "%s moves aside, revealing a passageway.\n",
				 obj->short_description);
			CAP(buf);
			send_to_room(buf, in_room);
		}
		else
		{
			if (door == DIR_DOWN)
			{
				strcpy(buf,
				       "Part of the floor moves aside, revealing a passageway.\n");
			}
			else if (door == DIR_UP)
			{
				strcpy(buf,
				       "Part of the ceiling moves aside, revealing a passageway.\n");
			}
			else
			{
				snprintf(buf, MAX_STRING_LENGTH,
					 "The %s wall moves aside, revealing a passageway.\n",
					 dirs[door]);
			}
			send_to_room(buf, in_room);
		}
	}
	return TRUE;
}

/* 'Mayhem', chaotic sword of the night */

int labelas(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;
	int room;
	char Gbuf1[MAX_STRING_LENGTH];

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}
	if (!IS_ALIVE(ch) || (!arg && (cmd != CMD_CD)))
	{
		return FALSE;
	}
	if ((cmd != CMD_SACRIFICE) && (cmd != CMD_TERMINATE) && (cmd != CMD_CD))
	{
		return FALSE;
	}
	one_argument(arg, Gbuf1);
	if ((!*Gbuf1) && (cmd != CMD_CD))
	{
		send_to_char("Who?\n", ch);
		return TRUE;
	}
	if (!OBJ_WORN_BY(obj, ch) || obj->loc.wearing->equipment[HOLD] != obj)
	{
		return FALSE;
	}
	if (cmd != CMD_CD && !(victim = get_char_room_vis(ch, Gbuf1)))
	{
		send_to_char("Who?\n", ch);
		return (TRUE);
	}
	/*
	   if (str_cmp(ch->player.name, "Labelas")) {
	 */
	if (isname(ch->player.name, obj->name))
	{
		if (IS_PC(ch))
		{
			act("&+gYou feel a wave of holy justice wash over you as the staff &+Wglows.",
			    FALSE, ch, 0, 0, TO_CHAR);
			act("&+gThe Staff flies out of your hand and drives its sharp end into your chest!",
			    FALSE, ch, 0, 0, TO_CHAR);
			act("&+g$n attempts to use the Staff of the Implementors!", TRUE, ch, 0, 0,
			    TO_ROOM);
			act("&+gThe Staff &+Wglows&+g with power as it spins in the air and drives its pointed end into $n's chest!",
			    TRUE, ch, 0, 0, TO_ROOM);

			statuslog(
				ch->player.level,
				"%s killed while trying to use the someone elses Implementor staff.",
				GET_NAME(ch));
			logit(LOG_WIZ, "%s killed while trying to use the Staff of Labelas.",
			      GET_NAME(ch));
			die(ch, ch);
			return TRUE;
		}
		else
		{
			send_to_char("Monsters can't use the Staff of Labelas!\n", ch);
			return (TRUE);
		}
	}
	if (cmd == CMD_CD)
	{
		if (!str_cmp(ch->player.name, "Labelas"))
		{
			if (!*Gbuf1)
			{
				room = real_room(1213);
			}
			else if (is_number(Gbuf1))
			{
				room = real_room(atoi(Gbuf1));
				if (room == NOWHERE || room > top_of_world)
				{
					send_to_char("No room exists with that number.\n", ch);
					return TRUE;
				}
			}
			else if ((victim = get_char_vis(ch, Gbuf1)))
			{
				room = victim->in_room;
			}
			else
			{
				send_to_char("Sorry, I know of no one by that name!\n", ch);
				return TRUE;
			}
		}
		else
		{
			return FALSE;
		}

		act("&+g$n holds up $s staff and utters an arcane magical phrase.", TRUE, ch, 0, 0,
		    TO_ROOM);
		act("&+g$n steps into a sphere of time and is gone...", TRUE, ch, 0, 0, TO_ROOM);

		char_from_room(ch);
		char_to_room(ch, room, -1);

		act("&+WA cold wind suddenly blows through the area, chilling you to the bone.",
		    TRUE, ch, 0, 0, TO_ROOM);
		act("&+gFOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOSSSHHHHH!", TRUE, ch, 0,
		    0, TO_ROOM);
		act("&+gA huge twenty-foot-high shadow forms out of the ground, looming above you.",
		    TRUE, ch, 0, 0, TO_ROOM);
		act("&+gIts form twists and contorts into the shape of Labelas Enoreth!", TRUE, ch,
		    0, 0, TO_ROOM);
		act("&+gThe Elven god Labelas Enoreth stands before you in all his majesty!", TRUE,
		    ch, 0, 0, TO_ROOM);
		act("&+gThe winds fade...", TRUE, ch, 0, 0, TO_ROOM);

		return TRUE;
	}
	if (!(victim = get_char_room_vis(ch, Gbuf1)))
	{
		send_to_char("Use the Staff on whom?\n", ch);
		return TRUE;
	}
	if (ch == victim)
	{
		send_to_char("Don't use the Staff on yourself!\n", ch);
		return TRUE;
	}
	if (IS_NPC(victim))
	{
		send_to_char("Don't use the Staff on NPC's, okay?\n", ch);
		return TRUE;
	}
	act("&+gYou raise the staff high into the air, and strike at $N.", FALSE, ch, 0, victim,
	    TO_CHAR);
	act("&+gThe staff comes to life, moving with your hand at blinding speed", FALSE, ch, 0,
	    victim, TO_CHAR);
	act("&+gas if it had a hunger of its own for $N's heart!", FALSE, ch, 0, victim, TO_CHAR);

	act("&+g$n raises $s staff higher into the air and strikes at you!", FALSE, ch, 0, victim,
	    TO_VICT);
	act("&+gThe staff comes to life, freezing you where you stand in utter terror!", FALSE, ch,
	    0, victim, TO_VICT);
	act("&+gIt sings through the air towards your chest, craving your heart.", FALSE, ch, 0,
	    victim, TO_VICT);
	act("&+gYou cannot move as the staff strikes your chest with a sickening thud.", FALSE, ch,
	    0, victim, TO_VICT);
	act("&+gYou can hear your ribs crunch and break under the weapon's awesome power,", FALSE,
	    ch, 0, victim, TO_VICT);
	act("&+gYou feel a horrid ripping sensation and scream in uncontrollable agony!", FALSE, ch,
	    0, victim, TO_VICT);
	act("&+gThe staff wrenches free of you, your still-beating heart impaled on its sharp end.",
	    FALSE, ch, 0, victim, TO_VICT);
	act("&+LBlackness falls over you, and you fall to the ground.  You can literally", FALSE,
	    ch, 0, victim, TO_VICT);
	act("&+Lfeel your lifeforce flow into $n, sacrificed to the mighty Labelas.", FALSE, ch, 0,
	    victim, TO_VICT);

	act("&+g$n raises $s staff into the air and strikes at $N!", TRUE, ch, 0, victim,
	    TO_NOTVICT);
	act("&+gThe staff comes alive with a magical life of its own, and sings through", TRUE, ch,
	    0, victim, TO_NOTVICT);
	act("&+gthe air towards $N, who cowers in utter fear and is frozen in place.", TRUE, ch, 0,
	    victim, TO_NOTVICT);
	act("&+gThe staff strikes $S chest with a sickening thud. The sound of crunching", TRUE, ch,
	    0, victim, TO_NOTVICT);
	act("&+gribs and a horrible scream fill the air. The staff wrenches itself", TRUE, ch, 0,
	    victim, TO_NOTVICT);
	act("&+gfree of $S chest, with $S still-beating heart on its sharp end.", TRUE, ch, 0,
	    victim, TO_NOTVICT);
	act("&+g$N's lifeless body falls to the ground, $S face locked in a", TRUE, ch, 0, victim,
	    TO_NOTVICT);
	act("&+ghorrible expression as $S lifeforce is sacrificed to the mighty Labelas.", TRUE, ch,
	    0, victim, TO_NOTVICT);

	if (cmd == CMD_SACRIFICE)
	{
		if (IS_PC(victim))
		{
			statuslog(ch->player.level, "%s was sacrificed to Labelas.",
				  GET_NAME(victim));
			logit(LOG_WIZ, "%s was sacrificed to Labelas.", GET_NAME(victim));
		}
		die(victim, ch);
		return (TRUE);
	}
	else if (cmd == CMD_TERMINATE)
	{
		if (IS_NPC(victim))
		{
			logit(LOG_WIZ, "%s was sacrificed to Labelas.", GET_NAME(victim));
			die(victim, ch);
			send_to_char("You can't terminate mobs!!  Sacrificed instead.\n", ch);
			return TRUE;
		}
		if (GET_LEVEL(victim) > GET_LEVEL(ch))
		{
			send_to_char("Now, now, don't try to terminate your superiors!\n", ch);
			return TRUE;
		}
		act(".", FALSE, ch, 0, victim, TO_CHAR);
		act(".", FALSE, ch, 0, victim, TO_CHAR);
		act("&+gYou call the pure might of the Forgers down upon $N", FALSE, ch, 0, victim,
		    TO_CHAR);
		act("&+gWith great magic, you devour $N's soul completely and utterly, ", FALSE, ch,
		    0, victim, TO_CHAR);
		act("&+gobliterating $M from this world of Duris, forever...", FALSE, ch, 0, victim,
		    TO_CHAR);
		act("&+gThe world stands in awe of your awesome power... ;)", FALSE, ch, 0, victim,
		    TO_CHAR);

		act(".", FALSE, ch, 0, victim, TO_NOTVICT);
		act(".", FALSE, ch, 0, victim, TO_NOTVICT);
		act("&+g$n raises $s hand, and calls down the might of the Forgers on $N.", FALSE,
		    ch, 0, victim, TO_NOTVICT);
		act("&+g$n slowly devours $N's soul, utterly obliterating $S from this world forever.",
		    FALSE, ch, 0, victim, TO_NOTVICT);
		act("&+gYou can hear $N's soul scream in agony one last time, then fade on the winds....",
		    FALSE, ch, 0, victim, TO_NOTVICT);

		act(".", FALSE, ch, 0, victim, TO_VICT);
		act(".", FALSE, ch, 0, victim, TO_VICT);
		act("&+g$n raises $s hand, and calls down the might of the Forgers upon you!",
		    FALSE, ch, 0, victim, TO_VICT);
		act("&+g$n slowly devours your soul as it rises out of your dead body.", FALSE, ch,
		    0, victim, TO_VICT);
		act("&+WAAAAAAAAAAHHHHHHH!!!! The pain is agonizing, though there is no escape.",
		    FALSE, ch, 0, victim, TO_VICT);
		act("&+WYour soul is destroyed utterly, forever obliterated from this world...",
		    FALSE, ch, 0, victim, TO_VICT);

		statuslog(ch->player.level,
			  "%s's soul was just utterly destroyed by the power of Labelas!",
			  GET_NAME(victim));
		logit(LOG_WIZ, "%s was terminated by the power of Labelas.", GET_NAME(victim));
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

		return TRUE;
	}
	logit(LOG_DEBUG, "Somehow reached the end of labelas()");
	return FALSE;
}

int tyr_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim = NULL, tch1, tch2;
	int room;
	char Gbuf1[MAX_STRING_LENGTH];

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}
	if ((!ch) || (!arg && (cmd != CMD_CD)))
	{
		return FALSE;
	}
	if ((cmd != CMD_CD) && (cmd != CMD_SACRIFICE) && (cmd != CMD_TERMINATE))
	{
		return FALSE;
	}
	one_argument(arg, Gbuf1);
	if ((!*Gbuf1) && (cmd != CMD_CD))
	{
		send_to_char("Who?\n", ch);
		return FALSE;
	}
	if ((!OBJ_WORN_BY(obj, ch)) || ((obj->loc.wearing->equipment[WIELD] != obj) &&
					(obj->loc.wearing->equipment[SECONDARY_WEAPON] != obj)))
	{
		return FALSE;
	}
	if ((cmd != CMD_CD) && (!(victim = get_char_room_vis(ch, Gbuf1))))
	{
		send_to_char("Who?\n", ch);
		return FALSE;
	}
	if (str_cmp(ch->player.name, "Tyr") && str_cmp(ch->player.name, "Gruumsh"))
	{
		if (IS_PC(ch))
		{
			act("Gruumsh's Flaming Bringer of Vengeance glows briefly with a bright light.",
			    FALSE, ch, 0, victim, TO_CHAR);
			act("The sword wrenches free of your grip, diving into your chest!", FALSE,
			    ch, 0, victim, TO_CHAR);
			act("$n attempts to kill $N with Gruumsh's Flaming Bringer of Vengeance!",
			    TRUE, ch, 0, victim, TO_NOTVICT);
			act("The sword wrenches free from $n, and carves its way into $s chest!",
			    TRUE, ch, 0, victim, TO_NOTVICT);
			act("$n attempts to kill you with Gruumsh's Flaming Bringer of Vengeance!",
			    FALSE, ch, 0, victim, TO_VICT);
			act("The sword glows brightly before carving into $n's chest!", FALSE, ch,
			    0, victim, TO_VICT);
			statuslog(ch->player.level, "%s killed while fooling with Gruumsh's sword.",
				  GET_NAME(ch));
			logit(LOG_WIZ, "%s killed trying to use Gruumsh's sword.", GET_NAME(ch));
			die(ch, ch);
			return TRUE;
		}
		else
		{
			send_to_char("Monsters can't use this mighty Sword!\n", ch);
			return TRUE;
		}
	}
	if (cmd == CMD_CD)
	{
		if ((!str_cmp(ch->player.name, "Gruumsh")) || (!str_cmp(ch->player.name, "Tyr")))
		{
			if (!*Gbuf1)
			{
				room = real_room(1206);
			}
			else if (is_number(Gbuf1))
			{
				room = real_room(atoi(Gbuf1));
				if ((room == NOWHERE) || (room > top_of_world))
				{
					send_to_char("No room exists with that number.\n", ch);
					return TRUE;
				}
			}
			else if ((victim = get_char_vis(ch, Gbuf1)))
			{
				room = victim->in_room;
			}
			else
			{
				send_to_char("Sorry, I know of no one by that name!\n", ch);
				return TRUE;
			}
		}
		else
		{
			return FALSE;
		}

		act("You step into a portal that opens abruptly in front of you.\n", FALSE, ch, 0,
		    0, TO_CHAR);
		act("$n steps into a portal that opens abruptly in front of $m.", TRUE, ch, 0, 0,
		    TO_ROOM);

		char_from_room(ch);
		char_to_room(ch, room, -1);

		act("$n steps out of a portal that opens abruptly in front of you.\n"
		    "$n &+RFARTS&n loudly!!",
		    TRUE, ch, 0, 0, TO_ROOM);
		/*
		   stolen from mobact.c -- DTS 8/12/95
		 */
		for (tch1 = world[ch->in_room].people; tch1; tch1 = tch2)
		{
			tch2 = tch1->next_in_room;

			if (IS_TRUSTED(tch1))
				continue;
			if (CAN_SEE(tch1, ch))
			{
				if (GET_LEVEL(tch1) < (GET_LEVEL(ch) / 3))
				{
					do_flee(tch1, 0, 2); /*
					                        panic flee, no save
					                      */
				}
				else if (!NewSaves(tch1, SAVING_PARA, 1))
				{
					do_flee(tch1, 0, 1); /*
					                        fear, but not panic
					                      */
				}
			}
		}

		return TRUE;
	}
	if (ch == victim)
	{
		send_to_char("Don't use your Sword on yourself, idiot!\n", ch);
		return TRUE;
	}
	if (IS_NPC(victim))
	{
		send_to_char("Don't use the Sword on monsters!\n", ch);
		return TRUE;
	}
	act("Your sword, Bringer of Vengeance, flares blue as you call down\n"
	    "the fist of vengeance upon $N.  $S body is ravaged by a searing heat,\n"
	    "causing $M to scream in agony.  $N crumbles in a heap at your feet.\n",
	    FALSE, ch, 0, victim, TO_CHAR);

	act("$n points $s Bringer of Vengeance at you, which flares blue\n"
	    "as $e calls down vengeance upon you.  Your body is ravaged by a searing heat,\n"
	    "causing you to scream in agony.  As you look down, you see your\n"
	    "former body, crumpled in a heap at $n's feet.\n",
	    FALSE, ch, 0, victim, TO_VICT);

	act("$n points $s sword, Bringer of Vengeance, at $N.\n"
	    "The sword flares blue as $n enacts vengeance upon $N, whose body\n"
	    "is ravaged by a searing heat, causing $M to scream in agony.\n"
	    "$N crumbles at $n's feet.\n",
	    TRUE, ch, 0, victim, TO_NOTVICT);

	if (cmd == CMD_SACRIFICE)
	{
		statuslog(ch->player.level, "%s was sacrificed by %s.", GET_NAME(victim),
			  GET_NAME(ch));
		logit(LOG_WIZ, "%s was sacrificed by %s.", GET_NAME(victim), GET_NAME(ch));
		die(victim, ch);
		return TRUE;
	}
	else if (cmd == CMD_TERMINATE)
	{
		act(".", FALSE, ch, 0, victim, TO_CHAR);
		act(".", FALSE, ch, 0, victim, TO_CHAR);
		act("You call the pure might of the Forgers down upon $N.\n"
		    "With great magic, you devour $N's soul, utterly obliterating\n"
		    "$M from the world of Duris forever...\n"
		    "The world stands in awe of your awesome power... ;)",
		    FALSE, ch, 0, victim, TO_CHAR);

		act(".", FALSE, ch, 0, victim, TO_NOTVICT);
		act(".", FALSE, ch, 0, victim, TO_NOTVICT);
		act("$n points at $N, and whispers \"Vengeance\" to $s sword,\n"
		    "Bringer of Vengeance.  $n raises $s hand, and calls down the\n"
		    "might of the Forgers on $N.  $n slowly devours $N's soul,\n"
		    "utterly destroying $M from this world of Duris forever...\n"
		    "You can hear $N's soul scream in agony one last time,\n"
		    "then fade on the winds....",
		    FALSE, ch, 0, victim, TO_NOTVICT);

		act(".", FALSE, ch, 0, victim, TO_VICT);
		act(".", FALSE, ch, 0, victim, TO_VICT);
		act("$n points at you, and whispers \"Vengeance\" to $s sword,\n"
		    "Bringer of Vengeance.  $n raises $s hand, and calls down\n"
		    "the might of the Forgers upon you!\n"
		    "$n slowly devours your soul as it rises out of your dead body.\n"
		    "AAAAAAAAAAAAAAAAAAAHHHHH!!!! The pain is agonizing, but there is no escape\n"
		    "Your soul is destroyed utterly, forever obliterated from this world.",
		    FALSE, ch, 0, victim, TO_VICT);
		statuslog(ch->player.level, "%s was just terminated by the power of %s.",
			  GET_NAME(victim), GET_NAME(ch));
		logit(LOG_WIZ, "%s terminated by %s.", GET_NAME(victim), GET_NAME(ch));
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
		return TRUE;
	}
	logit(LOG_DEBUG, "Somehow reached the end of tyr_sword().");
	return FALSE;
}

// Subtract 1 from values[0] each cast.  When reaching 0, poof item.

/* Woundhealer is in specs.undermountain.c -> woundhealer_scimitar.
int woundhealer(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      dam;
  P_char   vict;

  // Check for periodic event calls
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if (cmd != CMD_MELEE_HIT)
    return (FALSE);

  if (!ch)
    return (FALSE);

  if (!OBJ_WORN_POS(obj, WIELD) && !OBJ_WORN_POS(obj, HOLD))
    return (FALSE);

  vict = legacy_proc_arg<P_char>(arg);
  dam = BOUNDED(0, (GET_HIT(vict) + 9), number(1, 8));

  if ((obj->loc.wearing == ch) && vict)
  {
    GET_HIT(ch) += dam;
    GET_HIT(vict) -= dam;
  }
  return (TRUE);
}
*/

#if 0

/* func is buggy..  if you want to fix it you'll have to fix the part
   that accesses the dir array and maybe other stuff */

int cursed_mirror(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      is_worn, is_carried = FALSE;
  char     buf[20];

  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if ((cmd < CMD_NORTH || cmd > CMD_UP) && cmd != CMD_LOOK)
    return FALSE;

  if (OBJ_WORN_BY(obj, ch))
  {
    is_worn = TRUE;
  }
  else if (OBJ_CARRIED_BY(obj, ch))
  {
    is_carried = TRUE;
  }
  if (cmd == CMD_LOOK)
  {
    if (is_worn && (number(1, 101) < 15) && !IS_DARK(ch->in_room))
    {
      send_to_char
        ("The cursed mirror in your hands leaps up into your line of sight!\n"
         "You find yourself looking back behind you.\n\n", ch);
      switch (number(0, 6))
      {
      case 0:
      case 1:
      case 3:
        strcpy(buf, dirs[(int) rev_dir[cmd]]);
        do_look(ch, buf, -4);
        break;
      case 4:
        send_to_char("You think you see someone staring at you.\n", ch);
        break;
      case 5:
        send_to_char
          ("You see a blurr as a shadowed figure darts out of your vision.\n",
           ch);
        break;
      case 6:
        send_to_char("You see nothing suspicious.\n", ch);
        break;
      }
      return TRUE;
    }
    else
    {
      return FALSE;
    }
  }
  else
  {
    if( (!is_worn && !is_carried) || (number(1, 101) > 15)
      || IS_DARK(ch->in_room) || IS_TRUSTED(ch) )
    {
      return FALSE;
    }
    else
    {
/*
   old_cmd = cmd;
   while (cmd == old_cmd) {
   cmd = number(CMD_NORTH, CMD_DOWN);
   }
 */
      if (!world[ch->in_room].dir_option[(int) rev_dir[cmd]] ||
          IS_SET(EXIT(ch, (int) rev_dir[cmd])->exit_info, EX_SECRET) ||
          IS_SET(EXIT(ch, (int) rev_dir[cmd])->exit_info, EX_BLOCKED))
      {
        do_move(ch, 0, rev_dir[cmd]);
        send_to_char
          ("The light glints off your mirror, temporarily blinding\n"
           "and confusing you! You stumble off in the reverse direction!\n\n",
           ch);
        return TRUE;
      }
      else
        return FALSE;
    }
    return FALSE;
  }
  return FALSE;
}
#endif

#if 0 /*                                                                                                                                                                                               \
         commented out till I can fix it...DTS                                                                                                                                                         \
       */
int cursed_mirror(P_obj obj, P_char ch, int cmd, char *arg)
{
  int      is_worn, is_carried, match;
  char     buf[200];
  char     tmpbuf[200];

  is_worn = is_carried = match = FALSE;
  /*
     check for periodic event calls
   */
  if (cmd == CMD_SET_PERIODIC)
    return FALSE;

  if ((cmd < CMD_NORTH || cmd > CMD_UP) && cmd != CMD_LOOK)
    return FALSE;

  if (OBJ_WORN_BY(obj, ch))
  {
    is_worn = TRUE;
  }
  else if (OBJ_CARRIED_BY(obj, ch))
  {
    is_carried = TRUE;
  }
  if (cmd == CMD_LOOK)
  {
    if (is_worn && arg && (number(1, 101) < 15) && !IS_DARK(ch->in_room))
    {
      sscanf(arg, " %s", tmpbuf);
      match = search_block(tmpbuf, dirs, FALSE);
      if (match == -1)
        return FALSE;
      send_to_char
        ("The cursed mirror in your hands leaps up into your line of sight!\n"
         "You find yourself looking back behind you.\n\n", ch);
      switch (number(0, 6))
      {
      case 0:
      case 1:
      case 2:
      case 3:
        switch (match)
        {
        case 0:
          strcpy(buf, dirs[2]);
          break;
        case 1:
          strcpy(buf, dirs[3]);
          break;
        case 2:
          strcpy(buf, dirs[0]);
          break;
        case 3:
          strcpy(buf, dirs[1]);
          break;
        case 4:
          strcpy(buf, dirs[5]);
          break;
        case 5:
          strcpy(buf, dirs[4]);
          break;
        default:
          send_to_char("Serious error with mirror. Notify a forger!\n", ch);
          return FALSE;
        }
        do_look(ch, buf, -4);
        break;
      case 4:
        send_to_char("You think you see someone staring at you.\n", ch);
        break;
      case 5:
        send_to_char
          ("You see a blurr as a shadowed figure darts out of your vision.\n",
           ch);
        break;
      case 6:
        send_to_char("You see nothing suspicious.\n", ch);
        break;
      }
      return TRUE;
    }
    else
    {
      return FALSE;
    }
    return FALSE;
  }
  else
  {
    if( (!is_worn && !is_carried) || (number(1, 101) > 15)
      || IS_DARK(ch->in_room) || IS_TRUSTED(ch) )
    {
      return FALSE;
    }
    else
    {
      if (!world[ch->in_room].dir_option[(int) rev_dir[cmd]] ||
          IS_SET(EXIT(ch, (int) rev_dir[cmd])->exit_info, EX_SECRET) ||
          IS_SET(EXIT(ch, (int) rev_dir[cmd])->exit_info, EX_BLOCKED))
      {
        do_move(ch, 0, rev_dir[cmd]);
        send_to_char
          ("The light glints off your mirror, temporarily blinding\n"
           "and confusing you! You stumble off in the reverse direction!\n\n",
           ch);
        return TRUE;
      }
      else
        return FALSE;
    }
    return FALSE;
  }
  return FALSE;
}
#endif

/*
   This is mostly ripped off from item_switch(), but with modifications
   * specific to this particular item...
   * -- DTS 2/21/95
 */

/*
   This obj proc lets a person offer a held TREASURE and receive some sort
   ** of goodie, detailed below.  The treasures are usually (presumably) gems.
   ** Chars will always get either a bless spell or a bit of money
   ** (amount varies).  There is a 6 in 700 chance of getting something extra
   ** cool, such as either of two charmies, or a cool potion or scroll, or a
   ** vitality spell.
 */

int llyms_altar(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_obj treasure = NULL, tempobj = NULL;
	char treas_name[MAX_INPUT_LENGTH], altar_name[MAX_INPUT_LENGTH];
	P_char tempchar = NULL;
	int money = 0;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (cmd != CMD_OFFER || !obj || !IS_ALIVE(ch) || !IS_AWAKE(ch))
	{
		return FALSE;
	}

	arg = one_argument(arg, treas_name);
	arg = one_argument(arg, altar_name);

	if (!*treas_name || !*altar_name)
	{
		return FALSE;
	}

	// Skip tracks when looking for altar.
	generic_find(altar_name, FIND_OBJ_ROOM | FIND_NO_TRACKS, ch, &tempchar, &tempobj);
	generic_find(treas_name, FIND_OBJ_EQUIP, ch, &tempchar, &treasure);
	if (tempobj != obj || !treasure)
	{
		return FALSE;
	}

	// Must be a TREASURE to work
	if (!CAN_SEE_OBJ(ch, treasure) || !OBJ_WORN_BY(treasure, ch) ||
	    !OBJ_WORN_POS(treasure, HOLD) || GET_ITEM_TYPE(treasure) != ITEM_TREASURE)
	{
		return FALSE;
	}
	if (economic_gameplay_authority::active())
	{
		send_to_char("The altar cannot accept offerings while accounting is active.\r\n",
			     ch);
		return TRUE;
	}

	act("You offer up $p to $P.", TRUE, ch, treasure, obj, TO_CHAR);
	act("$n offers up $p to $P.", TRUE, ch, treasure, obj, TO_ROOM);

	if (IS_ARTIFACT(treasure))
	{
		act("$P rumbles briefly, then is silent.\n\rYour offering is refused.", TRUE, ch, 0,
		    obj, TO_CHAR);
		act("$P rumbles briefly, then is silent.", TRUE, ch, 0, obj, TO_ROOM);
		return TRUE;
	}

	// It better be worth something -- at some point I'm gonna make reward depend on the value of the treasure
	if (treasure->cost < 10000)
	{
		act("$P rumbles briefly, then is silent.", TRUE, ch, 0, obj, TO_CHAR);
		act("$P rumbles briefly, then is silent.", TRUE, ch, 0, obj, TO_ROOM);
		act("Your offering of $p is no good.", TRUE, ch, treasure, 0, TO_CHAR);
		act("$n's offering is inadequate.", TRUE, ch, 0, 0, TO_ROOM);
		return TRUE;
	}
	act("A &+Bblue light&n streaks from $P and strikes your hand, enveloping $p.", TRUE, ch,
	    treasure, obj, TO_CHAR);
	act("A &+Bblue light&n streaks from $P and strikes $n's hand, enveloping $p.", TRUE, ch,
	    treasure, obj, TO_ROOM);
	/* This message got reworked.
	  act ("$p is wrenched from your hand and plunges into $P!", TRUE, ch, treasure, obj, TO_CHAR);
	  act ("$p is wrenched from $n's hand and plunges into $P!", TRUE, ch, treasure, obj, TO_ROOM);
	 */

	obj_to_char(unequip_char(ch, HOLD), ch);
	obj_from_char(treasure);
	extract_obj(treasure, TRUE); // Not an arti, but 'in game.'

	// They get blessed, unless they're already blessed, in which case, they  get money.
	//   Then there's a small chance of something extra cool happening.
	if (!affected_by_spell(ch, SPELL_BLESS))
	{
		act("$p throbs for a minute, and you suddenly feel blessed.", TRUE, ch, obj, 0,
		    TO_CHAR);
		act("$p throbs for a minute, and $n glows with a pure &+Bwhite&n light.", TRUE, ch,
		    obj, 0, TO_ROOM);
		act("The light swiftly fades around $n.", TRUE, ch, 0, 0, TO_ROOM);
		spell_bless(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
	}
	else
	{
		act("Your purse suddenly feels heavier!", TRUE, ch, 0, 0, TO_CHAR);
		money = number(1, 4);
		switch (money)
		{
		case 1:
			ADD_MONEY(ch, number(1, 10) * 1000);
			break;
		case 2:
			ADD_MONEY(ch, number(1, 10) * 100);
			break;
		case 3:
			ADD_MONEY(ch, number(1, 10) * 10);
			break;
		case 4:
			ADD_MONEY(ch, number(1, 10));
			break;
		default:
			break;
		}
	}
	// Low chance of something really cool happening.
	switch (number(1, 700))
	{
	case 1:
		tempchar = read_mobile(88814, VIRTUAL);
		if (!tempchar)
		{
			logit(LOG_SYS, "error in read_mobile: llyms_altar()");
			send_to_char("Error in altar proc.  Inform a god.\n", ch);
			return FALSE;
		}
		char_to_room(tempchar, ch->in_room, -1);
		act("$n appears in a flash of light!", TRUE, tempchar, 0, 0, TO_ROOM);
		setup_pet(tempchar, ch, -1, PET_NOCASH);
		add_follower(tempchar, ch);
		break;
	case 2:
		tempchar = read_mobile(88815, VIRTUAL);
		if (!tempchar)
		{
			logit(LOG_SYS, "error in read_mobile: llyms_altar()");
			send_to_char("Error in altar proc.  Inform a god.\n", ch);
			return FALSE;
		}
		char_to_room(tempchar, ch->in_room, -1);
		act("$n appears in a flash of light!", TRUE, tempchar, 0, 0, TO_ROOM);
		setup_pet(tempchar, ch, -1, PET_NOCASH);
		add_follower(tempchar, ch);
		break;
	case 3:
		tempobj = read_object(88831, VIRTUAL);
		if (!tempobj)
		{
			logit(LOG_SYS, "error in read_object: llyms_altar()");
			send_to_char("Error in altar proc. Inform a god.\n", ch);
			return FALSE;
		}
		obj_to_room(tempobj, ch->in_room);
		act("$p appears before you in a flash of light!", TRUE, ch, tempobj, 0, TO_CHAR);
		act("$p appears in a flash of light!", TRUE, ch, tempobj, 0, TO_ROOM);
		break;
	case 4:
		tempobj = read_object(88832, VIRTUAL);
		if (!tempobj)
		{
			logit(LOG_SYS, "error in read_object: llyms_altar()");
			send_to_char("Error in altar proc. Inform a god.\n", ch);
			return FALSE;
		}
		obj_to_room(tempobj, ch->in_room);
		act("$p appears before you in a flash of light!", TRUE, ch, tempobj, 0, TO_CHAR);
		act("$p appears in a flash of light!", TRUE, ch, tempobj, 0, TO_ROOM);
		break;
	case 5:
		tempobj = read_object(88833, VIRTUAL);
		if (!tempobj)
		{
			logit(LOG_SYS, "error in read_object: llyms_altar()");
			send_to_char("Error in altar proc. Inform a god.\n", ch);
			return (FALSE);
		}
		obj_to_room(tempobj, ch->in_room);
		act("$p appears before you in a flash of light!", TRUE, ch, tempobj, 0, TO_CHAR);
		act("$p appears in a flash of light!", TRUE, ch, tempobj, 0, TO_ROOM);
		break;
	case 6:
		if (!affected_by_spell(ch, SPELL_VITALITY))
		{
			act("$p throbs for a minute, and you suddenly feel vitalized!", TRUE, ch,
			    obj, 0, TO_CHAR);
			act("$p throbs for a minute, and $n appears vitalized!", TRUE, ch, obj, 0,
			    TO_ROOM);
			spell_vitality(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
		}
		break;
	default:
		break;
	}
	return TRUE;
}

/*
   This little routine will make the ruby monocle "load randomly" if it's
   * on the ground in one of the given rooms at zone reset.  The room range is
   * 90124-90142.
   * -- DTS 4/4/95
 */

int zarbon_shaper(P_obj obj, P_char ch, int cmd, char *arg)
{
	int curr_time;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!OBJ_WORN_POS(obj, HOLD))
		return (FALSE);

	if (cmd == CMD_PERIODIC)
	{
		/* can either remove the next line or set the second number higher if it hums too often */
		if (!number(0, 2))
			hummer(obj);
		return TRUE;
	}

	if (arg && (cmd == CMD_SAY))
	{
		if (isname(arg, "blink"))
		{
			curr_time = time(NULL);

			if (obj->timer[1] + number(1, 5) <= curr_time)
			{
				act("You say 'blink'", FALSE, ch, 0, 0, TO_CHAR);
				act("Your $q hums briefly, and you feel your body begin to vibrate.",
				    FALSE, ch, obj, obj, TO_CHAR);

				act("$n says 'blink'", TRUE, ch, obj, NULL, TO_ROOM);
				act("$n's $q hums briefly, and their body begins to vibrate violently!",
				    TRUE, ch, obj, NULL, TO_ROOM);
				spell_blink(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
				obj->timer[1] = curr_time;
				return TRUE;
			}
		}
		else if (isname(arg, "deflect"))
		{
			curr_time = time(NULL);

			if (obj->timer[2] + 500 <= curr_time)
			{
				act("You say 'deflect'", FALSE, ch, 0, 0, TO_CHAR);
				act("Your $q begins to send out ripples of pure magical energy!",
				    FALSE, ch, obj, obj, TO_CHAR);
				act("$n says 'deflect'", FALSE, ch, obj, obj, TO_ROOM);
				act("$n's $q begins to send out ripples of pure magical energy!",
				    TRUE, ch, obj, 0, TO_ROOM);
				if (ch->group)
					cast_as_area(ch, SPELL_DEFLECT, 50, 0);
				else
					spell_deflect(60, ch, 0, SPELL_TYPE_SPELL, ch, 0);
				obj->timer[2] = curr_time;
				return TRUE;
			}
		}
		else
			return FALSE;
	}
	curr_time = time(NULL);

	if (obj->timer[0] + number(1, 30) <= curr_time)
	{
		int spell = memorize_last_spell(ch);

		if (spell)
		{
			char buf[256];
			snprintf(buf, 256, "&+WYou feel your power of %s &+Wreturning to you.&n\n",
				 skills[spell].name);
			send_to_char(buf, ch);
			obj->timer[0] = curr_time;
			return FALSE;
		}
	}

	return FALSE;
}

int trans_tower_sword(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000, curr_time;
	P_char victim;

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch && cmd == CMD_PERIODIC)
	{
		hummer(obj);
		return TRUE;
	}

	if (!ch)
		return FALSE;

	if (!OBJ_WORN_BY(obj, ch))
		return (FALSE);

	if (arg && (cmd == CMD_SAY))
	{
		if (isname(arg, "stone"))
		{
			curr_time = time(NULL);

			if (obj->timer[0] + 60 <= curr_time)
			{
				act("You say 'stone'", FALSE, ch, 0, 0, TO_CHAR);
				act("Your $q hums briefly.", FALSE, ch, obj, obj, TO_CHAR);

				act("$n says 'stone'", TRUE, ch, obj, NULL, TO_ROOM);
				act("$n's $q hums briefly.", TRUE, ch, obj, NULL, TO_ROOM);
				spell_stone_skin(40, ch, 0, SPELL_TYPE_SPELL, ch, 0);

				obj->timer[0] = curr_time;

				return TRUE;
			}
		}
	}

	if (obj->loc.wearing->equipment[WIELD] != obj)
		return (FALSE);

	if (!dam)
		return FALSE;

	victim = legacy_proc_arg<P_char>(arg);
	if (!victim)
		return (FALSE);
	if (number(0, 20))
		return (FALSE);
	act("$n's $q &+Wglows white&n, unleashing a massive ball of ice towards $N!", TRUE, ch, obj,
	    victim, TO_NOTVICT);
	act("Your $q &+Wglows white&n, unleashing a massive ball of ice towards $N!", TRUE, ch, obj,
	    victim, TO_CHAR);
	act("$n's $q &+Wglows white&n, unleashing a massive ball of ice towards _YOU_!", TRUE, ch,
	    obj, victim, TO_VICT);
	spell_harm(50, ch, NULL, 0, victim, obj);
	spell_arieks_shattering_iceball(30, ch, NULL, SPELL_TYPE_SPELL, victim, obj);
	return (TRUE);
}

/* object burns on all commands I can think of that involve touching it */

int druid_sabre(P_obj /*obj*/, P_char /*ch*/, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;
	return 0;
}

int glowing_necklace(P_obj obj, P_char ch, int cmd, char *arg)
{
	int curr_time;
	P_char watermental;
	int sum, elesize;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!OBJ_WORN(obj) || !IS_ALIVE(ch) || IS_ROOM(ch->in_room, ROOM_LOCKER))
	{
		return FALSE;
	}

	if (arg && (cmd == CMD_RUB))
	{
		if (isname(arg, "necklace") || isname(arg, "glowing"))
		{
			if (IS_FIGHTING(ch))
			{
				send_to_char(
					"You're too busy fighting for your life to reach up for your necklace.\n",
					ch);
				return FALSE;
			}
			curr_time = time(NULL);
			if ((((obj->timer[0] + (60 * 3)) + number(0, 120)) <= curr_time) ||
			    IS_TRUSTED(ch))
			{
				// Raise to 100 if you want spec pets to occur
				elesize = number(1, 98);

				if (elesize == 100)
				{
					if (!IS_TRUSTED(ch) &&
					    !can_conjure_greater_elem(ch, GET_LEVEL(ch)))
					{
						elesize = 90;
					}
					else
					{
						act("&+bAs you rub your $q&+b, &+Bwater &+bbeings to &+Bflow &+bout of your $q&+b.  As the\n"
						    "&+Bwaters &+bsubside, the form of &+Ba water elemental&+b is left before you.\n"
						    "&+BA powerful triton says &N'How may I serve you'",
						    FALSE, ch, obj, obj, TO_CHAR);
						act("&+bAs $n rub $s $q&+b, &+Bwater &+bbeings to &+Bflow &+bout of $s $q&+b.  As the\n"
						    "&+Bwaters &+bsubside, the form of &+Ba water elemental&+b is left before you.\n"
						    "&+BA powerful triton says &N'How may I serve you'",
						    FALSE, ch, obj, obj, TO_ROOM);
						watermental = read_mobile(1142, VIRTUAL);
					}
				}
				else if (elesize == 99)
				{
					if (!IS_TRUSTED(ch) &&
					    !can_conjure_greater_elem(ch, GET_LEVEL(ch)))
					{
						elesize = 90;
					}
					else
					{
						act("&+bAs you rub your $q&+b, &+Bwater &+bbeings to &+Bflow &+bout of your $q&+b.  As the\n"
						    "&+Bwaters &+bsubside, the form of &+Ba water elemental&+b is left before you.\n"
						    "&+bA &+Bb&+e&+Ba&+bu&+Bt&+bi&+Bf&+bu&+Bl undine&N says &N'How may I serve you'",
						    FALSE, ch, obj, obj, TO_CHAR);
						act("&+bAs $n rub $s $q&+b, &+Bwater &+bbeings to &+Bflow &+bout of $s $q&+b.  As the\n"
						    "&+Bwaters &+bsubside, the form of &+Ba water elemental&+b is left before you.\n"
						    "&+bA &+Bb&+e&+Ba&+bu&+Bt&+bi&+Bf&+bu&+Bl undine&N says &N'How may I serve you'",
						    FALSE, ch, obj, obj, TO_ROOM);
						watermental = read_mobile(1141, VIRTUAL);
					}
				}
				else if (elesize > 90)
				{
					if (!IS_TRUSTED(ch) &&
					    !can_conjure_greater_elem(ch, GET_LEVEL(ch)))
					{
						elesize = 90;
					}
					else
					{
						act("&+bAs you rub your $q&+b, &+Bwater &+bbeings to &+Bflow &+bout of your $q&+b.  As the\n"
						    "&+Bwaters &+bsubside, the form of &+Ba HUGE water elemental&+b is left before you.\n"
						    "&+bA HUGE water elemental says &N'How may I serve you'",
						    FALSE, ch, obj, obj, TO_CHAR);
						act("&+bAs $n rub $s $q&+b, &+Bwater &+bbeings to &+Bflow &+bout of $s $q&+b.  As the\n"
						    "&+Bwaters &+bsubside, the form of &+Ba HUGE water elemental&+b is left before you.\n"
						    "&+bA HUGE water elemental says &N'How may I serve you'",
						    FALSE, ch, obj, obj, TO_ROOM);
						watermental = read_mobile(1140, VIRTUAL);
					}
				}
				if (elesize <= 90)
				{
					if (!IS_TRUSTED(ch) &&
					    !can_conjure_lesser_elem(ch, GET_LEVEL(ch)))
					{
						act("&+bYour $q&+L hums briefly...&N", FALSE, ch,
						    obj, obj, TO_CHAR);
						act("&+b$n's $q&+L hums briefly...&N", FALSE, ch,
						    obj, obj, TO_ROOM);
						return FALSE;
					}
					else
					{
						act("&+bAs you rub your $q&+b, &+Bwater &+bbeings to &+Bflow &+bout of your $q&+b.  As the\n"
						    "&+Bwaters &+bsubside, the form of &+Ba water elemental&+b is left before you.\n"
						    "&+bA water elemental says &N'How may I serve you'",
						    FALSE, ch, obj, obj, TO_CHAR);
						act("&+bAs $n rub $s $q&+b, &+Bwater &+bbeings to &+Bflow &+bout of $s $q&+b.  As the\n"
						    "&+Bwaters &+bsubside, the form of &+Ba water elemental&+b is left before you.\n"
						    "&+bA water elemental says &N'How may I serve you'",
						    FALSE, ch, obj, obj, TO_ROOM);
						watermental = read_mobile(1103, VIRTUAL);
					}
				}
				if (!watermental)
				{
					act("&=LBTHERE IS NO WATER ELEMENTAL, TELL A GOD!!&N",
					    FALSE, ch, obj, obj, TO_CHAR);
					return FALSE;
				}
				char_to_room(watermental, ch->in_room, 0);

				GET_SIZE(watermental) = SIZE_MEDIUM;
				watermental->player.m_class = CLASS_WARRIOR;
				watermental->player.level = 45;
				sum = dice(GET_LEVEL(watermental) * 4, 8) +
				      (GET_LEVEL(watermental) * 3);
				while (watermental->affected)
				{
					affect_remove(watermental, watermental->affected);
				}
				if (!IS_SET(watermental->specials.act, ACT_MEMORY))
				{
					clearMemory(watermental);
				}
				SET_BIT(watermental->specials.affected_by, AFF_INFRAVISION);
				remove_plushit_bits(watermental);
				GET_MAX_HIT(watermental) = GET_HIT(watermental) =
					watermental->points.base_hit = sum;
				watermental->points.base_hitroll = watermental->points.hitroll =
					GET_LEVEL(watermental) / 3;
				watermental->points.base_damroll = watermental->points.damroll =
					GET_LEVEL(watermental) / 3;
				/* Does nothing because watermental is not a monk.
				        MonkSetSpecialDie(watermental);
				*/
				GET_EXP(watermental) = 0;
				balance_affects(watermental);
				setup_pet(watermental, ch, 1500, PET_NOCASH);
				add_follower(watermental, ch);
				obj->timer[0] = curr_time;
				return TRUE;
			}
			else
			{
				act("&+bAs you rub your $q, &+ba single drop of water drips out...&N",
				    FALSE, ch, obj, obj, TO_CHAR);
				act("&+bAs $n &+brubs $s $q, &+Ba single drop of water drips out...&N",
				    FALSE, ch, obj, obj, TO_ROOM);
			}
		}
	}
	return FALSE;
}

/* Procs made by Sev 2006 */

//-------------------------------------------------
// OK. various portals stuff here (HOOK ACTIONS)
//-------------------------------------------------

int portal_race(P_char ch)
{
	if (IS_NPC(ch))
	{
		if (!ch->following)
			return -2;
		if (IS_ILLITHID(ch->following))
			return 0;
		if (EVIL_RACE(ch->following))
			return -1;
		return 1;
	}
	if (IS_ILLITHID(ch))
		return 0;
	if (EVIL_RACE(ch))
		return -1;
	return 1;
}

void soul_taking_check(P_char ch, P_char tch)
{
	P_obj stiletto;
	// read_object(SOUL_TAKING_STILETTO, VIRTUAL);

	if (GET_CLASS(ch, CLASS_ROGUE))
	{
		if ((stiletto = ch->equipment[WIELD]) &&
		    obj_index[stiletto->R_num].virtual_number == SOUL_TAKING_STILETTO)
		{
			if (vamp(ch, 1.5 * GET_LEVEL(tch),
				 VAMPPERCENT(ch) * static_cast<double>(GET_MAX_HIT(ch))))
			{
				act("&+LYour stiletto &+Wglows &+Lwith power as it &+wdevours &+Lanother &+Wsoul!&n",
				    FALSE, ch, 0, 0, TO_CHAR);
				act("&+L$p &+Wglows brightly in $n&+L's hands as it &+wdevours &+Lanother &+Wsoul!&n",
				    FALSE, ch, stiletto, 0, TO_ROOM);
			}
		}
	}
}

// This function prevents high level chars from entering a teleporter.
