/* Special procedures for the unique item area. */

#include <time.h>

#include "core/prototypes.h"
#include "combat/defense_resolution.h"
#include "combat/spell_wards.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "item/native_artifact_actions.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "world/db.h"
#include "world/handler.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"

extern P_room world;

int dranum_mask(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	P_char vict;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!OBJ_WORN_POS(obj, WEAR_FACE) || cmd != CMD_PERIODIC || ch)
	{
		return FALSE;
	}
	if (!(ch = obj->loc.wearing))
	{
		return FALSE;
	}

	// 1/10 chance.
	if (IS_FIGHTING(ch) && !number(0, 9))
	{
		vict = GET_OPPONENT(ch);
		// no cheesing mobs/dead ppl.
		if (!IS_ALIVE(vict) || IS_NPC(vict))
		{
			return FALSE;
		}
		act("$p &+Rscares &N&+rthe &+Rliving &+LSHIT &N&+rout of $N!&N", TRUE, ch, obj,
		    vict, TO_CHAR);
		act("&+r$n's&N $p &+Rscares &N&+rthe &+Rliving &+LSHIT &N&+rout of $N!&N", TRUE, ch,
		    obj, vict, TO_NOTVICT);
		act("&+r$n's&N $p &+Rscares &N&+rthe &+Rliving &+LSHIT &N&+rout of YOU!&N", TRUE,
		    ch, obj, vict, TO_VICT);
		if (!fear_check(vict))
		{
			do_flee(vict, 0, 2);
			return TRUE;
		}
	}
	return FALSE;
}

int golem_chunk(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char tar_ch, next;
	int curr_time;
	struct affected_type af;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (cmd == CMD_PERIODIC)
	{
		if (!IS_SET(obj->extra_flags, ITEM_NODROP))
		{
			SET_BIT(obj->extra_flags, ITEM_NODROP);
		}
		if (OBJ_WORN(obj) && (ch = obj->loc.wearing))
		{
			// 1/5 chance.
			if (IS_ALIVE(ch) && !affected_by_spell(ch, SPELL_CURSE) && !number(0, 4) &&
			    !NewSaves(ch, SAVING_SPELL, 0))
			{
				bzero(&af, sizeof(af));
				af.type = SPELL_CURSE;
				af.duration = 24;
				af.modifier = -2;
				af.location = APPLY_HITROLL;
				affect_to_char(ch, &af);
				af.modifier = 5;
				af.location = APPLY_CURSE;
				affect_to_char(ch, &af);
				act("&+LThe Chu&Nnk &+Lon $n shakes vigoriously &+Wtossing&N them side to side then suddenly stops.&N",
				    FALSE, ch, obj, 0, TO_ROOM);
				act("&+LThe Chu&Nnk&+L shakes vigoriously, &+Wtossing&N you side to side, then suddenly stope.&N",
				    FALSE, ch, obj, 0, TO_CHAR);
			}
		}
		return TRUE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}

	if (arg && (cmd == CMD_SAY))
	{
		if (isname(arg, "slow"))
		{
			curr_time = time(NULL);
			// Every 24 minutes?
			if (obj->timer[0] + (60 * 24) <= curr_time)
			{
				obj->timer[0] = curr_time;
				act("&+L$n's $p&+L billows forth an enormous cloud of smoke enblankening you.&N",
				    FALSE, ch, obj, 0, TO_ROOM);
				act("&+LYour $p&+L billows forth an enormous cloud of smoke emblankening everything.&N",
				    FALSE, ch, obj, 0, TO_CHAR);
				for (tar_ch = world[ch->in_room].people; tar_ch; tar_ch = next)
				{
					next = tar_ch->next_in_room;
					if (tar_ch != ch && tar_ch->group != ch->group)
					{
						spell_slow(55, ch, 0, SPELL_TYPE_SPELL, tar_ch, 0);
					}
				}
				return TRUE;
			}
		}
	}
	return FALSE;
}

int mace_of_sea(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}
	victim = legacy_proc_arg<P_char>(arg);
	// 1/30 chance.
	if (!IS_ALIVE(victim) || number(0, 29))
	{
		return FALSE;
	}

	act("&+b$n's&N $q &n&+rglows blue...&N", TRUE, ch, obj, victim, TO_NOTVICT);
	act("&+bYour&N $q &n&+rglows blue...&N", TRUE, ch, obj, victim, TO_CHAR);
	act("&+b$n's&N $q &n&+rglows blue...&N", TRUE, ch, obj, victim, TO_VICT);
	// spell_lightning_bolt(40, ch, NULL, 0, victim, obj);
	spell_dread_wave(40, ch, NULL, 0, victim, obj);
	return TRUE;
}

