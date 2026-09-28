#include "core/prototypes.h"
#include "core/structs.h"
#include "world/db.h"
#include "net/comm.h"
#include "core/utils.h"
#include "core/defines.h"
#include "classes/disguise.h"
#include "magic/spells.h"
#include <strings.h>

/*
 * Returns TRUE if ch is wearing perm invis eq.
 */
int wearing_invis(P_char ch)
{
	int found = 0, k;

	for (k = 0; k < MAX_WEAR; k++)
		if (ch->equipment[k] && IS_SET(ch->equipment[k]->bitvector, AFF_INVISIBLE))
			found = 1;
	return found;
}

void appear(P_char ch, bool removeHide)
{
	P_char master;

	if (!ch)
	{
		logit(LOG_EXIT, "appear called in spell_visibility.c without ch");
		return;
	}

	// If someone is going vis via being ordered to do something, have the person doing the ordering go vis as well.
	if ((master = GET_MASTER(ch)) != NULL)
	{
		if (IS_AFFECTED5(master, AFF5_ORDERING))
			appear(master, removeHide);
	}

	// CMD_FIRE (do_fire) handles its own hide stuff.
	if (removeHide)
		REMOVE_BIT(ch->specials.affected_by, AFF_HIDE);

	if ((!IS_SET(ch->specials.affected_by, AFF_INVISIBLE) &&
	     !IS_SET(ch->specials.affected_by2, AFF2_CONCEALMENT) &&
	     !IS_SET(ch->specials.affected_by3, AFF3_ECTOPLASMIC_FORM) &&
	     !IS_SET(ch->specials.affected_by3, AFF3_NON_DETECTION)))
	{
		return;
	}

	affect_from_char(ch, SPELL_CONCEALMENT);
	affect_from_char(ch, TAG_PERMINVIS);
	affect_from_char(ch, SPELL_INVISIBILITY);
	affect_from_char(ch, SPELL_ECTOPLASMIC_FORM);

	REMOVE_BIT(ch->specials.affected_by, AFF_INVISIBLE);
	REMOVE_BIT(ch->specials.affected_by2, AFF2_CONCEALMENT);
	REMOVE_BIT(ch->specials.affected_by3, AFF3_ECTOPLASMIC_FORM);

	if (IS_SET(ch->specials.affected_by3, AFF3_NON_DETECTION))
	{
		struct affected_type *afp;

		for (afp = ch->affected; afp; afp = afp->next)
		{
			// Need to remove the mind blank effect without removing the cooldown.
			if (afp->type == SPELL_MIND_BLANK && afp->bitvector3 == AFF3_NON_DETECTION)
			{
				affect_remove(ch, afp);
				// If you decide not to break here, for whatever reason, you need to modify the loop.
				break;
			}
		}
		REMOVE_BIT(ch->specials.affected_by3, AFF3_NON_DETECTION);
	}

	if (wearing_invis(ch))
	{
		act("$n flickers into visibility.", TRUE, ch, 0, 0, TO_ROOM);
		act("You flicker into visibility.", FALSE, ch, 0, 0, TO_CHAR);
	}
	else
	{
		act("$n snaps into visibility.", TRUE, ch, 0, 0, TO_ROOM);
		act("You snap into visibility.", FALSE, ch, 0, 0, TO_CHAR);
	}
}

void spell_dispel_invisible(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			    P_obj obj)
{
	if (victim)
		if (resists_spell(ch, victim))
			return;

	if (obj)
	{
		if (IS_SET(obj->extra_flags, ITEM_INVISIBLE))
		{
			act("$p fades into visibility.", FALSE, ch, obj, 0, TO_CHAR);
			act("$p fades into visibility.", TRUE, ch, obj, 0, TO_ROOM);
			REMOVE_BIT(obj->extra_flags, ITEM_INVISIBLE);
		}
	}
	else
	{
		if (affected_by_spell(victim, SPELL_CONCEALMENT))
		{
			act("&+W$n slowly fades into existance.", TRUE, victim, 0, 0, TO_ROOM);
			send_to_char("&+WYou turn visible.\n", victim);
			affect_from_char(victim, SPELL_CONCEALMENT);
			if (IS_SET(victim->specials.affected_by, AFF_INVISIBLE))
				REMOVE_BIT(victim->specials.affected_by, AFF_INVISIBLE);
			if (IS_SET(victim->specials.affected_by2, AFF2_CONCEALMENT))
				REMOVE_BIT(victim->specials.affected_by2, AFF2_CONCEALMENT);
		}
	}
}

