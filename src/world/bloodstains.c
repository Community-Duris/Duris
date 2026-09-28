/* Creates ephemeral room bloodstains used by combat, tracking, and world events. */
#include <stdio.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "world/bloodstains.h"
#include "world/db.h"
#include "world/map.h"
#include "world/vnum.obj.h"

extern P_room world;

void make_bloodstain(P_char ch)
{
	P_obj blood, obj, next_obj;
	char buf[MAX_STRING_LENGTH];
	int msgnum;
	const char *long_desc[] = { "&+rFresh blood splatters cover the area.&n",
				    "&+rA few drops of fresh blood are scattered around the area.&n",
				    "&+rPuddles of fresh blood cover the ground.&n",
				    "&+rFresh blood covers everything in the area.&n" };

	if (!HAS_FOOTING(ch))
		return;

	if (GET_OPPONENT(ch) && (IS_UNDEADRACE(GET_OPPONENT(ch)) || IS_ANGEL(GET_OPPONENT(ch))))
		return;

	if (IS_UNDEADRACE(ch) || IS_ANGEL(ch))
		return;

	if (world[ch->in_room].contents)
	{
		for (obj = world[ch->in_room].contents; obj; obj = next_obj)
		{
			next_obj = obj->next_content;
			if (obj->R_num == real_object(VOBJ_BLOOD))
			{
				obj_from_room(obj);
				extract_obj(obj);
			}
		}
	}

	blood = read_object(4, VIRTUAL);
	if (!blood)
		return;

	blood->str_mask = (STRUNG_DESC1);

	msgnum = number(0, 3);
	blood->value[0] = msgnum;
	blood->value[1] = BLOOD_FRESH;
	snprintf(buf, MAX_STRING_LENGTH, "%s", long_desc[msgnum]);
	blood->description = str_dup(buf);

	// 15 minutes, changes to regular blood at 3 minutes and dry blood at 7 minutes.
	// Becomes NOSHOW at 90 seconds.
	set_obj_affected(blood, 3600, TAG_OBJ_DECAY, 0);

	if (ch->in_room == NOWHERE)
	{
		if (real_room(ch->specials.was_in_room) != NOWHERE)
			obj_to_room(blood, real_room(ch->specials.was_in_room));
		else
		{
			extract_obj(blood);
			blood = NULL;
		}
	}
	else
	{
		obj_to_room(blood, ch->in_room);
	}
}
