#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include "world/events.h"
#include <string.h>
#include <strings.h>
void event_holy_dharma(P_char ch, P_char /*victim*/, P_obj /*obj*/, void * /*data*/)
{
	int hits = 0, maxhits = 0;
	P_char opponent;

	if (!ch) // Something amiss.
	{
		logit(LOG_EXIT, "event_holy_dharma called in magic.c without ch");
		return;
	}
	if (ch) // Just making sure.
	{
		if (!IS_ALIVE(ch))
		{
			send_to_char("Lay still, you seem to be dead.\r\n", ch);
			return;
		}
		if (!IS_AFFECTED5(ch, AFF5_HOLY_DHARMA))
		{
			return;
		}
		if (!IS_AFFECTED2(ch, AFF2_SOULSHIELD))
		{
			send_to_char("Your soul is not properly prepared.\r\n", ch);
			if (IS_AFFECTED5(ch, AFF5_HOLY_DHARMA))
			{
				affect_from_char(ch, SPELL_HOLY_DHARMA);
			}
			return;
		}
		if (IS_AFFECTED5(ch, AFF5_HOLY_DHARMA))
		{
			if (IS_FIGHTING(ch))
			{
				opponent = GET_OPPONENT(ch);

				if (GET_HIT(opponent) > GET_MAX_HIT(opponent) && opponent)
				{
					hits = GET_HIT(opponent) - GET_MAX_HIT(opponent);
					maxhits =
						(int)(25 + (GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 2 *
							    get_property("vamping.Dharma.maxhitMod",
									 1.000)));
					hits = BOUNDED(1, hits, maxhits);
					act("&+wYou have a marvelous &+Ycosmic &+wrevelation. An &+Cunseen force &+wflows into you!&n",
					    FALSE, ch, 0, opponent, TO_CHAR);
					act("Your &+Llifeforce&n is assimilated into $n's body!",
					    FALSE, ch, 0, opponent, TO_VICT);
					act("&+WTranquility&n spreads across $n's face.", FALSE, ch,
					    0, opponent, TO_NOTVICT);

					GET_HIT(opponent) -= hits;
					GET_HIT(ch) += hits;

					if (maxhits >= 45 && opponent)
					{
						if (GET_CHAR_SKILL(ch, SKILL_DEVOTION) >= 20 &&
						    IS_AFFECTED(ch, AFF_BLIND))
						{
							spell_cure_blind(GET_LEVEL(ch), ch, NULL,
									 SPELL_TYPE_SPELL, ch, 0);
						}
						if (GET_CHAR_SKILL(ch, SKILL_DEVOTION) >= 60 &&
						    affected_by_spell(ch, SPELL_WITHER))
						{
							affect_from_char(ch, SPELL_WITHER);
						}
						if (GET_CHAR_SKILL(ch, SKILL_DEVOTION) >= 100 &&
						    !IS_AFFECTED4(ch, AFF4_SANCTUARY))
						{
							spell_sanctuary(GET_LEVEL(ch), ch, NULL,
									SPELL_TYPE_SPELL, ch, 0);
						}
					}
				}
			}
			if (IS_ALIVE(ch))
			{
				add_event(event_holy_dharma, PULSE_VIOLENCE * number(3, 8), ch, ch,
					  0, 0, 0, 0);
			}
		}
	}
}

void spell_holy_dharma(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		       P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!ch) // Something amiss. Nov08 -Lucrot
	{
		logit(LOG_EXIT, "spell_holy_dharma called in magic.c without ch");
		return;
	}
	if (ch) // Just making sure.
	{
		if (!IS_ALIVE(victim))
		{
			send_to_char("&+RLay still, you seem to be dead.\r\n", ch);
			return;
		}
		if (IS_AFFECTED5(victim, AFF5_HOLY_DHARMA))
		{
			send_to_char("&+cYour soul is already brushing upon the vast cosmos.\r\n",
				     ch);
			return;
		}
		if (!IS_AFFECTED2(victim, AFF2_SOULSHIELD))
		{
			send_to_char(
				"&+cYour soul is not properly prepared to embrace &+Crighteousness.\r\n",
				ch);
			return;
		}

		bzero(&af, sizeof(af));
		af.type = SPELL_HOLY_DHARMA;
		af.duration = (int)(5 + GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 5);
		af.bitvector5 = AFF5_HOLY_DHARMA;
		affect_to_char(victim, &af);

		send_to_char("&+cYou cast your soul into the cosmos seeking &+Wrighteousness.\r\n",
			     ch);

		add_event(event_holy_dharma, (PULSE_VIOLENCE * number(3, 6)), ch, ch, 0, 0, 0, 0);

		return;
	}
}