int serpent_blade(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}
	victim = legacy_proc_arg<P_char>(arg);
	// 1/30 chance.
	if (!IS_ALIVE(victim) || number(0, 29))
	{
		return FALSE;
	}

	act("$p&+L carried by $n &+WLASHES &+Rout as it bites $N!&N", TRUE, ch, obj, victim,
	    TO_NOTVICT);
	act("$p&+L carried by you &+WLASHES &+Rout as it bites $N!&N", TRUE, ch, obj, victim,
	    TO_CHAR);
	act("$p&+L carried by $n &+WLASHES &+Rout as it bites you!&N", TRUE, ch, obj, victim,
	    TO_VICT);
	spell_poison(40, ch, 0, 0, victim, obj);
	spell_minor_paralysis(40, ch, NULL, 0, victim, obj);
	return TRUE;
}

int lich_spine(P_obj obj, P_char ch, int cmd, char *arg)
{
	int save;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}
	victim = legacy_proc_arg<P_char>(arg);
	// 1/30 chance.
	if (!IS_ALIVE(victim) || number(0, 29))
	{
		return FALSE;
	}
	act("&+b$n's&N $q &n&+rglows with a devilish light...&N", TRUE, ch, obj, victim,
	    TO_NOTVICT);
	act("&+bYour&N $q &n&+rglows with a devilish light...&N", TRUE, ch, obj, victim, TO_CHAR);
	act("&+b$n's&N $q &n&+rglows with a devilish light...&N", TRUE, ch, obj, victim, TO_VICT);
	save = victim->specials.apply_saving_throw[SAVING_SPELL];
	victim->specials.apply_saving_throw[SAVING_SPELL] = 20;
	spell_wither(60, ch, NULL, 0, victim, obj);
	victim->specials.apply_saving_throw[SAVING_SPELL] = save;

	return TRUE;
}

int demo_scimitar(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}
	victim = legacy_proc_arg<P_char>(arg);
	// 1/25 chance.
	if (!IS_ALIVE(victim) || number(0, 24))
	{
		return FALSE;
	}

	act("$p &+Lcarried by $n&+L slices into $N's&+L soul...&n", TRUE, ch, obj, victim,
	    TO_NOTVICT);
	act("Your $q &+Lslices into $N's&+L soul...&n", TRUE, ch, obj, victim, TO_CHAR);
	act("$p &+Lcarried by $n&+L slices into your&+L soul...&n", TRUE, ch, obj, victim, TO_VICT);

	GET_VITALITY(victim) = MAX(0, GET_VITALITY(victim) - (number(10, 40)));

	act("&+L$N&+L goes limp for a moment.&n", FALSE, ch, 0, victim, TO_NOTVICT);
	act("&+LYou feel your body&+L go limp for a moment.&n", FALSE, ch, 0, victim, TO_VICT);
	act("&+L$N&+L goes limp for a moment.&n", FALSE, ch, 0, victim, TO_CHAR);

	return TRUE;
}

int church_door(P_obj obj, P_char ch, int cmd, char *arg)
{
	int curr_time;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}

	if (arg && (cmd == CMD_RUB))
	{
		if (isname(arg, "glass"))
		{
			curr_time = time(NULL);

			if (obj->timer[0] + 15 <= curr_time && !affected_by_spell(ch, SPELL_ARMOR))
			{
				act("$p &N&+rhums&N&+y briefly as you are &+Wenveloped&N&+y in a &+Bmagical&+C force&+R field.&N",
				    FALSE, ch, obj, obj, TO_CHAR);
				act("$p &N&+rhums&+y briefly as $n is &+Wenveloped&N&+y in a &+Bmagical &+Cforce&+R field.&N ",
				    TRUE, ch, obj, NULL, TO_ROOM);
				spell_armor(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
				spell_bless(50, ch, 0, SPELL_TYPE_SPELL, ch, 0);
				act("&+yThe &+Cforce &+Rfield&N&+y subsides and you feel able to better withstand your foes.&N",
				    TRUE, ch, obj, NULL, TO_CHAR);
				obj->timer[0] = curr_time;
			}
			else
			{
				act("$p &N&+rhums&N&+y briefly and is quiet.&N", FALSE, ch, obj,
				    obj, TO_CHAR);
				act("$p &N&+rhums&+y briefly and is quiet.&N ", TRUE, ch, obj, NULL,
				    TO_ROOM);
			}
			return TRUE;
		}
	}
	return FALSE;
}

