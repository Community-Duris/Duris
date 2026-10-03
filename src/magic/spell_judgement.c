#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "world/graph.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include "magic/spell_words_of_power.h"
#include <strings.h>

struct judgement_data
{
	int room;
	int data;
	int devotion;
};

static void spell_oldjudgement(int level, P_char ch, P_char victim, P_obj /*obj*/)
{
	P_char t, t_next;
	int lev, /*minalign, maxalign, dam, */ door, target_room, the_room, i, max_affected;

	if (!IS_ALIVE(ch))
	{
		return;
	}

	if (GET_LEVEL(ch) < MINLVLIMMORTAL)
	{
		if (GET_ALIGNMENT(ch) < 900)
		{
			send_to_char("Your god is not pleased with you, and reacts accordingly.",
				     ch);
			die(ch, ch);
			return;
		}

		if ((GET_ALIGNMENT(ch) > 899) && (GET_ALIGNMENT(ch) < 980))
		{
			send_to_char(
				"You have angered your god in some way, and refuses your request for aid!",
				ch);
			return;
		}
	}

	if (victim)
	{
		the_room = victim->in_room;
	}
	else
	{
		the_room = ch->in_room;
	}

	max_affected = (int)(get_property("spell.judgement.maxAffected", 4.000));

	i = 0;

	for (t = world[the_room].people; t; t = t_next)
	{
		t_next = t->next_in_room;

		if (should_area_hit(ch, t) && !number(0, 2))
		{
			if (GET_ALIGNMENT(t) > -351)
			{
				act("$N is not evil enough to be affected!", TRUE, ch, 0, t,
				    TO_CHAR);
			}
			else
			{
				if ((lev = GET_LEVEL(t)) < (level - 25))
				{ /* < 26 death */
					die(t, ch);
					continue;
				}
				else if (!resists_spell(ch, t))
				{
					if (lev <= (46)) /* <46 para */
					{
						spell_major_paralysis(-level, ch, 0, 0, t,
								      NULL); /* no save */
					}

					if (lev <= (53)) /* <53 blind */
					{
						spell_blindness(-level, ch, 0, 0, t,
								NULL); /* no save */
					}

					if ((GET_RACE(t) == RACE_DEMON) ||
					    (GET_RACE(t) == RACE_DEVIL))
					{
						spell_slow(
							-level, ch, 0, 0, t,
							NULL); /* nasty spells if we're really evil demons and devils */
						spell_dispel_magic(-level, ch, 0, 0, t, NULL);
					}

					if (!NewSaves(t, SAVING_FEAR,
						      MIN((GET_LEVEL(ch) - GET_LEVEL(t)), 5)))
					{
						door = number(0, NUM_EXITS - 1);

						if ((CAN_GO(t, door)) &&
						    (!check_wall(t->in_room, door)))
						{
							act("The power of your spell sends $N flying out of the room!",
							    FALSE, ch, 0, t, TO_CHAR);
							act("The power of $n's spell sends you flying out of the room!",
							    FALSE, ch, 0, t, TO_VICT);
							act("The power of $n's spell sends $N flying out of the room!",
							    FALSE, ch, 0, t, TO_NOTVICT);

							target_room = world[t->in_room]
									      .dir_option[door]
									      ->to_room;
							char_from_room(t);
							char_to_room(t, target_room, -1);

							act("$n flies in, crashing on the floor!",
							    TRUE, t, 0, 0, TO_ROOM);

							SET_POS(t, POS_PRONE + GET_STAT(t));

							stop_fighting(t);
							if (CAN_ACT(t))
							{
								Stun(t, ch, PULSE_VIOLENCE * 2,
								     FALSE);
								CharWait(t, PULSE_VIOLENCE * 2);
							}
						}
						else
						{
							act("Your spell sends $N crashing into the wall!",
							    FALSE, ch, 0, t, TO_CHAR);
							act("The power of $n's spell sends you crashing into the wall!",
							    FALSE, ch, 0, t, TO_VICT);
							act("The power of $n's spell sends $N crashing into the wall!",
							    FALSE, ch, 0, t, TO_NOTVICT);

							SET_POS(t, POS_SITTING + GET_STAT(t));

							stop_fighting(t);

							if (CAN_ACT(t))
							{
								Stun(t, ch, PULSE_VIOLENCE * 2,
								     FALSE);
								CharWait(t, PULSE_VIOLENCE * 3);
							}
						}
					}
					astral_banishment(ch, t, BANISHMENT_HOLY_WORD, level);
				}

				i++;
			}

			if (i >= max_affected)
			{
				break;
			}
		}
	}
	CharWait(ch, PULSE_VIOLENCE * 2);
}

