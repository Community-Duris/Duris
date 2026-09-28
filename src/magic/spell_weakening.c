#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include <strings.h>
void spell_wither(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		  P_obj /*obj*/)
{
	int percent, dam;
	struct affected_type af;

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	/* Commented this out.. need && (type == SPELL_TYPE_SPELL) if you re-add it.
	 * Was commented 'cause was stopping wither traps from working.
	  if( victim == ch )
	  {
	    send_to_char("You may not wither yourself.", ch);
	    return;
	  }
	*/

	if (IS_CONSTRUCT(victim))
	{
		act("&+LBeing an artificial construct, $E &+Lis &+Wimmune&+L to your spell!", TRUE,
		    ch, 0, victim, TO_CHAR);
		return;
	}

	if (IS_UNDEADRACE(victim))
	{
		if (!number(0, 1) && resists_spell(ch, victim))
		{
			send_to_char("Your victim resisted your &+Lwither&n attempt!\r\n", ch);
			return;
		}

		struct damage_messages wither_undead = {
			"$N &+Lwavers under the power of your will, as $S &+Lundead flesh withers away!",
			"$n's &+Lpure will causes your undead flesh to wither away!",
			"$N &+Lwavers under the power of&n $n's &+Lwill, as $S &+Lundead flesh withers away!",
			"$N &+Lwithers away and is completely destroyed!",
			"$n's &+Lorders your flesh to wither away - and it obeys $m! &+LYou return into the cold sleep of death...",
			"$N&+L is completely withered away by&n $n's &+Lraw will!",
			0
		};

		dam = dice(MIN(level, 46), 10);
		spell_damage(ch, victim, dam, SPLDAM_GENERIC,
			     SPLDAM_NOSHRUG | SPLDAM_GLOBE | SPLDAM_GRSPIRIT, &wither_undead);
		return;
	}

	/* if(resists_spell(ch,victim))
	   return;*/

	percent = victim->specials.apply_saving_throw[SAVING_SPELL] * number(1, 5);

	percent += (int)(GET_C_POW(ch) - GET_C_POW(victim));
	percent += (int)(GET_LEVEL(ch) - GET_LEVEL(victim));

	int mod = get_default_save_mod(victim, ch, SAVING_FEAR, SPELL_WITHER);
	if (NewSaves(victim, SAVING_FEAR, mod))
	{
		percent = (3 * percent) / 4;
	}

	percent = BOUNDED(0, percent, 100);

	if (IS_TRUSTED(victim) || percent < 1)
	{
		send_to_char("They are too powerful to wither this way!\n", ch);
		return;
	}

	if (affected_by_spell(victim, SPELL_WITHER))
	{
		send_to_char(
			"If you withered them any more, they'd be some sort of filthy prune.\n",
			ch);
		return;
	}

	if (affected_by_spell(victim, SPELL_RAY_OF_ENFEEBLEMENT))
	{
		act("$E is already a &+ywithered prune.&n Enfeebling $M is not possible.", TRUE, ch,
		    0, victim, TO_CHAR);
		act("&+LLuckily for you, you cannot possibly get more feeble!&n", TRUE, ch, 0,
		    victim, TO_VICT);
		return;
	}

	if ((percent > 0 &&
	     (IS_AFFECTED4(victim, AFF4_NEG_SHIELD) || IS_AFFECTED2(victim, AFF2_SOULSHIELD))) ||
	    IS_UNDEADRACE(victim) || IS_GREATER_RACE(victim))
	{
		percent = (int)(percent * 0.75);
	}

	bzero(&af, sizeof(af));

	if (percent > 90)
	{
		act("$N is &+Lwithered&n COMPLETELY by $n's touch!", TRUE, ch, 0, victim,
		    TO_NOTVICT);
		act("$n &+Lwithers&n you with his touch. You feel your muscles COMPLETELY shrink!",
		    FALSE, ch, 0, victim, TO_VICT);
		act("$N is &+Lwithered&n COMPLETELY by your touch!", FALSE, ch, 0, victim, TO_CHAR);
		af.type = SPELL_WITHER;
		af.duration = 1;
		af.modifier = 0 - (int)(GET_HITROLL(victim) / 4);
		af.location = APPLY_HITROLL;
		af.bitvector2 = AFF2_SLOW;
		affect_to_char(victim, &af);
		af.modifier = 0 - (int)(GET_DAMROLL(victim) / 4);
		af.location = APPLY_DAMROLL;
		affect_to_char(victim, &af);
		af.modifier = +100;
		af.location = APPLY_AC;
		affect_to_char(victim, &af);
	}
	else if (percent > 70)
	{
		act("$N is &+Lwithered&n and starts to FADE with $n's touch!", FALSE, ch, 0, victim,
		    TO_NOTVICT);
		act("$n &+Lwithers&n you with his touch. You feel your body FADE!", FALSE, ch, 0,
		    victim, TO_VICT);
		act("$N is &+Lwithered&n and starts to FADE by your touch!", FALSE, ch, 0, victim,
		    TO_CHAR);
		af.type = SPELL_WITHER;
		af.duration = 1;
		af.modifier = 0 - (int)(GET_HITROLL(victim) / 4);
		af.location = APPLY_HITROLL;
		affect_to_char(victim, &af);
		af.modifier = 0 - (int)(GET_DAMROLL(victim) / 4);
		af.location = APPLY_DAMROLL;
		affect_to_char(victim, &af);
		af.modifier = +50;
		af.location = APPLY_AC;
		affect_to_char(victim, &af);
	}
	else if (percent > 40)
	{
		act("$N is &+Lwithered&n and starts to DIMINISH with $n's touch!", FALSE, ch, 0,
		    victim, TO_NOTVICT);
		act("$n &+Lwithers&n you with his touch. You feel your body DIMINISH!", FALSE, ch,
		    0, victim, TO_VICT);
		act("$N is &+Lwithered&n and starts to DIMINISH by your touch!", FALSE, ch, 0,
		    victim, TO_CHAR);
		af.type = SPELL_WITHER;
		af.duration = 1;
		af.modifier = 0 - (int)(GET_HITROLL(victim) / 5);
		af.location = APPLY_HITROLL;
		// af.bitvector2 = AFF2_SLOW;
		affect_to_char(victim, &af);
		af.modifier = 0 - (int)(GET_DAMROLL(victim) / 5);
		af.location = APPLY_DAMROLL;
		affect_to_char(victim, &af);
		af.modifier = +25;
		af.location = APPLY_AC;
		affect_to_char(victim, &af);
	}
	else if (percent > 10)
	{
		act("$N is &+Lwithered&n and starts to decrease in size with $n's touch!", FALSE,
		    ch, 0, victim, TO_NOTVICT);
		act("$n &+Lwithers&n you with his touch. You begin to DECREASE in size!", FALSE, ch,
		    0, victim, TO_VICT);
		act("$N is &+Lwithered&n and begins to DECREASE in size by your touch!", FALSE, ch,
		    0, victim, TO_CHAR);
		af.type = SPELL_WITHER;
		af.duration = 1;
		af.modifier = 0 - (int)(GET_HITROLL(victim) / 8);
		af.location = APPLY_HITROLL;
		affect_to_char(victim, &af);
		af.modifier = 0 - (int)(GET_DAMROLL(victim) / 8);
		af.location = APPLY_DAMROLL;
		affect_to_char(victim, &af);
		af.modifier = +20;
		af.location = APPLY_AC;
		affect_to_char(victim, &af);
	}
	else
	{
		act("$N &+Lseems unaffected by your attempt to wither.", FALSE, ch, 0, victim,
		    TO_CHAR);
	}
}