void spell_restore_spirit(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			  P_char victim, P_obj /*tar_obj*/)
{
	int result, dam;

	struct damage_messages messages = {
		"You restore your soul with the help of $N's soul.&n",
		"You feel less &+renergetic&n as $n uses your soul to restore his spirit.",
		"A &n&+Wholy &n&+Yglow&n around $n &n&+yre&n&+Ypl&n&+Weni&n&+Ysh&n&+yes&n their health and endurance.",
		"$N crumples as you &+Rkill&n $M by draining $S &+rspirit.&n",
		"&+LYour neurons fry as&n $n &+Rspirit seeps away!&n",
		"$n drains $N's spirit, draining $M of every last bit of $S &+rspirit!&n"
	};

	bool saved = FALSE;

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim) || victim == ch)
	{
		return;
	}

	if (IS_ANGEL(victim))
	{
		send_to_char("&+wThat wouldn't be very nice, they are angelic.\r\n", ch);
		return;
	}

	if (resists_spell(ch, victim))
	{
		return;
	}

	dam = (int)((level * 2.5) + number(-10, 10));

	if (IS_PC(ch) && !(GET_CLASS(ch, CLASS_THEURGIST | CLASS_PALADIN)))
	{
		send_to_char(
			"&+rLacking the proper training in divinity, you do not utilize the full potential of the spell!\r\n",
			ch);
		dam = (int)(dam * 0.80);
	}

	if (IS_AFFECTED4(victim, AFF4_DEFLECT))
	{
		if (GET_LEVEL(ch) >= 50)
		{
			dam <<= 1;
		}

		spell_damage(ch, victim, dam, SPLDAM_HOLY, 0, &messages);
		return;
	}

	if (saves_spell(victim, SAVING_SPELL))
	{
		saved = TRUE;
		dam >>= 1;
	}

	if (GET_LEVEL(ch) >= 50)
	{
		dam <<= 1;
	}

	vamp(ch, (int)(dam / 4), (int)(GET_MAX_HIT(ch) * VAMPPERCENT(ch)));

	if (GET_VITALITY(victim) >= 10 && !IS_AFFECTED2(victim, AFF2_SOULSHIELD))
	{
		GET_VITALITY(victim) = MAX(10, GET_VITALITY(victim) - 5);
		GET_VITALITY(ch) += 10;
	}

	StartRegen(ch, regen_resource::vitality);
	StartRegen(victim, regen_resource::vitality);

	result = spell_damage(ch, victim, dam, SPLDAM_HOLY, SPLDAM_NOSHRUG, &messages);

	if (result == DAM_NONEDEAD && !saved)
	{
		if (IS_AFFECTED2(victim, AFF2_SOULSHIELD))
		{
			send_to_char(
				"&+LYour soulshield protects you from lasting effects from the restore spell!&n\r\n",
				victim);
			send_to_char(
				"&+LYour victim is too well protected against divine power - no lingering effects of the restore spell will hold...&n\r\n",
				ch);
			return;
		}
		struct affected_type af;
		memset(&af, 0, sizeof(af));
		af.type = SPELL_RESTORE_SPIRIT;

		af.duration =
			number(GET_LEVEL(ch) / 4, saved ? (GET_LEVEL(ch) / 2) : GET_LEVEL(ch)) *
			PULSE_VIOLENCE;
		af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL;

		af.location = APPLY_MOVE;
		af.modifier = -(MIN(saved ? 7 : 15, GET_VITALITY(victim)));

		if (affected_by_spell(victim, SPELL_RESTORE_SPIRIT))
		{
			send_to_char(
				"&+LThey're already affected by a restore spell - you only prolong and enhance the buzz!",
				ch);
			send_to_char("&+LYour buzz is enhanced as another restore spell hits you!",
				     victim);
			affect_join(victim, &af, FALSE, FALSE);
			return;
			affect_to_char_with_messages(
				victim, &af,
				"&+LYour life energy was drained, leaving you a bit shaken.",
				"&+LYou manage to shake off negative effects of the restore spell.");
		}
	}
}

