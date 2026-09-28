#include "core/prototypes.h"
#include "core/structs.h"
#include "world/events.h"
#include "net/comm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include <string.h>

void event_fleshdecay(P_char victim, P_char ch, P_obj /*obj*/, void *data)
{
	int dam;
	int num_waves = *((int *)data);
	struct damage_messages dam_msgs1 = {
		"A &+gslimy&n piece of $N's &+rflesh &nbreaks apart from $S body and falls to the &+yground&n.\n",
		"&nA &+gslimy &npiece of your &+rflesh&n breaks apart from your body and falls to the &+yground&n.\n",
		"A &+gslimy&n piece of $N's &+rflesh &nbreaks apart from $S body and falls to the &+yground&n.\n",
		"A &+gslimy&n piece of $N's &+rflesh &nbreaks apart from $S body and falls to the &+yground&n.\n",
		"&nA &+gslimy &npiece of your &+rflesh&n breaks apart from your body and falls to the &+yground&n.\n",
		"A &+gslimy&n piece of $N's &+rflesh &nbreaks apart from $S body and falls to the &+yground&n.\n",
		0
	};

	struct damage_messages dam_msgs2 = {
		"$N's skin continues to &+gde&+Gca&+Ly &nas the spell consumes $S &+rflesh&n.",
		"&nYour skin continues to &+gde&+Gca&+Ly &nas the spell consumes your &+rflesh&n.\n",
		"$N's skin continues to &+gde&+Gca&+Ly &nas the spell consumes $S &+rflesh&n.",
		"$N's skin continues to &+gde&+Gca&+Ly &nas the spell consumes $S &+rflesh&n.",
		"&nYour skin continues to &+gde&+Gca&+Ly &nas the spell consumes your &+rflesh&n.\n",
		"$N's skin continues to &+gde&+Gca&+Ly &nas the spell consumes $S &+rflesh&n.",
		0
	};

	if (!IS_AFFECTED5(victim, AFF5_DECAYING_FLESH))
		return;

	dam = dice(5, 10);

	if ((GET_HIT(victim) - dam) > 0)
	{
		if (number(1, 100) > 50)
		{
			melee_damage(ch, victim, dam, PHSDAM_NOREDUCE, &dam_msgs1);
		}
		else
		{
			melee_damage(ch, victim, dam, PHSDAM_NOREDUCE, &dam_msgs2);
		}
	}

	if (--num_waves > 0)
	{
		// ch and victim is backwards so disarm will work right.
		add_event(event_fleshdecay, PULSE_VIOLENCE, victim, ch, NULL, 0, &num_waves,
			  sizeof(num_waves));
	}
	else
	{
		affect_from_char(victim, SPELL_DECAYING_FLESH);
	}
}

