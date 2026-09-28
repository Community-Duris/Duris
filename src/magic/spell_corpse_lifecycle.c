#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "core/mm.h"
#include "world/graph.h"
#include "world/vnum.obj.h"
#include "world/achievements.h"
#include "classes/necromancy.h"
#include "item/objmisc.h"
#include "item/item_movement_transaction.h"
#include "item/item_ownership_runtime.h"
#include "persistence/persistence_checkpoint.h"
#include "magic/spells.h"
#include "sql/sql.h"
#include "core/safe_format.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern P_char character_list;
extern P_obj object_list;
extern float exp_mods[EXPMOD_MAX + 1];
extern const int top_of_world;

static bool resurrection_item_is_transient(P_obj item)
{
	return item && IS_SET(item->extra_flags, ITEM_TRANSIENT);
}

void spell_unmaking(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		    P_obj obj)
{
	int clevel;
	P_obj cobj, next_obj;

	if (!OBJ_IN_ROOM(obj, ch->in_room))
	{
		send_to_char("Corpse is nowhere to be found, tell a god.\n", ch);
		return;
	}
	if (GET_ITEM_TYPE(obj) == ITEM_CORPSE)
	{
		clevel = obj->value[2];
		if (IS_SET(obj->value[1], PC_CORPSE) && clevel < 0)
			clevel = -clevel;
		if (persistence_defer_corpse_unmaking(obj, ch, level, clevel))
			return;
		/* dump items on ground */
		for (cobj = obj->contains; cobj; cobj = next_obj)
		{
			next_obj = cobj->next_content;
			obj_from_obj(cobj);
			obj_to_room(cobj, ch->in_room);
		}

		if (GET_CLASS(ch, CLASS_THEURGIST))
		{
			act("The $p turns to &+ydust&n and &+wb&+Llow&+ws&n away as its &+Wsoul&n is returned to whence it came.",
			    FALSE, ch, obj, 0, TO_CHAR);
			act("The $p turns to &+ydust&n and &+wb&+Llow&+ws&n away as its &+Wsoul&n is returned to whence it came.",
			    FALSE, ch, obj, 0, TO_ROOM);
		}
		else
		{
			act("&+L$p&+L begins to &n&+gwither&+L and &n&+yrot&+L as you absorb its essence.",
			    FALSE, ch, obj, 0, TO_CHAR);
			act("&+L$p&+L begins to &n&+gwither&+L and &n&+yrot&+L as $n&+L absorbs its essence.",
			    FALSE, ch, obj, 0, TO_ROOM);
		}

		if (GET_MAX_HIT(ch) > GET_HIT(ch))
			GET_HIT(ch) =
				MIN(GET_MAX_HIT(ch), GET_HIT(ch) + (clevel * 4) + (level * 2));
		extract_obj(obj);
		update_pos(ch);
	}
	else
	{
		send_to_char("That is not a corpse!\n", ch);
	}
}

