#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "magic/spells.h"

static void spell_single_icestorm(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
				  P_obj /*obj*/)
{
	struct damage_messages messages = { "You crush $N with your &+Cstorm of ice.",
					    "$n bashes you with a &+Cstorm of ice.",
					    "$n crushes $N with a &+Cstorm of ice.",
					    "$N is ripped apart by your &+Cstorm of ice.",
					    "You are ripped to shreds by $n's &+Cice storm.",
					    "$N is ripped to shreds by $n's &+Cice storm.",
					    0 };

	int num_dice = MIN(level, 36);
	int dam = dice(num_dice, 8);
	dam = dam * get_property("spell.area.damage.factor.iceStorm", 1.000);

	if (is_hot_in_room(victim->in_room))
	{
		send_to_char("&+bYou are splashed by water&n!\n", victim);
		act("$n is splashed with &+bwater&n!", FALSE, victim, 0, 0, TO_ROOM);
		make_wet(victim, WAIT_MIN);
	}
	else
	{
		if (IS_PC(ch) && IS_PC(victim))
			dam = dam * get_property("spell.area.damage.to.pc", 0.5);

		send_to_char("You are blasted by the storm!\n", victim);
		spell_damage(ch, victim, dam, SPLDAM_COLD, SPLDAM_GLOBE, &messages);
	}
}

void spell_ice_storm(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		     P_obj /*obj*/)
{
	if (victim == ch)
	{
		send_to_char("You suddenly decide against that, oddly enough.\n", ch);
		return;
	}

	if (is_hot_in_room(ch->in_room))
	{
		send_to_char(
			"Your storm of ice turns into a fountain of &+bwater&n from the heat&n!\n",
			ch);
		act("$n conjures an ice storm!", FALSE, ch, 0, 0, TO_ROOM);
		act("The heat in here makes all ice melt!", FALSE, ch, 0, 0, TO_ROOM);
	}
	else
	{
		send_to_char("&+WYou conjure a storm of ice&n!\n", ch);
		act("$n conjures an ice storm!", FALSE, ch, 0, 0, TO_ROOM);
	}

	cast_as_damage_area(ch, spell_single_icestorm, level, victim,
			    get_property("spell.area.minChance.iceStorm", 90),
			    get_property("spell.area.chanceStep.iceStorm", 10));
	zone_spellmessage(ch->in_room, FALSE, "&+CYou feel a blast of &+Bcold!\n",
			  "&+CYou feel a blast of &+Bcold &+Cfrom the %s!\n");
}
