/*
 ***************************************************************************
 *  File: specs.jot.c                                 Part of Duris        *
 *  Usage: Special Procs for Jot area                                      *
 *  Copyright  1997 - Tim Devlin (Cython)  cython@duris.org                *
 *  Copyright  1994, 1997 - Duris Dikumud                                  *
 ***************************************************************************
 */

#include "core/prototypes.h"
#include "combat/defense_resolution.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include <stdio.h>
#include <string.h>
#include "world/specs.prototypes.h"
#include "magic/spells.h"

/*
   extern variables
 */

extern P_room world;
extern struct zone_data *zone_table;
extern P_index mob_index;

/*
   item procs
 */

int icicle_cloak(P_obj obj, P_char ch, int cmd, char *arg)
{
	int curr_time;

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!ch || !OBJ_WORN(obj))
		return (FALSE);

	if (arg && (cmd == CMD_SAY))
	{
		if (isname(arg, "icicle"))
		{
			curr_time = time(NULL);

			if (curr_time >= obj->timer[0] + 60)
			{
				act("You say 'icicle' to your $q...", FALSE, ch, obj, 0, TO_CHAR);
				act("$n says 'icicle' to $q...&N", TRUE, ch, obj, NULL, TO_ROOM);
				act("Your $q starts to glow brilliantly!.", FALSE, ch, obj, obj,
				    TO_CHAR);

				act("$n's $q starts to glow brilliantly!.", TRUE, ch, obj, NULL,
				    TO_ROOM);
				spell_ice_storm(60, ch, 0, 0, 0, NULL);
				obj->timer[0] = curr_time;
				return TRUE;
			}
		}
	}
	return (FALSE);
}