int sword_whirlwinds(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (cmd != CMD_MELEE_HIT || !IS_ALIVE(ch) || !OBJ_WORN_BY(obj, ch))
	{
		return FALSE;
	}
	victim = legacy_proc_arg<P_char>(arg);
	// 1/30 chance.
	if (!IS_ALIVE(victim) || number(0, 29))
	{
		return FALSE;
	}

	act("&+b$n's&N $q &n&+rglows blue...&N", TRUE, ch, obj, victim, TO_NOTVICT);
	act("&+bYour&N $q &n&+rglows blue...&N", TRUE, ch, obj, victim, TO_CHAR);
	act("&+b$n's&N $q &n&+rglows blue...&N", TRUE, ch, obj, victim, TO_VICT);
	// spell_lightning_bolt(40, ch, NULL, 0, victim, obj);
	// spell_asphyxiate( 51, ch, NULL, 0, victim, obj);
	spell_cyclone(46, ch, NULL, 0, victim, obj);
	spell_cyclone(46, ch, NULL, 0, victim, obj);
	spell_cyclone(46, ch, NULL, 0, victim, obj);
	return TRUE;
}

int rod_of_magic(P_obj obj, P_char ch, int cmd, char *arg)
{
	int dam = cmd / 1000;
	P_char victim;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!dam || !IS_ALIVE(ch) || !OBJ_WORN(obj) || obj->loc.wearing != ch)
	{
		return FALSE;
	}

	victim = legacy_proc_arg<P_char>(arg);
	// 1/16 chance.
	if (!IS_ALIVE(victim) || number(0, 15))
	{
		return (FALSE);
	}

	act("&+L$n's&N $q &+Lspews &+wforth &+mmeta&+Bmagic&+W!&N", TRUE, ch, obj, victim,
	    TO_NOTVICT);
	act("&+LYour&N $q &+Lspews &+wforth &+mmeta&+Bmagic&+W!&N", TRUE, ch, obj, victim, TO_CHAR);
	act("&+L$n's&N $q &+Lspews &+wforth &+mmeta&+Bmagic&+W!&N&N", TRUE, ch, obj, victim,
	    TO_VICT);
	if (victim)
	{
		spell_stornogs_lowered_magical_res(60, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
	}
	return TRUE;
}

