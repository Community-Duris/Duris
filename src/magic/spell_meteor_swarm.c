#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "magic/spells.h"

static void spell_single_meteorswarm(int level, P_char ch, char * /*arg*/, int /*type*/,
				     P_char victim, P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"You smash $N with your controlled meteors.",
		"$n smashes you with $s swarm of meteors, stunning you.",
		"$n massacres $N to little pieces with $s meteor swarm.",
		"$N is whacked by your swarm of meteors.",
		"You are whacked by a swarm of $n's meteors.",
		"$n obliterates $N with a swarm of meteors.",
		0
	};

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	dam = 80 + level * 7 + number(1, 40);
	if (IS_PC(ch) && IS_PC(victim))
	{
		dam = dam * get_property("spell.area.damage.to.pc", 0.5);
	}
	dam = dam * get_property("spell.area.damage.factor.meteorSwarm", 1.000);
	if (GET_SPEC(ch, CLASS_SORCERER, SPEC_WIZARD))
	{
		dam = dam * 1.4;
	}
	spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
}

void spell_meteorswarm(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		       P_obj /*obj*/)
{
	if (!OUTSIDE(ch))
	{
		send_to_char("You must be outside to cast this spell!\n", ch);
		return;
	}

	/*   if(IS_PC(ch) &&
	      stack_area(ch, SPELL_METEOR_SWARM,
	                 (int) get_property("spell.area.stackTimer.meteorSwarm", 5)))
	  {
	    send_to_char
	      ("Someone just summoned meteors from the above sky, but you try anyway.\n",
	       ch);
	    act("&+r$n tries to conjure a deadly meteor swarm, but only few fall.",
	        FALSE, ch, 0, 0, TO_ROOM);
	    level /= 2;
	  }
	  else */
	{
		act("&+rYou've conjured up a fearsome meteor swarm!", FALSE, ch, 0, 0, TO_CHAR);
		act("&+r$n conjures up a fearsome meteor swarm!", FALSE, ch, 0, 0, TO_ROOM);
	}

	zone_spellmessage(ch->in_room, TRUE, "&+rThe sky is full of &+Rflaming meteors!\r\n",
			  "&+rThe sky to %s is full of &+Rflaming meteors!\r\n");
	cast_as_damage_area(ch, spell_single_meteorswarm, level, victim,
			    get_property("spell.area.minChance.meteorSwarm", 50),
			    get_property("spell.area.chanceStep.meteorSwarm", 20));
}
