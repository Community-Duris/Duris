/* Shared wall object procedure and message helper. */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "combat/damage.h"
#include "combat/justice.h"
#include "world/graph.h"
#include "world/handler.h"
#include "world/map.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"

extern P_char character_list;
extern P_room world;
extern Skill skills[];
extern char *command[];
extern const char *dirs[];
int do_simple_move_skipping_procs(P_char, int, unsigned int);

/* size is the caller's real buffer size; this used to format with
   MAX_STRING_LENGTH into 512-byte buffers. */
void prepare_wall_messages(const char *color_string, char *ch_buffer, char *room_buffer,
			   size_t size)
{
	snprintf(ch_buffer, size,
		 "...and are enveloped by a %s&N field as you try to pass through it.",
		 color_string);
	snprintf(room_buffer, size,
		 "...and is enveloped by a %s&N field as $e tries to pass through it.",
		 color_string);
}

/*
 * This is a generic wall procedure. Since all walls are to large extent similar in their
 * behavior, it is strongly recommended to extend this function for the new walls instead
 * of adding other functions. This would lead to code duplication and general mess.
 * All walls have the same virtual number, with custom descriptions. Values on the item
 * have the following meaning:
 *
 * 0 - room number, where the other side of the wall is
 * 1 - direction this wall is blocking
 * 2 - wall's power ie. damage it deals or its 'hitpoints'
 * 3 - wall type ie. stone, flame, cube, illusionary etc.
 * 4 - wall's level, used for dispelling
 * 5 - number of wall's owner
 */
