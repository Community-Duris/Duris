/* Khildarak equipment procedures. */

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

extern P_room world;

int khildarak_warhammer(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!obj)
	{
		wizlog(56, "khildarak_warhammer: &+WNo obj in proc.&n");
		return FALSE;
	}

	if (!OBJ_WORN(obj) || !(ch = obj->loc.wearing) || cmd != CMD_PERIODIC)
	{
		return FALSE;
	}

	// 1/30 chance for spellup.
	if (!number(0, 29))
	{
		act("&+LThe strength of $p&+L infuses you!", FALSE, ch, obj, 0, TO_CHAR);

		switch (number(0, 6))
		{
		case 0:
			spell_barkskin(40, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
			break;
		case 1:
			spell_hawkvision(40, ch, 0, 0, ch, 0);
			break;
		case 2:
			spell_bless(40, ch, 0, 0, ch, 0);
			break;
		case 3:
			spell_bearstrength(40, ch, 0, 0, ch, 0);
			break;
		case 4:
			spell_combat_mind(32, ch, 0, 0, ch, 0);
			break;
		case 5:
			spell_spirit_armor(40, ch, 0, 0, ch, 0);
			break;
		case 6:
			spell_vigorize_serious(40, ch, 0, 0, ch, 0);
			break;
		}
		return FALSE;
	}
	return FALSE;
}
