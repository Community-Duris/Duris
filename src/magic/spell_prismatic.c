#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "core/utils.h"
#include "core/defines.h"
#include "magic/spells.h"
#include "combat/damage.h"
#include <stdio.h>
#include <string.h>

extern P_room world;
#define RAY_YELLOW BIT_1
#define RAY_ORANGE BIT_2
#define RAY_VIOLET BIT_3
#define RAY_GREEN BIT_4
#define RAY_BLUE BIT_5
#define RAY_INDIGO BIT_6
#define RAY_AZURE BIT_7
#define RAY_RED BIT_8
#define RAY_COUNT 8

/* size is the caller's real buffer size; this used to format with
   MAX_STRING_LENGTH into 512-byte buffers. */
static void prepare_ray_messages(const char *color_string, char *ch_buffer, char *vict_buffer,
				 char *room_buffer, size_t size)
{
	snprintf(ch_buffer, size, "You send a %s shaft of light streaking towards $N!",
		 color_string);
	snprintf(vict_buffer, size, "$n sends a %s shaft of light streaking towards YOU!",
		 color_string);
	snprintf(room_buffer, size, "$n sends a %s shaft of light streaking towards $N!",
		 color_string);
}

static void show_ray_messages(const char *color_string, P_char ch, P_char victim)
{
	char buffer[512];

	snprintf(buffer, 512, "You send a %s shaft of light streaking towards $N!", color_string);
	act(buffer, FALSE, ch, 0, victim, TO_CHAR);
	snprintf(buffer, 512, "$n sends a %s shaft of light streaking towards YOU!", color_string);
	act(buffer, FALSE, ch, 0, victim, TO_VICT);
	snprintf(buffer, 512, "$n sends a %s shaft of light streaking towards $N!", color_string);
	act(buffer, FALSE, ch, 0, victim, TO_NOTVICT);
}

static void spell_single_prismatic_ray(int level, P_char ch, char *arg, int /*type*/, P_char victim,
				       P_obj /*obj*/)
{
	int dam, ray_type;
	char char_message[512], victim_message[512], room_message[512];
	struct damage_messages messages = {
		char_message,
		victim_message,
		room_message,
		"Your &+rmu&+Rlt&+Yi&+gco&+bl&+Bo&+Mr&+med&n rays of light shatter $N's existence into a million pieces.",
		"&+rMu&+Rlt&+Yi&+gco&+bl&+Bo&+Mr&+med&n rays of light shatter your existence into a million pieces.",
		"&+rMu&+Rlt&+Yi&+gco&+bl&+Bo&+Mr&+med&n rays of light shatter $N's existence into a million pieces.",
		0
	};

	ray_type = 0;
	if (arg)
		memcpy(&ray_type, arg, sizeof(ray_type));

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_PRISMATIC_RAY);

	switch (ray_type)
	{
	case 0:
		prepare_ray_messages("&+rsh&+Ri&+Ym&n&+gm&+be&+Br&+Mi&n&+mng&+L", char_message,
				     victim_message, room_message, sizeof char_message);
		spell_damage(ch, victim, dice(level, 9), SPLDAM_GENERIC, 0, &messages);
		break;
	case RAY_RED:
		dam = 400 + dice(level, 2);
		prepare_ray_messages("&+rred&+w", char_message, victim_message, room_message,
				     sizeof char_message);
		if (NewSaves(victim, SAVING_SPELL, mod))
			dam >>= 1;
		spell_damage(ch, victim, dam, SPLDAM_FIRE, 0, &messages);
		break;
	case RAY_ORANGE:
		dam = 275 + dice(level, 2);
		prepare_ray_messages("&+Rorange&n", char_message, victim_message, room_message,
				     sizeof char_message);
		if (NewSaves(victim, SAVING_SPELL, mod))
			dam >>= 1;
		spell_damage(ch, victim, dam, SPLDAM_FIRE, 0, &messages);
		break;
	case RAY_BLUE:
		dam = 150 + dice(level, 2);
		prepare_ray_messages("&+Bblue&n", char_message, victim_message, room_message,
				     sizeof char_message);
		if (NewSaves(victim, SAVING_SPELL, mod))
			dam >>= 1;
		spell_damage(ch, victim, dam, SPLDAM_COLD, 0, &messages);
		break;
	case RAY_YELLOW:
		show_ray_messages("&+Yyellow&n", ch, victim);
		spell_minor_paralysis(level, ch, NULL, 0, victim, NULL);
		break;
	case RAY_INDIGO:
		/*show_ray_messages("&+bindigo&n", ch, victim);
			spell_feeblemind(level, ch, NULL, 0, victim, NULL);
			break; -This is crashing us and I cant figure out why - Drannak 3/19/14*/
		show_ray_messages("&+Yyellow&n", ch, victim);
		spell_minor_paralysis(level, ch, NULL, 0, victim, NULL);
		break;
	case RAY_GREEN:
		show_ray_messages("&+Ggreen&n", ch, victim);
		spell_poison(level, ch, 0, 0, victim, NULL);
		break;
	case RAY_VIOLET:
		show_ray_messages("&+mviolet&n", ch, victim);
		spell_dispel_magic(level, ch, 0, SPELL_TYPE_SPELL, victim, NULL);
		break;
	case RAY_AZURE:
		show_ray_messages("&+Bazure&n", ch, victim);
		spell_blindness(level, ch, 0, 0, victim, NULL);
		break;
	}
}