void spell_decaying_flesh(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			  P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	int num_waves = number(2, 6);

	if (ch == victim)
	{
		send_to_char("You decide that would not be the best use of your dark arts.&n\n",
			     ch);
		return;
	}

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	if (IS_AFFECTED5(victim, AFF5_DECAYING_FLESH))
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
		{
			if (af1->type != SPELL_DECAYING_FLESH)
				continue;
			if (af1->modifier >= 5)
			{
				act("&n$S flesh cannot be afflicted any further.", FALSE, ch, 0,
				    victim, TO_CHAR);
			}
			else
			{
				act("&+R$n &nagain points at &+L$N &ncausing the existing &+gdecay&n to worsen&n.",
				    FALSE, ch, 0, victim, TO_ROOM);
				act("&+RYou &nagain point at &+L$N &ncausing the existing &+gdecay&n to worsen&n.",
				    FALSE, ch, 0, victim, TO_CHAR);

				af1->modifier++;
			}
			break;
		}
		disarm_char_nevents(victim, event_fleshdecay);
		// This is backwards so disarm will work right.
		add_event(event_fleshdecay, PULSE_VIOLENCE, victim, ch, NULL, 0, &num_waves,
			  sizeof(num_waves));
	}
	else
	{
		act("&+R$n &nraises $s hand and points directly at &+L$N.\n"
		    "&+L$N &nsuddenly turns &+ggreen &nas $S &+Rflesh &nbegins to &+Lwither&n and &+rrot&n.",
		    FALSE, ch, 0, victim, TO_NOTVICT);
		act("You &nraise your hand and point directly at &+L$N.\n"
		    "&+L$N &nsuddenly turns &+ggreen &nas $S &+Rflesh &nbegins to &+Lwither&n and &+rrot&n.",
		    FALSE, ch, 0, victim, TO_CHAR);
		act("&+R$n &nraises $s hand and points directly at &+LYOU&n!\n"
		    "Your &+Rskin&n suddenly turns &+ggreen &nand starts &+Lwithering &nand &+rrotting &nright before your eyes!",
		    FALSE, ch, 0, victim, TO_VICT);

		memset(&af, 0, sizeof(af));
		af.type = SPELL_DECAYING_FLESH;
		af.bitvector5 = AFF5_DECAYING_FLESH;
		af.duration = -1;
		af.flags = AFFTYPE_NODISPEL | AFFTYPE_NOSAVE;
		af.modifier = 1;
		affect_to_char(victim, &af);
		// This is backwards so disarm will work right.
		add_event(event_fleshdecay, PULSE_VIOLENCE, victim, ch, NULL, 0, &num_waves,
			  sizeof(num_waves));
	}

	attack_back(ch, victim, TRUE);
}

void event_immolate(P_char ch, P_char vict, P_obj /*obj*/, void *data)
{
	int dam, burntime;
	struct damage_messages messages = {
		"&+RF&+rl&+Ram&+Wes &+ysmother $N, &+Lse&+war&+Ling&+y and &+rba&+Rki&+rng&+y $M.",
		"&+RThe fire burns &+Wwhite &+Rhot as it consumes your flesh!",
		"&+RF&+Yir&+We &+Ls&+wea&+Lrs&+y and &+rb&+Rak&+res&n $N!",
		"$N screams in agony as &+Rthe flames&n consume $M completely.",
		"You scream in agony as &+Rthe flames&n consume you completely.",
		"$N screams in agony as &+Rthe flames&n consume $M completely.",
		0
	};

	if (!ch)
	{
		logit(LOG_EXIT, "event_immolate called in magic.c with no ch");
		return;
	}
	if (!vict)
	{
		return;
	}
	if (ch && // Just making sure.
	    vict)
	{
		if (!IS_ALIVE(ch) || !IS_ALIVE(vict))
		{
			return;
		}
		burntime = *((int *)data);
		if ((burntime >= 3 && number(0, burntime)) || burntime == 6)
		{
			act("The &+Yfi&+Rer&+Yy&N &+rconflagration&n subsides!", FALSE, ch, 0, vict,
			    TO_VICT);
			if (ch->in_room == vict->in_room)
				act("The &+Yfi&+Rer&+Yy&N &+rconflagration&N burning $N subsides!",
				    FALSE, ch, 0, vict, TO_CHAR);
			act("The &+Yfi&+Rer&+Yy&N &+rconflagration&N burning $N subsides!", FALSE,
			    ch, 0, vict, TO_NOTVICT);
			return;
		}
		else
		{
			burntime++;
		}
		dam = (int)GET_LEVEL(ch) * 2 + number(4, 20);

		if (burntime >= 3)
			dam = (int)GET_LEVEL(ch) * 2 - number(4, 20);

		if (burntime >= 4)
			dam = (int)GET_LEVEL(ch) + number(4, 12);

		if (IS_ALIVE(vict) && spell_damage(ch, vict, dam, SPLDAM_FIRE, SPLDAM_NODEFLECT,
						   &messages) == DAM_NONEDEAD)
		{
			if (ch &&
			    IS_ALIVE(vict)) // Added another check due to reported double death bug.
			{
				add_event(event_immolate, PULSE_VIOLENCE, ch, vict, NULL, 0,
					  &burntime, sizeof(burntime));

				if (4 > number(1, 10))
					if (!IS_DRAGOON(vict))
						stop_memorizing(vict);
			}
		}
	}
}

