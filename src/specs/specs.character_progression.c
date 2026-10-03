/* Character progression object procedures. */

#include <stdio.h>
#include <string.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "world/handler.h"
#include "classes/specializations.h"
#include "magic/spells.h"
#include "world/specs.prototypes.h"

extern P_room world;
extern Skill skills[];

int skill_beacon(P_obj obj, P_char ch, int cmd, char *argument)
{
	int skill = obj->value[0];
	int requirement = obj->value[1];
	int cap = obj->value[2];
	int l, t, maxlearn, i;
	bool active = IS_SET(obj->extra2_flags, ITEM2_MAGIC);
	char buf[1024];

	static const struct
	{
		int room;
		int skills[5];
	} beacon_loads[] = { { 26860, // neg
			       { SKILL_DODGE, SKILL_MEDITATE, SKILL_SPELL_KNOWLEDGE_MAGICAL,
				 SKILL_2H_BLUDGEON, SKILL_SHIELD_BLOCK } },
			     { 25087, // brass
			       { SKILL_1H_SLASHING, SKILL_UNARMED_DAMAGE,
				 SKILL_SPELL_KNOWLEDGE_CLERICAL, SKILL_PARRY, SKILL_RESCUE } },
			     { 81094, // ceothia
			       { SKILL_QUICK_CHANT, SKILL_BASH, SKILL_1H_PIERCING,
				 SKILL_2H_SLASHING, SKILL_HIDE } },
			     { 25922, // baha
			       { SKILL_1H_FLAYING, SKILL_TACKLE, SKILL_GAZE, SKILL_DOUBLE_ATTACK,
				 SKILL_SPRINGLEAP } },
			     { 45718, // cel
			       { SKILL_SWEEPING_THRUST, SKILL_1H_BLUDGEON, SKILL_BACKSTAB,
				 SKILL_HEADBUTT, SKILL_TRIP } },
			     { 34804, // 4horsemen
			       { SKILL_RIPOSTE, SKILL_FLANK, SKILL_RAGE, SKILL_MARTIAL_ARTS,
				 SKILL_ARCANE_RIPOSTE } },
			     {} };

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd == CMD_PERIODIC)
	{
		if (!number(0, 5) && active)
		{
			act("You hear a cracking noise as twisting &+Bthreads of &-Lelectric&-l&+B discharges&n crawl up $p.",
			    FALSE, 0, obj, 0, TO_ROOM);
			for (i = 0; beacon_loads[i].room; i++)
			{
				if (world[obj->loc.room].number == beacon_loads[i].room)
				{
					obj->value[0] = beacon_loads[i].skills[number(0, 4)];
					break;
				}
			}
		}
		else if (!number(0, 10) && !active)
		{
			act("$p flashes brightly then blurs and with a loud rumble falls apart leaving nothing but a pile of debris.",
			    FALSE, 0, obj, 0, TO_ROOM);
			extract_obj(obj, TRUE); // Not an arti, but 'in game.'
		}
		return FALSE;
	}

	if (!ch || IS_NPC(ch) || (cmd != CMD_TOUCH && cmd != CMD_EXAMINE) || !argument)
		return FALSE;

	one_argument(argument, buf);

	if (obj != get_obj_in_list_vis(ch, buf, world[ch->in_room].contents))
		return FALSE;

	l = ch->only.pc->skills[skill].learned;
	t = ch->only.pc->skills[skill].taught;
	maxlearn = MAX(SKILL_DATA_ALL(ch, skill).maxlearn[0],
		       SKILL_DATA_ALL(ch, skill).maxlearn[ch->player.spec]);

	if (cmd == CMD_TOUCH)
	{
		if (l < t || (requirement && t < requirement))
		{
			act("As you touch $p you feel the power surge under its surface but you can sense you"
			    " are not ready yet to extend your capabilities.",
			    FALSE, ch, obj, 0, TO_CHAR);
			act("$n touches $p but nothing seems to happen.", FALSE, ch, obj, 0,
			    TO_ROOM);
			return TRUE;
		}

		if (t == maxlearn || (cap && cap <= t))
		{
			act("As you touch $p you feel the power surge under its surface but you can sense it is"
			    " not enough to extend your capabilities any further.",
			    FALSE, ch, obj, 0, TO_CHAR);
			act("$n touches $p but nothing seems to happen.", FALSE, ch, obj, 0,
			    TO_ROOM);
			return TRUE;
		}

		ch->only.pc->skills[skill].taught =
			MIN(ch->only.pc->skills[skill].taught + 2, maxlearn);
		snprintf(buf, 1024,
			 "As you reach towards $p, suddenly a &+Bcracking bolt&n\n"
			 "jumps from it binding you for a second in an immobilizing\n"
			 "grip. In a sudden flash of understanding you feel you can\n"
			 "now progress further in &+W%s&n!",
			 skills[skill].name);
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		act("Upon $n's touch a &+B&-Lcracking bolt&n shoots out from $p "
		    "binding $m for a second in an immobilizing grip.",
		    FALSE, ch, obj, 0, TO_ROOM);
		REMOVE_BIT(obj->extra2_flags, ITEM2_MAGIC);
		// Maybe add a cooldown timer here instead of removing the magic flag?
		return TRUE;
	}
	else if (cmd == CMD_EXAMINE)
	{
		if (IS_TRUSTED(ch))
		{
			snprintf(
				buf, sizeof buf,
				"This is a skill beacon object. The following values are used to configure it:\n"
				"  &+Wval0&n   skill number\n"
				"  &+Wval1&n   minimal skill level to use the beacon\n"
				"  &+Wval2&n   maximal skill level beacon will grant");
			if (skill)
				checked_snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
						 "\n$p is %sactive and grants skill &+W%s&n.",
						 active ? "" : "in", skills[skill].name);
			if (requirement)
				checked_snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
						 "\nrequired skill level is &+W%d&n", requirement);
			if (cap)
				checked_snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
						 "\nit will not raise skill above &+W%d&n", cap);
		}
		else if (GET_C_INT(ch) > number(50, 150))
			snprintf(
				buf, sizeof buf,
				"$p is a monolithic block of an unidentified metal. There are some runes drawn on"
				" it which you decipher as referring to the art of &+W%s&n.",
				skills[skill].name);
		else
			snprintf(
				buf, sizeof buf,
				"$p is a monolithic block of an unidentified metal. There are some runes drawn on"
				" it which you can not decipher at all.");
		act(buf, FALSE, ch, obj, 0, TO_CHAR);
		return TRUE;
	}

	return FALSE;
}

int unspec_altar(P_obj obj, P_char ch, int cmd, char *arg)
{
	if (cmd != CMD_PRAY || !IS_ALIVE(ch) || !IS_PC(ch))
		return FALSE;

	if (!strstr(arg, "altar"))
		return FALSE;

	/*
	    if(GET_SPEC(ch, CLASS_SORCERER, SPEC_WIZARD))
	      {
	ch->only.pc->skills[SKILL_SPELL_PENETRATION].taught = 0;
	ch->only.pc->skills[SKILL_SPELL_PENETRATION].learned = 0;
	do_save_silent(ch, 1); // racial skills require a save.
	       }*/

	unspecialize(ch, obj);
	return TRUE;
}