void complete_player_resurrection_after_commit(P_char ch, P_char t_ch, P_obj obj, bool lesser,
					       int old_room)
{
	P_obj obj_in_corpse, next_obj, t_obj;
	struct affected_type *af, *next_af;

	if (!ch || !t_ch || !obj || old_room <= NOWHERE || old_room > top_of_world)
		return;

	if (!lesser && IS_PC(t_ch) && IS_RIDING(t_ch))
		stop_riding(t_ch);

	if (lesser)
	{
		act("$n &+Mhowls in pain&n as $s body &+Lcrumbles to dust.&n", FALSE, t_ch, 0, 0,
		    TO_ROOM);
		act("You &+Mhowl in pain&n as your &+wsoul&n leaves your body to return to your corpse.",
		    FALSE, t_ch, 0, 0, TO_CHAR);
	}
	else
	{
		act("$n &+rhowls &+win pain as $s body crumbles to &+Ldust&+w.&n", FALSE, t_ch, 0,
		    0, TO_ROOM);
		act("&+wYou &+rhowl &+win pain as your soul leaves your body to return to your &+Lcorpse&+w.&n",
		    FALSE, t_ch, 0, 0, TO_CHAR);
	}

	t_ch->only.pc->pc_timer[PC_TIMER_HEAVEN] = 0;
	if (GET_OPPONENT(t_ch))
		stop_fighting(t_ch);
	StopAllAttackers(t_ch);
	if (lesser && IS_PC(t_ch) && IS_RIDING(t_ch))
		stop_riding(t_ch);

	for (af = t_ch->affected; af; af = next_af)
	{
		next_af = af->next;
		if (!(af->flags & AFFTYPE_NODISPEL))
			affect_remove(t_ch, af);
	}

	for (t_obj = t_ch->carrying; t_obj; t_obj = next_obj)
	{
		next_obj = t_obj->next_content;
		if (resurrection_item_is_transient(t_obj))
			extract_obj(t_obj, TRUE);
		else
		{
			obj_from_char(t_obj);
			obj_to_room(t_obj, old_room);
		}
	}
	for (int slot = 0; slot < MAX_WEAR; ++slot)
	{
		if (!t_ch->equipment[slot])
			continue;
		t_obj = unequip_char(t_ch, slot);
		if (IS_SET(t_obj->extra_flags, ITEM_TRANSIENT))
			extract_obj(t_obj, TRUE);
		else
			obj_to_room(t_obj, old_room);
	}

	char_from_room(t_ch);
	char_to_room(t_ch, ch->in_room, -2);
	for (obj_in_corpse = obj->contains; obj_in_corpse; obj_in_corpse = next_obj)
	{
		next_obj = obj_in_corpse->next_content;
		obj_from_obj(obj_in_corpse);
		if (obj_in_corpse->type == ITEM_MONEY)
			extract_obj(obj_in_corpse);
		else
			obj_to_char(obj_in_corpse, t_ch);
	}

	if (lesser)
	{
		act("&+CAn aura of &+Wsoft&n &+Clight surrounds&n $n &+C for a moment.", TRUE, t_ch,
		    0, 0, TO_ROOM);
		act("$n &+Ccomes to life again!&+C Taking a deep breath,&n $n&+C opens $s eyes!",
		    TRUE, t_ch, 0, 0, TO_ROOM);
		act("&+LYou are extremely tired after resurrecting&n $N.", TRUE, ch, 0, t_ch,
		    TO_CHAR);
		act("You are &+yextremely tired&n after being resurrected!", TRUE, t_ch, 0, 0,
		    TO_CHAR);
	}
	else
	{
		if (IS_EVIL(ch))
			act("&+wAn aura of intense &+Lbitter darkness &+wsurrounds&n $n &+wfor a moment.&n",
			    TRUE, t_ch, 0, 0, TO_ROOM);
		else
			act("&+wAn aura of intensely &+Ybright &+Wlight &+wsurrounds&n $n &+wfor a moment.&n",
			    TRUE, t_ch, 0, 0, TO_ROOM);
		act("$n &+wcomes to &+Wlife &+wagain! Taking a &+Cdeep breath&+W, $n opens $s eyes!&n",
		    TRUE, t_ch, 0, 0, TO_ROOM);
		act("&+wYou are &+cextremely tired &+wafter resurrecting $N.", TRUE, ch, 0, t_ch,
		    TO_CHAR);
		act("&+wYou are &+cextremely tired &+wafter being resurrected!&n", TRUE, t_ch, 0, 0,
		    TO_CHAR);
	}

	GET_VITALITY(ch) = MIN(0, GET_VITALITY(ch));
	StartRegen(ch, regen_resource::vitality);
	if (!lesser && !IS_TRUSTED(t_ch))
	{
		long resu_exp = obj->value[4];
		if (resu_exp == 0)
			wizlog(56, "MEMORY ERROR: Player corpse with zero exp!");
		if (!IS_TRUSTED(ch))
		{
			if (GET_LEVEL(t_ch) >= 56)
				resu_exp = static_cast<long>(resu_exp * 0.500);
			else if (EVIL_RACE(t_ch))
				resu_exp = static_cast<long>(resu_exp * exp_mods[EXPMOD_RES_EVIL]);
			else
				resu_exp =
					static_cast<long>(resu_exp * exp_mods[EXPMOD_RES_NORMAL]);
		}
		logit(LOG_EXP,
		      "Resu debug: %s (%d) by %s (%d): old exp: %d, new exp: %ld, +exp: %ld",
		      GET_NAME(t_ch), GET_LEVEL(t_ch), GET_NAME(ch), GET_LEVEL(ch), GET_EXP(t_ch),
		      GET_EXP(t_ch) + resu_exp, resu_exp);
		debug("&+RResurrect&n: %s (%d) by %s (%d): old exp: %d, new exp: %ld, +exp: %ld",
		      GET_NAME(t_ch), GET_LEVEL(t_ch), GET_NAME(ch), GET_LEVEL(ch), GET_EXP(t_ch),
		      GET_EXP(t_ch) + resu_exp, resu_exp);
		gain_exp(t_ch, NULL, resu_exp, EXP_RESURRECT);
	}

	GET_HIT(t_ch) = GET_MAX_HIT(t_ch);
	GET_MANA(t_ch) = MAX(0, GET_MAX_MANA(t_ch) >> 2);
	GET_VITALITY(t_ch) = MIN(0, GET_MAX_VITALITY(t_ch));
	if (GET_COND(t_ch, FULL) > 0)
		GET_COND(t_ch, FULL) = 0;
	if (GET_COND(t_ch, THIRST) > 0)
		GET_COND(t_ch, THIRST) = 0;
	if (!lesser)
		SET_POS(ch, POS_STANDING + STAT_NORMAL);
	StartRegen(t_ch, regen_resource::vitality);
	StartRegen(t_ch, regen_resource::mana);
	if (!lesser && affected_by_spell(t_ch, SPELL_POISON))
		affect_from_char(t_ch, SPELL_POISON);
	if (!lesser && IS_AFFECTED2(t_ch, AFF2_POISONED))
		REMOVE_BIT(t_ch->specials.affected_by2, AFF2_POISONED);

	if (!writeCharacter(t_ch, 1, t_ch->in_room))
	{
		logit(LOG_DEBUG, "Problem saving player %s in spell_resurrect()", GET_NAME(t_ch));
		send_to_char("There was a problem saving your character!\n", t_ch);
		send_to_char("Contact an Implementor ASAP.\n", t_ch);
	}
	extract_obj(obj);
}

