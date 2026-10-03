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

void spell_single_incendiary_cloud(int level, P_char ch, char * /*arg*/, int /*type*/,
				   P_char victim, P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"You wave the gases towards $N and have the satisfaction of seeing $M enveloped in &+rflames&N.",
		"You are enveloped in a cloud of incendiary gases sent by $n - OUCH!!",
		"$n cackles as $s incendiary cloud torches $N.",
		"Your wall of incendiary gases torches $N instantly, causing immediate death!",
		"As the gases surround you, $n grins evilly -  then you burst into flames and die.",
		"The incendiary cloud from $n turns $N into a charred corpse.",
		0
	};

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	dam = dice(3 * level, 7) / 2;

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_INCENDIARY_CLOUD);
	if (!NewSaves(victim, SAVING_SPELL, mod))
	{
		dam = dam * 1.5;
	}

	if (IS_PC(ch) && IS_PC(victim))
	{
		dam = dam * get_property("spell.area.damage.to.pc", 0.5);
	}
	dam = dam * get_property("spell.area.damage.factor.incendiaryCloud", 1.000);
	if (GET_SPEC(ch, CLASS_SORCERER, SPEC_WIZARD))
	{
		dam = dam * 1.4;
	}

	spell_damage(ch, victim, dam, SPLDAM_FIRE, 0, &messages);
}

void spell_incendiary_cloud(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			    P_char victim, P_obj /*obj*/)
{
	P_char tch;
	int room = ch->in_room;
	struct room_affect raf;

	/*   if(IS_PC(ch) &&
	      (tch =
	       stack_area(ch, SPELL_INCENDIARY_CLOUD,
	                  (int) get_property("spell.area.stackTimer.incendiaryCloud",
	                                     5))))
	  {
	    act("&+rYour incendiary gases add to $N's raging inferno.", FALSE, ch, 0,
	        tch, TO_CHAR);
	    act
	      ("&+rClouds of incendiary gases pour from $n's fingertips and add to the raging inferno!",
	       FALSE, ch, 0, 0, TO_ROOM);
	  }
	  else */
	{
		send_to_char("&+rBillowing clouds of incendiary gases pour from your fingertips.\n",
			     ch);
		act("&+rBillowing clouds of incendiary gases pour from $n's fingertips!", FALSE, ch,
		    0, 0, TO_ROOM);
	}

	zone_spellmessage(
		ch->in_room, TRUE,
		"&+yOff in the distance there is a &+Ythundering &+Rroar &+yand &+wbillowing &+Lsmoke.\n",
		"&+yOff in the distance to the %s there is a &+Ythundering &+Rroar &+yand &+wbillowing &+Lsmoke.\n");

	cast_as_damage_area(ch, spell_single_incendiary_cloud, level, victim,
			    get_property("spell.area.minChance.incendiaryCloud", 50),
			    get_property("spell.area.chanceStep.incendiaryCloud", 20));

	memset(&raf, 0, sizeof(raf));
	raf.type = SPELL_INCENDIARY_CLOUD;
	raf.duration = int(1.5 * PULSE_VIOLENCE);
	affect_to_room(room, &raf);

	for (tch = world[room].people; tch; tch = tch->next_in_room)
		if (IS_AFFECTED5(tch, AFF5_WET))
		{
			send_to_char("The heat of the cloud dried up your clothes.\n", tch);
			make_dry(tch);
		}
}