void spell_enervation(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		      P_char victim, P_obj /*tar_obj*/)
{
	int result, dam;

	struct damage_messages messages = {
		"You enervate $N, &+Ldraining&n $M of some of $S &+renergy.&n",
		"You feel less &+renergetic&n as $n enervates you.",
		"$n enervates $N, leaving $M &+yvisibly shaken!&n",
		"$N crumples as you &+Rkill&n $M by draining $S &+renergy.&n",
		"&+LYour neurons fry as&n $n &+Renervates &+Lthem and drains you drains your last bit of energy!&n",
		"$n enervates $N, draining $M of every last bit of $S &+renergy!&n"
	};

	bool saved = FALSE;

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim) || victim == ch)
	{
		return;
	}

	if (IS_UNDEADRACE(victim))
	{
		send_to_char("&+LEnervating the undead is impossible.\r\n", ch);
		return;
	}

	// Shrug
	if (resists_spell(ch, victim))
	{
		return;
	}

	dam = (int)((level * 2.75) + number(-10, 10));

	if (IS_PC(ch) && !(GET_CLASS(ch, CLASS_NECROMANCER | CLASS_ANTIPALADIN)))
	{
		dam = (int)(dam * 0.80);
	}

	if (IS_AFFECTED4(victim, AFF4_DEFLECT))
	{
		if (GET_LEVEL(ch) >= 50)
		{
			dam <<= 1;
		}
		spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, 0, &messages);
		return;
	}

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_ENERVATION);
	// Made it harder to save against.
	if (NewSaves(victim, SAVING_SPELL, mod))
	{
		saved = TRUE;
		dam >>= 1;
	}

	if (GET_LEVEL(ch) >= 50)
	{
		dam <<= 1;
	}

	vamp(ch, (int)(dam / 4), (int)(GET_MAX_HIT(ch) * VAMPPERCENT(ch)));

	if (GET_VITALITY(victim) >= 10 && !IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
	{
		GET_VITALITY(victim) = MAX(10, GET_VITALITY(victim) - 5);
		GET_VITALITY(ch) += 10;
	}

	StartRegen(ch, regen_resource::vitality);
	StartRegen(victim, regen_resource::vitality);

	result = spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_NOSHRUG, &messages);

	if (result == DAM_NONEDEAD && !saved)
	{
		if (IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
		{
			send_to_char(
				"&+LYour negative energy shield protects you from lasting effects from the enervation spell!&n\r\n",
				victim);
			send_to_char(
				"&+LYour victim is too well protected against necromancy - no lingering effects of the enervation spell will hold...&n\r\n",
				ch);
			return;
		}
		struct affected_type af;
		memset(&af, 0, sizeof(af));
		af.type = SPELL_ENERVATION;

		af.duration =
			number(GET_LEVEL(ch) / 4, saved ? (GET_LEVEL(ch) / 2) : GET_LEVEL(ch)) *
			PULSE_VIOLENCE;
		af.flags = AFFTYPE_SHORT | AFFTYPE_NODISPEL;

		af.location = APPLY_MOVE;
		af.modifier = saved ? -7 : -15;
		// Do not put them below 1 max movement point: the prompt does not like that.
		if (af.modifier < -GET_MAX_VITALITY(victim) + 1)
			af.modifier = -GET_MAX_VITALITY(victim) + 1;

		if (affected_by_spell(victim, SPELL_ENERVATION))
		{
			send_to_char(
				"&+LThey're already affected by enervation - you only prolong and enhance the suffering!\n\r",
				ch);
			send_to_char(
				"&+LYour suffering is enhanced as another enervation spell hits you!\n\r",
				victim);
			affect_join(victim, &af, FALSE, FALSE);
			return;
		}

		affect_to_char_with_messages(
			victim, &af, "&+LYour life energy was drained, leaving you a bit shaken.",
			"&+LYou manage to shake off the negative effects of the enervation spell.");
	}
}