void spell_resurrect(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		     P_char /*victim*/, P_obj obj)
{
	bool loss_flag = FALSE;
	int chance, l, found, ss_roll;
	long resu_exp;
	P_obj obj_in_corpse, next_obj, t_obj, money;
	struct affected_type *af, *next_af;
	P_char t_ch;

	if (!IS_ALIVE(ch) || !obj)
	{
		return;
	}

	if (obj->type != ITEM_CORPSE)
	{
		send_to_char("You can only resurrect corpses!\n", ch);
		return;
	}

	if (!GET_CLASS(ch, CLASS_CLERIC) || level < 56)
	{
		CharWait(ch, 25 * WAIT_SEC);
	}
	else
	{
		CharWait(ch, 10 * WAIT_SEC);
	}

	if (IS_NPC(ch) && IS_PC_PET(ch))
	{
		return;
	}

	if (IS_SET(obj->value[1], NPC_CORPSE))
	{
		if (!obj->value[3] || !IS_TRUSTED(ch))
		{
			send_to_char("You can't resurrect this corpse!\n", ch);
			return;
		}
		t_ch = read_mobile(obj->value[3], VIRTUAL);
		if (!t_ch)
		{
			logit(LOG_DEBUG, "spell_resurrect(): mob %d not loadable", obj->value[3]);
			send_to_char("You can't resurrect this corpse!\n", ch);
			return;
		}
		/*
		 * res'ed mobs will mark as their birth the room they are res'ed
		 * in -Neb
		 */
		GET_BIRTHPLACE(t_ch) = world[ch->in_room].number;
		if (!IS_SET(t_ch->specials.act, ACT_MEMORY))
		{
			clearMemory(t_ch);
		}
	}
	else
	{
		// Res a player
		found = 0;

		for (t_ch = character_list; t_ch; t_ch = t_ch->next)
		{
			if (t_ch && IS_PC(t_ch) &&
			    !str_cmp(t_ch->player.name, obj->action_description))
			{
				if (t_ch == ch)
				{
					send_to_char("You can't resurrect your own corpse!\n", ch);
					return;
				}
				if ((obj->value[2] < 0) && !IS_TRUSTED(ch))
				{
					send_to_char("This corpse is not resurrectable.\n", ch);
					return;
				}
				if (!is_linked_to(ch, t_ch, LNK_CONSENT) &&
				    !(IS_TRUSTED(ch) || IS_NPC(ch)))
				{
					/* In AD&D, when a person dies, its not just a matter of losing
					   xp, but a complete end to the char.  Things work differently
					   in the mud, and often a res is a Bad Thing.  Therefore to stop
					   people from PK'ing, res'ing, PK'ing, etc, just to fuck up the
					   persons xp and stats, consent will be REQUIRED to res */
					send_to_char("That person must consent to you first.\n",
						     ch);
					return;
				}
				found = 1;
				break;
			}
		}
		if (!found)
		{
			send_to_char("You can't find a soul to reunite with this corpse!\n", ch);
			return;
		}

		if (GET_PID(t_ch) != obj->value[3])
		{
			if (IS_TRUSTED(ch))
			{
				send_to_char("Different IDs, but you are godly..  enjoy.\n", ch);
			}
			else
			{
				send_to_char("Similar, but not the same!  You fail.\n", ch);
				return;
			}
		}

		if (isCarved(obj))
		{
			if (IS_TRUSTED(ch))
			{
				send_to_char("You regenerate the carved body parts.\n", ch);
			}
			else
			{
				send_to_char("The soul you found rejects this mutilated corpse.\n",
					     ch);
				return;
			}
		}

		/*
		 * new bit, resurrectee must make a CON system shock save, if
		 * they: make the save - spell works, they regain 80% of lost exp.
		 * fail the save by < 50 - spell works, but they lose a semi-random
		 * number of permanent stat points, usually from Con, but other
		 * losses are possible. fail the spell by > 50 - spell fails,and
		 * THIS corpse can never be resurrected (except by a god).  (No stat
		 * losses, but they don't get any exp back either.)
		 *
		 * save maxes at 97% (for players), so it can fail on ANYONE, chance
		 * has to fall below 50% before the spell can completely fail though
		 * (Con (REAL) of < 40), stat losses will be fairly common though.
		 * JAB
		 */

		//    ss_save = con_app[STAT_INDEX(GET_C_CON(t_ch))].shock;
		chance = 90 - (4 * (56 - GET_LEVEL(ch))); /* 90% success at 56, 50% at 46 */
		ss_roll = number(1, 100);

		if (!IS_TRUSTED(ch))
		{
			// if((ss_save + (100 - ss_save) / 2) < ss_roll) {
			if (ss_roll > chance)
			{
				// Complete failure, corpse is unressable.
				logit(LOG_DEATH,
				      "%s ressed %s:  Failed roll: %3d Chance: %3d Level: %2d",
				      GET_NAME(ch), GET_NAME(t_ch), ss_roll, chance, level);
			}
		}

		if (persistence_defer_corpse_resurrection(obj, ch, t_ch, false))
			return;

		if (IS_PC(t_ch) && IS_RIDING(t_ch))
		{
			stop_riding(t_ch);
		}

		act("$n &+rhowls &+win pain as $s body crumbles to &+Ldust&+w.&n", FALSE, t_ch, 0,
		    0, TO_ROOM);
		act("&+wYou &+rhowl &+win pain as your soul leaves your body to return to your &+Lcorpse&+w.&n",
		    FALSE, t_ch, 0, 0, TO_CHAR);

		t_ch->only.pc->pc_timer[PC_TIMER_HEAVEN] = 0;

		if (GET_OPPONENT(t_ch))
		{
			stop_fighting(t_ch);
		}
		StopAllAttackers(t_ch);

		for (af = t_ch->affected; af; af = next_af)
		{
			next_af = af->next;
			if (!(af->flags & AFFTYPE_NODISPEL))
			{
				affect_remove(t_ch, af);
			}
		}

		if (GET_MONEY(t_ch) > 0)
		{
			/*
			 * make a 'pile of coins' object to hold victim's cash
			 */

			money = create_money(GET_COPPER(t_ch), GET_SILVER(t_ch), GET_GOLD(t_ch),
					     GET_PLATINUM(t_ch));
			SUB_MONEY(t_ch, GET_MONEY(t_ch), 0);
			obj_to_room(money, t_ch->in_room);
		}
		for (t_obj = t_ch->carrying; t_obj != NULL; t_obj = next_obj)
		{
			next_obj = t_obj->next_content;
			if (resurrection_item_is_transient(t_obj))
			{
				extract_obj(t_obj, TRUE); // Transient artis?
				t_obj = NULL;
			}
			else
			{
				obj_from_char(t_obj);
				obj_to_room(t_obj, t_ch->in_room);
			}
		}

		/*
		 * clear equipment_list
		 */
		for (l = 0; l < MAX_WEAR; l++)
		{
			if (t_ch->equipment[l])
			{
				t_obj = unequip_char(t_ch, l);
				if (IS_SET(t_obj->extra_flags, ITEM_TRANSIENT))
				{
					extract_obj(t_obj, TRUE); // Transient artis?
					t_obj = NULL;
				}
				else
				{
					obj_to_room(t_obj, t_ch->in_room);
				}
			}
		}
		char_from_room(t_ch);
	}
	char_to_room(t_ch, ch->in_room, -2);

	/*
	 * move objects in corpse to victim's inventory
	 */
	for (obj_in_corpse = obj->contains; obj_in_corpse; obj_in_corpse = next_obj)
	{
		next_obj = obj_in_corpse->next_content;
		obj_from_obj(obj_in_corpse);
		if (obj_in_corpse->type == ITEM_MONEY)
		{
			ADD_MONEY(t_ch, obj_in_corpse->value[0] + obj_in_corpse->value[1] * 10 +
						obj_in_corpse->value[2] * 100 +
						obj_in_corpse->value[3] * 1000);
			extract_obj(obj_in_corpse);
			obj_in_corpse = NULL;
		}
		else
		{
			obj_to_char(obj_in_corpse, t_ch);
		}
	}

	if (IS_EVIL(ch))
	{
		act("&+wAn aura of intense &+Lbitter darkness &+wsurrounds&n $n &+wfor a moment.&n",
		    TRUE, t_ch, 0, 0, TO_ROOM);
	}
	else
	{
		act("&+wAn aura of intensely &+Ybright &+Wlight &+wsurrounds&n $n &+wfor a moment.&n",
		    TRUE, t_ch, 0, 0, TO_ROOM);
	}
	act("$n &+wcomes to &+Wlife &+wagain! Taking a &+Cdeep breath&+W, $n opens $s eyes!&n",
	    TRUE, t_ch, 0, 0, TO_ROOM);
	act("&+wYou are &+cextremely tired &+wafter resurrecting $N.", TRUE, ch, 0, t_ch, TO_CHAR);

	if (loss_flag)
	{
		act("You feel drained!", TRUE, t_ch, 0, 0, TO_CHAR);
	}
	act("&+wYou are &+cextremely tired &+wafter being resurrected!&n", TRUE, t_ch, 0, 0,
	    TO_CHAR);

	GET_VITALITY(ch) = MIN(0, GET_VITALITY(ch));
	StartRegen(ch, regen_resource::vitality);

	/*
	 * restore lost exp from death
	 */

	if (IS_PC(t_ch) && !IS_TRUSTED(t_ch))
	{
		resu_exp = obj->value[4];
		if (resu_exp == 0)
		{
			// send_to_char("&-RERROR! Player corpse with zero exp, please notify a god!&n\r\n", ch);
			wizlog(56, "MEMORY ERROR: Player corpse with zero exp!");
		}

		if (!IS_TRUSTED(ch))
		{
			if (GET_LEVEL(t_ch) >= 56)
			{
				resu_exp = (long)(resu_exp * 0.500);
			}
			else if (EVIL_RACE(t_ch))
			{
				resu_exp = (long)(resu_exp * exp_mods[EXPMOD_RES_EVIL]);
			}
			else
			{
				resu_exp = (long)(resu_exp * exp_mods[EXPMOD_RES_NORMAL]);
			}
		}

		logit(LOG_EXP,
		      "Resu debug: %s (%d) by %s (%d): old exp: %d, new exp: %ld, +exp: %ld",
		      GET_NAME(t_ch), GET_LEVEL(t_ch), GET_NAME(ch), GET_LEVEL(ch), GET_EXP(t_ch),
		      GET_EXP(t_ch) + resu_exp, resu_exp);
		debug("&+RResurrect&n: %s (%d) by %s (%d): old exp: %d, new exp: %ld, +exp: %ld",
		      GET_NAME(t_ch), GET_LEVEL(t_ch), GET_NAME(ch), GET_LEVEL(ch), GET_EXP(t_ch),
		      GET_EXP(t_ch) + resu_exp, resu_exp);

		gain_exp(t_ch, NULL, resu_exp, EXP_RESURRECT);
	}

	GET_HIT(t_ch) = GET_MAX_HIT(t_ch);
	GET_MANA(t_ch) = MAX(0, GET_MAX_MANA(t_ch) >> 2);
	GET_VITALITY(t_ch) = MIN(0, GET_MAX_VITALITY(t_ch));

	if (GET_COND(t_ch, FULL) > 0)
	{
		GET_COND(t_ch, FULL) = 0;
	}
	if (GET_COND(t_ch, THIRST) > 0)
	{
		GET_COND(t_ch, THIRST) = 0;
	}

	SET_POS(ch, POS_STANDING + STAT_NORMAL);

	StartRegen(t_ch, regen_resource::vitality);
	StartRegen(t_ch, regen_resource::mana);

	if (t_ch && affected_by_spell(t_ch, SPELL_POISON))
	{
		affect_from_char(t_ch, SPELL_POISON);
	}
	if (t_ch && IS_AFFECTED2(t_ch, AFF2_POISONED))
	{
		REMOVE_BIT(t_ch->specials.affected_by2, AFF2_POISONED);
	}

	/*
	 * Added by DTS 7/30/95
	 */
	if (IS_PC(t_ch) && !writeCharacter(t_ch, 1, t_ch->in_room))
	{
		logit(LOG_DEBUG, "Problem saving player %s in spell_resurrect()", GET_NAME(t_ch));
		send_to_char("There was a problem saving your character!\n", t_ch);
		send_to_char("Contact an Implementor ASAP.\n", t_ch);
	}

	extract_obj(obj);
}

