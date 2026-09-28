#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "magic/spells.h"
#include "economy/economic_gameplay_authority.h"
#include <string.h>

extern const int top_of_world;

void spell_control_weather(int /*level*/, P_char /*ch*/, P_char /*victim*/, P_obj /*obj*/)
{
	/* Control Weather is not possible here!!! */
	/* Better/Worse can not be transferred */
	/* (e.g. Its done in spells.c instead) */
}

void spell_wandering_woods(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			   P_char /*victim*/, P_obj /*obj*/)
{
	struct room_affect af;
	int temp;

	if (GET_LEVEL(ch) < 50)
	{
		temp = 2;
	}
	else if ((GET_LEVEL(ch) >= 50) && (GET_LEVEL(ch) <= 55))
	{
		temp = 3;
	}
	else
	{
		temp = 4;
	}

	if (!ch)
		return;
	if (!(world[ch->in_room].sector_type == SECT_FOREST ||
	      world[ch->in_room].sector_type == SECT_SWAMP) ||
	    get_spell_from_room(&world[ch->in_room], SPELL_WANDERING_WOODS))
	{
		send_to_char("Nothing happens.\n", ch);
		return;
	}
	send_to_char("&+GThe wilderness around you comes alive for just a moment!\n", ch);
	act("&+GThe land around you becomes dark and mysterious!", 0, ch, 0, 0, TO_ROOM);

	memset(&af, 0, sizeof(struct room_affect));
	af.type = SPELL_WANDERING_WOODS;
	af.duration = (1 + temp) * 40;
	af.ch = ch;
	affect_to_room(ch->in_room, &af);
}

void spell_consecrate_land(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			   P_char /*victim*/, P_obj /*obj*/)
{
	struct room_affect *raf;
	struct room_affect af;

	if (!ch || get_spell_from_room(&world[ch->in_room], SPELL_CONSECRATE_LAND))
		return;

	if ((raf = get_spell_from_room(&world[ch->in_room], SPELL_DESECRATE_LAND)))
	{
		affect_room_remove(ch->in_room, raf);
		send_to_char("&+YYou destroy the &+Crunes&+Y laying about the area.&n\r\n", ch);
		act("&+Y$n&+Y's prayer shatters the &+Crunes&+Y laying around the area.&n", 0, ch,
		    0, 0, TO_ROOM);
		return;
	}

	switch (world[ch->in_room].sector_type)
	{
	case SECT_INSIDE:
	case SECT_UNDRWLD_INSIDE:
		send_to_char("Try again, OUTDOORS this time.\r\n", ch);
		return;
		break;
	case SECT_CITY:
	case SECT_ROAD:
	case SECT_CASTLE_WALL:
	case SECT_CASTLE_GATE:
	case SECT_UNDRWLD_CITY:
	case SECT_CASTLE:
		send_to_char("Nothing happens.  Perhaps you need to be farther outdoors...\r\n",
			     ch);
		return;
		break;
	case SECT_SWAMP:
	case SECT_UNDRWLD_SLIME:
	case SECT_FIELD:
	case SECT_FOREST:
	case SECT_HILLS:
	case SECT_MOUNTAIN:
	case SECT_UNDRWLD_WILD:
	case SECT_UNDRWLD_MUSHROOM:
	case SECT_UNDRWLD_MOUNTAIN:
	case SECT_UNDRWLD_LOWCEIL:
	case SECT_DESERT:
		send_to_char(
			"&+GYour god &+Wblesses &+Gthis area with the awesome power of &+Ctranquility&+G.&n\r\n",
			ch);
		act("&+G$n&+G's prayer brings a &+Ctranquil &+Wlight &+Gfrom the &+WHe&N&+waven&+Ws.&n",
		    0, ch, 0, 0, TO_ROOM);
		memset(&af, 0, sizeof(struct room_affect));
		af.type = SPELL_CONSECRATE_LAND;
		af.duration = (GET_LEVEL(ch) * 4);
		af.ch = ch;
		affect_to_room(ch->in_room, &af);
		break;
	case SECT_PLANE_OF_AVERNUS:
		send_to_char("There's far too much vileness for your meager will to overcome.\r\n",
			     ch);
		return;
		break;
	case SECT_NO_GROUND:
	case SECT_WATER_SWIM:
	case SECT_WATER_NOSWIM:
	case SECT_UNDRWLD_NOSWIM:
	case SECT_UNDRWLD_WATER:
	case SECT_FIREPLANE:
	case SECT_UNDRWLD_LIQMITH:
	case SECT_NEG_PLANE:
	case SECT_UNDERWATER:
	case SECT_UNDRWLD_NOGROUND:
	case SECT_UNDERWATER_GR:
	case SECT_OCEAN:
		send_to_char("Consecrate _LAND_...  There is no land here!\r\n", ch);
		return;
		break;
	default:
		logit(LOG_DEBUG, "Bogus sector_type (%d) in consecrate_land",
		      world[ch->in_room].sector_type);
		send_to_char("How strange!  This terrain doesn't seem to exist!\r\n", ch);
		return;
		break;
	}
}