void spell_immolate(int /*level*/, P_char ch, char * /*arg*/, int type, P_char victim,
		    P_obj /*obj*/)
{
	int burn = 0;
	if (!ch)
	{
		logit(LOG_EXIT, "spell_immolate called in magic.c with no ch");
		return;
	}
	if (!victim)
	{
		return;
	}
	if (ch && // Just making sure...
	    victim)
	{
		if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
		{
			return;
		}
		act("Your &+Yfi&+Rer&+Yy&N blast strikes $N full on!", TRUE, ch, 0, victim,
		    TO_CHAR);
		act("A &+Yfi&+Rer&+Yy&N &+rconflagration&N spews from $n striking you full on!",
		    FALSE, ch, 0, victim, TO_VICT);
		act("A &+Yfi&+Rer&+Yy&N &+rconflagration&N spews from $n striking $N full on!",
		    FALSE, ch, 0, victim, TO_NOTVICT);
		if (ch && victim)
		{
			engage(ch, victim);
		}

		int dam = GET_LEVEL(ch) * 4 + number(4, 20);
		if (type > 1)
			dam += dam * 0.05;

		if (IS_ALIVE(victim) && (spell_damage(ch, victim, dam, SPLDAM_FIRE,
						      SPLDAM_NODEFLECT, NULL) == DAM_NONEDEAD))
		{
			if (ch && IS_ALIVE(victim)) // Adding another check.
			{
				// gain_exp(ch, victim, 0, EXP_DAMAGE);
				add_event(event_immolate, PULSE_VIOLENCE, ch, victim, NULL, 0,
					  &burn, sizeof(burn));
			}
		}
	}
}

void event_acidimmolate(P_char ch, P_char vict, P_obj /*obj*/, void *data)
{
	int dam, acidburntime;
	struct damage_messages messages = {
		"&+GG&+gr&+Ge&+ge&+Gn s&+gl&+Gi&+gm&+Ge eats away at $N&+G's skin!&N",
		"&+GG&+gr&+Ge&+ge&+Gn s&+gl&+Gi&+gm&+Ge eats away at your skin!&N",
		"&+GG&+gr&+Ge&+ge&+Gn s&+gl&+Gi&+gm&+Ge eats away at $N&+G's skin!&N",
		"$N &+gmelts into a pile of &+GGOO&n ... $E is no more!",
		"&+gThe &+Gs&+gl&+Gi&+gm&+Ge &+gconsuming your flesh devours you completely!",
		"$N &+gmelts into a pile of &+GGOO&n ... $E is no more!",
		0
	};

	// Missing means that something went very wrong.
	if (!ch || !vict)
	{
		logit(LOG_EXIT, "event_acidimmolate called in magic.c with no ch");
		return;
	}
	// Dead just means something died but hasn't 'gone to heaven' yet.
	if (!IS_ALIVE(ch) || !IS_ALIVE(vict))
	{
		return;
	}
	acidburntime = *((int *)data);

	if ((acidburntime >= 4 && number(0, acidburntime--)) || acidburntime == 7)
	{
		act("&+GThe &+Ymo&+yrd&+Yant &+Gsubstance oozes off you!", FALSE, ch, 0, vict,
		    TO_VICT);

		if (ch->in_room == vict->in_room)
		{
			act("&+GThe &+Ymo&+yrd&+Yant &+Gsubstance oozes off of $N.", FALSE, ch, 0,
			    vict, TO_CHAR);
		}
		act("&+GThe &+Ymo&+yrd&+Yant &+Gsubstance oozes off of $N.", FALSE, ch, 0, vict,
		    TO_NOTVICT);
		return;
	}
	else
	{
		acidburntime++;
	}

	dam = (int)(GET_LEVEL(ch) * 2);
	// dam = 1; //this is now just a pulling spell.
	if (!GET_CLASS(ch, CLASS_CONJURER))
	{
		if (acidburntime >= 4)
		{
			dam = (int)GET_LEVEL(ch) + number(4, 12);
		}
		else if (acidburntime == 3)
		{
			dam = (int)(GET_LEVEL(ch) * 2 - number(4, 20));
		}
	}
	if (!number(0, 4))
	{
		// Had to split this up 'cause !0 && Dead char leads to another call to spell_damage
		if (spell_damage(ch, vict, dam, SPLDAM_ACID, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG,
				 &messages) != DAM_NONEDEAD)
		{
			return;
		}
		add_event(event_acidimmolate, PULSE_VIOLENCE, ch, vict, NULL, 0, &acidburntime,
			  sizeof(acidburntime));
		if (8 > number(1, 10))
		{
			stop_memorizing(vict);
		}
	}
	// This is still strange, as it's almost identical to above, aside from the stop_memming chance.
	else if (spell_damage(ch, vict, dam, SPLDAM_ACID, SPLDAM_NODEFLECT | SPLDAM_NOSHRUG,
			      &messages) == DAM_NONEDEAD)
	{
		add_event(event_acidimmolate, PULSE_VIOLENCE, ch, vict, NULL, 0, &acidburntime,
			  sizeof(acidburntime));
		if (3 > number(1, 10))
		{
			stop_memorizing(vict);
		}
	}
}