void spell_preserve(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		    P_char /*victim*/, P_obj obj)
{
	char Gbuf1[MAX_STRING_LENGTH];

	if (!(obj))
		return;

	if (obj->type != ITEM_CORPSE)
	{
		send_to_char("You can only preserve a corpse!\n", ch);
		return;
	}
	// find a decay affect
	struct obj_affect *af;
	af = get_obj_affect(obj, TAG_OBJ_DECAY);

	if (!af)
	{
		act("$p doesn't seem to be in any danger of decaying.", TRUE, ch, obj, 0, TO_CHAR);
		return;
	}
	else if (!IS_TRUSTED(ch))
	{
		unsigned timePres;
		timePres = MAX(10, (level / 2));
		snprintf(Gbuf1, MAX_STRING_LENGTH, "$p is preserved for an additional %d hours.",
			 timePres);
		act(Gbuf1, 0, ch, obj, 0, TO_CHAR);
		act("$p glows briefly.", 0, ch, obj, 0, TO_ROOM);

		// convert timePres into game pulses
		timePres *= (SECS_PER_MUD_HOUR * WAIT_SEC);
		// add the old event time to timePres
		timePres += obj_affect_time(obj, af);
		// remove the old affect
		affect_from_obj(obj, TAG_OBJ_DECAY);
		// and create a new one
		set_obj_affected(obj, timePres, TAG_OBJ_DECAY, 0);
	}
	else
	{
		// just remove the decay affect completely.
		affect_from_obj(obj, TAG_OBJ_DECAY);

		act("$p is preserved forever!", FALSE, ch, obj, 0, TO_CHAR);
		act("$p glows brightly!", FALSE, ch, obj, 0, TO_ROOM);
	}
	if (obj && (obj->type == ITEM_CORPSE) && IS_SET(obj->value[1], PC_CORPSE))
		writeCorpse(obj);
}