struct airy_water_data
{
	int room;
	int old_sect;
	int readd_uw;
};

void spell_binding_wind(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			P_char /*victim*/, P_obj /*obj*/)
{
	struct room_affect af;
	struct room_affect *afp;

	if (!ch || get_spell_from_room(&world[ch->in_room], SPELL_BINDING_WIND))
		return;

	if (!OUTSIDE(ch))
	{
		send_to_char(
			"&+CYou cannot manipulate winds where there are none! Next time try it outside.\n",
			ch);
		return;
	}

	send_to_char("&+CYou summon up the wind speed so that it is hard to move!\n", ch);
	act("&+C$n waves his hands and the wind picks up!&n", 0, ch, 0, 0, TO_ROOM);

	memset(&af, 0, sizeof(struct room_affect));
	af.type = SPELL_BINDING_WIND;
	af.duration = 250;
	af.ch = ch;
	affect_to_room(ch->in_room, &af);

	if ((afp = get_spell_from_room(&world[ch->in_room], SPELL_WIND_TUNNEL)))
	{
		affect_room_remove(ch->in_room, afp);
	}
	return;
}

void spell_wind_tunnel(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		       P_char /*victim*/, P_obj /*obj*/)
{
	struct room_affect af;
	struct room_affect *afp;

	send_to_char("&+cYou summon up the wind speed so that its easy to move!\n", ch);
	act("&+c$n waves his hands and the wind picks up! Hey, it's easier to move!&n", 0, ch, 0, 0,
	    TO_ROOM);

	memset(&af, 0, sizeof(struct room_affect));
	af.type = SPELL_WIND_TUNNEL;
	af.duration = 250;
	af.ch = ch;
	affect_to_room(ch->in_room, &af);

	if ((afp = get_spell_from_room(&world[ch->in_room], SPELL_BINDING_WIND)))
	{
		affect_room_remove(ch->in_room, afp);
	}
	return;
}

void event_airy_water_dissipate(P_char /*ch*/, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	struct airy_water_data *d = (struct airy_water_data *)data;
	P_char tch, next;

	if ((d->room < 0) || (d->room > top_of_world))
	{
		logit(LOG_EXIT, "Airy water dissipation in invalid room.");
		return;
	}
	if (d->readd_uw)
		SET_BIT(world[d->room].room_flags, ROOM_UNDERWATER);

	world[d->room].sector_type = d->old_sect;

	send_to_room("The bubble slowly shrinks, allowing water to fill the area once more.\n",
		     d->room);

	/* update everyone's underwater status */

	for (tch = world[d->room].people; tch; tch = next)
	{
		next = tch->next_in_room;

		underwatersector(tch);
	}
}

void spell_airy_water(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		      P_obj /*obj*/)
{
	struct airy_water_data data;
	P_char next, tch;

	if (!IS_UNDERWATER(ch))
	{
		send_to_char("This spell only works underwater.\n", ch);
		return;
	}
	send_to_char(
		"A large air bubble slowly expands around you, displacing all water in the area.\n",
		ch);
	act("A large air bubble slowly expands around $n&n, displacing all water in the area.",
	    FALSE, ch, 0, 0, TO_ROOM);

	data.readd_uw = IS_ROOM(ch->in_room, ROOM_UNDERWATER);
	data.old_sect = world[ch->in_room].sector_type;
	data.room = ch->in_room;

	REMOVE_BIT(world[ch->in_room].room_flags, ROOM_UNDERWATER);
	world[ch->in_room].sector_type = SECT_WATER_SWIM;

	/* update everyone's underwater status */

	for (tch = world[ch->in_room].people; tch; tch = next)
	{
		next = tch->next_in_room;
		underwatersector(tch);
	}

	add_event(event_airy_water_dissipate, level * 10, 0, 0, 0, 0, &data, sizeof(data));
}

void spell_natures_call(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
			P_obj /*obj*/)
{
	int room = ch->in_room;

	send_to_room("&+gThe &+Gforces of nature&+g flows through the area...&n\n", ch->in_room);
	add_event(event_natures_call, PULSE_VIOLENCE, ch, 0, 0, 0, &room, sizeof(room));
}