void spell_concealment(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		       P_char victim, P_obj obj)
{
	struct affected_type af;

	if (obj)
	{
		act("&+L$p flares up for a second, then is still again.", FALSE, ch, obj, 0,
		    TO_CHAR);
		act("&+L$p flares up for a second, then is still again.", TRUE, ch, obj, 0,
		    TO_ROOM);
	}
	else
	{ /*
	   * Then it is a PC | NPC
	   */
		if (!affected_by_spell(victim, SPELL_CONCEALMENT))
		{
			act("&+L$n slowly fades out of existence.", TRUE, victim, 0, 0, TO_ROOM);
			send_to_char("&+LYou vanish.\n", victim);

			bzero(&af, sizeof(af));

			af.type = SPELL_CONCEALMENT;
			af.duration = level / 2;

			if (GET_SPEC(ch, CLASS_SORCERER, SPEC_SHADOW) && ch == victim &&
			    !IS_WATER_ROOM(ch->in_room) &&
			    world[ch->in_room].sector_type != SECT_OCEAN)
			{
				af.bitvector = AFF_HIDE;
			}

			af.bitvector2 = AFF2_CONCEALMENT;
			affect_to_char(victim, &af);
		}
		else
		{
			struct affected_type *af1;

			for (af1 = victim->affected; af1; af1 = af1->next)
				if (af1->type == SPELL_CONCEALMENT)
				{
					af1->duration = level / 2;
				}
		}
	}
}

void spell_invisibility(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			P_char victim, P_obj obj)
{
	struct affected_type af;

	if (obj)
	{
		if (!IS_SET(obj->extra_flags, ITEM_INVISIBLE) && IS_SET(obj->wear_flags, ITEM_TAKE))
		{
			act("&+L$p turns invisible.", FALSE, ch, obj, 0, TO_CHAR);
			act("&+L$p turns invisible.", TRUE, ch, obj, 0, TO_ROOM);
			SET_BIT(obj->extra_flags, ITEM_INVISIBLE);
			REMOVE_BIT(obj->extra_flags, ITEM_LIT);
		}
	}
	else
	{ /*
	   * Then it is a PC | NPC
	   */
		if (!affected_by_spell(victim, SPELL_INVISIBILITY))
		{
			act("&+L$n slowly fades out of existence.", TRUE, victim, 0, 0, TO_ROOM);
			send_to_char("&+LYou vanish.\n", victim);

			bzero(&af, sizeof(af));

			af.type = SPELL_INVISIBILITY;
			af.duration = level / 2;
			af.bitvector = AFF_INVISIBLE;
			affect_to_char(victim, &af);
		}
		else
		{
			struct affected_type *af1;

			for (af1 = victim->affected; af1; af1 = af1->next)
				if (af1->type == SPELL_INVISIBILITY)
				{
					af1->duration = level / 2;
				}
		}
	}
}

void spell_mass_invisibility(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			     P_obj /*obj*/)
{
	if (!IS_ALIVE(ch))
		return;

	LOOP_THRU_PEOPLE(victim, ch)
		if (!IS_TRUSTED(ch) && !IS_TRUSTED(victim))
			spell_invisibility(level, ch, 0, 0, victim, 0);
		else if (IS_TRUSTED(ch))
			spell_invisibility(level, ch, 0, 0, victim, 0);

	// for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
	// {
	// if(IS_SET(obj->wear_flags, ITEM_TAKE))
	// spell_invisibility(level, ch, 0, 0, 0, obj);
	// }
}