int wall_generic(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam, type, dircmd, spl;
	int was_in, to_room = 0;
	P_char illusionist = NULL;
	char buffer[MAX_STRING_LENGTH], Gbuf1[MAX_STRING_LENGTH];
	char arg1[512], arg2[512];
	struct follow_type *k, *next_dude;
	P_obj tempobj;
	struct affected_type af;
	bool drag_followers, downexit = FALSE;
	char char_message[512], room_message[512];
	struct damage_messages messages = {
		char_message,
		char_message,
		room_message,
		"Your head is filled with all the colours of the rainbow before it bursts.",
		"Your head is filled with all the colours of the rainbow before it bursts.",
		"$n charges against $p turning $sself into a colourful corpse.",
		0,
		obj
	};

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (obj->loc_p != LOC_ROOM)
		return FALSE;

	type = obj->value[3];

	if (obj->value[1] == DIR_DOWN)
		downexit = TRUE;

	if (cmd == CMD_DECAY)
	{
		if (world[obj->loc.room].people)
		{
			if (type == WALL_OF_FLAMES)
			{
				act("&+R$p&n&+R fades away into thin air...&n", FALSE,
				    world[obj->loc.room].people, obj, 0, TO_ROOM);
				act("&+R$p&n&+R fades away into thin air...&n", FALSE,
				    world[obj->loc.room].people, obj, 0, TO_CHAR);
			}
			else
			{
				act("$p crumbles to dust and blows away.", TRUE,
				    world[obj->loc.room].people, obj, 0, TO_ROOM);
				act("$p crumbles to dust and blows away.", TRUE,
				    world[obj->loc.room].people, obj, 0, TO_CHAR);
			}
		}
		if (!VIRTUAL_EXIT(obj->loc.room, obj->value[1]))
		{
			logit(LOG_DEBUG,
			      "Decay(): error - wall is not on valid exit - room rnum #%d, value[1] %d (trying to remove EX_WALLED)\r\n",
			      obj->loc.room, obj->value[1]);
		}
		else
		{
			REMOVE_BIT(VIRTUAL_EXIT(obj->loc.room, obj->value[1])->exit_info,
				   EX_WALLED);
			REMOVE_BIT(VIRTUAL_EXIT(obj->loc.room, obj->value[1])->exit_info,
				   EX_BREAKABLE);
			REMOVE_BIT(VIRTUAL_EXIT(obj->loc.room, obj->value[1])->exit_info,
				   EX_ILLUSION);
		}
		if (downexit && (world[obj->loc.room].sector_type == SECT_NO_GROUND ||
				 world[obj->loc.room].sector_type == SECT_UNDRWLD_NOGROUND))
		{
			int speed = 1;
			was_in = obj->loc.room;
			for (tempobj = world[was_in].contents; tempobj;
			     tempobj = tempobj->next_content)
			{
				if (tempobj == obj)
					continue;
				add_event(event_falling_obj, 4, NULL, NULL, tempobj, 0, &speed,
					  sizeof(speed));
			}
		}
		return TRUE;
	}

	if (cmd == CMD_FOUND && type == ILLUSIONARY_WALL)
	{
		REMOVE_BIT(VIRTUAL_EXIT(obj->loc.room, obj->value[1])->exit_info, EX_ILLUSION);
		return FALSE;
	}

	if (!ch || (ch->specials.z_cord != 0))
		return FALSE;

	if (type == WATCHING_WALL && cmd >= 1 && cmd < 1000)
	{
		if (obj->loc_p == LOC_ROOM && ch && !IS_TRUSTED(ch) &&
		    (time(NULL) - obj->value[6]) > 10)
		{
			obj->value[6] = time(NULL);

			for (illusionist = character_list; illusionist;
			     illusionist = illusionist->next)
				if (IS_PC(illusionist) && GET_PID(illusionist) == obj->value[5])
					break;

			if (illusionist != NULL && ch->in_room != illusionist->in_room &&
			    !number(0, 2))
			{
				send_to_char("&=LWYou receive a vision from elsewhere.&n&n\n",
					     illusionist);
				new_look(illusionist, "", CMD_LOOK, obj->loc.room);
			}
		}
	}

	if ((cmd == CMD_HIT) &&
	    IS_SET(VIRTUAL_EXIT(obj->loc.room, obj->value[1])->exit_info, EX_BREAKABLE))
	{ // destroy wall by hitting it
		if (arg)
			argument_split_2(arg, arg1, arg2);
		else
			return FALSE;

		if (strcmp(arg1, "wall") || obj->value[1] != dir_from_keyword(arg2))
			return FALSE;

		if (type == WALL_OF_STONE || type == WALL_OF_BONES || type == WATCHING_WALL ||
		    type == WALL_OF_ICE)
			dam = GET_DAMROLL(ch);
		else if (type == WATCHING_WALL && illusionist && IS_ALIVE(illusionist) &&
			 GET_PID(illusionist) == obj->value[5])
		{
			dam = GET_DAMROLL(ch);
			if (ch->in_room != illusionist->in_room && !number(0, 1))
			{
				send_to_char("&+WYou receive a vision from elsewhere.\r\n",
					     illusionist);
				new_look(illusionist, "", CMD_LOOK, obj->loc.room);
			}
		}
		else if (type == WALL_OF_IRON)
			dam = GET_DAMROLL(ch) / 2;
		else if (type == WALL_OF_FORCE)
			dam = GET_DAMROLL(ch) / 3;
		else if (type == WALL_OUTPOST)
			dam = 1;
		else
			return FALSE;

		if (type != WALL_OUTPOST)
			dam += GET_LEVEL(ch) / 4;

		act("You try to destroy $p...", TRUE, ch, obj, 0, TO_CHAR);
		act("$n tries to destroy $p...", TRUE, ch, obj, 0, TO_NOTVICT);

		if (!ac_can_see_obj(ch, obj))
		{
			act("You barely feel the wall, but yet you try!", TRUE, ch, obj, 0,
			    TO_CHAR);
			dam = dam / 3;
		}

		if (obj->value[2] < dam || IS_TRUSTED(ch) ||
		    (IS_PC(ch) && obj->value[5] == GET_PID(ch)) ||
		    (IS_NPC(ch) && obj->value[5] == GET_RNUM(ch)))
		{
			act("Your mighty hit totally destroys $p...", TRUE, ch, obj, 0, TO_CHAR);
			act("$n's mighty hit totally destroys $p...", TRUE, ch, obj, 0, TO_NOTVICT);
			// level 70 ensures that its dispelled..
			spell_dispel_magic(70, ch, NULL, SPELL_TYPE_SPELL, 0, obj);
		}
		else
		{
			obj->value[2] -= dam;
			damage(ch, ch, MAX(GET_LEVEL(ch) / 2, GET_DAMROLL(ch) * 2 - dam),
			       TYPE_UNDEFINED);
		}
		if (type == WALL_OUTPOST)
			CharWait(ch, PULSE_VIOLENCE);
		else
			CharWait(ch, PULSE_VIOLENCE * 4);
		return TRUE;
	}

	dircmd = cmd_to_exitnumb(cmd);

	/* we go further only if a direction command was executed */
	if (dircmd == -1)
		return FALSE;

	/* does this wall really block the attempted direction */
	if (obj->value[1] != dircmd)
		return FALSE;

	if (IS_TRUSTED(ch))
	{
		act("&+WYou ignore the physical limitations of the world.&n", TRUE, ch, obj, NULL,
		    TO_CHAR);
		act("$n &+Wsteps through $p grinning.&n", TRUE, ch, obj, NULL, TO_ROOM);
		do_simple_move_skipping_procs(ch, dircmd, 0);
		return TRUE;
	}

	drag_followers = FALSE;
	was_in = ch->in_room;
	to_room = real_room0(obj->value[0]);
	dam = obj->value[2];

	if (to_room == NOWHERE)
	{
		send_to_char("Bug with a wall! Report this to a god.\n", ch);
		return FALSE;
	}

	if (IS_AFFECTED(ch, AFF_WRAITHFORM))
	{
		send_to_char("&+RYou enter what appears to be a magical wall...&n\n"
			     "&+RYou chuckle at the limitations of the material world.&n\n",
			     ch);
		do_simple_move_skipping_procs(ch, dircmd, 0);
		return TRUE;
	}

	if (!leave_by_exit(ch, dircmd))
	{
		return TRUE;
	}

	switch (type)
	{
	case WALL_OF_FLAMES:
		snprintf(buffer, MAX_STRING_LENGTH,
			 "$n &+Ris surrounded by flames as $e goes to the %s.", dirs[dircmd]);
		act(buffer, TRUE, ch, obj, NULL, TO_ROOM);
		/* XXX */
		if (!ENJOYS_FIRE_DAM(ch))
		{
			send_to_char("&+RYou enter into a wall of flames...OUCH!&n\n", ch);

			if (IS_AFFECTED(ch, AFF_PROT_FIRE))
				dam /= 3;

			if (IS_NPC(ch) && !IS_MORPH(ch) && !IS_PC_PET(ch))
				dam = 1;

			if (((GET_HIT(ch) - 8) < dam))
			{
				send_to_char(
					"&+RYou are overwhelmed by the heat and&n&+L fall into darkness...\n",
					ch);
				do_simple_move_skipping_procs(ch, dircmd, 0);
				act("$n &+Rfalls through the flames burnt to a crisp!&n", FALSE, ch,
				    obj, NULL, TO_NOTVICT);
				die(ch, ch);
				return TRUE;
			}

			GET_HIT(ch) -= dam;
			spell_blindness(obj->value[4], ch, 0, SPELL_TYPE_SPELL, ch, NULL);
			do_simple_move_skipping_procs(ch, dircmd, 0);
			act("$n &+Rsteps through the flames!&n", TRUE, ch, NULL, NULL, TO_ROOM);
		}
		else
		{
			send_to_char("&+RYou feel the healing power of the flames!&n\n", ch);
			GET_HIT(ch) = MIN(GET_HIT(ch) + dam, GET_MAX_HIT(ch));
			do_simple_move_skipping_procs(ch, dircmd, 0);
			act("$n &+Rsteps through the flames grinning!&n", TRUE, ch, NULL, NULL,
			    TO_ROOM);
		}

		update_pos(ch);
		StartRegen(ch, regen_resource::hit);
		drag_followers = TRUE;
		break;
		/* XXX */

	case WALL_OF_ICE:
		if (IS_PC(ch) && has_innate(ch, INNATE_WALL_CLIMBING) && number(1, 100) > 60)
		{
			act("&+L$n&+L slams up against the wall, slinks into a shadow, and quickly darts over the wall.",
			    TRUE, ch, obj, 0, TO_ROOM);
			act("&+LYou thrust yourself up against the wall and slip into a nearby shadow, quickly darting over the top of the wall and away.",
			    TRUE, ch, obj, 0, TO_CHAR);
			do_simple_move_skipping_procs(ch, dircmd, 0);
			if (IS_AFFECTED2(ch, AFF2_PROT_COLD))
				dam -= dam / 3;
			GET_HIT(ch) = MAX(GET_HIT(ch) - MAX(dam, 20), 1);
			update_pos(ch);
			return TRUE;
		}

		act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
		act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);

		if (IS_AFFECTED2(ch, AFF2_PROT_COLD))
			dam -= dam / 3;

		send_to_char("&+WYou shiver from the &+bfreezing cold &+Wof the &+Cice&+W!&n\n",
			     ch);
		GET_HIT(ch) = MAX(GET_HIT(ch) - MAX(dam / 2, 20), 1);
		update_pos(ch);
		StartRegen(ch, regen_resource::vitality);

		break;
	case LIGHTNING_CURTAIN:
		snprintf(buffer, MAX_STRING_LENGTH,
			 "$n &+Bis surrounded by lightning as $e goes to the %s.&n", dirs[dircmd]);
		act(buffer, TRUE, ch, obj, NULL, TO_ROOM);

		if (IS_AFFECTED2(ch, AFF2_PROT_LIGHTNING))
			dam -= dam / 3;

		if (IS_NPC(ch) && !IS_MORPH(ch))
			dam = 1;

		if (((GET_HIT(ch) - 10) < dam))
		{
			send_to_char("&+BYou are shocked to death!&n\n", ch);
			do_simple_move_skipping_procs(ch, dircmd, 0);
			act("$n &+Bfalls through the curtain looking quite charred!&n", FALSE, ch,
			    obj, NULL, TO_NOTVICT);
			die(ch, ch);
			return TRUE;
		}

		GET_HIT(ch) = MAX(GET_HIT(ch) - dam, 1);
		do_simple_move_skipping_procs(ch, dircmd, 0);
		act("$n &+Bsteps through the lightning curtain!&n", TRUE, ch, NULL, NULL, TO_ROOM);
		act("&+YOUCH!  &+BYou step through the lightning curtain!&n", TRUE, ch, NULL, NULL,
		    TO_CHAR);
		update_pos(ch);
		StartRegen(ch, regen_resource::hit);

		drag_followers = TRUE;
		break;

	case PRISMATIC_WALL:

		if ((IS_PC(ch) && GET_PID(ch) == obj->value[5]) ||
		    (IS_NPC(ch) && GET_RNUM(ch) == obj->value[5]))
		{
			act("You walk through your own wall.", TRUE, ch, obj, NULL, TO_CHAR);
			do_simple_move_skipping_procs(ch, dircmd, 0);
			act("$n steps through the wall.", TRUE, ch, obj, NULL, TO_ROOM);
			return TRUE;
		}

		// let them walk through the wall 33% of the time
		if (!number(0, 2) && IS_PC(ch))
		{
			act("$p fades to shards of magic, and blows away...&n", TRUE, ch, obj, NULL,
			    TO_ROOM);
			send_to_char("The prismatic creation fades into nothing.\n", ch);
			spell_dispel_magic(60, ch, NULL, SPELL_TYPE_SPELL, 0, obj);
			act("You walk through the wall.", TRUE, ch, obj, NULL, TO_CHAR);
			act("$n steps through the wall.", TRUE, ch, obj, NULL, TO_ROOM);
			do_simple_move_skipping_procs(ch, dircmd, 0);
			act("$n steps through the wall.", TRUE, ch, obj, NULL, TO_ROOM);
			GET_HIT(ch) = MAX(1, GET_HIT(ch) - 75);
			return TRUE;
		}
		else if (IS_PC(ch) && has_innate(ch, INNATE_WALL_CLIMBING) && number(1, 100) > 60)
		{
			act("&+L$n&+L slams up against the wall, slinks into a shadow, and quickly darts over the wall.",
			    TRUE, ch, obj, 0, TO_ROOM);
			act("&+LYou thrust yourself up against the wall and slip into a nearby shadow, quickly darting over the top of the wall and away.",
			    TRUE, ch, obj, 0, TO_CHAR);
			do_simple_move_skipping_procs(ch, dircmd, 0);
			return TRUE;
		}
		else if (!number(0, 2) && IS_PC_PET(ch))
		{
			act("You walk through the wall.", TRUE, ch, obj, NULL, TO_CHAR);
			act("$n steps through the wall.", TRUE, ch, obj, NULL, TO_ROOM);
			do_simple_move_skipping_procs(ch, dircmd, 0);
			act("$n steps through the wall.", TRUE, ch, obj, NULL, TO_ROOM);
			GET_HIT(ch) = MAX(1, GET_HIT(ch) - 75);

			return TRUE;
		}
		else
		{
			act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
			act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);
		}

		dam = 0;
		spl = 0;

		switch (number(0, 6))
		{
		case 0:
			prepare_wall_messages("&+rred", char_message, room_message,
					      sizeof char_message);
			dam = 100;
			break;
		case 1:
			prepare_wall_messages("&+Rorange", char_message, room_message,
					      sizeof char_message);
			dam = 70;
			break;
		case 2:
			prepare_wall_messages("&+Yyellow", char_message, room_message,
					      sizeof char_message);
			dam = 40;
			break;
		case 3:
			prepare_wall_messages("&+Bblue", char_message, room_message,
					      sizeof char_message);
			spl = SPELL_MINOR_PARALYSIS;
			break;
		case 4:
			prepare_wall_messages("&+bindigo", char_message, room_message,
					      sizeof char_message);
			spl = SPELL_FEEBLEMIND;
			break;
		case 5:
			prepare_wall_messages("&+ggreen", char_message, room_message,
					      sizeof char_message);
			spl = SPELL_POISON;
			break;
		case 6:
			prepare_wall_messages("&+bazure", char_message, room_message,
					      sizeof char_message);
			spl = SPELL_BLINDNESS;
			break;
		}

		if (spl)
		{
			act(char_message, TRUE, ch, obj, 0, TO_CHAR);
			act(room_message, TRUE, ch, obj, 0, TO_NOTVICT);
			skills[spl].spell_pointer(50, ch, 0, 0, ch, NULL);
		}
		else if (dam > 0)
		{
			if (NewSaves(ch, SAVING_SPELL, 0))
				dam >>= 1;
			if (IS_NPC(ch) && !IS_MORPH(ch))
				dam = 1;
			spell_damage(ch, ch, dam, SPLDAM_FIRE, 0, &messages);
		}

		return TRUE;

	case WEB:
		snprintf(buffer, MAX_STRING_LENGTH,
			 "$n &+Wis enveloped in sticky webs as $e goes to the %s.", dirs[dircmd]);
		send_to_char("&+WYou enter into the web!&n\n", ch);
		act(buffer, TRUE, ch, obj, NULL, TO_ROOM);
		do_simple_move_skipping_procs(ch, dircmd, 0);
		act("$n &+Wsteps through the web&n", TRUE, ch, NULL, NULL, TO_ROOM);
		if (!NewSaves(ch, SAVING_PARA, 0))
		{
			if (!check_freedom_of_movement(ch, number(0, 1)))
			{
				bzero(&af, sizeof(af));
				af.type = SPELL_MINOR_PARALYSIS;
				af.flags = AFFTYPE_SHORT;
				af.duration = number(4, 15) * WAIT_SEC;
				af.bitvector2 = AFF2_MINOR_PARALYSIS;
				affect_to_char(ch, &af);
			}
		}
		spell_dispel_magic(45 + GET_ALT_SIZE(ch), ch, NULL, SPELL_TYPE_SPELL, 0, obj);
		update_pos(ch);

		drag_followers = TRUE;
		break;

	case LIFE_WARD:
		if ((IS_PC(ch) && GET_PID(ch) == obj->value[5]) ||
		    (IS_NPC(ch) && obj->value[5] == GET_RNUM(ch)) || !number(0, 3))
		{
			act("&+L$n&+L passes right through $p&+L unharmed.", TRUE, ch, obj, 0,
			    TO_ROOM);
			act("&+LYou pass right through $p&+L unharmed.", TRUE, ch, obj, 0, TO_CHAR);
			do_simple_move_skipping_procs(ch, dircmd, 0);
			return TRUE;
		}
		else if (IS_PC(ch) && has_innate(ch, INNATE_WALL_CLIMBING) && number(1, 100) > 60)
		{
			act("&+L$n&+L slams up against the wall, slinks into a shadow, and quickly darts over the wall.",
			    TRUE, ch, obj, 0, TO_ROOM);
			act("&+LYou thrust yourself up against the wall and slip into a nearby shadow, quickly darting over the top of the wall and away.",
			    TRUE, ch, obj, 0, TO_CHAR);
			do_simple_move_skipping_procs(ch, dircmd, 0);
			dam = number(10, 28);
			GET_VITALITY(ch) = MAX(GET_VITALITY(ch) - dam, 1);
			update_pos(ch);
			StartRegen(ch, regen_resource::vitality);
			return TRUE;
		}
		act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
		act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);

		dam = number(8, 22);

		send_to_char(
			"&+LYour limbs go numb as you try to pass through the &n&+bnegative energy.&n\n",
			ch);
		GET_VITALITY(ch) = MAX(GET_VITALITY(ch) - dam, 1);
		update_pos(ch);
		StartRegen(ch, regen_resource::vitality);
		drag_followers = TRUE;
		break;

	case ILLUSIONARY_WALL:
		if (!IS_SET(obj->extra_flags, ITEM_SECRET))
		{
			act("$p blows away&n", TRUE, ch, obj, NULL, TO_ROOM);
			send_to_char("The illusion dissipates.\n", ch);
			spell_dispel_magic(60, ch, NULL, SPELL_TYPE_SPELL, 0, obj);
			return FALSE;
		}
		if (number(0, 5))
		{
			send_to_char("Alas, you cannot go that way. . . .\n", ch);
			return TRUE;
		}
		if (!IS_TRUSTED(ch))
		{
			send_to_char("It was just an illusion!!\n", ch);
			if (IS_SET(obj->extra_flags, ITEM_SECRET))
			{
				REMOVE_BIT(obj->extra_flags, ITEM_SECRET);
			}
		}
		else
		{
			send_to_char("You see right through this petty illusion.\n", ch);
		}
		return FALSE;

	case WALL_OF_FORCE:
		/*if ((IS_PC(ch) && GET_PID(ch) == obj->value[5]) ||
			    (IS_NPC(ch) && GET_RNUM(ch) == obj->value[5])) */
		if (IS_PC(ch))
		{
			act("&+L$n&+L passes right through $p&+L.", TRUE, ch, obj, 0, TO_ROOM);
			act("&+LYou pass right through $p&+L.", TRUE, ch, obj, 0, TO_CHAR);

			do_simple_move_skipping_procs(ch, dircmd, 0);
		}
		else if (IS_PC(ch) && has_innate(ch, INNATE_WALL_CLIMBING))
		{
			int rand1 = number(1, 100);
			if (rand1 > 60)
			{
				act("&+L$n&+L slams up against the wall, slinks into a shadow, and quickly darts over the wall.",
				    TRUE, ch, obj, 0, TO_ROOM);
				act("&+LYou thrust yourself up against the wall and slip into a nearby shadow, quickly darting over the top of the wall and away.",
				    TRUE, ch, obj, 0, TO_CHAR);
				do_simple_move_skipping_procs(ch, dircmd, 0);
			}
			else
			{
				act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
				act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);
			}
		}
		else
		{
			act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
			act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);
		}
		return TRUE;
	case WALL_OUTPOST:
	case WATCHING_WALL:
	case WALL_OF_IRON:
		if (IS_PC(ch) && has_innate(ch, INNATE_WALL_CLIMBING))
		{
			if (number(1, 100) > 60)
			{
				act("&+L$n&+L slams up against the wall, slinks into a shadow, and quickly darts over the wall.",
				    TRUE, ch, obj, 0, TO_ROOM);
				act("&+LYou thrust yourself up against the wall and slip into a nearby shadow, quickly darting over the top of the wall and away.",
				    TRUE, ch, obj, 0, TO_CHAR);
				do_simple_move_skipping_procs(ch, dircmd, 0);
			}
			else
			{
				act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
				act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);
			}
		}
		else
		{
			act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
			act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);
		}
		return TRUE;
	case WALL_OF_STONE:
		if (IS_PC(ch) && has_innate(ch, INNATE_WALL_CLIMBING))
		{
			if (number(1, 100) > 60)
			{
				act("&+L$n&+L slams up against the wall, slinks into a shadow, and quickly darts over the wall.",
				    TRUE, ch, obj, 0, TO_ROOM);
				act("&+LYou thrust yourself up against the wall and slip into a nearby shadow, quickly darting over the top of the wall and away.",
				    TRUE, ch, obj, 0, TO_CHAR);
				do_simple_move_skipping_procs(ch, dircmd, 0);
			}
			else
			{
				act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
				act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);
			}
		}
		else
		{
			act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
			act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);
		}
		return TRUE;
	case WALL_OF_BONES:
		if (obj->value[2] <
		    10) /* Hackich assumption that if strength < 10 it's a thin dragonscale sheath */
		{
			if (obj->value[2] <= 1)
			{
				act("You bump into $p, destroying it in the process!", TRUE, ch,
				    obj, 0, TO_CHAR);
				act("$n bumps into $p, destroying it in the process!", TRUE, ch,
				    obj, 0, TO_NOTVICT);
				// level 70 ensures that its dispelled..
				spell_dispel_magic(70, ch, NULL, SPELL_TYPE_SPELL, 0, obj);
			}
			else if (IS_PC(ch) && has_innate(ch, INNATE_WALL_CLIMBING) &&
				 number(1, 100) > 60)
			{
				act("&+L$n&+L slams up against the wall, slinks into a shadow, and quickly darts over the wall.",
				    TRUE, ch, obj, 0, TO_ROOM);
				act("&+LYou thrust yourself up against the wall and slip into a nearby shadow, quickly darting over the top of the wall and away.",
				    TRUE, ch, obj, 0, TO_CHAR);
				do_simple_move_skipping_procs(ch, dircmd, 0);
				return TRUE;
			}
			else
			{
				act("You bump into $p, visibly weakening it!", TRUE, ch, obj, 0,
				    TO_CHAR);
				act("$n bumps into $p, visibly weakening it!", TRUE, ch, obj, 0,
				    TO_NOTVICT);
				if (!number(0, 2))
					obj->value[2] -= 1;
			}
		}
		else if (IS_PC(ch) && has_innate(ch, INNATE_WALL_CLIMBING) && number(1, 100) > 60)
		{
			act("&+L$n&+L slams up against the wall, slinks into a shadow, and quickly darts over the wall.",
			    TRUE, ch, obj, 0, TO_ROOM);
			act("&+LYou thrust yourself up against the wall and slip into a nearby shadow, quickly darting over the top of the wall and away.",
			    TRUE, ch, obj, 0, TO_CHAR);
			do_simple_move_skipping_procs(ch, dircmd, 0);
		}
		else /* a "normal" wall of bones */
		{
			act("Oof! You bump into $p...", TRUE, ch, obj, 0, TO_CHAR);
			act("Oof! $n bumps into $p...", TRUE, ch, obj, 0, TO_NOTVICT);
		}
		return TRUE;
	case WALL_OF_AIR:
		int chance, fall_chance;

		switch (GET_SIZE(ch))
		{
		case SIZE_NONE:
		case SIZE_TINY:
		case SIZE_SMALL:
			chance = 15;
			fall_chance = 20;
			break;
		case SIZE_MEDIUM:
			chance = 25;
			fall_chance = 15;
			break;
		case SIZE_LARGE:
		case SIZE_HUGE:
			chance = 50;
			fall_chance = 10;
			break;
		case SIZE_GIANT:
		case SIZE_GARGANTUAN:
			chance = 70;
			fall_chance = 0;
			break;
		case SIZE_DEFAULT:
		default:
			chance = 0;
			fall_chance = 100;
			break;
		}

		// Let stats play a minor role.
		chance += (GET_C_AGI(ch) + GET_C_STR(ch)) / 50;
		fall_chance -= (GET_C_AGI(ch) + GET_C_STR(ch)) / 50;

		if (chance >= number(1, 100))
		{
			act("$n steps through $p!", TRUE, ch, obj, NULL, TO_ROOM);
			act("&+cYou manage to break through the high winds on to the other side!&n",
			    FALSE, ch, obj, NULL, TO_CHAR);
			do_simple_move_skipping_procs(ch, dircmd, 0);
			update_pos(ch);
			drag_followers = TRUE;
		}
		else if (fall_chance >= number(1, 100))
		{
			// They got knocked down.
			act("&+WThe &+CHIGH winds &+Wsend you flying back into the room, crashing to the ground &+RH&+rA&+RR&+rD&+W!&n",
			    FALSE, ch, obj, NULL, TO_CHAR);
			act("&+WThe &+CHIGH winds &+Wsend $n&+W flying back into the room, who proceeds to fall to the ground.  Hah!&n",
			    TRUE, ch, obj, NULL, TO_ROOM);

			SET_POS(ch, GET_STAT(ch) + POS_SITTING);
			return TRUE;
		}
		else
		{
			// Didn't fall, but couldn't make it through.
			act("&+WThe high &+Cwinds &+Wwere too strong for you to go that way!&n",
			    FALSE, ch, obj, NULL, TO_CHAR);
			act("&+WThe high &+Cwinds &+Wwere too strong for $n&+W to make $s way through.",
			    TRUE, ch, obj, NULL, TO_ROOM);
			return TRUE;
		}
		break;
	default:
		logit(LOG_DEBUG, "Wrong value[3] set in wall.");
		send_to_char("Serious screw-up on wall! Tell a god.\n", ch);
		return FALSE;
	}

	if (drag_followers && was_in != ch->in_room && ch->followers)
	{
		for (k = ch->followers; k; k = next_dude)
		{
			next_dude = k->next;

			if ((was_in == k->follower->in_room) && CAN_ACT(k->follower) &&
			    MIN_POS(k->follower, POS_STANDING + STAT_RESTING) &&
			    !IS_FIGHTING(k->follower) && !NumAttackers(k->follower) &&
			    CAN_SEE(k->follower, ch))
			{
				act("You follow $N.", FALSE, k->follower, 0, ch, TO_CHAR);
				send_to_char("\n", k->follower);
				snprintf(Gbuf1, MAX_STRING_LENGTH, "%s %s",
					 command[exitnumb_to_cmd(dircmd) - 1], arg);
				command_interpreter(k->follower, Gbuf1);
			}
		}
	}

	return TRUE;
}