void spell_acidimmolate(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			P_obj /*obj*/)
{
	int acidburn = 0;

	if (!ch)
	{
		logit(LOG_EXIT, "spell_immolate called in magic.c with no ch");
		return;
	}

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	act("&+GYour bubbling spray of goo strikes $N&+G full on!&N", TRUE, ch, 0, victim, TO_CHAR);
	act("&+GA bubbling spray of goo spews from $n&+G striking you &+Gfull on!&N", FALSE, ch, 0,
	    victim, TO_VICT);
	act("&+GA bubbling spray of goo spews from $n&+G striking $N &+Gfull on!&N", FALSE, ch, 0,
	    victim, TO_NOTVICT);

	engage(ch, victim);

	if (spell_damage(ch, victim, (int)GET_LEVEL(ch) * 2 + number(20, 120), SPLDAM_ACID,
			 SPLDAM_NODEFLECT | SPLDAM_NOSHRUG, NULL) == DAM_NONEDEAD)
	{
		if (IS_ALIVE(victim)) // Adding double check.
		{
			add_event(event_acidimmolate, PULSE_VIOLENCE, ch, victim, NULL, 0,
				  &acidburn, sizeof(acidburn));
			// gain_exp(ch, victim, 0, EXP_DAMAGE);
		}
	}
}

void event_electrical_execution(P_char ch, P_char vict, P_obj /*obj*/, void *data)
{
	int dam, frytime;
	struct damage_messages messages = {
		"&+CElectrical arcs continue to &+Lblacken &+cand pincushion&n $N.",
		"The &+Celectrical arcs&n turn your exposed flesh &+Lblack!&n",
		"&+CElectrical arcs&n continue to &+Lblacken &+cand pincushion&n $N.",
		"$N's body &+Lsmokes&n and $M lifeless body turns &+Lblack.&n",
		"&+RYour brain overloads causing immediate death!!!&N",
		"$N's body &+Lsmokes&n as $M body is consumed by the &+Celectric arcs.&n",
		0
	};

	if (!ch)
	{
		logit(LOG_EXIT, "event_electrical_execution called in magic.c with no ch");
		return;
	}
	if (!IS_ALIVE(ch) || !IS_ALIVE(vict))
	{
		return;
	}

	frytime = *((int *)data);

	if ((frytime >= 5 && number(0, frytime)) || (frytime == 7))
	{
		act("&+cThe arcs of electricty surrounding you &+yground out!&n", FALSE, ch, 0,
		    vict, TO_VICT);
		act("&+cThe arcs of electricity surrounding&n $N &+yground out!&n", FALSE, ch, 0,
		    vict, TO_CHAR);
		act("&+cThe arcs of electricity surrounding&n $N &+yground out!&n", FALSE, ch, 0,
		    vict, TO_NOTVICT);
		return;
	}
	else
	{
		frytime++;
	}

	if (frytime >= 4)
	{
		dam = (int)(GET_LEVEL(ch) / 2 + number(4, 12));
	}
	else if (frytime >= 2)
	{
		dam = (int)((2 * GET_LEVEL(ch)) / 3 - number(-20, 20));
	}
	else
	{
		dam = (int)((3 * GET_LEVEL(ch)) / 2 + number(4, 20));
	}

	if (IS_ALIVE(vict) && spell_damage(ch, vict, dam, SPLDAM_LIGHTNING, SPLDAM_NODEFLECT,
					   &messages) == DAM_NONEDEAD)
	{
		if (IS_ALIVE(vict))
		{
			add_event(event_electrical_execution, (int)(0.5 * PULSE_VIOLENCE), ch, vict,
				  NULL, 0, &frytime, sizeof(frytime));
			if (6 > number(1, 10))
			{
				stop_memorizing(vict);
			}
		}
	}
}

