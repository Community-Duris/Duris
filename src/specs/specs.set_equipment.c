/*
 ***************************************************************************
 *  File: specs.set_equipment.c                            Part of Duris   *
 *  Usage: special procedures for equipment sets                           *
 ***************************************************************************
 */

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
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include "world/vnum.obj.h"

extern P_index obj_index;
extern P_room world;
extern int top_of_zone_table;
extern struct zone_data *zone_table;
extern Skill skills[];
extern bool has_skin_spell(P_char);
void bard_dragons(int, P_char, P_char, int);
void event_balance_affects(P_char, P_char, P_obj, void *);
void event_object_proc(P_char, P_char, P_obj, void *);

typedef int (*set_func)(P_char ch, P_obj obj, int count, int cmd, char *arg);

static int master_set_adapter(P_char ch, P_obj obj, int /*count*/, int cmd, char *arg)
{
	return master_set(obj, ch, cmd, arg);
}

struct random_set_wear_off
{
	struct affected_type *af;
	char zone_name[256];
};

void check_zone_spells(P_char ch, P_obj obj, int count, const char *zone_name);

void event_random_set_proc(P_char ch, P_char /*victim*/, P_obj obj, void *data)
{
	struct random_set_wear_off *rdata = (struct random_set_wear_off *)data;
	struct affected_type *afp, *afpp = rdata->af;
	char buffer[256];

	for (afp = ch->affected; afp; afp = afp->next)
	{
		if (afp == afpp)
		{
			checked_snprintf(buffer, 256, "Spirits of %s no longer support you.\n",
					 rdata->zone_name);
			send_to_char(buffer, ch);
			affect_remove(ch, afp);
			check_zone_spells(ch, obj, 0, rdata->zone_name);
		}
	}
}

extern struct zone_random_data
{
	int zone;
	int races[10];
	int proc_spells[3][2];
} zones_random_data[];

#define SETMSG_NONE 0
#define SETMSG_PROTECT 1
#define SETMSG_STRENGTH 2