void spell_life_leech(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		      P_obj /*obj*/)
{
	int dam;

	struct damage_messages messages = {
		"&+LYou reach out and touch $N, &+rleeching &+Lsome of $S &+Llife&+wfor&+Wce.&n",
		"&+LYour &+rlife &+Lforce seems to slip away as&n $n &+Ltouches you.&n",
		"$n &+Lseems to suck the &+rlife &+Lright out of&n $N!",
		"$N &+rapparently has no more &+Rlife &+rto leech!&n",
		"&+LAs&n $n &+Ltouches you, you feel the last bit of your &+rlife &+Lseep out of you.&n",
		"$n &+Lsucks the last bit of &+rlife &+Lout of &n$N &+Lwho falls to the &+yground &+Llifeless."
	};

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim) || victim == ch)
	{
		return;
	}

	if (resists_spell(ch, victim))
		return;

	dam = dice(1.5 * level, 5);

	if (IS_AFFECTED4(victim, AFF4_DEFLECT))
	{
		spell_damage(ch, victim, dam, SPLDAM_GENERIC,
			     SPLDAM_NOSHRUG | SPLDAM_NOVAMP | SPLDAM_NODEFLECT, &messages);
		return;
	}

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_LIFE_LEECH);
	if (NewSaves(victim, SAVING_SPELL, mod))
	{
		dam = (int)(dam * 0.80);
	}

	if (GET_LEVEL(victim) <= (level / 10))
	{
		/*
		 * Kill the sucker
		 */
		act(messages.death_attacker, FALSE, ch, 0, victim, TO_CHAR);
		act(messages.death_victim, FALSE, ch, 0, victim, TO_VICT);
		act(messages.death_room, FALSE, ch, 0, victim, TO_NOTVICT);
		die(victim, ch);
		victim = NULL;
	}
	else
	{
		if (!IS_AFFECTED4(victim, AFF4_NEG_SHIELD) && !IS_UNDEADRACE(victim))
		{
			if (IS_PC(ch) || IS_PC_PET(ch))
			{
				vamp(ch, (int)(dam / 5), (int)(GET_MAX_HIT(ch) * 1.00));
			}
			else
				vamp(ch, (int)(dam / 2), (int)(GET_MAX_HIT(ch) * 1.00));
		}

		StartRegen(ch, regen_resource::vitality);
		StartRegen(victim, regen_resource::vitality);

		// Vamping still occurs as above. We do not want double vamping undead.
		spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_NOSHRUG | SPLDAM_NOVAMP,
			     &messages);
	}

	if (!IS_ALIVE(victim) || !IS_ALIVE(ch))
		return;
}