int living_necroplasm(P_obj obj, P_char ch, int cmd, char * /*arg*/)
{
	P_char i;
	int slot;
	struct affected_type af;
	bool bHasOtherArti;
	int plasm_slot;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!obj || cmd != CMD_PERIODIC)
		return FALSE;
	const bool modern = native_artifact_owns(67243);

	if (OBJ_WORN(obj))
		ch = obj->loc.wearing;
	else if (OBJ_CARRIED(obj))
		ch = obj->loc.carrying;

	// recurse self
	if (!IS_SET(obj->extra_flags, ITEM_NODROP))
	{
		SET_BIT(obj->extra_flags, ITEM_NODROP);
	}

	// It's on body
	if (OBJ_WORN_BY(obj, ch) && !(obj == ch->equipment[HOLD]))
	{
		// verify that they have no other arti's - if they do, necro will poof from them
		bHasOtherArti = FALSE;
		plasm_slot = MAX_WEAR;
		for (int i = 0; i < MAX_WEAR; i++)
		{
			if (ch->equipment[i] == obj)
			{
				plasm_slot = i;
				continue;
			}
			if (ch->equipment[i] && IS_ARTIFACT(ch->equipment[i]))
			{
				bHasOtherArti = TRUE;
			}
		}
		if (bHasOtherArti && (plasm_slot != MAX_WEAR) && ch->equipment[plasm_slot])
		{
			act("$p &+Rburns &+ran angry red &+Las it retreats from $n's body&n", FALSE,
			    ch, obj, 0, TO_ROOM);
			act("$p &+Rburns &+ran angry red &+Las it retreats from your body&n", FALSE,
			    ch, obj, 0, TO_CHAR);
			obj_to_char(unequip_char(ch, plasm_slot), ch);
			// dispel any SPELL_VAMPIRE to prevent cheesing of removing arti, wearing plasm, wearing other arti.
			if (!modern)
				affect_from_char(ch, SPELL_VAMPIRE);
			return TRUE;
		}
		if (IS_PC(ch) && !number(0, 3) && !NewSaves(ch, SAVING_PARA, 6))
		{
			act("&+MYou feel queasy as $p &+Msends its tendrils deep into your body, harvesting your lifeforce!",
			    FALSE, ch, obj, 0, TO_CHAR);
			GET_HIT(ch) = MAX(1, GET_HIT(ch) - 20);
		}
		if (!affected_by_spell(ch, SPELL_CURSE) && !number(0, 4) &&
		    !NewSaves(ch, SAVING_SPELL, 0))
		{
			bzero(&af, sizeof(af));
			af.type = SPELL_CURSE;
			af.duration = 24;
			af.modifier = -2;
			af.location = APPLY_HITROLL;
			affect_to_char(ch, &af);
			af.modifier = 5;
			af.location = APPLY_CURSE;
			affect_to_char(ch, &af);
			act("&+r$n &+rhowls in pain as $s $q &+rglows &+Rred hot!", FALSE, ch, obj,
			    0, TO_ROOM);
			act("&+rYou howl in pain as your $q &+rglows &+Rred hot!", FALSE, ch, obj,
			    0, TO_CHAR);
		}
		if (modern)
		{
			// Equipment, class restrictions and harmful symbiosis stay native.
			// Only the magical form grant uses the owned, paid passive action.
			begin_necroplasm_form(obj, ch);
		}
		else if (!affected_by_spell(ch, SPELL_VAMPIRE))
		{
			act("$p &+Lruns its &+Gtendrils&+L through $n's&+L body, transforming $m!",
			    FALSE, ch, obj, 0, TO_ROOM);
			act("$p &+Lruns its &+Gtendrils&+L through your body, transforming you!",
			    FALSE, ch, obj, 0, TO_CHAR);
			spell_vampire(55, ch, 0, 0, ch, 0);
		}
		return TRUE;
	}
	// obj on ground
	if (OBJ_ROOM(obj))
	{
		for (i = world[obj->loc.room].people; i; i = i->next_in_room)
		{
			if (IS_NPC(i) || IS_TRUSTED(i))
			{
				continue;
			}
			if (!number(0, 4))
			{
				obj_from_room(obj);
				obj_to_char(obj, i);
				act("$p &+Lcrawls over to $n &+Land jumps at $m!", FALSE, i, obj, 0,
				    TO_ROOM);
				act("$p &+Lcrawls up to you and jumps at you!", FALSE, i, obj, 0,
				    TO_CHAR);
				ch = i;
				break;
			}
		}
	}

	// if it's in the HOLD slot, return to inventory
	if (OBJ_WORN_BY(obj, ch) && (obj == ch->equipment[HOLD]))
	{
		obj_to_char(unequip_char(ch, HOLD), ch);
	}
	// if not worn, but carried, equip self
	if (OBJ_CARRIED_BY(obj, ch))
	{
		// Only folks who are primary allowed class can use this item
		if (can_prime_class_use_item(ch, obj))
		{
			// verify that they have no other arti's before equip'ing
			for (int i = 0; i < MAX_WEAR; i++)
			{
				if (ch->equipment[i] && IS_ARTIFACT(ch->equipment[i]))
				{
					return FALSE;
				}
			}

			if (modern)
			{
				slot = GET_RACE(ch) == RACE_CENTAUR ? WEAR_HORSE_BODY :
				       GET_RACE(ch) == RACE_DRIDER  ? WEAR_SPIDER_BODY :
								      WEAR_BODY;
			}
			else
			{
				if (GET_RACE(ch) == RACE_CENTAUR)
					slot = WEAR_HORSE_BODY;
				if (GET_RACE(ch) == RACE_DRIDER)
					slot = WEAR_SPIDER_BODY;
				else
					slot = WEAR_BODY;
			}
			if (ch->equipment[slot])
			{
				obj_to_char(unequip_char(ch, slot), ch);
			}
			obj_from_char(obj);
			const uint64_t actor_id = ch->runtime_id, source_uid = obj->obj_uid;
			equip_char(ch, obj, slot, FALSE);
			if (modern)
			{
				ch = find_character_by_runtime_id(actor_id);
				if (!IS_ALIVE(ch) || !(obj = ch->equipment[slot]) ||
				    obj->obj_uid != source_uid)
					return TRUE;
			}
			act("$p &+Mbegins to envelop your body!", FALSE, ch, obj, 0, TO_CHAR);
			act("$p &+Mwraps itself around $n!", FALSE, ch, obj, 0, TO_ROOM);
			act("&+LThe nausea is too much, and the world passes away...", FALSE, ch,
			    obj, 0, TO_CHAR);
			act("&+cJust as abruptly, the nausea subsides, and you feel yourself strangely transformed!",
			    FALSE, ch, obj, 0, TO_CHAR);
			act("&+C$n &+cgrows pale as $s body begins pulsing underneath $p&+c.",
			    FALSE, ch, obj, 0, TO_ROOM);
			return TRUE;
		}
	}
	return FALSE;
}

