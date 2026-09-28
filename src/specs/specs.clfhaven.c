/* Area-owned special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "world/events.h"
#include "economy/economic_gameplay_authority.h"
#include "economy/tradeskill.h"

extern P_room world;
extern P_obj object_list;

// Artifact types.
#define ARTIFACT_MAJOR 1
#define ARTIFACT_UNIQUE 2
#define ARTIFACT_IOUN 3

int llyren(P_char ch, P_char pl, int cmd, char *arg)
{
	P_obj t_obj, container;
	P_char owner = NULL;
	char buffer[256];
	int arti_type, cost;

	if (cmd != CMD_LIST)
		return FALSE;
	if (economic_gameplay_authority::active())
	{
		send_to_char(
			"Artifact location purchases are unavailable while active accounting is enabled.\r\n",
			pl);
		return TRUE;
	}

	arg = skip_spaces(arg);

	// Uniques are default.
	if (!arg || *arg == '\0' || is_abbrev(arg, "unique"))
	{
		arti_type = ARTIFACT_UNIQUE;
		cost = 50 * 1000;
	}
	else if (is_abbrev(arg, "main") || is_abbrev(arg, "major"))
	{
		arti_type = ARTIFACT_MAJOR;
		cost = 500 * 1000;
	}
	else if (is_abbrev(arg, "ioun"))
	{
		arti_type = ARTIFACT_IOUN;
		cost = 175 * 1000;
	}
	else
	{
		do_say(ch, writable_arg("I'm not quite sure what you meant by that..."), CMD_SAY);
		do_say(ch, writable_arg("What type of artifacts did you want me to show you?"),
		       CMD_SAY);
		return TRUE;
	}

	// Cost 50 p.
	if (!transact(pl, NULL, ch, cost))
		return TRUE;

	// To prevent cheese of killing the mob to get coins back.
	act("$N stores the cash in an alternate dimension.", FALSE, pl, NULL, ch, TO_CHAR);
	GET_PLATINUM(ch) = 0;
	GET_GOLD(ch) = 0;
	GET_SILVER(ch) = 0;
	GET_COPPER(ch) = 0;

	act("$N grins and forces your mind into $S &+Wgl&+Co&+Wbe&n, which scatters it across the ether..",
	    FALSE, pl, NULL, ch, TO_CHAR);
	for (t_obj = object_list; t_obj; t_obj = t_obj->next)
	{
		// Revenants crown won't be shown as it's a rareload
		if (!IS_ARTIFACT(t_obj) || obj_index[t_obj->R_num].virtual_number == 22070)
		{
			continue;
		}
		// Skip non-uniques if arti type is uniques.
		if ((arti_type == ARTIFACT_UNIQUE) && (strstr(t_obj->name, "unique") == NULL))
		{
			continue;
		}
		// Skip uniques and iouns if arti type is major.
		if ((arti_type == ARTIFACT_MAJOR) &&
		    ((strstr(t_obj->name, "unique") != NULL) || IS_IOUN(t_obj)))
		{
			continue;
		}
		// Skip non-iouns if arti type is ioun.
		if ((arti_type == ARTIFACT_IOUN) && !IS_IOUN(t_obj))
		{
			continue;
		}

		if (OBJ_WORN(t_obj))
			owner = t_obj->loc.wearing;
		else if (OBJ_CARRIED(t_obj))
			owner = t_obj->loc.carrying;
		else if (OBJ_ROOM(t_obj))
		{
			snprintf(buffer, 256, "You see %s in %s.\n", t_obj->short_description,
				 world[t_obj->loc.room].name);
			send_to_char(buffer, pl);
			continue;
		}
		else if (OBJ_INSIDE(t_obj) && ((container = t_obj->loc.inside) != NULL))
		{
			while (OBJ_INSIDE(container))
			{
				container = container->loc.inside;
			}
			if (IS_PC_CORPSE(container))
			{
				continue;
			}
			if (OBJ_ROOM(container))
			{
				snprintf(buffer, 256, "You see %s inside %s in %s.\n",
					 OBJ_SHORT(t_obj), OBJ_SHORT(container),
					 world[container->loc.room].name);
				send_to_char(buffer, pl);
				continue;
			}
			else if (OBJ_WORN(container))
			{
				owner = t_obj->loc.wearing;
			}
			else if (OBJ_CARRIED(container))
			{
				owner = t_obj->loc.carrying;
			}
			else
				continue;
		}
		else
			continue;

		if (IS_PC(owner))
			continue;

		snprintf(buffer, sizeof buffer, "You see %s in posession of %s.\n",
			 t_obj->short_description, owner->player.short_descr);
		send_to_char(buffer, pl);
	}

	return TRUE;
}