void spell_lesser_resurrect(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			    P_char /*victim*/, P_obj obj)
{
	bool loss_flag = FALSE;
	int chance, l, found, ss_roll;
	P_obj obj_in_corpse, next_obj, t_obj, money;
	struct affected_type *af, *next_af;
	P_char t_ch;

	if (!IS_ALIVE(ch) || !(obj))
		return;

	if (obj->type != ITEM_CORPSE)
	{
		send_to_char("You can only resurrect corpses!\n", ch);
		return;
	}

	if (IS_NPC(ch))
		return;

	if ((GET_CHAR_SKILL(ch, SKILL_DEVOTION) <= 20 && !IS_TRUSTED(ch)) ||
	    (GET_CLASS(ch, CLASS_SHAMAN) && level < 52))
	{
		CharWait(ch, 100);
	}

	if (IS_SET(obj->value[1], NPC_CORPSE))
	{
		if (!obj->value[3] || !IS_TRUSTED(ch))
		{
			send_to_char("You can't resurrect this corpse!\n", ch);
			return;
		}
		t_ch = read_mobile(obj->value[3], VIRTUAL);
		if (!t_ch)
		{
			logit(LOG_DEBUG, "spell_resurrect(): mob %d not loadable", obj->value[3]);
			send_to_char("You can't resurrect this corpse!\n", ch);
			return;
		}
		/*
		 * res'ed mobs will mark as their birth the room they are res'ed
		 * in -Neb
		 */
		GET_BIRTHPLACE(t_ch) = world[ch->in_room].number;
		if (!IS_SET(t_ch->specials.act, ACT_MEMORY))
			clearMemory(t_ch);
	}
	else
	{
		/*
		 * res a player
		 */

		found = 0;

		for (t_ch = character_list; t_ch; t_ch = t_ch->next)
		{
			if (t_ch && IS_PC(t_ch) &&
			    !str_cmp(t_ch->player.name, obj->action_description))
			{
				if (t_ch == ch)
				{
					send_to_char("You can't resurrect your own corpse!\n", ch);
					return;
				}
				if ((obj->value[2] < 0) && !IS_TRUSTED(ch))
				{
					send_to_char("This corpse is not resurrectable.\n", ch);
					return;
				}
				if (!is_linked_to(ch, t_ch, LNK_CONSENT) && !IS_TRUSTED(ch))
				{
					/* In AD&D, when a person dies, its not just a matter of losing
					   xp, but a complete end to the char.  Things work differently
					   in the mud, and often a res is a Bad Thing.  Therefore to stop
					   people from PK'ing, res'ing, PK'ing, etc, just to fuck up the
					   persons xp and stats, consent will be REQUIRED to res */
					send_to_char("That person must consent to you first.\n",
						     ch);
					return;
				}
				found = 1;
				break;
			}
		}
		if (!found)
		{
			send_to_char("You can't find a soul to reunite with this corpse!\n", ch);
			return;
		}

		if (GET_PID(t_ch) != obj->value[3])
		{
			if (IS_TRUSTED(ch))
				send_to_char("Different IDs, but you are godly..  enjoy.\n", ch);
			else
			{
				send_to_char("Similar, but not the same!  You fail.\n", ch);
				return;
			}
		}

		if (isCarved(obj))
		{
			if (IS_TRUSTED(ch))
				send_to_char("You regenerate the carved body parts.\n", ch);
			else
			{
				send_to_char("The soul you found rejects this mutilated corpse.\n",
					     ch);
				return;
			}
		}

		/*
		 * new bit, resurrectee must make a CON system shock save, if
		 * they: make the save - spell works, they regain 80% of lost exp.
		 * fail the save by < 50 - spell works, but they lose a semi-random
		 * number of permanent stat points, usually from Con, but other
		 * losses are possible. fail the spell by > 50 - spell fails,and
		 * THIS corpse can never be resurrected (except by a god).  (No stat
		 * losses, but they don't get any exp back either.)
		 *
		 * save maxes at 97% (for players), so it can fail on ANYONE, chance
		 * has to fall below 50% before the spell can completely fail though
		 * (Con (REAL) of < 40), stat losses will be fairly common though.
		 * JAB
		 */

		//    ss_save = con_app[STAT_INDEX(GET_C_CON(t_ch))].shock;
		chance = 90 - (4 * (56 - GET_LEVEL(ch))); /* 90% success at 56, 50% at 46 */
		ss_roll = number(1, 100);

		if (!IS_TRUSTED(ch) && level <= 50)
		{
			/*
			   if((ss_save + (100 - ss_save) / 2) < ss_roll) {
			 */
			if (ss_roll > chance)
			{
				/*
				 * complete failure, corpse is unressable.
				 */
				logit(LOG_DEATH,
				      "%s ressed %s:  Failed roll: %3d Chance: %3d Level: %2d",
				      GET_NAME(ch), GET_NAME(t_ch), ss_roll, chance, level);
			}
		}

		if (persistence_defer_corpse_resurrection(obj, ch, t_ch, true))
			return;

		act("$n &+Mhowls in pain&n as $s body &+Lcrumbles to dust.&n", FALSE, t_ch, 0, 0,
		    TO_ROOM);
		act("You &+Mhowl in pain&n as your &+wsoul&n leaves your body to return to your corpse.",
		    FALSE, t_ch, 0, 0, TO_CHAR);

		t_ch->only.pc->pc_timer[PC_TIMER_HEAVEN] = 0;

		if (GET_OPPONENT(t_ch))
			stop_fighting(t_ch);
		StopAllAttackers(t_ch);

		if (IS_PC(t_ch) && IS_RIDING(t_ch))
			stop_riding(t_ch);

		for (af = t_ch->affected; af; af = next_af)
		{
			next_af = af->next;
			if (!(af->flags & AFFTYPE_NODISPEL))
				affect_remove(t_ch, af);
		}

		if (GET_MONEY(t_ch) > 0)
		{
			/*
			 * make a 'pile of coins' object to hold victim's cash
			 */

			money = create_money(GET_COPPER(t_ch), GET_SILVER(t_ch), GET_GOLD(t_ch),
					     GET_PLATINUM(t_ch));
			SUB_MONEY(t_ch, GET_MONEY(t_ch), 0);
			obj_to_room(money, t_ch->in_room);
		}
		for (t_obj = t_ch->carrying; t_obj != NULL; t_obj = next_obj)
		{
			next_obj = t_obj->next_content;
			// WHY ON EARTH WOULD WE WANT TO DO THIS? - KVARK
			//      if(IS_ROOM(t_ch->in_room, ROOM_DEATH) || IS_SET(obj->extra_flags, ITEM_TRANSIENT))
			if (resurrection_item_is_transient(t_obj))
			{
				extract_obj(t_obj, TRUE); // Transient artis?
				t_obj = NULL;
			}
			else
			{
				obj_from_char(t_obj);
				obj_to_room(t_obj, t_ch->in_room);
			}
		}

		/*
		 * clear equipment_list
		 */
		for (l = 0; l < MAX_WEAR; l++)
			if (t_ch->equipment[l])
			{
				t_obj = unequip_char(t_ch, l);
				/*
				 * below used to contain: IS_ROOM(t_ch->in_room, ROOM_DEATH)
				 * Alas, that is not good.
				 * /
				 */
				if (IS_SET(t_obj->extra_flags, ITEM_TRANSIENT))
				{
					extract_obj(t_obj, TRUE); // Transient artis?
					t_obj = NULL;
				}
				else
					obj_to_room(t_obj, t_ch->in_room);
			}
		char_from_room(t_ch);
	}
	char_to_room(t_ch, ch->in_room, -2);

	/*
	 * move objects in corpse to victim's inventory
	 */
	for (obj_in_corpse = obj->contains; obj_in_corpse; obj_in_corpse = next_obj)
	{
		next_obj = obj_in_corpse->next_content;
		obj_from_obj(obj_in_corpse);
		if (obj_in_corpse->type == ITEM_MONEY)
		{
			ADD_MONEY(t_ch, obj_in_corpse->value[0] + obj_in_corpse->value[1] * 10 +
						obj_in_corpse->value[2] * 100 +
						obj_in_corpse->value[3] * 1000);
			extract_obj(obj_in_corpse);
			obj_in_corpse = NULL;
		}
		else
			obj_to_char(obj_in_corpse, t_ch);
	}

	act("&+CAn aura of &+Wsoft&n &+Clight surrounds&n $n &+C for a moment.", TRUE, t_ch, 0, 0,
	    TO_ROOM);
	act("$n &+Ccomes to life again!&+C Taking a deep breath,&n $n&+C opens $s eyes!", TRUE,
	    t_ch, 0, 0, TO_ROOM);
	act("&+LYou are extremely tired after resurrecting&n $N.", TRUE, ch, 0, t_ch, TO_CHAR);
	if (loss_flag)
		act("You feel drained!", TRUE, t_ch, 0, 0, TO_CHAR);
	act("You are &+yextremely tired&n after being resurrected!", TRUE, t_ch, 0, 0, TO_CHAR);

	GET_VITALITY(ch) = MIN(0, GET_VITALITY(ch));
	StartRegen(ch, regen_resource::vitality);

	/*
	 * restore lost exp from death
	 */

	GET_HIT(t_ch) = GET_MAX_HIT(t_ch);
	GET_MANA(t_ch) = MAX(0, GET_MAX_MANA(t_ch) >> 2);
	GET_VITALITY(t_ch) = MIN(0, GET_MAX_VITALITY(t_ch));

	if (GET_COND(t_ch, FULL) > 0)
		GET_COND(t_ch, FULL) = 0;
	if (GET_COND(t_ch, THIRST) > 0)
		GET_COND(t_ch, THIRST) = 0;

	StartRegen(t_ch, regen_resource::vitality);
	StartRegen(t_ch, regen_resource::mana);

	/*
	 * Added by DTS 7/30/95
	 */
	if (IS_PC(t_ch) && !writeCharacter(t_ch, 1, t_ch->in_room))
	{
		logit(LOG_DEBUG, "Problem saving player %s in spell_resurrect()", GET_NAME(t_ch));
		send_to_char("There was a problem saving your character!\n", t_ch);
		send_to_char("Contact an Implementor ASAP.\n", t_ch);
	}
	/*  if(clevel == 56)
	    advance_level(t_ch);*/
	extract_obj(obj);
}