// VAPOR
int vapor(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char vict;
	int slot, curr_time;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	if (!(OBJ_WORN(obj) || OBJ_CARRIED(obj)) || !IS_ALIVE(ch))
	{
		return FALSE;
	}

	if ((OBJ_WORN(obj) && ch != obj->loc.wearing) ||
	    (OBJ_CARRIED(obj) && ch != obj->loc.carrying))
	{
		return FALSE;
	}

	/*
	  Damage proc on.
	*/
	curr_time = time(NULL);
	if (obj->timer[1] + 30 < curr_time)
	{
		if (IS_FIGHTING(ch) && OBJ_WORN(obj))
		{
			vict = GET_OPPONENT(ch);
			if (!IS_ALIVE(vict))
			{
				return FALSE;
			}
			// 1/10 chance.
			if (!number(0, 9) && GET_HIT(vict) > 40 &&
			    GET_MAX_HIT(ch) < (int)(GET_HIT(ch) * 1.250))
			{
				act("&+LSuddenly the &+wgr&+gee&+Gn &N&+gha&+wze&n &+Laround you comes alive and a &+bchilling feeling &+Lcreeps down\r\n"
				    "&+Lyour spine. Two &+wwri&+Wthi&+Lng te&+Wnta&N&+wcles of &+Gmist &+Lspring out wrapping&n &+wthemselves around\r\n"
				    "&+Lthe chest of $N&+L.  Moments later $E lets out an agonized scream as the warmth\r\n"
				    "&+Lis &+bdrained from $S &+Lbody.&n",
				    FALSE, ch, obj, vict, TO_CHAR);
				act("&+LThe &+wgr&+gee&+Gn c&N&+glo&+wud &+Lencasing $n &+Lsuddenly turns pitch-black. &+LTwin\r\n"
				    "&+wten&+Wtac&+Lles of &+wwri&+Wth&+Lin&+Wg &+Gmist &+Lleap out from the haze wrapping themselves around\r\n"
				    "&+Lthe chest of $N &+Lwho &N&+bshudders &+Lfrom the &N&+bcold.  &+LMoments later $E\r\n"
				    "&+Llets out an agonized scream as the warmth is &+bdrained from &+L$S body.&n",
				    FALSE, ch, obj, vict, TO_NOTVICT);
				act("&+LThe &N&+wgr&+gee&+Gn &N&+gha&+wze &+Lencasing $n &+Lsuddenly turns pitch-black and unleashes &+Ltwin\r\n"
				    "&+wten&+Wtac&+Lles &+Wof &+Gmist &+Ldirectly at you, which wrap themselves around your chest. &+bA chilling\r\n"
				    "&+bcold &+Lspreads throughout your body, &N&+bnumbing you to the core.  &+LMoments later, you\r\n"
				    "&+Lfeel your life's essence being tapped from your body.&n",
				    FALSE, ch, obj, vict, TO_VICT);

				GET_HIT(ch) += 50;
				GET_HIT(vict) -= 50;
				if (GET_HIT(vict) < 0)
				{
					GET_HIT(vict) = 0;
				}
				update_pos(vict);

				obj->timer[1] = curr_time;
				return FALSE;
			}
		}
	}
	// End normal proc.

	if (cmd == CMD_SAY)
	{
		if (isname(arg, "ignite") && !affected_by_spell(ch, SPELL_FIRESHIELD))
		{
			act("$n says 'ignite' to $p.", FALSE, ch, obj, 0, TO_ROOM);
			act("You say 'ignite'", FALSE, ch, 0, 0, TO_CHAR);
			if (affected_by_spell(ch, SPELL_COLDSHIELD))
			{
				act("&+rYou let out a silence scream as the $p &+rfeeds on your life force.&n ",
				    FALSE, ch, obj, 0, TO_CHAR);
				act("&+r$n lets out a silent scream as the $p &+rfeeds on $m!&n",
				    FALSE, ch, obj, 0, TO_ROOM);
				GET_HIT(ch) = MAX(1, GET_HIT(ch) - 30);
				affect_from_char(ch, SPELL_COLDSHIELD);
			}
			act("$p &+Yignites &+Linto a &+rf&+Ri&+rr&+Re&+ry &+Lshield of protection!",
			    FALSE, ch, obj, 0, TO_ROOM);
			act("$p &+Yignites &+Linto a &+rf&+Ri&+rr&+Re&+ry &+Lshield of protection!",
			    FALSE, ch, obj, 0, TO_CHAR);
			spell_fireshield(55, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
			return TRUE;
		}
		else if (isname(arg, "freeze") && !affected_by_spell(ch, SPELL_COLDSHIELD))
		{
			act("$n says 'freeze' to $p.", FALSE, ch, obj, 0, TO_ROOM);
			act("You say 'freeze'", FALSE, ch, 0, 0, TO_CHAR);
			if (affected_by_spell(ch, SPELL_FIRESHIELD))
			{
				act("&+rYou let out a silence scream as the $p &+rfeeds on your life force.&n ",
				    FALSE, ch, obj, 0, TO_CHAR);
				act("&+r$n lets out a silent scream as the $p &+rfeeds on $m!&n",
				    FALSE, ch, obj, 0, TO_ROOM);
				GET_HIT(ch) = MAX(1, GET_HIT(ch) - 30);
				affect_from_char(ch, SPELL_FIRESHIELD);
			}
			act("$p &+Cfreezes &+Linto a &+cc&+Ch&+ci&+Cl&+cl&+Ci&+cn&+Cg &+Lshield of protection!",
			    FALSE, ch, obj, 0, TO_ROOM);
			act("$p &+Cfreezes &+Linto a &+cc&+Ch&+ci&+Cl&+cl&+Ci&+cn&+Cg &+Lshield of protection!",
			    FALSE, ch, obj, 0, TO_CHAR);
			spell_coldshield(55, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
			return TRUE;
		}
	}

	if (cmd == CMD_GOTHIT)
	{
		// recurse self
		if (!IS_SET(obj->extra_flags, ITEM_NODROP))
		{
			SET_BIT(obj->extra_flags, ITEM_NODROP);
		}
		// It's on body
		if (OBJ_WORN_BY(obj, ch) && !affected_by_spell(ch, SPELL_GLOBE) &&
		    !IS_AFFECTED2(ch, AFF2_GLOBE) &&
		    spell_ward_item_callback_allowed(ch, SPELL_GLOBE))
		{
			if (IS_PC(ch))
			{
				act("&+rYou let out a silence scream as the $p &+rfeeds on your life force.&n ",
				    FALSE, ch, obj, 0, TO_CHAR);
				act("&+r$n lets out a silent scream as the $p &+rfeeds on $m!&n",
				    FALSE, ch, obj, 0, TO_ROOM);
				GET_HIT(ch) = MAX(1, GET_HIT(ch) - 30);
			}
			spell_globe(55, ch, NULL, SPELL_TYPE_SPELL, ch, 0);
			return FALSE;
		}

		if (OBJ_CARRIED_BY(obj, ch))
		{
			if (GET_RACE(ch) == RACE_CENTAUR)
			{
				slot = WEAR_HORSE_BODY;
			}
			else if (GET_RACE(ch) == RACE_DRIDER)
			{
				slot = WEAR_SPIDER_BODY;
			}
			else
			{
				slot = WEAR_LEGS;
			}
			if (ch->equipment[slot])
			{
				obj_to_char(unequip_char(ch, slot), ch);
			}
			obj_from_char(obj);
			equip_char(ch, obj, slot, FALSE);

			act("&+LSuddenly the &+ggreen vapor &+Lon the ground starts to sw&+wi&+Wrl &+Las if coming&n\n"
			    "&+Lalive.  You gasp in horror when you feel it caressing your legs as&n\n"
			    "&+Lit coils itself around you.  &+WWr&+wi&+Wthing tentacles &+Lstart to probe you&n\n"
			    "&+Llike the arms of a hungry octopus.  Within seconds your whole being&n\n"
			    "&+Lis encased in a &+bchilling cloud&n &+Lof &+ggreen vapor&+L.&n",
			    FALSE, ch, obj, 0, TO_CHAR);
			act("&+LSuddenly the &+ggreen vapor &+Lby&n $n's &+Lfeet starts to sw&+wi&+Wrl &+Las if&n\n"
			    "&+Lcoming alive.  Staring wide-eyed, as if trying to deny reality, he&n\n"
			    "&+Lwatches as the &+wvapor &+Lslowly coils itself around his legs.  &+WWr&+wi&+Wthing&n\n"
			    "&+Wtentacles &+Lstart to probe $s body like the arms of a hungry octopus&n\n"
			    "&+Land within seconds $e is encased in a &+bchilling &+Lcloud of vapor.&n",
			    FALSE, ch, obj, 0, TO_ROOM);
			return FALSE;
		}
	}
	return FALSE;
}
int vigor_mask(P_obj obj, P_char ch, int cmd, char *arg)
{
	P_char vict;
	struct affected_type *af;

	if (cmd == CMD_SET_PERIODIC)
	{
		return (TRUE);
	}

	if (!OBJ_WORN_POS(obj, WEAR_FACE))
		return (FALSE);

	if (ch || cmd)
		return (FALSE);

	if (cmd == CMD_REMOVE)
	{
		if (isname(arg, "mask") || isname(arg, "bahamut") || isname(arg, "vigor") ||
		    isname(arg, "unique"))
		{
			for (af = ch->affected; af; af = af->next)
			{
				if (af->type == SPELL_VITALITY)
				{
					break;
				}
			}
			affect_remove(ch, af);
		}
	}
	ch = obj->loc.wearing;

	if (!number(0, 9) && cmd == CMD_PERIODIC)
	{
		switch (number(0, 9))
		{
		case 0:
		case 1:
		case 2:
			act("&+LA faint silhouette of a dragon briefly surrounds $n, then quickly dissipates.&N",
			    TRUE, ch, obj, ch, TO_ROOM);
			send_to_char("&+LYou feel a faint chill run down your spine.&N\n", ch);
			break;
		case 3:
		case 4:
		case 5:
			act("&+LSmall tendrils of smoke seep out of $n's mouth behind the mask.&N",
			    TRUE, ch, obj, ch, TO_ROOM);
			send_to_char("&+LSmall tendrils of smoke flick out of your mouth.\n", ch);
			break;
		case 6:
		case 7:
		case 8:
		case 9:
			act("&+L$n's eyes glow briefly for a few seconds, then return to normal.&N",
			    TRUE, ch, obj, ch, TO_ROOM);
			send_to_char("&+LYour vision became hazed for a few seconds.\n", ch);
			break;
		default:
			break;
		}
		return TRUE;
	}

	if (IS_FIGHTING(ch) && !number(0, 9) && cmd == 0)
	{
		vict = GET_OPPONENT(ch);
		send_to_char(
			"&+WThe essense of Bahamut streams of of your mask and attacks your victim!&N\n",
			ch);
		if (affected_by_spell(ch, SPELL_VITALITY))
		{
			send_to_char("&+Byou feel revitalized.\n", ch);
			for (af = ch->affected; af; af = af->next)
			{
				if (af->type == SPELL_VITALITY)
				{
					af->duration = 24;
					af->modifier = GET_LEVEL(vict) * 4;
				}
			}
		}
		else
		{
			spell_vitality(GET_LEVEL(vict), ch, NULL, 0, ch, 0);
		}
		if (affected_by_spell(vict, SPELL_VITALITY))
		{
			send_to_char("&+L   /\\             /\\\n", vict);
			send_to_char("&+L   \\ \\           / /\n", vict);
			send_to_char("&+L    | \\  /\\_/\\  / |\n", vict);
			send_to_char("&+L    \\  \\/  _  \\/  /\n", vict);
			send_to_char("&+L     |  \\_/^\\_/  |\n", vict);
			send_to_char("&+L    /   \\  ^  /   \\\n", vict);
			send_to_char("&+L    \\  &+R(\\  &+L^  &+R/)&+L  /\n", vict);
			send_to_char("&+L     \\  &+R\\) &+L^ &+R(/&+L  /\n", vict);
			send_to_char("&+L     /\\    ^    /\\\n", vict);
			send_to_char("&+L     ||\\/\\ ^ /\\/||\n", vict);
			send_to_char("&+L     ||&Nv&+L\\\\&N. .&+L//&Nv&+L||\n", vict);
			send_to_char("&+L     || &Nv&+L\\___/&Nv&+L ||\n", vict);
			send_to_char("&+L     ||  &NvVVVv&+L  ||\n", vict);
			send_to_char("&+L     \\\\&Nn &+L\\   / &Nn&+L//\n", vict);
			send_to_char("&+L      \\\\&Nn &+L||| &Nn&+L//\n", vict);
			send_to_char(
				"&+L       \\\\&Nn&+L\\|/&Nn&+L// &+WT&Nh&+Le Spirit of bahamut leaps at y&No&+Wu\n",
				vict);
			send_to_char(
				"&+L        \\\\&Nnnn&+L//  &+Wa&Nn&+Ld sucks your vitality awa&Ny&+W!\n",
				vict);
			send_to_char("&+L         \\___/&N\n", vict);

			for (af = vict->affected; af; af = af->next)
			{
				if (af->type == SPELL_VITALITY)
				{
					break;
				}
			}
			if (af)
			{
				// will removing vit kill the victim?
				if (af->modifier >= (GET_HIT(vict) + 10))
				{
					die(vict, ch);
				}
				else
				{
					affect_remove(vict, af);
					update_pos(vict);
				}
			}
		}
		else
		{
			act("&+W$n's $q glows briefly and uses your essence to &+Brevitalize&N $n!&N",
			    TRUE, ch, obj, vict, TO_VICT);
		}
		if (is_char_in_room(vict, ch->in_room))
		{
			act("&+W$q glows for a second and drains the essence of $N.&N\n&+B$n becomes revitalized!&N",
			    TRUE, ch, obj, vict, TO_NOTVICT);
		}
	}

	return (FALSE);
}

int staff_of_air_conjuration(P_obj obj, P_char ch, int cmd, char *arg)
{
	int curr_time, i;
	P_char victim = NULL;

	if (cmd == CMD_SET_PERIODIC)
	{
		return TRUE;
	}

	curr_time = time(NULL);

	if (cmd == CMD_PERIODIC)
	{
		if ((obj->timer[1] + SECS_PER_REAL_DAY) <= curr_time)
		{
			if (obj->value[2] == 0)
			{
				if (OBJ_WORN(obj) && ((ch = obj->loc.wearing) != NULL))
					act("&+cA small &+Wj&+Co&+cl&+Yt&+c of electricity &+Ca&+Wr&+Bcs&+c from $p&+c to your hand.&n",
					    FALSE, ch, obj, NULL, TO_CHAR);
				obj->value[2] = 1;
				obj->timer[1] = curr_time;
			}
		}
		return FALSE;
	}

	if (!IS_ALIVE(ch) || !OBJ_WORN(obj) || (obj->loc.wearing != ch))
	{
		return FALSE;
	}

	if (cmd == CMD_USE)
	{
		if (obj == get_object_in_equip_vis(ch, arg, &i) && obj->value[2] == 0)
		{
			send_to_char(
				"&+cA small voice inside your head whispers, \"No energy for that right now, try '&+wsay lightning&+c'.\"\n",
				ch);
			return TRUE;
		}
	}

	if (arg && (cmd == CMD_SAY))
	{
		if (isname(arg, "lightning"))
		{
			if (!OUTSIDE(ch))
			{
				send_to_char(
					"&+cA small voice inside your head whispers, \"Try it outside.\"\n",
					ch);
				return TRUE;
			}

			if (obj->timer[0] + SECS_PER_REAL_HOUR <= curr_time)
			{
				act("&+cElectrical c&+Wh&+Ca&+Br&+bges&+c begin to &+Ca&+Wr&+Bc&+c and &+Ws&+Cp&+Ba&+brk&+c on the ground randomly..&n",
				    FALSE, ch, obj, NULL, TO_CHAR);
				act("&+cElectrical c&+Wh&+Ca&+Br&+bges&+c begin to &+Ca&+Wr&+Bc&+c and &+Ws&+Cp&+Ba&+brk&+c on the ground randomly..&n",
				    FALSE, ch, obj, NULL, TO_ROOM);
				cast_call_lightning(56, ch, 0, SPELL_TYPE_SPELL, NULL, 0);
				obj->timer[0] = curr_time;
				return TRUE;
			}
			else
			{
				act("&+cA small &+Ws&+Cp&+Ba&+brk&+c of electricity &+Ca&+Wr&+Bcs&+c from $p to your hand, but nothing else seems to happen.&n",
				    FALSE, ch, obj, victim, TO_CHAR);
				return TRUE;
			}
		}
	}
	return FALSE;
}