static void event_judgement(P_char ch, P_char victim, P_obj /*obj*/, void *data)
{
	struct affected_type af;
	struct judgement_data *j = (struct judgement_data *)data;

	P_char tch, sunray_target = NULL;
	int opponents, damage, vict_index, repeats, rnumber;
	bool handled;
	struct damage_messages messages = {
		"$N falls to $S knees screaming in pain and begging everyone to forgive $M.",
		"&+WYour &+wd&+Lar&+wk &+Wsoul screams out in pain as all &+wyour deeds &+Ware revealed!",
		"$N falls to $S knees screaming in pain and begging everyone to forgive $M.",
	};

	if (!IS_ALIVE(ch))
	{
		return;
	}

	if (j->data == 0)
	{
		j->data += 1;

		act("&+wAs &+Wheavenly &+Ylight &+wdescends upon the battlefield, both friend and foe stare skyward\n"
		    "&+win awe. The tension builds until the very air crackles with &+Yenergy&+w.  A &+Cgust &+wof &+Wdivine\n"
		    "&+wwind hurtles down from the &+WHeavens &+wseeking out the impure hearts of &+Levil.&n",
		    FALSE, ch, 0, 0, TO_ROOM);
		act("&+wAs &+Wheavenly &+Ylight &+wdescends upon the battlefield, both friend and foe stare skyward\n"
		    "&+win awe. The tension builds until the very air crackles with &+Yenergy&+w.  A &+Cgust &+wof &+Wdivine\n"
		    "&+wwind hurtles down from the &+WHeavens &+wseeking out the impure hearts of &+Levil.&n",
		    FALSE, ch, 0, 0, TO_CHAR);

		add_event(event_judgement, PULSE_VIOLENCE / 2, ch, 0, 0, 0, j, sizeof(*j));
		return;
	}

	if (number(0, 1))
	{
		act("&+WBOOO&+wOOOO&+LOOM! &+WDivine &+Ypower &+wsweeps through the room slamming into the ranks of\n"
		    "&+wthe &+rfoul&+L-&+rhearted &+Lcreatures&+w, who writhe in agony.&n",
		    FALSE, ch, 0, 0, TO_ROOM);
		act("&+WBOOO&+wOOOO&+LOOM! &+WDivine &+Ypower &+wsweeps through the room slamming into the ranks of\n"
		    "&+wthe &+rfoul&+L-&+rhearted &+Lcreatures&+w, who writhe in agony.&n",
		    FALSE, ch, 0, 0, TO_CHAR);

		spell_oldjudgement(GET_LEVEL(ch), ch, 0, 0);

		return;
	}

	if (ch->in_room != j->room)
	{
		send_to_char("&+WThe forces of light are unable to aid you!\n", ch);
		return;
	}

	opponents = 0;

	if (!IS_MAGIC_LIGHT(ch->in_room))
	{
		spell_continual_light(50, ch, 0, 0, NULL, NULL);
	}

	for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
	{
		if (ch != tch && !grouped(ch, tch) && GET_ALIGNMENT(tch) < 0)
		{
			opponents++;
			sunray_target = tch;
		}
	}

	if (!opponents)
	{
		return;
	}

	if (opponents <= get_property("spell.judgement.maxAffected", 4.000) &&
	    should_area_hit(ch, sunray_target))
	{
		spell_holy_word(50, ch, NULL, 0, NULL, NULL);
	}
	else if (IS_FIGHTING(ch))
	{
		spell_sunray(-(GET_LEVEL(ch)), ch, NULL, 0, GET_OPPONENT(ch), 0);
	}
	else if (sunray_target)
	{
		spell_sunray(-(GET_LEVEL(ch)), ch, NULL, 0, sunray_target, 0);
	}

	if (!IS_ALIVE(ch))
	{
		return;
	}

	repeats = (opponents > 0) ? (1 + opponents / 3) : 0;

	while (repeats--)
	{
		vict_index = number(1, opponents);

		for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
		{
			if (ch != tch && !grouped(ch, tch) && GET_ALIGNMENT(tch) < 0)
			{
				if (--vict_index == 0)
				{
					break;
				}
			}
		}
		// its possible (somehow) that the above for() loop is breaking
		// because tch is NULL.  When that happens, the below code is crashing
		// when the null pointer is dereferenced.  Deal with it:
		if (!tch || IS_TRUSTED(tch))
		{
			continue;
		}

		victim = tch;

		handled = FALSE;

		damage = dice(20, (int)(GET_LEVEL(ch) *
					get_property("spell.judgement.dam.modifier", 0.500)));

		if (GET_CLASS(victim, CLASS_ANTIPALADIN | CLASS_NECROMANCER) && IS_PC(ch))
		{
			send_to_char(
				"The &+Wholy light&n &+Rbur&+rns&n at your very existence!!!\n",
				victim);

			damage = (int)(damage *
				       get_property("spell.judgement.dam.modifier.apnecro", 1.000));

			if (spell_damage(ch, victim, damage, SPLDAM_HOLY, RAWDAM_NOKILL, &messages))
			{
				if (!IS_ALIVE(ch))
				{
					return;
				}

				opponents--;
			}
		}
		else
		{
			if (spell_damage(ch, victim, damage, SPLDAM_HOLY, RAWDAM_NOKILL, &messages))
			{
				if (!IS_ALIVE(ch))
				{
					return;
				}

				opponents--;
			}
		}

		if (GET_RACE(victim) == RACE_TROLL)
		{
			if (!NewSaves(victim, SAVING_FEAR, 3) &&
			    !check_freedom_of_movement(victim, true))
			{
				bzero(&af, sizeof(af));

				af.type = SPELL_MAJOR_PARALYSIS;
				af.flags = AFFTYPE_SHORT;
				af.duration = (int)(1.5 * PULSE_VIOLENCE);
				af.bitvector2 = AFF2_MAJOR_PARALYSIS;

				affect_to_char(victim, &af);

				send_to_char(
					"You momentarily turn to &+Lstone&n as the &+Wholy light&n shines upon your &+gskin&N!\n",
					victim);
				act("$n momentarily turns to &+Lstone&n as the &+Wholy light&n shines upon $s skin!&N",
				    FALSE, victim, 0, 0, TO_ROOM);

				handled = TRUE;
			}
		}
		else if (IS_UNDEADRACE(victim) || IS_PUNDEAD(victim))
		{
			if (!number(0, 2))
			{
				send_to_char(
					"&+WSuddenly a g&+wh&+Wa&+ws&+Wt&+wl&+Wy image appears in front of you, and you begin to remember...\n&n",
					victim);
				send_to_char(
					"Your &+WSOUL&n stares at you: '&+WWhy couldn't my body find rest? Sleep my poor thing.&n'\n",
					victim);

				rnumber = number(1, 3);

				bzero(&af, sizeof(af));

				af.type = SPELL_SLEEP;
				af.flags = AFFTYPE_SHORT;
				af.duration = rnumber * PULSE_VIOLENCE;
				af.bitvector = AFF_SLEEP;

				stop_fighting(victim);

				if (GET_STAT(victim) > STAT_SLEEPING)
				{
					act("$n &+Wenters tupor.", TRUE, victim, 0, 0, TO_ROOM);
					SET_POS(victim, GET_POS(victim) + STAT_SLEEPING);
				}

				affect_to_char(victim, &af);

				StopMercifulAttackers(victim);

				handled = TRUE;
			}
		}

		if (!handled)
		{
			if (!number(0, 2))
			{
				act("&+WYou suddenly realize how &+Lwrong &+Wyour life has been!\n"
				    "&+WYou feel compelled to change your ways, but the feeling goes away...\n",
				    TRUE, victim, 0, 0, TO_CHAR);
				act("As a ray of &+Wheavenly light&n shines upon $n, $e suddenly relaxes and sinks deeply into thought.&n",
				    TRUE, victim, 0, victim, TO_ROOM);

				stop_fighting(victim);

				CharWait(victim, PULSE_VIOLENCE * 3);
			}
			else if (!number(0, 2) || IS_AFFECTED4(victim, AFF4_NOFEAR))
			{
				spell_damage(ch, victim, dice(10, GET_LEVEL(ch)), SPLDAM_HOLY,
					     RAWDAM_NOKILL, &messages);

				if (!IS_ALIVE(ch))
				{
					return;
				}

				SET_POS(victim, POS_SITTING + GET_STAT(victim));
				CharWait(victim, PULSE_VIOLENCE * 2);
			}
			else
			{
				send_to_char(
					"&+RJudged and humiliated, you desperately search for &+Wescape...!\n",
					victim);
				act("As a ray of &+Wheavenly light&n shines upon $n, $e tries to escape &+Wjustice&n.",
				    TRUE, victim, 0, victim, TO_ROOM);

				do_flee(victim, 0, 1);

				CharWait(victim, PULSE_VIOLENCE * 2);
			}
		}

		if (!is_char_in_room(victim, ch->in_room))
		{
			opponents--;
		}
	}

	if ((GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 5 > number(1, 100)) && IS_ALIVE(ch) &&
	    j->devotion < 3)
	{
		j->devotion += 1;

		act("&+WThe divine &+Ypower &+wcontinues to sweep through the area!!!&n", FALSE, ch,
		    0, 0, TO_ROOM);

		add_event(event_judgement, PULSE_VIOLENCE, ch, 0, 0, 0, j, sizeof(*j));
	}
}