void spell_mass_embalm(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		       P_char victim, P_obj obj)
{
	for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
	{
		if (obj->type == ITEM_CORPSE)
			spell_embalm(level, ch, 0, SPELL_TYPE_SPELL, victim, obj);
	}
}

void spell_mass_preserve(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			 P_char victim, P_obj obj)
{
	for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
	{
		if (obj->type == ITEM_CORPSE)
			spell_preserve(level, ch, 0, 0, victim, obj);
	}
}

void spell_embalm(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		  P_char /*victim*/, P_obj obj)
{
	char buf[MAX_STRING_LENGTH];
	unsigned embalm_time;

	/*
	 * Check to if it is a corpse.
	 */
	if (obj->type != ITEM_CORPSE)
	{
		send_to_char("All you know how to preserve is corpses!\n", ch);
		return;
	}

	// find a decay affect
	struct obj_affect *af;
	af = get_obj_affect(obj, TAG_OBJ_DECAY);

	if (!af)
	{
		act("$p probably will not decay in the near future.", TRUE, ch, obj, 0, TO_CHAR);
		return;
	}
	embalm_time = MAX(50, level * 2);

	snprintf(buf, MAX_STRING_LENGTH, "$p is preserved for an additional %d hours.",
		 embalm_time);
	act(buf, 0, ch, obj, 0, TO_CHAR);
	act("$p glows &+rblood red&n briefly.", 0, ch, obj, 0, TO_ROOM);

	// convert embalm_time into game pulses
	embalm_time *= (SECS_PER_MUD_HOUR * WAIT_SEC);
	// add the old event time to embalm_time
	embalm_time += obj_affect_time(obj, af);
	// remove the old affect
	affect_from_obj(obj, TAG_OBJ_DECAY);
	// and create a new one
	set_obj_affected(obj, embalm_time, TAG_OBJ_DECAY, 0);

	if (obj && (obj->type == ITEM_CORPSE) && IS_SET(obj->value[1], PC_CORPSE))
		writeCorpse(obj);

	return;
}