void spell_shadow_projection(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			     P_char /*victim*/, P_obj /*obj*/)
{
	struct affected_type af;

	if (!IS_ALIVE(ch))
		return;

	if (affected_by_spell(ch, SPELL_FAERIE_FIRE))
	{
		act("&+MThe magical fire surrounding you negates the spell!&n", FALSE, ch, 0, 0,
		    TO_CHAR);
		return;
	}

	if (!affected_by_spell(ch, SPELL_SHADOW_PROJECTION))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_SHADOW_PROJECTION;
		af.duration = 1;
		af.bitvector = AFF_SNEAK;
		af.bitvector |= AFF_HIDE;
		af.bitvector2 = AFF2_PASSDOOR;
		affect_to_char(ch, &af);

		act("&+L$n &+Lfades into nothingness, and simply ceases to exist.&n", FALSE, ch, 0,
		    0, TO_ROOM);
		act("&+LYou blend into the shadows, seemingly ceasing to exist in this realm.&n",
		    FALSE, ch, 0, 0, TO_CHAR);
	}
}

void spell_faerie_fire(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		       P_char victim, P_obj /*obj*/)
{
	struct affected_type af, *af2, *af3;
	int a;

	if (GET_STAT(ch) == STAT_DEAD)
		return;

	if (affected_by_spell(victim, SPELL_FAERIE_FIRE))
	{
		send_to_char("Nothing new seems to happen.\n", ch);
		return;
	}
	act("$n points at $N.", TRUE, ch, 0, victim, TO_NOTVICT);
	act("You point at $N.", TRUE, ch, 0, victim, TO_CHAR);
	act("$n points at you.", TRUE, ch, 0, victim, TO_VICT);
	if (affected_by_spell(ch, SPELL_CONCEALMENT))
		for (af2 = victim->affected; af2; af2 = af3)
		{
			af3 = af2->next;
			if (af2->type == SPELL_CONCEALMENT)
				affect_remove(victim, af2);
		}
	a = 0;
	if (IS_SET(victim->specials.affected_by, AFF_INVISIBLE) ||
	    IS_AFFECTED2(victim, AFF2_CONCEALMENT))
	{
		a = 1;
		REMOVE_BIT(victim->specials.affected_by, AFF_INVISIBLE);
		REMOVE_BIT(victim->specials.affected_by2, AFF2_CONCEALMENT);
	}
	else if (!number(0, 1) && IS_DISGUISE(victim))
	{
		remove_disguise(victim, TRUE);
	}
	act("$N is surrounded by the dancing outline of &+mpurplish flames!&n", TRUE, ch, 0, victim,
	    TO_NOTVICT);
	act("$N is surrounded by the dancing outline of &+mpurplish flames!&n", TRUE, ch, 0, victim,
	    TO_CHAR);
	act("You are surrounded by the dancing outline of &+mpurplish flames!&n", TRUE, ch, 0,
	    victim, TO_VICT);
	if (a)
		SET_BIT(victim->specials.affected_by2, AFF2_CONCEALMENT);
	bzero(&af, sizeof(af));
	af.type = SPELL_FAERIE_FIRE;
	af.duration = 5;
	af.modifier = 20;
	af.location = APPLY_ARMOR;

	affect_to_char(victim, &af);

	if (IS_NPC(victim) && CAN_SEE(victim, ch))
	{
		remember(victim, ch);
		if (!IS_FIGHTING(victim))
			MobStartFight(victim, ch);
	}
}