int betrayal(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 400;
	P_char vict;

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!dam) /*
	             if dam is not 0, we have been called when
	             weapon hits someone
	           */
		return (FALSE);

	if (!ch)
		return (FALSE);

	if (!OBJ_WORN_POS(obj, WIELD))
		return (FALSE);

	vict = legacy_proc_arg<P_char>(arg);

	if (!vict)
		return (FALSE);

	if (obj->loc.wearing == ch)
	{
		if (!number(0, 30))
		{
			act("Your $q glows brightly as an &+Lunholy light&n streaks out of it.",
			    FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
			act("$n's $q glows brightly as an &+Lunholy light&N streaks out of it!",
			    FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
			spell_fireball(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
		}
		else
		{
			if (!GET_OPPONENT(ch))
				set_fighting(ch, vict);
		}
	}
	if (GET_OPPONENT(ch))
		return (FALSE); /*
		                   do the normal hit damage as well
		                 */
	else
		return (TRUE);
}

int faith(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	int dam = cmd / 1000;
	P_char t_vict;
	P_char vict;
	P_char temp;

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	// If dam is not 0, we have been called when weapon hits someone
	if (!dam || !IS_ALIVE(ch))
	{
		return FALSE;
	}

	if (!OBJ_WORN_POS(obj, WIELD))
	{
		return FALSE;
	}

	if (!IS_FIGHTING(ch) || !(vict = GET_OPPONENT(ch)))
	{
		return FALSE;
	}

	if (obj->loc.wearing == ch)
	{
		// 1/20 chance.
		if (!number(0, 19))
		{
			act("Your $q glows brightly as a &+Yholy wave of energy&n strikes out of it.",
			    FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
			act("$n's $q glows brightly as a &+Yholy wave of energy&n strikes out of it!",
			    FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
			if (GET_ALIGNMENT(ch) > 0)
				spell_holy_word(35, ch, 0, SPELL_TYPE_SPELL, vict, 0);
			else
				spell_unholy_word(35, ch, 0, SPELL_TYPE_SPELL, vict, 0);
			// (un)holy word can kill.
			if (!char_in_list(ch))
				return TRUE;
			for (t_vict = world[ch->in_room].people; t_vict; t_vict = temp)
			{
				temp = t_vict->next_in_room;
				if ((t_vict != ch) && !grouped(t_vict, ch) &&
				    (CAN_SEE(ch, t_vict) && !number(0, 3) && !IS_TRUSTED(vict)))
				{
					if (GET_ALIGNMENT(ch) > 0)
					{
						spell_dispel_evil(51, ch, 0, SPELL_TYPE_SPELL,
								  t_vict, 0);
					}
					else
					{
						spell_dispel_good(51, ch, 0, SPELL_TYPE_SPELL,
								  t_vict, 0);
					}
				}

				// ya never know
				if (!char_in_list(ch))
					return TRUE;
			}
		}
	}
	// Do the normal hit damage as well
	if (GET_OPPONENT(ch))
		return FALSE;
	else
		return TRUE;
}

int mistweave(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	int mistdamage = 50;
	int dodamage;
	P_char vict;
	P_char tch;

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	// If dam is not 0, we have been called when weapon hits someone
	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WIELD))
	{
		return FALSE;
	}

	if (!(vict = legacy_proc_arg<P_char>(arg)))
	{
		return FALSE;
	}

	if (obj->loc.wearing == ch)
	{
		// 1/30 chance
		if (!number(0, 29))
		{
			dodamage = mistdamage;
			act("&+mYour $q &+mproduces an odd sound as &+Lblack smoke&+m pours from the tip!&N",
			    FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
			act("$n's $q &+mproduces an odd sound as &+Lblack smoke&+m pours from it!",
			    FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
			if (saves_spell(vict, SAVING_SPELL))
			{
				dodamage /= 2;
			}
			if ((GET_HIT(vict) - dodamage) <= 0)
			{
				dodamage = (GET_HIT(vict) - dice(1, 4));
			}
			GET_HIT(vict) -= dodamage;
			update_pos(vict);
			act("&+mThe &+Lblack smoke &+mengulfs $N&+m!", FALSE, ch, 0, vict,
			    TO_NOTVICT);
			act("&+mThe &+Lblack smoke &+mengulfs you&+m!", FALSE, ch, 0, vict,
			    TO_VICT);
			act("&+mThe &+Lblack smoke &+mengulfs $N&+m!", FALSE, ch, 0, vict, TO_CHAR);

			mistdamage /= 4;
			for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
			{
				if (should_area_hit(ch, tch) && !number(0, 4))
				{
					act("&+mThe &+Lblack smoke &+mengulfs $N&+m!", FALSE, ch, 0,
					    tch, TO_NOTVICT);
					act("&+mThe &+Lblack smoke &+mengulfs you&+m!", FALSE, ch,
					    0, tch, TO_VICT);
					act("&+mThe &+Lblack smoke &+mengulfs $N&+m!", FALSE, ch, 0,
					    tch, TO_CHAR);
					if (saves_spell(tch, SAVING_SPELL))
					{
						dodamage = (mistdamage / 2);
					}
					else
					{
						dodamage = mistdamage;
					}
					if ((GET_HIT(tch) - dodamage) <= 0)
					{
						dodamage = (GET_HIT(tch) - dice(1, 4));
					}
					GET_HIT(tch) -= dodamage;
					update_pos(tch);
				}
			}
		}
		else
		{
			if (!GET_OPPONENT(ch))
			{
				set_fighting(ch, vict);
			}
		}
	}
	// Do the normal hit damage as well
	if (GET_OPPONENT(ch))
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

int leather_vest(P_obj obj, P_char ch, int cmd, char *arg)
{
	struct proc_data *data;
	P_char vict;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	// Only on getting hit, chance 1/50
	if (cmd != CMD_GOTHIT || number(0, 49))
	{
		return FALSE;
	}

	// IMPORTANT: Can do this type cast (next line) ONLY if cmd was CMD_GOTHIT!
	data = legacy_proc_arg<struct proc_data *>(arg);
	vict = data->victim;
	act("&+LHuge &+wsp&+Wi&+wkes &+Ljet out from your $q &+Lstopping $N's &+Llunge at you.",
	    FALSE, ch, obj, vict, TO_CHAR | ACT_NOTTERSE);
	act("&+LHuge &+wsp&+Wi&+wkes &+Ljet out from $n&+L's $q &+Lstopping your futile lunge at $m.",
	    FALSE, ch, obj, vict, TO_VICT | ACT_NOTTERSE);
	act("&+LHuge &+wsp&+Wi&+wkes &+Ljet out from $n&+L's $q &+Lstopping $N&+L's lunge.", FALSE,
	    ch, obj, vict, TO_NOTVICT | ACT_NOTTERSE);

	return TRUE;
}

int deva_cloak(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	int curr_time;
	P_char tch;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd != 0 || !obj || !OBJ_WORN(obj) || !(ch = obj->loc.wearing))
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch) || !ch->in_room)
	{
		return FALSE;
	}

	curr_time = time(NULL);
	if (!IS_ROOM(ch->in_room, ROOM_NO_MAGIC))
	{
		if ((obj->timer[0] + 360) <= curr_time)
		{
			obj->timer[0] = curr_time;
			act("$n's $q&+w sends forth a whirlwind of &+Wfeathers&+w!", FALSE, ch, obj,
			    0, TO_NOTVICT);
			act("Your $q&+w sends forth a whirlwind of &+Wfeathers&+w!", FALSE, ch, obj,
			    0, TO_CHAR);

			for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
			{
				if ((ch->group && ch->group == tch->group))
				{
					if (!IS_AFFECTED(tch, AFF_FLY) ||
					    !IS_AFFECTED(tch, AFF_LEVITATE))
					{
						act("&+wThe &+Wfeathers &+wswirl around you!",
						    FALSE, tch, obj, 0, TO_CHAR);
						act("&+wThe &+Wfeathers &+wswirl around $n!", FALSE,
						    tch, obj, 0, TO_ROOM);
						spell_levitate(51, ch, 0, SPELL_TYPE_SPELL, tch, 0);
						spell_fly(51, ch, 0, SPELL_TYPE_SPELL, tch, 0);
					}
				}
			}
			return FALSE;
		}
	}
	return FALSE;
}

int ogrebane(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char vict;

	/*
	   check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	// If dam is not 0, we have been called when weapon hits someone
	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WIELD) ||
	    !(vict = legacy_proc_arg<P_char>(arg)))
	{
		return FALSE;
	}

	if (GET_RACE(vict) != RACE_OGRE)
	{
		return FALSE;
	}

	if (obj->loc.wearing == ch)
	{
		// 1/15 chance (It's ogre only)
		if (!number(0, 14))
		{
			act("Your $q &+Wglows brightly at the sight of the foul &N&+bOGRE!&N",
			    FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
			act("$n's $q &+Wglows brightly a the sight of the foul&N&+b OGRE!&N", FALSE,
			    obj->loc.wearing, obj, 0, TO_ROOM);
			spell_bigbys_clenched_fist(61, ch, 0, SPELL_TYPE_SPELL, vict, 0);
		}
		else
		{
			if (!GET_OPPONENT(ch))
			{
				set_fighting(ch, vict);
			}
		}
	}
	// Do the normal hit damage as well
	if (GET_OPPONENT(ch))
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

int giantbane(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char vict;

	// Check for periodic event calls
	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	// If dam is not 0, we have been called when weapon hits someone
	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WIELD) ||
	    !(vict = legacy_proc_arg<P_char>(arg)))
	{
		return FALSE;
	}

	if ((GET_SIZE(vict) != SIZE_GIANT) || (IS_DRAGON(vict)))
	{
		return FALSE;
	}

	if (obj->loc.wearing == ch)
	{
		// 1/20 chance (It's giant size only and !dragon)
		if (!number(0, 19))
		{
			act("Your $q &+Wglows brightly at the sight of the foul GIANT!", FALSE,
			    obj->loc.wearing, obj, 0, TO_CHAR);
			act("$n's $q &+Wglows brightly a the sight of the foul GIANT!", FALSE,
			    obj->loc.wearing, obj, 0, TO_ROOM);
			spell_bigbys_clenched_fist(51, ch, 0, SPELL_TYPE_SPELL, vict, 0);
		}
		else
		{
			if (!GET_OPPONENT(ch))
			{
				set_fighting(ch, vict);
			}
		}
	}
	// Do the normal hit damage as well
	if (GET_OPPONENT(ch))
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

int dwarfslayer(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char vict;

	// Check for periodic event calls
	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WIELD) ||
	    !(vict = legacy_proc_arg<P_char>(arg)))
	{
		return FALSE;
	}

	if (GET_RACE(vict) != RACE_MOUNTAIN && GET_RACE(vict) != RACE_DUERGAR)
	{
		return FALSE;
	}

	if (obj->loc.wearing == ch)
	{
		// 1/25 chance for 2 races.
		if (!number(0, 24))
		{
			act("Your $q &+mHums loudly at the sight of the Dwarf!", FALSE,
			    obj->loc.wearing, obj, 0, TO_CHAR);
			act("$n's $q &+Whums loudly a the sight of the Dwarf!", FALSE,
			    obj->loc.wearing, obj, 0, TO_ROOM);

			spell_wither(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
			spell_lightning_bolt(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
		}
		else
		{
			if (!GET_OPPONENT(ch))
			{
				set_fighting(ch, vict);
			}
		}
	}
	// Do the normal hit damage as well
	if (GET_OPPONENT(ch))
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

int mindbreaker(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char vict;
	int save;

	// Check for periodic event calls
	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	// If dam is not 0, we have been called when weapon hits someone
	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN_POS(obj, WIELD) ||
	    !(vict = legacy_proc_arg<P_char>(arg)))
	{
		return FALSE;
	}

	if (obj->loc.wearing == ch)
	{
		// 1/30 chance of uber feeblmind.
		if (!number(0, 29))
		{
			act("Your $q suddenly makes a &+WVERY LOUD CRACKING SOUND&N and a wave of energy flows out!&N",
			    FALSE, obj->loc.wearing, obj, 0, TO_CHAR);
			act("$n's $q suddenly makes a &+WVERY LOUD CRACKING SOUND&N as a wave of energy flows out!",
			    FALSE, obj->loc.wearing, obj, 0, TO_ROOM);
			save = vict->specials.apply_saving_throw[SAVING_SPELL];
			// HACK! Rofl!
			vict->specials.apply_saving_throw[SAVING_SPELL] += 15;
			spell_feeblemind(60, ch, 0, SPELL_TYPE_SPELL, vict, 0);
			vict->specials.apply_saving_throw[SAVING_SPELL] = save;
		}
		else
		{
			if (!GET_OPPONENT(ch))
			{
				set_fighting(ch, vict);
			}
		}
	}
	// Do the normal hit damage as well
	if (GET_OPPONENT(ch))
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

int reliance_pegasus(P_obj obj, P_char ch, int cmd, char *arg)
{
	int curr_time;
	P_char mount;
	struct char_link_data *cld;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!IS_ALIVE(ch) || !(obj) || !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}

	if (arg && (cmd == CMD_SAY))
	{
		if (strstr(arg, "reliance"))
		{
			if (IS_RIDING(ch))
			{
				send_to_char("While mounted? I don't think so...\r\n", ch);
				return TRUE;
			}

			if (IS_FIGHTING(ch))
			{
				send_to_char(
					"Try again whenever you are NOT fighting something.\r\n",
					ch);
				return TRUE;
			}

			if (!is_prime_plane(ch->in_room))
			{
				send_to_char("&+WThe pegasus cannot be called here.\r\n", ch);
				return TRUE;
			}

			if (IS_ROOM(ch->in_room, ROOM_LOCKER) ||
			    IS_ROOM(ch->in_room, ROOM_SINGLE_FILE))
			{
				send_to_char("A pegasus couldn't fit in here!\r\n", ch);
				return TRUE;
			}

			curr_time = time(NULL);

			if (obj->timer[0] + 200 <= curr_time)
			{
				act("You say 'reliance' to your $q...", FALSE, ch, obj, 0, TO_CHAR);
				act("$n says 'reliance' to $q...&N", TRUE, ch, obj, NULL, TO_ROOM);

				for (cld = ch->linked; cld; cld = cld->next_linked)
				{
					if (IS_NPC(cld->linking) && GET_VNUM(cld->linking) == 40429)
					{
						if (GET_RIDER(cld->linking))
						{
							send_to_char(
								"The pegasus fails to answer the call.\r\n",
								ch);
							return true;
						}

						if (ch->in_room != cld->linking->in_room)
						{
							act("&+WA magnificent pegasus descends from the heavens to your aid.&n",
							    FALSE, ch, obj, obj, TO_CHAR);
							act("&+WA magnificent pegasus descends from the heavens to $n's &+Waid.&n",
							    TRUE, ch, obj, NULL, TO_ROOM);
							char_from_room(cld->linking);
							char_to_room(cld->linking, ch->in_room, -1);
						}
						act("$n whinnies loudly.", FALSE, cld->linking, 0,
						    0, TO_ROOM);
						CharWait(ch, PULSE_VIOLENCE * 4);
						send_to_char("You feel slighting drained.\r\n", ch);
						return TRUE;
					}
				}

				act("&+WA glorious white light pours forth from $p&+W, answering your call.&n",
				    FALSE, ch, obj, obj, TO_CHAR);
				act("&+WA magnificent pegasus descends from the heavens to your aid.&n",
				    FALSE, ch, obj, obj, TO_CHAR);
				act("&+WA glorious white light pours from from &N$n's $q&+W, answering his call.&n",
				    TRUE, ch, obj, NULL, TO_ROOM);
				act("&+WA magnificent pegasus descends from the heavens to $n's &+Waid.&n",
				    TRUE, ch, obj, NULL, TO_ROOM);
				mount = read_mobile(40429, VIRTUAL);
				char_to_room(mount, ch->in_room, -1);
				setup_pet(mount, ch, -1, PET_NOCASH);
				add_follower(mount, ch);
				SET_BIT(mount->specials.act, ACT_MOUNT);
				obj->timer[0] = curr_time;
				return TRUE;
			}
		}
	}
	return (FALSE);
}

/* Jotunheim mobile procedures */

int jotun_thrym(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict;
	struct affected_type af;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd)
		return FALSE;

	if (ch && IS_FIGHTING(ch))
		if (!number(0, 2))
		{
			vict = GET_OPPONENT(ch);
			if (!vict || check_freedom_of_movement(vict, true))
				return FALSE;

			act("&+BA blue bolt of energy streaks from&n $N's&n&+B hands, encasing&n $n &n&+Bin a solid block of ice!",
			    0, vict, 0, ch, TO_NOTVICT);
			act("&+BA blue bolt of energy streaks from&n $n's&n&+B hands, encasing you in a solid block of ice!",
			    0, ch, 0, vict, TO_VICT);
			act("&+BA blue bolt of energy streaks from your hands, encasing&n $N &+Bin a solid block of ice!",
			    0, ch, 0, vict, TO_CHAR);

			/*
			 * Shut em down!
			 */

			StopCasting(vict);
			if (IS_FIGHTING(vict))
				stop_fighting(vict);
			bzero(&af, sizeof(af));
			af.type = SPELL_MAJOR_PARALYSIS;
			af.flags = AFFTYPE_SHORT;
			af.duration = 120 * WAIT_SEC;
			af.bitvector2 = AFF2_MAJOR_PARALYSIS;
			affect_to_char(vict, &af);
			CharWait(vict, af.duration);

			return TRUE;
		}
	return FALSE;
}

int jotun_utgard_loki(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict, next;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd)
		return FALSE;

	if (IS_FIGHTING(ch) && !number(0, 2))
	{
		act("$n &N&+Lcalls forth visions of immense horror!", 0, ch, 0, 0, TO_ROOM);
		for (vict = world[ch->in_room].people; vict; vict = next)
		{
			next = vict->next_in_room;
			if (!IS_GIANT(vict) && (vict != ch))
			{
				if (GET_LEVEL(vict) < 19)
				{ /*
				   * 20 and below, see ya...
				   */
					do_flee(vict, 0, 2);
					act("&+LThe fear of it all overwhelms you!", 0, vict, 0, 0,
					    TO_VICT);
				}
				if (GET_LEVEL(vict) < 31) /*
				                           * 21-30, slight chance
				                           */
					if (!NewSaves(vict, SAVING_FEAR, -2) && !fear_check(vict))
					{
						do_flee(vict, 0, 2);
						act("&+LThe fear of it all overwhelms you!", 0,
						    vict, 0, 0, TO_VICT);
					}
				if (GET_LEVEL(vict) <= MAXLVLMORTAL) /*
				                                      * 31-56, good chance of staying
				                                      */
					if (!NewSaves(vict, SAVING_FEAR, 0) && !fear_check(vict))
					{
						do_flee(vict, 0, 1);
						act("&+LThe fear of it all overwhelms you!", 0,
						    vict, 0, 0, TO_VICT);
					}
				if (ch->in_room != vict->in_room)
					if (IS_FIGHTING(vict))
						stop_fighting(vict);
			}
			return TRUE;
		}
	}
	return FALSE;
}

int jotun_balor(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict, next;
	int dam;
	struct affected_type af;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd == CMD_DEATH)
	{ /*
	   * explode upon death
	   */
		act("$n &N&+rEXPLODES in a mass of fire and energy!", 0, ch, 0, 0, TO_ROOM);
		for (vict = world[ch->in_room].people; vict; vict = next)
		{
			next = vict->next_in_room;
			if ((ch == vict) || IS_TRUSTED(vict))
				continue;

			if (!IS_AFFECTED(vict, AFF_BLIND) && IS_AFFECTED(ch, AFF_INFRAVISION))
			{
				bzero(&af, sizeof(af));
				if (vict->in_room != NOWHERE)
					send_to_char("Aaarrrggghhh!!  The heat blinds you!!\n", ch);
				blind(ch, vict, 30 * WAIT_SEC);
			}

			if (IS_AFFECTED(vict, AFF_PROT_FIRE))
				dam = 150; /*
				            * Allow a slight help, but not just fire
				            */
			else
				dam = 250; /*
				            * so skip all the elemental type checks
				            */
			if ((GET_HIT(vict) - dam) < -10)
			{
				act("Your wounds prove too much for you!", FALSE, ch, 0, 0,
				    TO_CHAR);
				act("$n's wounds prove too much for $m!", TRUE, ch, 0, 0, TO_ROOM);
				logit(LOG_DEATH, "%s died from jotun_balor() explosion in room %d.",
				      GET_NAME(vict), world[vict->in_room].number);
				die(vict, ch);
			}
			else
				GET_HIT(vict) -= dam;
		}
		return TRUE;
	}
	return FALSE;
}