void spell_corpse_portal(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			 P_obj /*obj*/)
{
	P_obj tobj;
	int found = 0;
	int location;

	// find most recent corpse
	for (tobj = object_list; tobj; tobj = tobj->next)
	{
		if (400220 == obj_index[tobj->R_num].virtual_number &&
		    isname(GET_NAME(victim), tobj->name))
		{
			found = 1;
			break;
		}
	}

	// if no corpse, fail!
	if (!found || tobj->loc_p != LOC_ROOM)
	{
		act("&+L$n attempts to call upon the spirit realm, but is unable to make a connection.&N",
		    TRUE, ch, 0, 0, TO_ROOM);
		act("&+gThe &+Gspirit &+grealm fails to answer your call for help.&N", FALSE, ch, 0,
		    0, TO_CHAR);
		return;
	}

	location = world[ch->in_room].number;

	act("&+G$n's &+gbody begins to shake violently as if possessed by some &+Gother worldly &+gpower. With a blast of &+Gghastly &+Yflames&+g a portal suddenly materializes before them.&N",
	    TRUE, ch, 0, 0, TO_ROOM);
	act("&+gYour body calls out to the &+Gspirit&+g realm, begging for assistance.  Every fiber within your body begins to &+Gtingle&+g as a &+Gghastly &+gessence begins to fill the room. Suddenly "
	    "and without warning, a &+Gmystic &+gportal materializes before you, beckoning you to enter.&N",
	    FALSE, ch, 0, 0, TO_CHAR);
	obj_from_room(tobj);
	obj_to_room(tobj, real_room(location));
}