void spell_energy_drain(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			P_char victim, P_obj /*obj*/)
{
	int dam, moves;
	bool saved = FALSE;

	struct damage_messages messages = {
		"You drain $N of some of $S &+Wenergy.&n",
		"&+LYou feel less energetic as&n $n &+Ldrains you.&n",
		"$n &+rdrains&n $N - what a waste of energy!",
		"$N &+rcrumples as you kill&n $M by draining $S energy.",
		"&+LAs&n $n &+Ldrains your last bit of energy, you look forward to the peace of the graveyard.&n",
		"$n &+Ldrains the &+Wenergy&n of $N who crumbles into a lifeless husk."
	};

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim) || victim == ch)
	{
		return;
	}

	if (resists_spell(ch, victim))
		return;

	// 20% increase in level (lowered from 25%, but added to actual hps damage).
	if (GET_SPEC(ch, CLASS_NECROMANCER, SPEC_REAPER) ||
	    GET_SPEC(ch, CLASS_THEURGIST, SPEC_THAUMATURGE))
	{
		level = (int)(level * get_property("damage.increase.reaper", 1.200));
	}

	// dam = dice(3 * level, 5);
	// At level 56: 168 to 840 -> 336 + (56 to 280) = 392 to 616
	// Avg stays 504 == 126 real damage
	// But new range is 224 == 56 real as opposed to 672 == 168 real (+/- 28 vs 84).
	dam = (6 * level) + dice(level, 5);

	if (IS_AFFECTED4(victim, AFF4_DEFLECT))
	{
		spell_damage(ch, victim, dam, SPLDAM_GENERIC,
			     SPLDAM_NOSHRUG | SPLDAM_NOVAMP | SPLDAM_NODEFLECT, &messages);
		return;
	}

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_ENERGY_DRAIN);
	if (NewSaves(victim, SAVING_SPELL, mod))
	{
		saved = TRUE;
		dam = (int)(dam * 0.80);
	}

	if (GET_LEVEL(victim) <= (level / 10))
	{
		/*
		 * Kill the sucker
		 */
		act(messages.death_attacker, FALSE, ch, 0, victim, TO_CHAR);
		act(messages.death_victim, FALSE, ch, 0, victim, TO_VICT);
		act(messages.death_room, FALSE, ch, 0, victim, TO_NOTVICT);
		die(victim, ch);
		victim = NULL;
	}
	else
	{
		if (!IS_AFFECTED4(victim, AFF4_NEG_SHIELD) && !IS_UNDEADRACE(victim))
		{
			if (IS_PC(ch) || IS_PC_PET(ch))
			{
				vamp(ch, (int)(dam / 5), (int)(GET_MAX_HIT(ch) * VAMPPERCENT(ch)));
			}
			else
			{
				vamp(ch, (int)(dam / 2), (int)(GET_MAX_HIT(ch) * VAMPPERCENT(ch)));
			}
		}

		if (GET_SPEC(ch, CLASS_NECROMANCER, SPEC_REAPER))
		{
			send_to_char("&+LYour life energy is &+rtapped&+L.\n", victim);
			if (GET_VITALITY(victim) >= 25 && !IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
			{
				moves = number(5, 30); // old value (5, level)

				if (affected_by_spell(victim, SPELL_ENERGY_DRAIN))
				{
					moves /= 2;
				}

				GET_VITALITY(victim) = MAX(1, (GET_VITALITY(victim) - moves));
				GET_VITALITY(ch) += moves;
				debug("E DRAIN: (%s&n) loses and (%s&n) gained (%d) moves.",
				      J_NAME(victim), J_NAME(ch), moves);
			}
			StartRegen(ch, regen_resource::vitality);
			StartRegen(victim, regen_resource::vitality);
		}

		// Vamping still occurs as above. We do not want double vamping undead.
		spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_NOSHRUG | SPLDAM_NOVAMP,
			     &messages);
	}

	if (!IS_ALIVE(victim) || !IS_ALIVE(ch))
	{
		return;
	}

	if (IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
	{
		send_to_char(
			"&+LYour negative energy shield protects you from lasting effects from the energy drain!&n\r\n",
			victim);
		send_to_char(
			"&+LYour victim is too well protected against necromancy - no lingering effects of the energy drain will hold.&n\r\n",
			ch);
		return;
	}

	if (affected_by_spell(victim, SPELL_ENERGY_DRAIN))
	{
		struct affected_type *af1;
		for (af1 = victim->affected; af1; af1 = af1->next)
		{
			if (af1->type == SPELL_ENERGY_DRAIN)
			{
				af1->duration += 1;
			}
		}
	}
	else if (!saved)
	{
		// send_to_char("&+LYour spell saps the energy from your foe, leaving you invigorated in return.\r\n", ch);
		// send_to_char("&+LYour energy is sapped! You feel sluggish and weak...\r\n", victim);
		struct affected_type af;
		memset(&af, 0, sizeof(af));
		af.type = SPELL_ENERGY_DRAIN;
		af.flags = AFFTYPE_NODISPEL | AFFTYPE_SHORT;
		af.duration = (level / 10) * WAIT_SEC;

		af.location = APPLY_MOVE_REG;
		af.modifier = -(level / 20);
		affect_to_char(victim, &af);

		af.location = APPLY_HIT_REG;
		af.modifier = -(level / 20);
		affect_to_char(victim, &af);
	}
	send_to_char("&+LYour life energy was drained, leaving you a bit shaken.\r\n", victim);
}

void spell_vampiric_touch(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			  P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	int duration_i = (int)(level / 5);

	if (IS_AFFECTED2(victim, AFF2_VAMPIRIC_TOUCH))
	{
		act("$N is already affected by the spell!", TRUE, ch, 0, victim, TO_CHAR);
		return;
	}
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_VAMPIRIC_TOUCH;
		af.location = APPLY_NONE;
		af.modifier = level;
		af.duration = BOUNDED(2, duration_i, 10);
		af.bitvector2 = AFF2_VAMPIRIC_TOUCH;
		affect_to_char(ch, &af);
		af.location = APPLY_HITROLL;
		af.modifier = 3 + level / 8;
		affect_to_char(ch, &af);
	}
	act("$n's hands start to glow &+RRED&n as blood..", FALSE, ch, 0, 0, TO_ROOM);
	act("Your hands start to glow &+RRED&n as blood..", FALSE, ch, 0, 0, TO_CHAR);
}