void apply_zone_spell(P_char ch, int count, const char *zone_name, int zone_index, P_obj obj,
		      int spell)
{
	int message = SETMSG_NONE;
	char buffer[512];

	switch (spell)
	{
	case SPELL_REGENERATION:
	case SPELL_ACCEL_HEALING:
	case SPELL_PACTUM_SERPENTIS:
		if (!affected_by_spell(ch, SPELL_REGENERATION) &&
		    !affected_by_spell(ch, SPELL_PACTUM_SERPENTIS) &&
		    !affected_by_spell(ch, SKILL_REGENERATE) &&
		    !affected_by_spell(ch, SPELL_ACCEL_HEALING))
		{
			(skills[spell].spell_pointer)(MIN(56, count * 10), ch, 0, 0, ch, 0);
			message = SETMSG_PROTECT;
		}
		break;
	case SPELL_STONE_SKIN:
		if (!has_skin_spell(ch) &&
		    obj->timer[0] + get_property("timer.stoneskin.generic", 60) < time(NULL))
		{
			spell_stone_skin(count * 5, ch, 0, 0, ch, 0);
			obj->timer[0] = time(NULL);
			message = SETMSG_PROTECT;
		}
		break;
	case SPELL_ARMOR:
	case SPELL_BARKSKIN:
	case SPELL_THORNSKIN:
	case SPELL_SPIRIT_ARMOR:
	case SPELL_FLESH_ARMOR:
		if (!IS_AFFECTED(ch, AFF_ARMOR))
		{
			(skills[spell].spell_pointer)(MIN(56, count * 10), ch, 0, 0, ch, 0);
			message = SETMSG_PROTECT;
		}
		break;
	case SPELL_HASTE:
		if (!IS_AFFECTED(ch, AFF_HASTE))
		{
			spell_haste(MIN(56, count * 10), ch, 0, 0, ch, 0);
			message = SETMSG_STRENGTH;
		}
		break;
	case SPELL_FIRESHIELD:
		if (!IS_AFFECTED2(ch, AFF2_FIRESHIELD))
		{
			spell_fireshield(MIN(56, count * 10), ch, 0, 0, ch, 0);
			message = SETMSG_STRENGTH;
		}
		break;
	case SPELL_INFERNAL_FURY:
		if (!IS_AFFECTED(ch, AFF_INFERNAL_FURY))
		{
			spell_infernal_fury(MIN(56, count * 10), ch, 0, 0, ch, 0);
			message = SETMSG_STRENGTH;
		}
		break;
	case SPELL_STRENGTH:
	case SPELL_BLESS:
		if (!affected_by_spell(ch, spell))
		{
			(skills[spell].spell_pointer)(MIN(56, count * 10), ch, 0, 0, ch, 0);
			message = SETMSG_STRENGTH;
		}
		break;
	case SPELL_CONJURE_ELEMENTAL:
		if (obj->timer[0] + get_property("timer.conjureElement.generic", 300) < time(NULL))
		{
			(skills[spell].spell_pointer)(MAX(30, count * 10), ch, 0, 0, ch, 0);
			obj->timer[0] = time(NULL);
			message = SETMSG_STRENGTH;
		}
		break;
	case SPELL_INVIGORATE:
		if (obj->timer[0] + get_property("timer.invigorate.generic", 60) < time(NULL) &&
		    GET_VITALITY(ch) < GET_MAX_VITALITY(ch))
		{
			(skills[spell].spell_pointer)(MAX(30, count * 10), ch, 0, 0, ch, 0);
			obj->timer[0] = time(NULL);
			message = SETMSG_STRENGTH;
		}
		break;
	case SPELL_ENDURANCE:
		if (!affected_by_spell(ch, spell) &&
		    !affected_by_spell(ch, SPELL_MIELIKKI_VITALITY))
		{
			(skills[spell].spell_pointer)(MAX(30, count * 10), ch, 0, 0, ch, 0);
			message = SETMSG_STRENGTH;
		}
		break;
	case SONG_DRAGONS:
		if (!affected_by_spell(ch, spell))
		{
			P_char tch = NULL, next = NULL;
			for (tch = world[ch->in_room].people; tch; tch = next)
			{
				next = tch->next_in_room;

				if ((ch != tch) && !grouped(ch, tch))
				{
					continue;
				}

				// Sing the song.
				bard_dragons(MIN(56, count * 10), ch, tch, spell);
				struct affected_type *paf = get_spell_from_char(tch, SONG_DRAGONS);
				if (paf)
				{
					// hack to keep the song_dragons affect on the character
					// for the same duration as the prot spells when proc'ing
					// from an item, otherwise it has a duration of 1 and falls
					// frequently, negating the !fear ability of the song for
					// many group members
					paf->duration = GET_LEVEL(ch) / 3;
					SET_BIT(paf->flags, AFFTYPE_SET_AFFECT | AFFTYPE_NOSAVE);
					paf->context = reinterpret_cast<void *>(zone_index);
				}
			}
		}
		break;
	case SKILL_EPIC_STRENGTH:
	case SKILL_EPIC_POWER:
	case SKILL_EPIC_AGILITY:
	case SKILL_EPIC_INTELLIGENCE:
	case SKILL_EPIC_DEXTERITY:
	case SKILL_EPIC_WISDOM:
	case SKILL_EPIC_CONSTITUTION:
	case SKILL_EPIC_CHARISMA:
	case SKILL_EPIC_LUCK:
		if (!affected_by_skill(ch, spell))
		{
			int level = MIN(56, count * 10);
			struct affected_type af;
			memset(&af, 0, sizeof(af));
			af.type = spell;
			af.duration = level / 10;
			af.location = APPLY_SKILL_GRANT;
			af.loc2 = spell;
			af.modifier = level;
			affect_to_char(ch, &af);
			message = SETMSG_STRENGTH;
		}
		break;
	default:
		if (!affected_by_spell(ch, spell))
		{
			(skills[spell].spell_pointer)(MIN(56, count * 10), ch, 0, 0, ch, 0);
			message = SETMSG_PROTECT;
		}
		break;
	}

	if (message == SETMSG_PROTECT)
	{
		snprintf(buffer, 512, "The spirits of %s grant you their protection.\n", zone_name);
		send_to_char(buffer, ch);
	}
	else if (message == SETMSG_STRENGTH)
	{
		snprintf(buffer, 512, "The spirits of %s grant you their strength.\n", zone_name);
		send_to_char(buffer, ch);
	}

	if (message != SETMSG_NONE)
	{
		// mark all affects matching the skill as from a set and nosave
		struct affected_type *paf = NULL;
		for (paf = ch->affected; paf; paf = paf->next)
		{
			if (paf->type == spell)
			{
				SET_BIT(paf->flags, AFFTYPE_SET_AFFECT | AFFTYPE_NOSAVE);
				paf->context = reinterpret_cast<void *>(zone_index);
			}
		}
	}
}