void spell_natures_calling(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			   P_obj /*obj*/)
{
	int num, effectiveness;

	if (!IS_ALIVE(ch))
	{
		return;
	}

	num = dice(2, 6);
	effectiveness = (int)(level / 2);

	switch (num)
	{
	case 2:
		cast_vines(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
		break;
	case 3:
		spell_stone_skin(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
		break;
	case 4:
		spell_protection_from_fire(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
		spell_protection_from_cold(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
		spell_protection_from_lightning(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim,
						0);
		spell_protection_from_gas(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
		break;
	case 5:
		spell_virtue(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
		break;
	case 6:
		spell_armor(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
		break;
	case 7:
		spell_barkskin(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
		break;
	case 8:
		spell_armor(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
		break;
	case 9:
		spell_virtue(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
		break;
	case 10:
		spell_protection_from_fire(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
		spell_protection_from_cold(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
		spell_protection_from_lightning(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim,
						0);
		spell_protection_from_gas(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
		break;
	case 11:
		spell_stone_skin(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
		break;
	case 12:
		cast_vines(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
		break;
	}

	if (GET_ALIGNMENT(ch) > 350)
	{
		spell_bless(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
	}

	if (!(number(0, 2)))
	{
		spell_protection_from_evil(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
	}
	if (!(number(0, 2)))
	{
		spell_protection_from_animals(effectiveness, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
	}

	return;
}

void event_natures_call(P_char ch, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	int level;
	int room = *((int *)data);
	struct group_list *gl;

	if (!IS_ALIVE(ch))
	{
		return;
	}

	level = GET_LEVEL(ch);

	if (!number(0, 1))
	{
		send_to_room(
			"&+gThe &+Gforces of nature&+g come into balance, filling the room with life...\n",
			room);
		add_event(event_natures_call, PULSE_VIOLENCE, ch, 0, 0, 0, &room, sizeof(room));
	}

	act("&+gThe &+ycreatures of nature&+g in this area &+Gfill with energy!&n", FALSE, ch, 0, 0,
	    TO_CHAR);

	act("&+gThe &+ycreatures of nature&+g in this area &+Gfill with energy!&n", FALSE, ch, 0, 0,
	    TO_ROOM);

	if (ch->group)
	{
		for (gl = ch->group; gl; gl = gl->next)
		{
			if (gl->ch->in_room == ch->in_room)
			{
				spell_natures_calling(level, ch, NULL, SPELL_TYPE_SPELL, gl->ch, 0);
			}
		}
	}
	else
	{
		spell_natures_calling(level, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
	}
}

void spell_create_spring(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
			 P_obj /*obj*/)
{
	P_obj spring;

	/*  if(!OUTSIDE(ch)) {
	    send_to_char("Fountain indoors? Putz!\n", ch);
	    return;
	  }*/

	if (world[ch->in_room].sector_type == SECT_NO_GROUND ||
	    world[ch->in_room].sector_type == SECT_UNDRWLD_NOGROUND ||
	    world[ch->in_room].sector_type == SECT_OCEAN)
	{
		send_to_char("&+bA spring usually needs more solid ground for support!\n", ch);
		return;
	}
	if (economic_gameplay_authority::active())
	{
		send_to_char("A spring cannot be formed right now.\r\n", ch);
		return;
	}

	spring = read_object(750, VIRTUAL);
	if (!spring)
	{
		logit(LOG_DEBUG, "spell_create_spring(): obj 750 (spring) not loadable");
		send_to_char("Tell someone to make a spring object ASAP!\n", ch);
		return;
	}
	spring->value[0] = GET_LEVEL(ch);
	send_to_room("&+bA spring shoots up from the ground!\n", ch->in_room);
	set_obj_affected(spring, 60 * 10, TAG_OBJ_DECAY, 0);
	obj_to_room(spring, ch->in_room);
}

void spell_divine_font(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		       P_obj /*obj*/)
{
	P_obj font;
	if (economic_gameplay_authority::active())
	{
		send_to_char("A divine font cannot be formed right now.\r\n", ch);
		return;
	}

	font = read_object(469, VIRTUAL);
	if (!font)
	{
		logit(LOG_DEBUG, "spell_divine_font(): obj 469 (font) not loadable");
		send_to_char("Tell someone to make a font object ASAP!\n", ch);
		return;
	}
	font->value[0] = GET_LEVEL(ch);
	send_to_room("&+WA divine font slowly fades into existence, radiating holy power!&n\n",
		     ch->in_room);
	set_obj_affected(font, 60 * 10, TAG_OBJ_DECAY, 0);
	obj_to_room(font, ch->in_room);
}