void spell_judgement(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*vict*/,
		     P_obj /*obj*/)
{
	struct judgement_data j;
	P_char tch;

	if (!IS_ALIVE(ch))
	{
		return;
	}

	if (GET_LEVEL(ch) < MINLVLIMMORTAL)
	{
		if (GET_ALIGNMENT(ch) < 900)
		{
			act("A &+Rblood red&n ray from the &+Wheavens&n strikes $N!!!&n", TRUE, ch,
			    0, 0, TO_ROOM);
			send_to_char("Your god is not pleased with you, and reacts accordingly.",
				     ch);
			die(ch, ch);
			return;
		}

		if ((GET_ALIGNMENT(ch) > 899) && (GET_ALIGNMENT(ch) < 980))
		{
			send_to_char(
				"You have angered your god in some way, and he refuses your request for aid!",
				ch);
			return;
		}
	}

	appear(ch);

	for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
	{
		// Only one judgement per room for players. 2 Apr 09 -Lucrot
		if (IS_PC(ch) && get_scheduled(tch, event_judgement))
		{
			send_to_char("Your call upon the &+Wforces&n of &+WLight&N fails!\n", ch);
			return;
		}
	}

	send_to_char("You call upon the &+Wforces&n of &+WLight&N to aid you.\n", ch);
	act("$n calls upon the &+Wforces&n of &+WLight&N!", FALSE, ch, 0, 0, TO_ROOM);

	j.room = ch->in_room;
	j.data = 0;
	j.devotion = 0;

	add_event(event_judgement, PULSE_VIOLENCE / 2, ch, 0, 0, 0, &j, sizeof(j));
}
