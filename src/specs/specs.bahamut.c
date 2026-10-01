/* Bahamut special procedures. */

#include <time.h>

#include "core/prototypes.h"
#include "item/objmisc.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include "world/vnum.obj.h"

extern P_char character_list;
extern const struct racial_data_type racial_data[];
extern bool has_skin_spell(P_char);

int mist_claymore(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (cmd != CMD_MELEE_HIT)
		return (FALSE);
	if (!ch)
		return (FALSE);

	if (!OBJ_WORN(obj) || (obj->loc.wearing != ch))
		return (FALSE);
	victim = legacy_proc_arg<P_char>(arg);
	if (!victim)
		return (FALSE);
	if (number(0, 30))
		return (FALSE);
	act("&+W$n's&N $q &+Wglows with an &+Geerie flame&+W!!&N", TRUE, ch, obj, victim,
	    TO_NOTVICT);
	act("&+WYour&N $q &+Wglows with an &+Geerie flame&+W!!&N", TRUE, ch, obj, victim, TO_CHAR);
	act("&+W$n's&N $q &+Wglows with an &+Geerie flame&+W!!&N", TRUE, ch, obj, victim, TO_VICT);
	spell_incendiary_cloud(50, ch, NULL, 0, victim, obj);
	return (TRUE);
}

#define BAHAMUT_HELPER_LIMIT 3
int bahamut(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char i;
	int num, count = 0;
	P_char dragon;
	P_obj t_obj, next, heart;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd == CMD_DEATH)
	{
		act("&+WWith his very last breath, Bahamut closes his eyes.\n&n"
		    "&+WBahamut's body begins to shimmer brilliantly, his flesh becoming more and more translucent!\n&n"
		    "&+WAs the light subsides, only a few pieces of his once great body remain.&n",
		    FALSE, ch, 0, 0, TO_ROOM);

		for (t_obj = ch->carrying; t_obj; t_obj = next)
		{
			next = t_obj->next_content;
			obj_from_char(t_obj);
			obj_to_room(t_obj, ch->in_room);
		}

		for (num = 0; num < MAX_WEAR; num++)
			if (ch->equipment[num])
				obj_to_room(unequip_char(ch, num), ch->in_room);

		heart = read_object(VOBJ_WH_DRAGONHEART_BAHAMUT, VIRTUAL);
		obj_to_room(heart, ch->in_room);
		// 3 mud days to complete (object ticks are the same as mob ticks.. *sigh*).
		// (Sec / MudDay) * (Pulse / Sec) * (ObjPulse / Pulse) = ObjPulse / MudWeek
		heart->value[0] = (3 * SECS_PER_MUD_DAY * WAIT_SEC) / PULSE_MOBILE;
		// Time in minutes = ObjPulse * (Pulse / ObjPulse) * (Sec / Pulse) = Secs
		heart->value[1] = ((heart->value[0] * PULSE_MOBILE) / WAIT_SEC);
		// Secs / 3600 = Hrs
		heart->value[2] = heart->value[1] / 3600;
		// (Secs / 60) % 60 = Remainder of Mins
		heart->value[3] = (heart->value[1] / 60) % 60;
		// Secs % 60 = Remainder of Secs.
		heart->value[4] = (heart->value[1]) % 60;
		debug("&+WBahamut death: Heart decays in &+C%d&+W obj ticks = &+C%d&+W sec = &+C%d&+W:&+C%02d&+W:&+C%02d&+W.&n",
		      heart->value[0], heart->value[1], heart->value[2], heart->value[3],
		      heart->value[4]);
		logit(LOG_OBJ,
		      "Bahamut death: Heart decays in %d obj ticks = %d sec = %d:%02d:%02d.",
		      heart->value[0], heart->value[1], heart->value[2], heart->value[3],
		      heart->value[4]);
		// Value1 is break chance haha.. need to 0 that out.
		heart->value[1] = 0;
		return TRUE;
	}

	if (cmd != CMD_PERIODIC)
	{
		return FALSE;
	}

	if (IS_FIGHTING(ch))
	{
		/*
		 * attempt to "summon" a silver dragon...only possible if less than BAHAMUT_HELP_LIMIT
		 * * in world
		 */
		for (i = character_list; i; i = i->next)
		{
			if ((IS_NPC(i)) && (GET_VNUM(i) == 25758))
			{
				count++;
			}
		}
		if (count < BAHAMUT_HELPER_LIMIT)
		{
			if (number(1, 100) < 50)
			{
				dragon = read_mobile(25758, VIRTUAL);
				if (!dragon)
				{
					logit(LOG_MOB, "bahamut: could not load helper mob 25758");
					return FALSE;
				}
				act("$n &+Wraises onto his hind legs and releases a tremendous &+RROAR!!!!&n\r\n"
				    "&+BA magnificant &+Wsilver dragon&+B steps out of a &+Lportal&+B that closes instantly...&n\r\n",
				    FALSE, ch, 0, dragon, TO_ROOM);
				char_to_room(dragon, ch->in_room, 0);
				return TRUE;
			}
		}
	}

	return FALSE;
}

