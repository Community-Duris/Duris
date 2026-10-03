#include "core/prototypes.h"
#include "core/structs.h"
#include "world/db.h"
#include "net/comm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include <strings.h>

void spell_ghastly_touch(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			 P_obj /*obj*/)
{
	struct damage_messages messages = {
		"&+LYou direct a &+wghast &+Ltowards&n $N&+L, and its shadowy fingers brush $m.",
		"&+LAn incorporeal ghastly figure touches you!",
		"&+LAn incorporeal ghastly figure brushes&n $N &+Lwith its shadowy &+wfingers.",
		"$N &+Lconvulses and dies a quick and quiet &+rdeath.",
		"&+LYou feel spectral fingers sap the last bit of &+clifeforce &+Lfrom you.",
		"$N &+Lquietly collapses and &+rdies!",
		0
	};

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	if (IS_UNDEADRACE(victim))
	{
		act("&+LYour ghast balks at attacking another &+rundead &+Land disperses quickly.&n",
		    FALSE, ch, 0, victim, TO_CHAR);
		return;
	}

	if (GET_LEVEL(victim) < (level / 5))
	{
		spell_damage(ch, victim, 10000, SPLDAM_NEGATIVE, SPLDAM_NODEFLECT, &messages);
		return;
	}

	int dam = (int)number(level * 5, level * 7);

	if (IS_PC(ch) && IS_PC(victim))
	{
		dam = dam * get_property("spell.area.damage.to.pc", 0.5);
	}
	dam = dam * get_property("spell.area.damage.factor.summonGhasts", 1.000);

	if (spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_NODEFLECT, &messages) ==
	    DAM_NONEDEAD)
	{
		if (GET_LEVEL(victim) < level / 2)
		{
			spell_minor_paralysis((int)(level / 2), ch, NULL, 0, victim, NULL);
		}
	}
}

void event_summon_ghasts(P_char ch, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	int room;
	room = *((int *)data);
	if (room != ch->in_room)
	{
		send_to_char("&+LThe incorporeal figures dissolve into nothing...\n", ch);
		;
		act("&+LThe ghastly creatures fade into oblivion...", TRUE, ch, 0, 0, TO_ROOM);
		return;
	}

	if (!number(0, 3))
	{
		act("$n&+L's ghastly figures glide about the area.", FALSE, ch, 0, 0, TO_ROOM);
		add_event(event_summon_ghasts, PULSE_VIOLENCE * 1, ch, 0, 0, 0, &room,
			  sizeof(room));
		return;
	}

	act("$n&+L's summoned creatures look towards $m &+Lfor guidance, before &+rattacking&+L!",
	    FALSE, ch, 0, 0, TO_ROOM);
	act("&+LYour creatures from beyond the grave look towards you for guidance.", FALSE, ch, 0,
	    0, TO_CHAR);

	cast_as_damage_area(ch, spell_ghastly_touch, GET_LEVEL(ch), NULL,
			    get_property("spell.area.minChance.summonGhasts", 90),
			    get_property("spell.area.chanceStep.summonGhasts", 10));
}

void spell_summon_ghasts(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
			 P_obj /*obj*/)
{
	int room;
	room = ch->in_room;
	send_to_room("&+LDeathly incorporeal ghasts enter the realm of the living...\n",
		     ch->in_room);
	zone_spellmessage(
		ch->in_room, FALSE,
		"&+LThe air &+cchills &+Land the odor of &+rdeath &+Land &+ydecay &+Lassault your senses.\n",
		"&+LThe air to the %s &+cchills &+Land the odor of &+rdeath &+Land &+ydecay &+Lassaults your senses.\n");

	add_event(event_summon_ghasts, PULSE_VIOLENCE * 1, ch, 0, 0, 0, &room, sizeof(room));

	CharWait(ch, (int)1 * PULSE_VIOLENCE);
}
