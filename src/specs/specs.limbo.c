/* Special procedures for Limbo. */

#include <stdio.h>
#include <time.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include "world/specs.prototypes.h"

// Mossi Modification:   Moving DECAY Procs

int blood_stains(P_obj obj, P_char /*ch*/, int cmd, char * /*argument*/)
{
	char buf[MAX_STRING_LENGTH];
	const char *long_desc_reg[] = { "&+rBlood splatters cover the area.&n",
					"&+rA few drops of blood are scattered around the area.&n",
					"&+rPuddles of blood cover the ground.&n",
					"&+rBlood covers everything in the area.&n" };
	const char *long_desc_dry[] = {
		"&+rDried blood splatters cover the area.&n",
		"&+rA few drops of dry blood are scattered around the area.&n",
		"&+rPuddles of crusty blood cover the ground.&n",
		"&+rCrusty blood covers everything in the area.&n"
	};

	// Set the timer when obj is loaded
	if (cmd == CMD_SET_PERIODIC)
	{
		obj->timer[0] = time(NULL);
		return TRUE;
	}

	// We don't show a decay message.
	if (cmd == CMD_DECAY)
	{
		return TRUE;
	}

	if (cmd == CMD_PERIODIC && obj->value[1] < BLOOD_DRY)
	{
		// Change it up after 90 seconds
		if (!IS_SET(obj->extra_flags, ITEM_NOSHOW) && (obj->timer[0] < (time(NULL) - 90)))
		{
			SET_BIT(obj->extra_flags, ITEM_NOSHOW);
			return TRUE;
		}

		// Change it up after 3 minutes
		if ((obj->value[1] == BLOOD_FRESH) && (obj->timer[0] < (time(NULL) - 180)))
		{
			snprintf(buf, MAX_STRING_LENGTH, "%s", long_desc_reg[obj->value[0]]);
			obj->description = str_dup(buf);
			obj->value[1] = BLOOD_REG;
			return TRUE;
		}

		// Change it up after 7 minutes
		if ((obj->value[1] == BLOOD_REG) && (obj->timer[0] < (time(NULL) - 420)))
		{
			snprintf(buf, MAX_STRING_LENGTH, "%s", long_desc_dry[obj->value[0]]);
			obj->description = str_dup(buf);
			obj->value[1] = BLOOD_DRY;
			return TRUE;
		}
	}
	return FALSE;
}