#undef BAHAMUT_HELPER_LIMIT

#ifdef THARKUN_ARTIS

int bloodfeast(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;
	struct damage_messages messages = {
		"&+WYou feel energy flowing from $N&+W into you!&N",
		"&+LYou feel the lifeforce being drained from your limbs!&N",
		"&+L$N&+L looks &n&+wpale &+Las $s lifeforce is drained by $n!&N",
		"",
		"",
		""
	};

	if (cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !(victim = legacy_proc_arg<P_char>(arg)))
	{
		return FALSE;
	}
	if (!IS_ALIVE(victim))
	{
		return FALSE;
	}

	// 1/25 chance.
	if (CheckMultiProcTiming(ch) && !number(0, 24))
	{
		act("$p &+Lemits a &n&+rblood curdling &+RsCrEaM!!!&n", TRUE, ch, obj, victim,
		    TO_CHAR);
		act("$p &+Lemits a &n&+rblood curdling &+RsCrEaM!!!&n", TRUE, ch, obj, victim,
		    TO_NOTVICT);
		act("$p &+Lemits a &n&+rblood curdling &+RsCrEaM!!!&n", TRUE, ch, obj, victim,
		    TO_VICT);
		spell_damage(ch, victim, 400, SPLDAM_NEGATIVE,
			     SPLDAM_NODEFLECT | SPLDAM_NOSHRUG | RAWDAM_NOKILL, &messages);
		vamp(ch, 50, (int)(GET_MAX_HIT(ch) * VAMPPERCENT(ch)));
		return TRUE;
	}
	return FALSE;
}

#else

/* Bahamut procs -Zod */

