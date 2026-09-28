#include "core/prototypes.h"
#include "core/structs.h"
#include "world/db.h"
#include "net/comm.h"
#include "core/utils.h"
#include "core/defines.h"
#include "magic/spells.h"
#include <stdio.h>
#include <string.h>

extern int top_of_world;

void cont_light_dissipate_event(P_char /*ch*/, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	int room, readd_dark, readd_twilight, had_light = TRUE;

	if (!require_data(data, "cont_light_dissipation", "invalid data"))
		return;

	/*
	 * Ok, let's rock
	 */

	sscanf((char *)data, "%d%d%d", &room, &readd_dark, &readd_twilight);

	if (room < 0 || room > top_of_world)
	{
		logit(LOG_DEBUG, "Cont light dissipation in invalid room. [%d]", room);
		return;
	}
	if (!IS_ROOM(room, ROOM_MAGIC_LIGHT))
		had_light = FALSE;

	REMOVE_BIT(world[room].room_flags, ROOM_MAGIC_LIGHT);

	if (readd_dark)
		SET_BIT(world[room].room_flags, ROOM_DARK);

	if (readd_twilight)
		SET_BIT(world[room].room_flags, ROOM_TWILIGHT);

	if (had_light)
		send_to_room("The light in the area seems to dissipate a bit.\n", room);
	room_light(room, REAL);

	return;
}

void spell_continual_light(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
			   P_obj obj)
{
	char buff[64];
	int readd_dark = FALSE, readd_twilight = FALSE;

	if (obj)
	{
		if (IS_OBJ_STAT(obj, ITEM_LIT))
		{
			act("$p &+Walready emits copious light.", FALSE, ch, obj, 0, TO_CHAR);
			return;
		}
		if (IS_OBJ_STAT2(obj, ITEM2_MAGIC) || IS_OBJ_STAT(obj, ITEM_GLOW))
		{
			act("&+W$p glows brightly for a moment, but the glow fades away.", FALSE,
			    ch, obj, 0, TO_CHAR);
			return;
		}
		act("&+W$p glows brilliantly!", FALSE, ch, obj, 0, TO_CHAR);
		SET_BIT(obj->extra_flags, ITEM_LIT);
	}
	else
	{
		/*
		  send_to_char("&+WThe room lights up&n and then dims slightly.\n", ch);
		  act("&+WThe room lights up.&n", 0, ch, 0, 0, TO_ROOM);
		  SET_BIT(world[ch->in_room].room_flags, ROOM_TWILIGHT);
		*/

		if (IS_ROOM(ch->in_room, ROOM_MAGIC_LIGHT))
		{
			send_to_char("The room already appears to be lit, so nothing happens.\n",
				     ch);
			return;
		}

		if (IS_ROOM(ch->in_room, ROOM_MAGIC_DARK))
		{
			REMOVE_BIT(world[ch->in_room].room_flags, ROOM_MAGIC_DARK);
			send_to_char("The room becomes much less stygian.\n", ch);
			act("The room becomes much less stygian.", 0, ch, 0, 0, TO_ROOM);
		}
		else
		{
			if (IS_ROOM(ch->in_room, ROOM_DARK))
			{
				REMOVE_BIT(world[ch->in_room].room_flags, ROOM_DARK);
				readd_dark = TRUE;
			}
			if (IS_ROOM(ch->in_room, ROOM_TWILIGHT))
			{
				REMOVE_BIT(world[ch->in_room].room_flags, ROOM_TWILIGHT);
				readd_twilight = TRUE;
			}
			snprintf(buff, 64, "%d %d %d", ch->in_room, readd_dark, readd_twilight);

			SET_BIT(world[ch->in_room].room_flags, ROOM_MAGIC_LIGHT);

			send_to_char("&+WThe room lights up!\n", ch);
			act("&+WThe room lights up!", 0, ch, 0, 0, TO_ROOM);

			add_event(cont_light_dissipate_event, level * 50, NULL, NULL, NULL, 0, buff,
				  strlen(buff) + 1);
			// AddEvent(EVENT_SPECIAL, level * 50, TRUE, cont_light_dissipate_event, buff);
		}
	}

	char_light(ch);
	room_light(ch->in_room, REAL);
}

void spell_holy_light(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
		      P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!IS_AFFECTED4(victim, AFF4_MAGE_FLAME) && !IS_AFFECTED4(victim, AFF4_GLOBE_OF_DARKNESS))
	{
		act("&+WA bright, white light suddenly appears over $n&+W's head!", TRUE, victim, 0,
		    0, TO_ROOM);
		act("&+WA bright, white light appears over your head!", TRUE, victim, 0, 0,
		    TO_CHAR);
		bzero(&af, sizeof(af));
		af.type = SPELL_HOLY_LIGHT;
		af.duration = (level / 2 + 3);
		af.modifier = level;
		af.bitvector4 = AFF4_MAGE_FLAME;
		affect_to_char(victim, &af);

		affect_total(victim, FALSE);

		char_light(victim);
		room_light(victim->in_room, REAL);
	}
	else
		send_to_char(
			"You must get rid of your existing holy light or globe of darkness before you can create a holy light.\n",
			victim);
}