void spell_faerie_fog(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		      P_obj /*obj*/)
{
	ulong a, a2;
	P_char tmp_victim;

	act("&+m$n&n&+m snaps $s fingers, and a cloud of purple smoke billows forth!", TRUE, ch, 0,
	    0, TO_ROOM);
	act("&+mYou snap your fingers, causing a cloud of purple smoke to billow forth.", TRUE, ch,
	    0, 0, TO_CHAR);

	LOOP_THRU_PEOPLE(tmp_victim, ch)
	{
		if ((ch != tmp_victim) && !IS_TRUSTED(tmp_victim) &&
		    (IS_AFFECTED(tmp_victim, AFF_INVISIBLE) || IS_AFFECTED(tmp_victim, AFF_HIDE) ||
		     IS_AFFECTED2(tmp_victim, AFF2_CONCEALMENT)))
		{
			if (NewSaves(tmp_victim, SAVING_SPELL,
				     (int)(level - GET_LEVEL(tmp_victim))))
			{
				a = tmp_victim->specials.affected_by;
				a2 = tmp_victim->specials.affected_by2;
				if (IS_AFFECTED2(tmp_victim, AFF2_CONCEALMENT))
					REMOVE_BIT(tmp_victim->specials.affected_by2,
						   AFF2_CONCEALMENT);
				if (IS_AFFECTED(tmp_victim, AFF_INVISIBLE))
					REMOVE_BIT(tmp_victim->specials.affected_by, AFF_INVISIBLE);
				if (IS_AFFECTED(tmp_victim, AFF_HIDE))
					REMOVE_BIT(tmp_victim->specials.affected_by, AFF_HIDE);
				act("$n is briefly revealed, but disappears again.", TRUE,
				    tmp_victim, 0, 0, TO_ROOM);
				act("You are briefly revealed, but disappear again.", TRUE,
				    tmp_victim, 0, 0, TO_CHAR);
				tmp_victim->specials.affected_by = a;
				tmp_victim->specials.affected_by2 = a2;
			}
			else
			{
				if (IS_AFFECTED(tmp_victim, AFF_INVISIBLE) ||
				    IS_AFFECTED2(tmp_victim, AFF2_CONCEALMENT))
					affect_from_char(tmp_victim, SPELL_CONCEALMENT);
				if (IS_AFFECTED2(tmp_victim, AFF2_CONCEALMENT))
					REMOVE_BIT(tmp_victim->specials.affected_by2,
						   AFF2_CONCEALMENT);
				if (IS_AFFECTED(tmp_victim, AFF_INVISIBLE))
					REMOVE_BIT(tmp_victim->specials.affected_by, AFF_INVISIBLE);
				if (IS_AFFECTED(tmp_victim, AFF_HIDE))
					affect_from_char(tmp_victim, SKILL_HIDE);
				if (IS_AFFECTED(tmp_victim, AFF_HIDE))
					REMOVE_BIT(tmp_victim->specials.affected_by, AFF_HIDE);
				act("$n is revealed!", TRUE, tmp_victim, 0, 0, TO_ROOM);
				act("You are revealed!", TRUE, tmp_victim, 0, 0, TO_CHAR);
			}
		}
		else if (ch != tmp_victim && IS_DISGUISE(tmp_victim))
		{
			if (!number(0, 1))
			{
				remove_disguise(tmp_victim, TRUE);
			}
			else if (IS_DISGUISE_ILLUSION(tmp_victim))
			{
				act("$n is cloaked in illusion!", TRUE, tmp_victim, 0, 0, TO_ROOM);
				act("Your illusion has been noticed!", TRUE, tmp_victim, 0, 0,
				    TO_CHAR);
			}
			else if (IS_DISGUISE_SHAPE(tmp_victim))
			{
				act("$n is not in their true form!", TRUE, tmp_victim, 0, 0,
				    TO_ROOM);
				act("Your true form has been noticed!", TRUE, tmp_victim, 0, 0,
				    TO_CHAR);
			}
			else
			{
				act("$n is wearing a disguise!", TRUE, tmp_victim, 0, 0, TO_ROOM);
				act("Your disguise has been noticed!", TRUE, tmp_victim, 0, 0,
				    TO_CHAR);
			}
		}
	}
}

void spell_blur(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		P_obj /*obj*/)
{
	struct affected_type af;
	struct affected_type *af1;

	if (!IS_ALIVE(ch))
	{
		return;
	}

	if (GET_CLASS(victim, CLASS_MONK))
	{
		send_to_char("Your monkliness makes blur non-blur-like.\n", victim);
		return;
	}

	if (!affected_by_spell(victim, SPELL_BLUR))
	{
		send_to_char(
			"&+BYour skin crackles with lightning as your form starts to &N&+Wblur.&N\n",
			victim);
		act("$n becomes a &+bBLUR&N of speed!", TRUE, victim, 0, 0, TO_ROOM);
		bzero(&af, sizeof(af));
		af.type = SPELL_BLUR;
		af.duration = 10;
		af.bitvector3 = AFF3_BLUR;
		affect_to_char(victim, &af);
	}
	else if (affected_by_spell(victim, SPELL_BLUR))
	{
		for (af1 = victim->affected; af1; af1 = af1->next)
		{
			if (af1->type == SPELL_BLUR)
			{
				send_to_char("Your blurring speed is revivified.&n\n", victim);
				af1->duration = 10;
			}
		}
	}
	return;
}