int bloodfeast(P_obj obj, P_char ch, int cmd, char *arg)
{
	int curr_time, dam;
	P_char victim;

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd == CMD_PERIODIC)
	{
		if (OBJ_WORN(obj) && obj->loc.wearing == ch)
		{
			curr_time = time(NULL);
			// Every 30 sec.
			if (obj->timer[0] + 30 <= curr_time && !has_skin_spell(temp_ch))
			{
				spell_stone_skin(45, temp_ch, 0, SPELL_TYPE_SPELL, temp_ch, 0);
				obj->timer[0] = curr_time;
				return TRUE;
			}
		}
		hummer(obj);
		return TRUE;
	}

	if (cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN(obj) || obj->loc.wearing != ch)
	{
		return FALSE;
	}
	victim = legacy_proc_arg<P_char>(arg);
	if (!IS_ALIVE(victim))
	{
		return FALSE;
	}

	if (!number(0, 24) && CheckMultiProcTiming(ch) && !IS_RACEUNDEAD(victim))
	{
		dam = BOUNDED(0, (GET_HIT(victim) + 9), 100);
		act("$p &+Lemits a &n&+rblood curdling &+RsCrEaM!!!&n", TRUE, ch, obj, victim,
		    TO_CHAR);
		act("$p &+Lemits a &n&+rblood curdling &+RsCrEaM!!!&n", TRUE, ch, obj, victim,
		    TO_NOTVICT);
		act("$p &+Lemits a &n&+rblood curdling &+RsCrEaM!!!&n", TRUE, ch, obj, victim,
		    TO_VICT);
		act("&+WYou feel energy flowing from $N&+W into you!&N", TRUE, ch, obj, victim,
		    TO_CHAR);
		act("&+LYou feel the lifeforce being drained from your limbs!&N", TRUE, ch, obj,
		    victim, TO_VICT);
		act("&+L$N&+L looks &n&+wpale &+Las $s lifeforce is drained by $n!&N", TRUE, ch,
		    obj, victim, TO_NOTVICT);
		vamp(ch, dam / 2, (int)(GET_MAX_HIT(ch) * 1.3));

		melee_damage(ch, victim, dam,
			     PHSDAM_NOSHIELDS | PHSDAM_NOREDUCE | PHSDAM_NOPOSITION, 0);
		return TRUE;
	}
	return FALSE;
}
#endif

int dragonlord_plate(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	int curr_time;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd == CMD_PERIODIC && OBJ_WORN(obj) && (ch = obj->loc.wearing) &&
	    IS_ALIVE(obj->loc.wearing))
	{
		curr_time = time(NULL);
		// Every 30 min.
		if (obj->timer[1] + 1800 <= curr_time && !IS_AFFECTED4(ch, AFF4_STORNOGS_SPHERES))
		{
			spell_stornogs_spheres(53, ch, 0, SPELL_TYPE_SPELL, ch, 0);
			obj->timer[1] = curr_time;
			return TRUE;
		}
		// Every 30 sec.
		if (obj->timer[0] + 30 <= curr_time && !affected_by_spell(ch, SPELL_STONE_SKIN))
		{
			spell_stone_skin(45, ch, 0, SPELL_TYPE_SPELL, ch, 0);
			obj->timer[0] = curr_time;
			return TRUE;
		}
	}
	return FALSE;
}

int sunblade(P_obj obj, P_char ch, int cmd, char *arg)
{
	int curr_time;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd == CMD_PERIODIC)
	{
		if (OBJ_WORN(obj) && (ch = obj->loc.wearing))
		{
			curr_time = time(NULL);

			if (!has_skin_spell(ch) &&
			    obj->timer[0] + (int)get_property("timer.stoneskin.artifact.sunblade",
							      30) <=
				    curr_time)
			{
				spell_stone_skin(45, ch, 0, SPELL_TYPE_POTION, ch, 0);
				obj->timer[0] = curr_time;
				return TRUE;
			}
		}
		hummer(obj);
		return TRUE;
	}

	if (cmd != CMD_MELEE_HIT || !OBJ_WORN(obj) || !IS_ALIVE(ch) || obj->loc.wearing != ch)
	{
		return FALSE;
	}
	victim = legacy_proc_arg<P_char>(arg);
	if (!IS_ALIVE(victim))
	{
		return FALSE;
	}
	// 4% chance
	if (!number(0, 24) && CheckMultiProcTiming(ch))
	{
		act("&+r$n's&n $q &+Rexplodes&n&+r in a torrent of pure &+Rflame!&n", TRUE, ch, obj,
		    victim, TO_CHAR);
		act("&+r$n's&n $q &+Rexplodes&n&+r in a torrent of pure &+Rflame!&n", TRUE, ch, obj,
		    victim, TO_NOTVICT);
		act("&+r$n's&n $q &+Rexplodes&n&+r in a torrent of pure &+Rflame!&n", TRUE, ch, obj,
		    victim, TO_VICT);
		spell_sunray(GET_LEVEL(ch), ch, NULL, 0, victim, 0);

		/*
		  if(OUTSIDE(ch))
		    spell_sunray(GET_LEVEL(ch), ch, NULL, 0, victim, 0);
		  else if(number(0, 1))
		    spell_solar_flare(GET_LEVEL(ch), ch, NULL, 0, victim, NULL);
		  else if(number(0, 1))
		    spell_firebrand(GET_LEVEL(ch), ch, NULL, 0, victim, 0);
		  else
		    spell_immolate(GET_LEVEL(ch), ch, NULL, 0, victim, 0);
		 */
		// if(GET_C_LUK(ch) > number(1, 1000))
		// {
		act("$p &+memits a &+Msoft glow&n&+m.&n", TRUE, ch, obj, victim, TO_CHAR);
		act("$p &+memits a &+Msoft glow&n&+m.&n", TRUE, ch, obj, victim, TO_NOTVICT);
		act("$p &+memits a &+Msoft glow&n&+m.&n", TRUE, ch, obj, victim, TO_VICT);
		spell_heal(GET_LEVEL(ch), ch, 0, 0, ch, 0);
		//  }
		return TRUE;
	}
	return FALSE;
}