#undef SETMSG_NONE
#undef SETMSG_PROTECT
#undef SETMSG_STRENGTH

// Random zone eq spellups (depends on # items worn == count).
void check_zone_spells(P_char ch, P_obj obj, int count, const char *zone_name)
{
	int zone_room, zone_idx = -1;
	int i;

	// Find the matching zone for the random eq.
	for (i = 0; i <= top_of_zone_table; i++)
	{
		if (strstr(zone_name, zone_table[i].name))
			break;
	}
	// If zone not found, return.
	if (i > top_of_zone_table)
	{
		return;
	}

	// Find the appropriate random_data for the zone.
	// This calculates the starting # for the zone as in the DE.
	zone_room = world[zone_table[i].real_bottom].number / 100;
	// Walk the list of random eq proc'ing zones.
	for (i = 0; zones_random_data[i].zone; i++)
	{
		if (zones_random_data[i].zone == zone_room)
		{
			zone_idx = i;
			break;
		}
	}
	// If random_data not found, return.
	if (zone_idx < 0)
	{
		return;
	}

	bool spellsRemoved = false;

	// For the three possible spellups,
	for (i = 0; i < 3; i++)
	{
		if (zones_random_data[zone_idx].proc_spells[i][0])
		{
			struct affected_type *paf;
			// If the required num of eq is met for spell
			if (zones_random_data[zone_idx].proc_spells[i][0] <= count)
			{
				// cast the spell on ch.
				apply_zone_spell(ch, count, zone_name, zone_idx, obj,
						 zones_random_data[zone_idx].proc_spells[i][1]);
			}
			else if ((paf = get_spell_from_char(
					  ch, zones_random_data[zone_idx].proc_spells[i][1],
					  reinterpret_cast<void *>(zone_idx),
					  AFFTYPE_SET_AFFECT)) != NULL)
			{
				// remove the spell from the character
				wear_off_message(ch, paf);
				affect_from_char(ch, zones_random_data[zone_idx].proc_spells[i][1]);
				spellsRemoved = true;
			}
		}
	}

	if (spellsRemoved)
	{
		char buffer[512];
		snprintf(buffer, 512, "The spirits of %s remove their blessings.\n", zone_name);
		send_to_char(buffer, ch);
	}
}

int random_set(P_char ch, P_obj obj, int count, int cmd, char * /*arg*/)
{
	struct affected_type af, *afp;
	char *zone_name, buffer[256];
	struct random_set_wear_off rdata;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd != CMD_PERIODIC || !IS_ALIVE(ch))
	{
		return FALSE;
	}

	zone_name = strstr(obj->short_description, " &+rfrom") + 9;
	int context;
	// Find the matching zone for the random eq.
	for (context = 0; context <= top_of_zone_table; context++)
	{
		if (strstr(zone_name, zone_table[context].name))
			break;
	}

	// Why do we return true here?
	if (count < 2)
	{
		return FALSE;
	}

	// Look for a random item proc..
	afp = get_spell_from_char(ch, TAG_SETPROC, reinterpret_cast<void *>(context));
	if (!afp)
	{
		memset(&af, 0, sizeof(af));
		af.type = TAG_SETPROC;
		af.flags = AFFTYPE_NOSAVE | AFFTYPE_NOSHOW | AFFTYPE_NODISPEL;
		af.location = APPLY_HIT;
		// This should be indefinite: Only changes upon eq removal/wear new eq.
		af.duration = -1;
		af.context = reinterpret_cast<void *>(context);
		afp = affect_to_char(ch, &af);
	}

	// This right here creates the argument between sets of two different zones being on one char.
	disarm_char_nevents(ch, event_random_set_proc);
	rdata.af = afp;
	strcpy(rdata.zone_name, zone_name);
	// Event to remove rdata.af from ch. PULSE_MOBILE + 5 = 35 pulses = 9 sec??
	add_event(event_random_set_proc, PULSE_MOBILE + 5, ch, 0, 0, 0, &rdata, sizeof(rdata));

	check_zone_spells(ch, obj, count, zone_name);

	if (afp->modifier > (count - 2) * 5)
	{
		snprintf(buffer, 256, "You feel some of the %s's spirits attention leave you.\n",
			 zone_name);
		send_to_char(buffer, ch);
	}
	else if (afp->modifier < (count - 2) * 5)
	{
		snprintf(buffer, 256, "You feel invigorated as the spirits of %s bless you.\n",
			 zone_name);
		send_to_char(buffer, ch);
	}
	else
	{
		return TRUE;
	}

	afp->modifier = (count - 2) * 5;
	add_event(event_balance_affects, 0, ch, 0, 0, 0, 0, 0);

	return TRUE;
}

