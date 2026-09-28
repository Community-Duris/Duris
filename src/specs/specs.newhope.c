/* Newhope special procedures. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/handler.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"

int tentacler_death(P_char tentacler, P_char /*ch*/, int cmd, char * /*arg*/)
{
	int obj_load;

	if (!tentacler)
	{
		return FALSE;
	}

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd == CMD_DEATH)
	{
		P_obj tempobj = NULL;
		obj_load = number(0, 4);
		switch (obj_load)
		{
		case 0:
			if (!(tempobj = read_object(89145, VIRTUAL)))
			{
				logit(LOG_DEBUG, "tentacler_death: object failed to load.");
				debug("tentacler_death: object failed to load.");
				return FALSE;
			}
			break;
		case 1:
			if (!(tempobj = read_object(89146, VIRTUAL)))
			{
				logit(LOG_DEBUG, "tentacler_death: object failed to load.");
				debug("tentacler_death: object failed to load.");
				return FALSE;
			}
			break;
		case 2:
			if (!(tempobj = read_object(89147, VIRTUAL)))
			{
				logit(LOG_DEBUG, "tentacler_death: object failed to load.");
				debug("tentacler_death: object failed to load.");
				return FALSE;
			}
			break;
		case 3:
			if (!(tempobj = read_object(89148, VIRTUAL)))
			{
				logit(LOG_DEBUG, "tentacler_death: object failed to load.");
				debug("tentacler_death: object failed to load.");
				return FALSE;
			}
			break;
		case 4:
			if (!(tempobj = read_object(89149, VIRTUAL)))
			{
				logit(LOG_DEBUG, "tentacler_death: object failed to load.");
				debug("tentacler_death: object failed to load.");
				return FALSE;
			}
			break;
		}
		if (!tempobj)
			return FALSE;
		obj_to_room(tempobj, real_room(89227));
		return TRUE;
	}

	return FALSE;
}