void spell_pass_without_trace(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			      P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (IS_AFFECTED3(victim, AFF3_PASS_WITHOUT_TRACE))
		return;

	if (!affected_by_spell(victim, SPELL_PASS_WITHOUT_TRACE))
	{
		send_to_char("&+gThe world seems to open new paths for you.&n\n", victim);

		bzero(&af, sizeof(af));
		af.type = SPELL_PASS_WITHOUT_TRACE;
		af.duration = 10;
		af.bitvector3 = AFF3_PASS_WITHOUT_TRACE;
		affect_to_char(victim, &af);
	}
	else
	{
		send_to_char("Nothing seems to happen.\n", ch);
	}
}

void spell_rope_trick(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		      P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	int howmany = 0;
	P_char temp_vict;

	if (ch != victim)
		if (!is_linked_to(ch, victim, LNK_CONSENT))
			if (resists_spell(ch, victim))
				return;

	temp_vict = victim;

	if (IS_WATER_ROOM(ch->in_room) || world[ch->in_room].sector_type == SECT_OCEAN)
	{
		send_to_char("It's too wet here to hide anything.\r\n", ch);
		return;
	}

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_ROPE_TRICK);
	if (!NewSaves(victim, SAVING_SPELL, mod) || (ch == victim) ||
	    is_linked_to(ch, victim, LNK_CONSENT))
	{
		LOOP_THRU_PEOPLE(temp_vict, ch)
		{
			if (affected_by_spell(temp_vict,
					      SPELL_ROPE_TRICK && IS_AFFECTED(temp_vict, AFF_HIDE)))
				howmany++;
		}
		if (howmany > 2)
		{
			act("&+LYou don't seem to be able to to conjure any more &+yropes&n&+L.&n",
			    TRUE, ch, 0, victim, TO_CHAR);
			return;
		}
		act("&+LA long &+yrope&+L appears, dangling in mid-air.&N\n$N &+Lscrambles up the &+yrope&+L and vanishes from sight.&n",
		    TRUE, ch, 0, victim, TO_CHAR);
		act("&+LA long &+yrope&+L appears, dangling in mid-air.&N\n&+LYou scramble up the &+yrope&+L and hide.&n",
		    TRUE, ch, 0, victim, TO_VICT);
		act("&+LA long &+yrope&+L appears, dangling in mid-air.&N\n$N &+Lscrambles up the &+yrope&+L and vanishes from sight.&n",
		    TRUE, ch, 0, victim, TO_NOTVICT);

		SET_BIT(victim->specials.affected_by, AFF_HIDE);
		bzero(&af, sizeof(af));
		af.type = SPELL_ROPE_TRICK;
		af.duration = 5;
		affect_to_char(victim, &af);
	}
}

void spell_tree(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		P_obj /*obj*/)
{
	P_obj tobj, next_tobj;

	if (world[ch->in_room].sector_type != SECT_FOREST)
	{
		send_to_char("&+GTrees&+g don't grow here!&n\n", ch);
		return;
	}

	// DALRETH - 3/05
	if (ch->specials.z_cord > 0)
	{
		send_to_char("There isn't enough forest up here.\r\n", ch);
		return;
	}
	if (IS_RIDING(ch))
	{
		send_to_char("&+WYour mount prevents you from blending into the forest...&n\r\n",
			     ch);
		return;
	}
	if (affected_by_spell(ch, SPELL_FAERIE_FIRE))
	{
		send_to_char("You can't hide while surrounded by &+Mflames!&n\n\r", ch);
		return;
	}

	if (IS_FIGHTING(ch))
	{
		send_to_char("Your magic isn't strong enough to hide you from battle!\r\n", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_HIDE))
		REMOVE_BIT(ch->specials.affected_by, AFF_HIDE);

	CharWait(ch, PULSE_VIOLENCE * 3);

	/*  destroy your tracks! */
	for (tobj = world[ch->in_room].contents; tobj; tobj = next_tobj)
	{
		next_tobj = tobj->next_content;
		if (tobj->R_num == real_object(VNUM_TRACKS))
		{
			extract_obj(tobj);
			tobj = NULL;
		}
	}

	send_to_char("&+wYou blend silently into the &+gforest.&n\r\n", ch);
	SET_BIT(ch->specials.affected_by, AFF_HIDE);
}