void spell_dispel_lifeforce(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			    P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (IS_UNDEADRACE(victim))
	{
		send_to_char("&+LYour victim is not among the living.&n\n", ch);
		return;
	}

	if (!affected_by_spell(victim, SPELL_DISPEL_LIFEFORCE) && !NewSaves(victim, SAVING_PARA, 0))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_DISPEL_LIFEFORCE;
		af.duration = 25;
		af.modifier = 30;
		af.location = APPLY_AC;
		affect_to_char(victim, &af);
		af.modifier = -30;
		af.location = APPLY_STR;
		affect_to_char(victim, &af);
		af.modifier = -10;
		af.location = APPLY_HITROLL;
		affect_to_char(victim, &af);
		af.modifier = -10;
		af.location = APPLY_DAMROLL;
		affect_to_char(victim, &af);

		act("&+L$n&+L is completely sapped of $s lifeforce!!", FALSE, victim, 0, 0,
		    TO_ROOM);
		act("&+LYou are completely sapped of your lifeforce!!", FALSE, victim, 0, 0,
		    TO_CHAR);
	}
}

void spell_ray_of_enfeeblement(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			       P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	int base_str = victim->base_stats.Str;
	int curr_str = GET_C_STR(victim);
	int mod;

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
		return;

	if (victim == ch)
	{
		send_to_char("You may not enfeeble yourself.", ch);
		return;
	}

	if (resists_spell(ch, victim))
		return;

	if (IS_ELITE(victim) || IS_GREATER_RACE(victim))
		return;

	if (affected_by_spell(victim, SPELL_RAY_OF_ENFEEBLEMENT))
	{
		act("$E is already pretty feeble.", TRUE, ch, 0, victim, TO_CHAR);
		act("&+LYou cannot possible get more feeble!&n", TRUE, ch, 0, victim, TO_VICT);
		return;
	}

	if (affected_by_spell(victim, SPELL_WITHER))
	{
		act("$E is withered and unaffected by enfeeblment.", TRUE, ch, 0, victim, TO_CHAR);
		return;
	}

	if (level < 1)
		level = 1;

	mod = level + 1;

	if (mod > base_str)
		mod = base_str;

	if (mod > curr_str)
		mod = curr_str;

	mod /= 2;

	int success_chance = BOUNDED(50, 70 + GET_LEVEL(ch) - GET_LEVEL(victim), 80);

	if (success_chance > number(1, 100))
	{
		send_to_char("A wave of weakness sweeps over you!\n", victim);
		act("$n pales, and seems to sag.", TRUE, victim, 0, 0, TO_ROOM);

		bzero(&af, sizeof(af));
		af.type = SPELL_RAY_OF_ENFEEBLEMENT;

		if (!NewSaves(victim, SAVING_PARA, 0))
		{
			af.duration = 2;
			af.modifier = -(mod);
			af.location = APPLY_STR;
			affect_to_char(victim, &af);

			mod = level / 10 + number(1, 6);
			// Don't go below 0 or it'll jump up to 255 'cause damroll is an unsigned byte.
			if (mod > victim->points.damroll)
				af.modifier = 0 - victim->points.damroll;
			else
				af.modifier = 0 - mod;
			af.location = APPLY_DAMROLL;
			affect_to_char(victim, &af);
		}
		else
		{
			af.duration = 1;
			af.modifier = -1 * mod / 10 - number(1, 6);
			af.location = APPLY_STR;
			affect_to_char(victim, &af);

			af.modifier = MIN(-1, (int)(-1 * (level / 10) - number(0, 5)));
			// Don't go below 0 or it'll jump up to 255 'cause damroll is an unsigned byte.
			if (af.modifier < 0 - victim->points.damroll)
				af.modifier = 0 - victim->points.damroll;
			af.location = APPLY_DAMROLL;
			affect_to_char(victim, &af);
		}
	}
	if (IS_NPC(victim) && CAN_SEE(victim, ch))
	{
		remember(victim, ch);
		if (!IS_FIGHTING(victim))
			MobStartFight(victim, ch);
	}
}