void spell_electrical_execution(int /*level*/, P_char ch, char * /*arg*/, int /*type*/,
				P_char victim, P_obj /*obj*/)
{
	int fry = 0;

	struct damage_messages messages = {
		"A huge shower of &=LBarcing electricity&n engulfs $N.",
		"A huge shower of &=LBarcing electricity&n from $n engulfs you.",
		"$N is engulfed by a shower of &=LBarcing electricity&n sent by $n.",
		"The shower of &=LBarcing electricity&n was more than $N could handle.",
		"Your hair and skin crackle and pop, then your brain activity stops!",
		"$N twitches and jerks violently to death from $n's shower of &=LBarcing electricity&n!",
		0
	};

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	engage(ch, victim);

	if (IS_ALIVE(victim) &&
	    spell_damage(ch, victim, (int)GET_LEVEL(ch) * 4 + number(4, 60), SPLDAM_LIGHTNING,
			 SPLDAM_NODEFLECT, &messages) == DAM_NONEDEAD)
	{
		if (IS_ALIVE(ch) && IS_ALIVE(victim))
		{
			add_event(event_electrical_execution, (int)(0.5 * PULSE_VIOLENCE), ch,
				  victim, NULL, 0, &fry, sizeof(fry));
		}
	}
}

void event_dread_wave(P_char ch, P_char vict, P_obj /*obj*/, void *data)
{
	int level, dam;
	struct affected_type *af;
	struct damage_messages messages = {
		"A &+Bdark blue, &+Cbitterly icy &+bwave&N flows over $N.",
		"A &+Bdark blue, &+Cbitterly icy&N &+bwave&N flows over you, choking the breath from your lungs.",
		"A &+Bdark blue, &+Cbitterly icy&N &+bwave&N flows over $N.",
		"A &+Bdark blue, &+Cbitterly icy&N &+bwave&N freezes $N to death!",
		"A &+Bdark blue, &+Cbitterly icy&N &+bwave&N freezed you to the core...",
		"A &+Bdark blue, &+Cbitterly icy&N &+bwave&N flows over $N, freezing $M to &=LRdeath!&n&n",
		0
	};

	if (!IS_ALIVE(ch) || !IS_ALIVE(vict))
		return;

	level = *((int *)data);

	for (af = vict->affected; af && af->type != SPELL_DREAD_WAVE; af = af->next)
		;

	if (af == NULL)
		return;

	if ((af->modifier)-- == 0)
	{
		send_to_char("&+bThe wave slowly dissipates.&N\n", vict);
		act("The &+cchilling wave &ncovering $N recedes and &+cv&+Ba&+cp&+Bo&+cr&+Bi&+cz&+Be&+cs...&n",
		    FALSE, ch, 0, vict, TO_CHAR);
		act("The &+cchilling wave &ncovering $N &+cv&+Ba&+cp&+Bo&+cr&+Bi&+cz&+Be&+cs...&n",
		    FALSE, ch, 0, vict, TO_NOTVICT);
		affect_from_char(vict, SPELL_DREAD_WAVE);
		return;
	}

	dam = MAX(1, (int)(((af->modifier + 2) * level) + number(-60, 0)));

	dam = (int)(dam * GET_LEVEL(ch) / 56);

	int mod = get_default_save_mod(vict, ch, SAVING_SPELL, SPELL_DREAD_WAVE);
	if (NewSaves(vict, SAVING_SPELL, mod) && NewSaves(vict, SAVING_BREATH, 0))
		dam = (int)(dam * 0.66);

	if (number(0, 2) && resists_spell(ch, vict))
	{
		dam = 1;
	}

	if (spell_damage(ch, vict, dam, SPLDAM_GENERIC, SPLDAM_NOSHRUG | SPLDAM_NODEFLECT,
			 &messages) == DAM_NONEDEAD)
	{
		add_event(event_dread_wave, PULSE_VIOLENCE, ch, vict, NULL, 0, &level,
			  sizeof(level));
		if (!number(0, 9) && !IS_GREATER_RACE(vict) && !IS_ELITE(vict))
			StopCasting(vict);
	}
}

