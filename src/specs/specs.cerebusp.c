/* Special procedures for Cerebusp. */

#include <string.h>
#include <time.h>

#include "core/prototypes.h"
#include "item/objmisc.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"

extern P_index obj_index;
extern const struct racial_data_type racial_data[];

void event_revenant_crown(P_char ch, P_char /*victim*/, P_obj obj, void * /*data*/)
{
	struct affected_type *af;
	P_obj armor = ch->equipment[WEAR_HEAD];

	if (GET_RACE(ch) != RACE_REVENANT)
	{
		act("Your skin blisters and boils start to form!&n", FALSE, ch, obj, 0, TO_CHAR);
		wizlog(57, " Reverant crown worn by %s begins to melt due race check conflict!",
		       GET_NAME(ch));
		GET_HIT(ch) >>= 1;
		CharWait(ch, 2 * WAIT_SEC);
	}
	else if ((af = get_spell_from_char(ch, TAG_RACE_CHANGE)) == NULL)
	{
		send_to_char(
			"&+WPossible serious screwup in the revenant helm proc! Tell a coder as once!&n\r\n",
			ch);
		wizlog(57,
		       "Char %s found with racechange event but without racechange affect! revenant proc",
		       GET_NAME(ch));
		return;
	}
	else if (armor != NULL && obj_index[armor->R_num].virtual_number == REVENANT_CROWN_VNUM)
	{
		add_event(event_revenant_crown, (int)(0.5 * PULSE_VIOLENCE), ch, 0, 0, 0, 0, 0);
		return;
	}
	else
	{
		ch->player.race = af->modifier;
		//    GET_AGE(ch) = racial_data[(int) GET_RACE(ch)].base_age*2;
		// Set birthdate + base_age + 5 years.
		ch->player.time.birth = time(NULL);
		// Add base_age to birthdate + base_age + 5 years.
		ch->player.time.birth -= (racial_data[GET_RACE(ch)].base_age) * SECS_PER_MUD_YEAR;
		affect_remove(ch, af);
		send_to_char(
			"The curse of the dark powers fade and your soul restores the body.\r\n",
			ch);
		int k = 0;
		P_obj temp_obj;
		for (k = 0; k < MAX_WEAR; k++)
		{
			temp_obj = ch->equipment[k];
			if (temp_obj)
			{
				if (obj_index[temp_obj->R_num].func.obj != NULL)
					invoke_object_special(temp_obj, ch, CMD_REMOVE,
							      (char *)"all");
				obj_to_char(unequip_char(ch, k), ch);
			}
		}
		send_to_char("Brr, you suddenly feel very naked.\r\n", ch);

		return;
	}
}

int revenant_helm(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	int k = 0;
	P_obj temp_obj;
	struct affected_type af;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd != CMD_PERIODIC || !OBJ_WORN(obj))
	{
		return FALSE;
	}
	// Need to do this in 2 steps, 'cause CMD_PERIODIC doesn't assign ch. *sigh*
	//   We could put an !(ch = obj->loc.wearing) .. but that assignment might not come before
	//   the IS_ALIVE etc checks.
	ch = obj->loc.wearing;
	if (!IS_ALIVE(ch) || IS_NPC(ch) || affected_by_spell(ch, TAG_RACE_CHANGE))
	{
		return FALSE;
	}

	// Remove all eq..
	for (k = 0; k < MAX_WEAR; k++)
	{
		temp_obj = ch->equipment[k];
		if (temp_obj && (obj != temp_obj))
		{
			if (obj_index[temp_obj->R_num].func.obj != NULL)
			{
				// Call the objects remove proc if there might be one.
				invoke_object_special(temp_obj, ch, CMD_REMOVE, (char *)"all");
			}
			obj_to_char(unequip_char(ch, k), ch);
			if (!IS_ALIVE(ch))
			{
				return FALSE;
			}
		}
	}

	CharWait(ch, 5 * WAIT_SEC);
	send_to_char("Brr, you suddenly feel _almost_ naked.\r\n\n", ch);

	memset(&af, 0, sizeof(af));
	af.type = TAG_RACE_CHANGE;
	af.flags = AFFTYPE_NOSAVE | AFFTYPE_NODISPEL | AFFTYPE_NOAPPLY;
	af.duration = -1;
	af.modifier = GET_RACE(ch);
	affect_to_char(ch, &af);
	add_event(event_revenant_crown, (int)(0.5 * PULSE_VIOLENCE), ch, 0, 0, 0, 0, 0);

	act("&+LThe figure of $n &+Lgrows darker and darker as $e absorbs all surrounding\n"
	    "&+Wlight&+L. A sphere of absolute darkness spreads out around $m expanding\n"
	    "&+Lrapidly outwards, and engulfing $n&+L. Moments later a loud boom echoes\n"
	    "&+Lloudly from the sphere and where $n once stood is now a creature\n"
	    "&+Lof great &+Bpower &+Land &N&+revil&+L...",
	    FALSE, ch, obj, 0, TO_ROOM);
	act("Your $q &+Lhums loudly as it draws &+Cenergy &+Lfrom\n"
	    "&+Lthe dark powers. You scream in agony as it melts the flesh of your\n"
	    "&+Lbody transforming you into a creature of &+Bcold &+Land &N&+wdeath&+L!",
	    FALSE, ch, obj, 0, TO_CHAR);

	ch->player.race = RACE_REVENANT;
	// Set birthdate + base_age + 5 years.
	ch->player.time.birth = time(NULL);
	// Add base_age to birthdate + base_age + 5 years.
	ch->player.time.birth -= (racial_data[RACE_REVENANT].base_age) * SECS_PER_MUD_YEAR;
	return TRUE;
}
