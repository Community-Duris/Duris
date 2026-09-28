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

extern P_room world;
static void spell_single_chain_lightning(int level, P_char ch, char *arg, int /*type*/,
					 P_char victim, P_obj /*obj*/)
{
	int dam, order;
	struct damage_messages primary_messages = {
		"and &=LBblasts&n into $N!",
		"and &=LBblasts&n into YOU!",
		"and &=LBblasts&n into $N!",
		"and turns $N into a charred &=LBsparkling&n corpse!",
		"and you die as a &=LBflashing light&n explodes in your face!",
		"and turns $N into a charred &=LBsparkling&n corpse!",
		0
	};
	struct damage_messages secondary_messages = {
		"then leaps and &=LBblasts&n into $N!",
		"then leaps and &=LBblasts&n into YOU!",
		"then leaps and &=LBblasts&n into $N!",
		"then leaps and turns $N into a charred &=LBsparkling&n corpse!",
		"and you die as a &=LBflashing light&n explodes in your face!",
		"then leaps and turns $N into a charred &=LBsparkling&n corpse!",
		0
	};

	memcpy(&order, arg, sizeof(order));
	const bool secondary_strike = order != 0;
	dam = 8 * MIN(51, level) + number(level / 3, level) + 30;
	while (order--)
	{
		dam = (int)(dam * 0.8);
	}

	if (IS_PC(ch) && IS_PC(victim))
	{
		dam = dam * get_property("spell.area.damage.to.pc", 0.5);
	}
	dam = dam * get_property("spell.area.damage.factor.chainlightning", 1.000);

	spell_damage(ch, victim, dam, SPLDAM_LIGHTNING, 0,
		     secondary_strike ? &secondary_messages : &primary_messages);
}

void spell_chain_lightning(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			   P_obj /*obj*/)
{
	int hit, room;

	room = ch->in_room;
	send_to_char("A writhing &=LBbolt of lightning&n leaves your hands...\n", ch);
	act("A writhing &=LBbolt of lightning&n leaves $n's hands...", FALSE, ch, 0, 0, TO_ROOM);
	zone_spellmessage(room, TRUE, "&=LBThe sky lights up with brilliant lightning flashes!\n",
			  "&=LBThe sky to the %s lights up with brilliant lightning flashes!\n");

	hit = cast_as_damage_area(ch, spell_single_chain_lightning, level, victim,
				  get_property("spell.area.minChance.chainLightning", 0),
				  get_property("spell.area.chanceStep.chainLightning", 25));

	if (!hit)
	{
		send_to_room("and grounds harmlessly.\n", room);
	}
	else
	{
		send_to_room("and finally fizzles out.\n", room);
	}
}

static void spell_single_lightning_ring(int level, P_char ch, char * /*arg*/, int /*type*/,
					P_char victim, P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"and &+Bsurges&n into $N!",
		"and &+Bsurges&n into YOU!",
		"and &+Bsurges&n into $N!",
		"and turns $N into a charred &=LBsparkling&n corpse!",
		"and you die as a &=LBflashing light&n explodes in your face!",
		"and turns $N into a charred &=LBsparkling&n corpse!",
		0
	};

	dam = (6 * MIN(51, level)) +
	      (GET_CLASS(ch, CLASS_ETHERMANCER) ? dice(level, 4) : number(1, level));
	if (IS_PC(ch) && IS_PC(victim))
	{
		dam = dam * get_property("spell.area.damage.to.pc", 0.5);
	}

	spell_damage(ch, victim, dam, SPLDAM_LIGHTNING, 0, &messages);
}

void spell_ring_lightning(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			  P_obj /*obj*/)
{
	send_to_char("An intense &=LBelectrical bolt&n leaps forth from your fingers... \n", ch);
	act("A wild electrical surge emits from $n's fingertips...", FALSE, ch, 0, 0, TO_ROOM);

	cast_as_damage_area(ch, spell_single_lightning_ring, level, victim,
			    get_property("spell.area.minChance.lightningRing", 30),
			    get_property("spell.area.chanceStep.lightningRing", 15));
}