void spell_dread_wave(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char vict,
		      P_obj /*obj*/)
{
	struct affected_type *af;

	if (!IS_ALIVE(ch) || !IS_ALIVE(vict))
	{
		return;
	}

	act("A &+Ldark, &+Cfreezing cold &+bwave&N sent by $n slowly covers $N making $M &+Lch&Nok&+Le &Nand freeze.",
	    TRUE, ch, 0, vict, TO_NOTVICT);
	act("A &+Ldark, &+Cfreezing cold &+bwave&N sent by $n slowly covers you making it hard to &+Lbr&Nea&+Lth&Ne.",
	    FALSE, ch, 0, vict, TO_VICT);
	act("You send a &+Ldark, &+Cbitterly cold &+bwave&N which slowly covers $N making $M &+Cfreeze and &+Lch&Nok&+Le.&N",
	    FALSE, ch, 0, vict, TO_CHAR);

	if (GET_RACE(vict) == RACE_W_ELEMENTAL)
	{
		send_to_char("&+CThe wave has no effect on &+Bwater elementals!\r\n", ch);
		return;
	}

	if ((af = (struct affected_type *)get_spell_from_char(vict, SPELL_DREAD_WAVE)) == NULL)
	{
		struct affected_type new_affect;

		af = &new_affect;
		memset(af, 0, sizeof(new_affect));
		af->type = SPELL_DREAD_WAVE;

		if (GET_SPEC(ch, CLASS_CONJURER, SPEC_WATER))
		{
			af->modifier = number(1, 3);
		}
		else
			af->modifier = 0;

		af->duration = 1;
		af->modifier += 2;
		affect_to_char(vict, af);
		add_event(event_dread_wave, 0, ch, vict, NULL, 0, &level, sizeof(level));
	}
	else
	{
		send_to_char(
			"&+BYour spell augments the &+Ldread waves&n &+Bcovering your victim!\r\n",
			ch);
		send_to_char("&+BThe bitterly icy flow surrounding you becomes more frigid!\r\n",
			     vict);

		if (GET_SPEC(ch, CLASS_CONJURER, SPEC_WATER))
		{
			af->modifier += 2;
		}
		else
			af->modifier += 1;
	}
}

void event_magma_burst(P_char ch, P_char vict, P_obj /*obj*/, void *data)
{
	int level, dam;
	struct affected_type *af;
	struct damage_messages messages = {
		"&+RThe fire burns hot as it consumes $N&+R's flesh!",
		"&+RThe fire burns hot as it consumes your flesh!",
		0,
		"$N screams in agony as &+Rthe flames&n consume $M completely.",
		"You scream in agony as &+Rthe flames&n consume you completely.",
		"$N screams in agony as &+Rthe flames&n consume $M completely.",
		0
	};

	if (!IS_ALIVE(vict))
	{
		return;
	}

	level = *((int *)data);

	for (af = vict->affected; af && af->type != SPELL_MAGMA_BURST; af = af->next)
	{
		;
	}

	if (af == NULL)
	{
		return;
	}

	if ((af->modifier)-- == 0)
	{
		send_to_char("&+RThe flames engulfing your body subside.\n", vict);
		affect_from_char(vict, SPELL_MAGMA_BURST);
		return;
	}

	dam = (3 * level) + number(0, 30);

	if (spell_damage(ch, vict, dam, SPLDAM_FIRE, SPLDAM_NODEFLECT, &messages) == DAM_NONEDEAD)
	{
		add_event(event_magma_burst, PULSE_VIOLENCE, ch, vict, NULL, 0, &level,
			  sizeof(level));
		if (5 > number(1, 10))
		{
			if (!IS_DRAGOON(vict))
				stop_memorizing(vict);
		}
	}
}