struct set_data
{
	set_func func;
	int items[MAX_WEAR];
} sets[] = { { master_set_adapter, { 22063, 22237, 22621, 45530, 45531, 75857, 82545, 82559 } },
	     { random_set, { VOBJ_RANDOM_ARMOR, VOBJ_RANDOM_WEAPON } },
	     { nullptr, { 0 } } };

int set_proc(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_obj tobj, included[MAX_WEAR], cobj = obj;
	int s, i = 0, j, count = 0;
	unsigned int flag = (cmd != CMD_PERIODIC) ? ITEM2_NOPROC : ITEM2_NOTIMER;
	char *c = NULL, *d;

	// Look through the sets for the right vnum.
	for (s = 0; sets[s].func; s++)
	{
		for (i = 0; sets[s].items[i]; i++)
		{
			// We found a set that obj is in.
			if (sets[s].items[i] == obj_index[obj->R_num].virtual_number)
			{
				break;
			}
		}
		if (sets[s].items[i])
		{
			break;
		}
	}

	// If we didn't find a set that obj was in
	if (sets[s].items[i] == 0)
	{
		return FALSE;
	}

	// Set the periodic timer if appropriate.
	if (cmd == CMD_SET_PERIODIC)
	{
		return sets[s].func(ch, obj, count, cmd, arg);
	}

	// Check random_set objs for the "<item> &+rfrom <zone>".
	if (sets[s].func == random_set)
	{
		c = strstr(obj->short_description, " &+rfrom ");
		if (!c)
		{
			// If not, remove the periodic timer.
			disarm_obj_nevents(obj, event_object_proc);
			return FALSE;
		}
	}

	// No proc if not worn.
	if (!OBJ_WORN(obj))
	{
		return FALSE;
	}

	if (IS_SET(obj->extra2_flags, flag))
	{
		REMOVE_BIT(obj->extra2_flags, flag);
		return FALSE;
	}

	if (cmd == CMD_PERIODIC)
	{
		ch = obj->loc.wearing;
	}

	memset(included, 0, sizeof(included));

	// Walk through worn equipment.
	for (i = 0; i < MAX_WEAR; i++)
	{
		tobj = ch->equipment[i];

		// Skip belted & back?  Shouldn't we allow prime belt slot? and skip empty slots.
		//   Allowing prime belt slot and on back.. 8/21/2014
		if (i == WEAR_ATTACH_BELT_3 || i == WEAR_ATTACH_BELT_2 || !tobj)
		//      || i == WEAR_ATTACH_BELT_1 || i == WEAR_BACK || !tobj )
		{
			continue;
		}
		// Walk through the current set..
		for (j = 0; sets[s].items[j]; j++)
		{
			// If the set vnum matches
			if (sets[s].items[j] == obj_index[tobj->R_num].virtual_number)
			{
				break;
			}
		}
		// If we didn't find a match, continue.
		if (!sets[s].items[j])
		{
			continue;
		}
		// If we have a random item..
		if (sets[s].func == random_set)
		{
			// If !zone or zones don't match.
			d = strstr(tobj->short_description, " &+rfrom ");
			if (!c || !d || strcmp(c, d) != 0)
			{
				continue;
			}
		}
		else
		{
			// Walk the included list.
			for (j = 0; included[j]; j++)
			{
				if (included[j]->R_num == tobj->R_num)
				{
					break;
				}
			}
			// Skip to next item if it's a duplicate vnum.
			if (included[j])
			{
				continue;
			}
		}
		// Set the no proc flag.
		if (tobj != obj)
		{
			SET_BIT(tobj->extra2_flags, flag);
		}

		// Add tobj to the end of the list of included objects.
		included[j] = tobj;
		// Save the object with the newest timer.
		if (cobj->timer[0] < tobj->timer[0])
		{
			cobj = tobj;
		}
		// Increment the counter of items in set.
		count++;
	}

	// If the set's function returns true..
	if (sets[s].func(ch, cobj, count, cmd, arg))
	{
		// If we're not periodic..
		if (cmd != CMD_PERIODIC)
		{
			// Remove the no proc flag from all items in set.
			for (i = 0; included[i]; i++)
			{
				REMOVE_BIT(included[i]->extra2_flags, ITEM2_NOPROC);
			}
		}
		return TRUE;
	}

	return FALSE;
}