int mrinlor_whip(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char vict;
	int dam = cmd / 1000;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!dam || !IS_ALIVE(ch) || !obj || !OBJ_WORN_POS(obj, WIELD) || !OBJ_WORN_BY(obj, ch) ||
	    !(vict = legacy_proc_arg<P_char>(arg)))
	{
		return FALSE;
	}

	// 1/14 chance
	if (IS_FIGHTING(ch) && !number(0, 13))
	{
		act("&nYour $p cracks as it whips $N and launches a &+rfireball&n!", TRUE, ch, obj,
		    vict, TO_CHAR);
		act("&n$n's $p violently whips $N and launches a &+rfireball!&n", TRUE, ch, obj,
		    vict, TO_NOTVICT);
		act("&+COUCH!&n $n just whipped you with his $p, leaving a &+rfireball&n coming right at you!&n",
		    TRUE, ch, obj, vict, TO_VICT);

		spell_fireball(30, ch, NULL, 0, vict, 0);
	}
	return FALSE;
}

void event_dragonlord_check(P_char ch, P_char /*victim*/, P_obj obj, void * /*data*/)
{
	struct affected_type *af;
	P_obj armor = ch->equipment[WEAR_BODY];
	int dragonlord_slot = MAX_WEAR;
	bool bHasOtherArti = false;

	if (GET_RACE(ch) != RACE_DRAGONKIN)
	{
		act("Your scales smoke and burn as they &+Rdisintegrate!&n", FALSE, ch, obj, 0,
		    TO_CHAR);
		wizlog(57, "Dragonlord armor worn by %s begins to melt due race check conflict!",
		       GET_NAME(ch));
		GET_HIT(ch) >>= 1;
		CharWait(ch, 2 * WAIT_SEC);
	}
	if ((af = get_spell_from_char(ch, TAG_RACE_CHANGE)) == NULL)
	{
		send_to_char(
			"&+WPossible serious screwup in the dragonlord proc! Tell a coder as once!&n\r\n",
			ch);
		wizlog(57,
		       "Char %s found with racechange event but without racechange affect! Dragonlord proc",
		       GET_NAME(ch));
		return;
	}
	// Same code Necroplasm uses.
	for (int i = 0; i < MAX_WEAR; i++)
	{
		if (ch->equipment[i] == armor)
		{
			dragonlord_slot = i;
		}
		if (ch->equipment[i] && IS_SET(ch->equipment[i]->extra_flags, ITEM_ARTIFACT) &&
		    (ch->equipment[i] != armor))
		{
			bHasOtherArti = true;
		}
	}

	if (bHasOtherArti && (dragonlord_slot != MAX_WEAR) && ch->equipment[dragonlord_slot])
	{
		act("The &+Wplatemail&n of the &+YDragonLord&n erupts acid and detaches from $n's body!&n",
		    FALSE, ch, obj, 0, TO_ROOM);
		act("The &+Wplatemail&n of the &+YDragonLord&n erupts acid as it detaches from your body!",
		    FALSE, ch, obj, 0, TO_CHAR);
		obj_to_char(unequip_char(ch, dragonlord_slot), ch);
		add_event(event_dragonlord_check, (int)(0.5 * PULSE_VIOLENCE), ch, 0, 0, 0, 0, 0);
	}
	else if (armor != NULL && obj_index[armor->R_num].virtual_number == DRAGONLORD_PLATE_VNUM &&
		 GET_STAT(ch) > STAT_DEAD)
	{
		add_event(event_dragonlord_check, (int)(0.5 * PULSE_VIOLENCE), ch, 0, 0, 0, 0, 0);
		return;
	}
	else
	{
		ch->player.race = af->modifier;
		ch->player.time.birth = time(NULL) - (racial_data[GET_RACE(ch)].base_age) * 2;
		//    GET_AGE(ch) = racial_data[(int) GET_RACE(ch)].base_age*2;
		// Set birthdate + base_age + 5 years.
		ch->player.time.birth = time(NULL);
		// Add base_age to birthdate + base_age + 5 years.
		ch->player.time.birth -= (racial_data[GET_RACE(ch)].base_age) * SECS_PER_MUD_YEAR;
		affect_remove(ch, af);
		send_to_char(
			"The curse of the dark powers fade and your soul restores the body.\r\n",
			ch);

		int k = 0;
		P_obj temp_obj;
		for (k = 0; k < MAX_WEAR; k++)
		{
			temp_obj = ch->equipment[k];
			if (temp_obj)
			{
				if (obj_index[temp_obj->R_num].func.obj != NULL)
					invoke_object_special(temp_obj, ch, CMD_REMOVE,
							      (char *)"all");
				obj_to_char(unequip_char(ch, k), ch);
			}
		}
		send_to_char("Brr, you suddenly feel very naked.\r\n", ch);

		return;
	}
	// int k = 0;
	// P_obj temp_obj;
	// for (k = 0; k < MAX_WEAR; k++)
	// {
	// temp_obj = ch->equipment[k];
	// if(temp_obj)
	// {
	// obj_to_char(unequip_char(ch, k), ch);
	// }
	// }
}