void spell_prismatic_ray(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			 P_obj /*obj*/)
{
	int rays, ray_type, room, i = 0;
	uint ray_flag;

	if (!IS_ALIVE(ch) || (room = ch->in_room) == NOWHERE)
	{
		return;
	}

	spell_single_prismatic_ray(level, ch, 0, 0, victim, 0);

	for (rays = 2, ray_flag = 0; rays;)
	{
		if (!is_char_in_room(victim, room) || !is_char_in_room(ch, room))
			break;
		ray_type = 1 << number(0, RAY_COUNT - 1);
		if (ray_flag & ray_type)
			continue;
		rays--;
		ray_flag |= ray_type;
		spell_single_prismatic_ray(level, ch, (char *)&ray_type, 0, victim, 0);
		i++;
		if (GET_STAT(victim) == STAT_DEAD)
			break;
	}
}

void spell_color_spray(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		       P_obj /*obj*/)
{
	int ray_type;
	P_char tch, next;

	send_to_char("You send beams of &+Gcol&+Bor spr&+Ray&n from your hands.\n", ch);
	act("$n sends beams of &+Gcol&+Bor spr&+Ray&n from $s hands!", FALSE, ch, 0, 0,
	    TO_VICTROOM);

	for (tch = world[ch->in_room].people; tch; tch = next)
	{
		next = tch->next_in_room;
		if (should_area_hit(ch, tch))
		{
			ray_type = 1 << number(0, 3);
			spell_single_prismatic_ray(level, ch, (char *)&ray_type, 0, tch, 0);
		}
	}
}

void spell_prismatic_spray(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
			   P_obj /*obj*/)
{
	P_char tch, next;
	int room, rays, ray_type;

	send_to_char(
		"You send a &+Rr&+Ba&+Yi&+Gn&+Cb&n&+co&+bw&n of prismatic spray from your hands.\n",
		ch);
	act("$n sends a &+Rr&+Ba&+Yi&+Gn&+Cb&n&+co&+bw&n of prismatic spray from $s hands!", FALSE,
	    ch, 0, 0, TO_VICTROOM);

	room = ch->in_room;

	for (tch = world[room].people; tch; tch = next)
	{
		next = tch->next_in_room;
		if (should_area_hit(ch, tch))
		{
			for (rays = number(1, 2); rays; rays--)
			{
				ray_type = 1 << number(0, RAY_COUNT - 1);
				spell_single_prismatic_ray(level, ch, (char *)&ray_type, 0, tch, 0);
				if (!is_char_in_room(tch, room) || (GET_STAT(tch) == STAT_DEAD))
					break;
			}
		}
	}

	zone_spellmessage(
		room, TRUE,
		"&+CC&+co&+Cl&+co&+Cr&+cf&+Cu&+cl&N &+Crays of &+Wlight &+Cstreak throughout the sky!&n\n\r",
		"&+CC&+co&+Cl&+co&+Cr&+cf&+Cu&+cl&N &+Crays of &+Wlight &+Cstreak throughout the sky to the %s!&n\n\r");
}

#undef RAY_RED
#undef RAY_BLUE
#undef RAY_GREEN
#undef RAY_YELLOW
#undef RAY_ORANGE
#undef RAY_INDIGO
#undef RAY_AZURE
#undef RAY_VIOLET