void spell_mage_flame(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
		      P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!IS_AFFECTED4(victim, AFF4_MAGE_FLAME) && !IS_AFFECTED4(victim, AFF4_GLOBE_OF_DARKNESS))
	{
		act("&+WA bright, unwavering torch suddenly appears over $n&+W's head!", TRUE,
		    victim, 0, 0, TO_ROOM);
		act("&+WA bright, unwavering torch appears over your head!", TRUE, victim, 0, 0,
		    TO_CHAR);
		bzero(&af, sizeof(af));
		af.type = SPELL_MAGE_FLAME;
		af.duration = (level / 2 + 3);
		af.modifier = level;
		af.bitvector4 = AFF4_MAGE_FLAME;
		affect_to_char(victim, &af);

		affect_total(victim, FALSE);

		char_light(victim);
		room_light(victim->in_room, REAL);
	}
	else
		send_to_char(
			"You must get rid of your existing mage flame or globe of Osrell before you can create a new mage flame.\n",
			victim);
}

void spell_globe_of_darkness(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
			     P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!IS_AFFECTED4(victim, AFF4_MAGE_FLAME) && !IS_AFFECTED4(victim, AFF4_GLOBE_OF_DARKNESS))
	{
		act("&+LA pitch-black sphere suddenly appears over $n&+L's head!", TRUE, victim, 0,
		    0, TO_ROOM);
		act("&+LA pitch-black sphere appears over your head!", TRUE, victim, 0, 0, TO_CHAR);
		bzero(&af, sizeof(af));
		af.type = SPELL_GLOBE_OF_DARKNESS;
		af.duration = (level / 4 + 1);
		af.modifier = level;
		af.bitvector4 = AFF4_GLOBE_OF_DARKNESS;
		affect_to_char(victim, &af);

		affect_total(victim, FALSE);

		char_light(victim);
		room_light(victim->in_room, REAL);
	}
	else
		send_to_char(
			"You must get rid of your existing mage flame or globe of darkness before you can create a globe of darkness.\n",
			victim);
}

void darkness_dissipate_event(P_char /*ch*/, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	int room;

	if (!data)
	{
		logit(LOG_EXIT, "Call to darkness dissipation with invalid data");
		return;
	}
	/*
	 * Ok, let's rock
	 */
	room = *((int *)data);
	if (room <= 0 || room > top_of_world)
	{
		logit(LOG_DEBUG, "Darkness dissipation in invalid room [%d]", room);
		return;
	}

	if (!IS_ROOM(room, ROOM_MAGIC_DARK))
		return;

	REMOVE_BIT(world[room].room_flags, ROOM_MAGIC_DARK);
	send_to_room("&+LThe darkness seems to lift a bit.\n", room);
	room_light(room, REAL);

	return;
}

void spell_darkness(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		    P_obj obj)
{
	char buff[64];

	if (obj)
	{
		if (IS_OBJ_STAT(obj, ITEM_LIT))
		{
			act("The illuminations of $p are negated.", FALSE, ch, obj, 0, TO_CHAR);
			REMOVE_BIT(obj->extra_flags, ITEM_LIT);
			if (OBJ_WORN(obj) || OBJ_CARRIED(obj))
				char_light(ch);
			room_light(ch->in_room, REAL);
			return;
		}
		act("$p &+Wglows brightly for a moment, but the glow fades away.", FALSE, ch, obj,
		    0, TO_CHAR);
		return;
	}
	else
	{
		/*
		    send_to_char("&+LThe room is carpeted in darkness!\n", ch);
		    act("&+LThe room goes dark!", 0, ch, 0, 0, TO_ROOM);
		    SET_BIT(world[ch->in_room].room_flags, ROOM_TWILIGHT);
		*/
		if (IS_ROOM(ch->in_room, ROOM_MAGIC_DARK))
		{
			send_to_char("Nothing happens.\n", ch);
			return;
		}

		if (IS_ROOM(ch->in_room, ROOM_MAGIC_LIGHT))
		{
			REMOVE_BIT(world[ch->in_room].room_flags, ROOM_MAGIC_LIGHT);
			send_to_char("&+LThe room becomes much more stygian.\n", ch);
			act("&+LThe room becomes much more stygian.", 0, ch, 0, 0, TO_ROOM);
		}
		else
		{
			snprintf(buff, 64, "%d", ch->in_room);
			SET_BIT(world[ch->in_room].room_flags, ROOM_MAGIC_DARK);
			send_to_char("&+LThe room is carpeted in darkness!\n", ch);
			act("&+LThe room goes dark!", 0, ch, 0, 0, TO_ROOM);

			add_event(darkness_dissipate_event, level * 50, NULL, NULL, NULL, 0,
				  &(ch->in_room), sizeof((ch->in_room)));
			// AddEvent(EVENT_SPECIAL, level * 50, TRUE, darkness_dissipate_event, buff);
		}
	}
	char_light(ch);
	room_light(ch->in_room, REAL);
}

void spell_innate_darkness(int level, P_char ch, P_char /*victim*/, P_obj /*obj*/)
{
	spell_darkness(level, ch, 0, 0, 0, 0);
}