int dragonlord_plate_old(P_obj obj, P_char /*ch*/, int cmd, char * /*arg*/)
{
	P_obj temp_obj;
	P_char temp_ch;
	struct affected_type af;
	int k = 0;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd == CMD_PERIODIC)
	{
		if (OBJ_WORN(obj))
		{
			temp_ch = obj->loc.wearing;

			if (!temp_ch || !IS_ALIVE(temp_ch) || !IS_PC(temp_ch))
			{
				return TRUE;
			}

			if (affected_by_spell(temp_ch, TAG_RACE_CHANGE))
			{
				return TRUE;
			}

			for (k = 0; k < MAX_WEAR; k++)
			{
				temp_obj = temp_ch->equipment[k];

				if (temp_obj && (obj != temp_obj))
				{
					if (obj_index[temp_obj->R_num].func.obj != NULL)
						invoke_object_special(temp_obj, temp_ch, CMD_REMOVE,
								      (char *)"all");
					obj_to_char(unequip_char(temp_ch, k), temp_ch);
				}
			}

			send_to_char("Brr, you suddenly feel _almost_ naked.\r\n\n", temp_ch);

			CharWait(temp_ch, 5 * WAIT_SEC);

			memset(&af, 0, sizeof(af));
			af.type = TAG_RACE_CHANGE;
			af.flags = AFFTYPE_NOSAVE | AFFTYPE_NODISPEL;
			af.duration = -1;
			af.modifier = GET_RACE(temp_ch);
			affect_to_char(temp_ch, &af);

			add_event(event_dragonlord_check, (int)(0.5 * PULSE_VIOLENCE), temp_ch, 0,
				  0, 0, 0, 0);

			act("&+RPain &+Llike you have never felt before renders you momentarily dazed as your\n&+Lflesh is ripped apart.  Your &N&+rmuscles &+Lripple and flex as they grow in size "
			    "and\n&+Lstrength and a new skin of &N&+whardened dragonscales begins to form upon your body.\n&+LYou emerge from the transformation, flex your mighty new wings and &+Rroar "
			    "&+Lloudly!&n\r\n",
			    FALSE, temp_ch, obj, 0, TO_CHAR);
			act("&+L$n shudders and drops to $s knees as the awesome transformation takes hold.\n&+L$n&+L's body grows more muscular, while thick scales replace the shedded skin\n&+Land two &+Rgreat "
			    "wings &+Lsprout from $s back. $n's face twists and is replaced by the\n&+Lvisage of a dragon, jaws filled with razor sharp teeth as $e roars loudly!&n\r\n",
			    FALSE, temp_ch, obj, 0, TO_ROOM);

			temp_ch->player.race = RACE_DRAGONKIN;

			temp_ch->player.time.birth =
				time(NULL) - (racial_data[RACE_DRAGONKIN].base_age) * 2;
			//      GET_AGE(temp_ch) += racial_data[RACE_DRAGONKIN].base_age * 2;
			// Set birthdate + base_age + 5 years.
			temp_ch->player.time.birth = time(NULL);
			// Add base_age to birthdate + base_age + 5 years.
			temp_ch->player.time.birth -=
				(racial_data[RACE_DRAGONKIN].base_age) * SECS_PER_MUD_YEAR;

			return TRUE;
		}
	}
	return FALSE;
}