int jotun_mimer(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	char Gbuf1[MAX_STRING_LENGTH];

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch)
		return FALSE;

	if ((ch->in_room != real_room(GET_BIRTHPLACE(ch))) || (ch->in_room == NOWHERE))
	{
		if (!IS_AWAKE(ch) || IS_FIGHTING(ch))
			return FALSE;
		act("$N looks around frantically, then vanishes in a small puff of smoke", FALSE,
		    ch, 0, 0, TO_ROOM);
		char_from_room(ch);
		char_to_room(ch, real_room(GET_BIRTHPLACE(ch)), -1);
		return FALSE;
	}
	if (pl && cmd)
	{
		if (cmd == CMD_WEST)
		{
			if (GET_LEVEL(pl) < 51 && !IS_GIANT(pl))
				mobsay(ch, "None but giants may pass through to my well.");
			else
			{
				snprintf(Gbuf1, MAX_STRING_LENGTH,
					 "$N bows before you, saying 'Right this way, My %s'",
					 (GET_SEX(pl) == SEX_FEMALE) ? "Lady" : "Lord");
				act(Gbuf1, FALSE, pl, 0, ch, TO_CHAR);
				snprintf(Gbuf1, MAX_STRING_LENGTH,
					 "$N bows before $n, saying 'Right this way, My %s'",
					 (GET_SEX(pl) == SEX_FEMALE) ? "Lady" : "Lord");
				act(Gbuf1, FALSE, pl, 0, ch, TO_ROOM);
				return FALSE;
			}
			return TRUE;
		}
	}
	return FALSE;
}