void spell_magma_burst(int level, P_char ch, char * /*arg*/, int type, P_char victim, P_obj /*obj*/)
{
	struct affected_type *af;

	if ((af = get_spell_from_char(victim, SPELL_MAGMA_BURST)) == NULL)
	{
		if (type >= 1)
		{
			act("A pillar of &+Wwhite flame &Nbellows from $n's gaping maw enveloping $N in a &+Rfi&+rery in&+Rferno&N.",
			    TRUE, ch, 0, victim, TO_NOTVICT);
			act("A pillar of &+Wwhite flame &Nbellows from $n's gaping maw enveloping you in a &+Rfi&+rery in&+Rferno&N.",
			    TRUE, ch, 0, victim, TO_VICT);
			act("A pillar of &+Wwhite flame &Nbellows from your gaping maw enveloping $N in a &+Rfi&+rery in&+Rferno&N.",
			    TRUE, ch, 0, victim, TO_CHAR);
		}
		else
		{
			act("Two pillars of &+Wwhite flame &Nleap from $n's outstreached palms enveloping $N in a &+Rfi&+rery in&+Rferno&N.",
			    TRUE, ch, 0, victim, TO_NOTVICT);
			act("Two pillars of &+Wwhite flame &Nleap from $n's outstreached palms enveloping you in a &+Rfi&+rery in&+Rferno&N.",
			    TRUE, ch, 0, victim, TO_VICT);
			act("Two pillars of &+Wwhite flame &Nleap from your outstreached palms enveloping $N in a &+Rfi&+rery in&+Rferno&N.",
			    TRUE, ch, 0, victim, TO_CHAR);
		}

		struct affected_type new_affect;

		af = &new_affect;
		memset(af, 0, sizeof(new_affect));
		af->type = SPELL_MAGMA_BURST;
		af->duration = 1;
		if (type > 1)
			af->modifier = 4; // dragoon dragon priest
		else
			af->modifier = 3; // anyone else
		affect_to_char(victim, af);
		add_event(event_magma_burst, 0, ch, victim, NULL, 0, &level, sizeof(level));

		// if( IS_ALIVE(ch) && IS_ALIVE(victim) )
		//   gain_exp(ch, victim, 0, EXP_DAMAGE);
	}
	else
	{
		if (type >= 1)
		{
			act("A pillar of &+Wwhite flame &Nbellows from $n's gaping maw, making the &+Rfi&+rery in&+Rferno&N burn &+Wbrighter&n.",
			    TRUE, ch, 0, victim, TO_NOTVICT);
			act("A pillar of &+Wwhite flame &Nbellows from $n's gaping maw, making the &+Rfi&+rery in&+Rferno&N burn &+Wbrighter&n.",
			    TRUE, ch, 0, victim, TO_VICT);
			act("A pillar of &+Wwhite flame &Nbellows from your gaping maw, making the &+Rfi&+rery in&+Rferno&N burn &+Wbrighter&n.",
			    TRUE, ch, 0, victim, TO_CHAR);
		}
		else
		{
			act("Two pillars of &+Wwhite flame &Nleap from $n's outstreached palms, making the &+Rfi&+rery in&+Rferno&N burn &+Wbrighter&n.",
			    TRUE, ch, 0, victim, TO_NOTVICT);
			act("Two pillars of &+Wwhite flame &Nleap from $n's outstreached palms, making the &+Rfi&+rery in&+Rferno&N burn &+Wbrighter&n.",
			    TRUE, ch, 0, victim, TO_VICT);
			act("Two pillars of &+Wwhite flame &Nleap from your outstreached palms, making the &+Rfi&+rery in&+Rferno&N burn &+Wbrighter&n.",
			    TRUE, ch, 0, victim, TO_CHAR);
		}

		af->modifier = (af->modifier < 3) ? 3 : af->modifier + 1;
	}
}