int dragonlord_plate_oldold(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	int curr_time;
	P_char temp_ch;

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd == CMD_PERIODIC)
	{
		if (!IS_ALIVE(ch))
		{
			return false;
		}

		if (OBJ_WORN(obj))
		{
			temp_ch = obj->loc.wearing;
			curr_time = time(NULL);

			if (obj->timer[1] + 1800 <= curr_time &&
			    !IS_AFFECTED4(temp_ch, AFF4_STORNOGS_SPHERES))
			{
				spell_stornogs_spheres(53, temp_ch, 0, SPELL_TYPE_SPELL, temp_ch,
						       0);

				obj->timer[1] = curr_time;

				return TRUE;
			}

			if (obj->timer[0] + 30 <= curr_time && !has_skin_spell(temp_ch))
			{
				spell_stone_skin(45, temp_ch, 0, SPELL_TYPE_SPELL, temp_ch, 0);

				obj->timer[0] = curr_time;

				return TRUE;
			}
		}
	}
	return (FALSE);
}

int dragon_helm(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;
	if (cmd == CMD_WEAR)
	{
		if (OBJ_WORN(obj))
		{
			if (obj->timer[0] == 0 && GET_RACE(ch) != RACE_DRAGON && IS_PC(ch))
			{
				obj->timer[0] = GET_RACE(ch);
				/* set them to dragon here */
				act("&+RYour blood runs like fire as you feel yourself take on a new form!&N",
				    FALSE, ch, 0, 0, TO_CHAR);
				return TRUE;
			}
		}
	}
	if (cmd == CMD_REMOVE)
	{
		if (obj && ch && IS_PC(ch) && GET_RACE(ch) == RACE_DRAGON && obj->timer[0] > 0)
		{
			/* set them to orig race here */
			obj->timer[0] = 0;
			act("&+RYou return to your original form!&N", FALSE, ch, 0, 0, TO_CHAR);
			return (TRUE);
		}
	}
	return (FALSE);
}
