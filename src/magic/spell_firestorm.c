#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include <string.h>

static void spell_single_firestorm(int level, P_char ch, char * /*arg*/, int /*type*/,
				   P_char victim, P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"You engulf $N with a red hot &+rfirestorm&N!",
		"$n surrounds you with &+Rsearing flames&N!",
		"$n bathes $N in cleansing &+rflame&N, but $E does not look thankful.",
		"$N is flash-fried, oooohh, crispy critter!",
		"$n kills and cremates you with a single spell, how efficient!",
		"$N is turned to ash by $n's &+rfirestorm&N!",
		0
	};

	dam = dice(level, 10);
	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_FIRESTORM);
	if (NewSaves(victim, SAVING_SPELL, mod))
		dam >>= 1;

	if (IS_PC(ch) && IS_PC(victim))
		dam = dam * get_property("spell.area.damage.to.pc", 0.5);

	dam = dam * get_property("spell.area.damage.factor.fireStorm", 1.000);
	spell_damage(ch, victim, dam, SPLDAM_FIRE, 0, &messages);
}

void spell_firestorm(int level, P_char ch, char * /*arg*/, int type, P_char victim, P_obj /*obj*/)
{
	struct room_affect raf;
	int room = ch->in_room;
	P_char tch;

	send_to_char("You call a &+Rraging&n &+rfirestorm&n to engulf your foes!\n", ch);
	act("$n creates a &+Rraging&n &+rfirestorm&n!", FALSE, ch, 0, 0, TO_VICTROOM);
	zone_spellmessage(ch->in_room, FALSE, "&+YYou feel a blast of &+Rheat!\n",
			  "&+YYou feel a blast of &+Rheat &+Yfrom the %s!\n");
	cast_as_damage_area(ch, spell_single_firestorm, level, victim,
			    get_property("spell.area.minChance.fireStorm", 90),
			    get_property("spell.area.chanceStep.fireStorm", 10));

	memset(&raf, 0, sizeof(raf));
	raf.type = SPELL_FIRESTORM;
	if (type >= 1)
		raf.duration = (int)(2 * PULSE_VIOLENCE); // dragoon dragon priest
	else
		raf.duration = (int)(1.5 * PULSE_VIOLENCE); // everyone else
	affect_to_room(room, &raf);

	for (tch = world[room].people; tch; tch = tch->next_in_room)
		if (IS_AFFECTED5(tch, AFF5_WET))
		{
			send_to_char("The heat of the firestorm dried up your clothes.\n", tch);
			make_dry(tch);
		}
}