void event_nova(P_char ch, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	int room = *((int *)data);

	if (!number(0, 2))
	{
		send_to_room(
			"&+WA massive &+Yball of light&+W slowly throbs, sucking all light from every corner of this area...\n",
			room);

		add_event(event_nova, PULSE_VIOLENCE * 2, ch, 0, 0, 0, &room, sizeof(room));
		return;
	}
	resolve_nova(ch);
}

void resolve_nova(P_char ch)
{
	act("&+LYour immense &+Wgathering of light&+w comes to fruition, &+Yexploding with violent force!",
	    FALSE, ch, 0, 0, TO_CHAR);
	act("&+L$n's&+L immense &+Wgathering of light&+w comes to fruition, &+Yexploding with violent force!",
	    FALSE, ch, 0, 0, TO_ROOM);

	zone_spellmessage(
		ch->in_room, TRUE,
		"&+YT&+yh&+Yi&+yn &+Yr&+ya&+Yy&+ys of &+Yli&+ygh&+Yt &+rex&+Rplo&+rde &+ythroughout the &+Warea!\n",
		"&+YT&+yh&+Yi&+yn &+Yr&+ya&+Yy&+ys of &+Yli&+ygh&+Yt &+rex&+Rplo&+rde &+ythroughout the &+Warea to the %s!\n");

	cast_as_damage_area(ch, spell_sunray,
			    IS_NPC(ch) ? GET_LEVEL(ch) : (int)(GET_LEVEL(ch) * 0.6), NULL,
			    get_property("spell.area.minChance.nova", 90),
			    get_property("spell.area.chanceStep.nova", 10));
}

void spell_nova(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		P_obj /*obj*/)
{
	int room = ch->in_room;

	send_to_room(
		"&+YA point of light appears in the middle of the room, slowly growing larger!\n",
		ch->in_room);

	add_event(event_nova, PULSE_VIOLENCE * 2, ch, 0, 0, 0, &room, sizeof(room));
}

void spell_plague(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		  P_obj /*obj*/)
{
	struct affected_type af;
	// int timer;

	if (!ch)
		return;

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	if (ch == victim)
	{
		send_to_char("Your god refuses your wish!\r\n", ch);
		return;
	}

	if (affected_by_spell(victim, SPELL_PLAGUE))
	{
		send_to_char("This person is all sick already!\n", ch);
		return;
	}

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_PLAGUE);
	if (IS_NPC(victim) && !NewSaves(victim, SAVING_SPELL, mod + 5))
	{
		act("$n's skin &+cpales&n and sweat drips down $s body.", TRUE, victim, 0, 0,
		    TO_ROOM);
		act("Upon $n's touch you suddenly feel sick.", TRUE, ch, 0, victim, TO_VICT);

		bzero(&af, sizeof(af));
		af.type = SPELL_PLAGUE;
		af.duration = level / 12;
		af.modifier = 3500;
		affect_to_char(victim, &af);
	}
	else if (IS_PC(victim) && !NewSaves(victim, SAVING_SPELL, mod))
	{
		act("$n's skin &+cpales&n and sweat drips down $s body.", TRUE, victim, 0, 0,
		    TO_ROOM);
		act("Upon $n's touch you suddenly feel sick.", TRUE, ch, 0, victim, TO_VICT);

		bzero(&af, sizeof(af));
		af.type = SPELL_PLAGUE;
		af.duration = (int)get_property("spell.plague.duration", 2);
		af.modifier = 500;
		affect_to_char(victim, &af);
	}
	else
	{
		act("&+LYour plague fails to afflict $N", FALSE, ch, 0, victim, TO_CHAR);
	}

	// timer = (int)get_property("spell.plague.spread.time", 60);
	// if(!get_scheduled(victim, event_plague))
	//   add_event(event_plague, WAIT_SEC * number(timer-20, timer+20), victim, 0, 0, 0, 0, 0);
}
