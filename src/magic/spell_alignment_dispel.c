#include "core/prototypes.h"
#include "cmd/interp.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include "item/objmisc.h"
#include "world/vnum.obj.h"
#include "world/specs.prototypes.h"

extern P_index obj_index;
extern P_room world;
extern const int rev_dir[];

typedef struct
{
	ulong bit;
	int spell;
} RemoveableSpellBit;

static RemoveableSpellBit mobAffects[] = {
	{ AFF_MINOR_GLOBE, SPELL_MINOR_GLOBE },
	{ AFF_BIOFEEDBACK, SPELL_BIOFEEDBACK },
	{ AFF_STONE_SKIN, SPELL_STONE_SKIN },
	{ AFF_INFERNAL_FURY, SPELL_INFERNAL_FURY },
	{ AFF_FREEDOM_OF_MVMNT, SPELL_FREEDOM_OF_MOVEMENT },
	{ AFF_INVISIBLE, SPELL_CONCEALMENT },
	{ AFF_DETECT_INVISIBLE, SPELL_DETECT_INVISIBLE },
	{ AFF_HASTE, SPELL_HASTE },
	{ AFF_ARMOR, SPELL_ARMOR },
	{ AFF_SLEEP, SPELL_SLEEP },
	{ AFF_BARKSKIN, SPELL_BARKSKIN },
	{ AFF_LEVITATE, SPELL_LEVITATE },
	{ AFF_FLY, SPELL_FLY },
};

static RemoveableSpellBit mobAffects2[] = {
	{ AFF2_MINOR_PARALYSIS, SPELL_MINOR_PARALYSIS },
	{ AFF2_MAJOR_PARALYSIS, SPELL_MAJOR_PARALYSIS },
	{ AFF2_GLOBE, SPELL_GLOBE },
	{ AFF2_PASSDOOR, SPELL_MOLECULAR_CONTROL },
};

static RemoveableSpellBit mobAffects3[] = {
	{ AFF3_ECTOPLASMIC_FORM, SPELL_ECTOPLASMIC_FORM },
	{ AFF3_SPIRIT_WARD, SPELL_SPIRIT_WARD },
	{ AFF3_GR_SPIRIT_WARD, SPELL_GREATER_SPIRIT_WARD },
	{ AFF3_INERTIAL_BARRIER, SPELL_INERTIAL_BARRIER },
	{ AFF3_TOWER_IRON_WILL, SPELL_TOWER_IRON_WILL },
	{ AFF3_BLUR, SPELL_BLUR },
};

static RemoveableSpellBit mobAffects4[] = {
	{ AFF4_STORNOGS_SPHERES, SPELL_STORNOGS_SPHERES },
	{ AFF4_STORNOGS_GREATER_SPHERES, SPELL_STORNOGS_GREATER_SPHERES },
	{ AFF4_BATTLE_ECSTASY, SPELL_BATTLE_ECSTASY },
	{ AFF4_DAZZLER, SPELL_DAZZLE },
	{ AFF4_DEFLECT, SPELL_DEFLECT },
	{ AFF4_HAWKVISION, SPELL_HAWKVISION },
	{ AFF4_SANCTUARY, SPELL_SANCTUARY },
	{ AFF4_HELLFIRE, SPELL_HELLFIRE },
};

static RemoveableSpellBit mobAffects5[] = {
	{ AFF5_FLESH_ARMOR, SPELL_FLESH_ARMOR },
	{ AFF5_THORNSKIN, SPELL_THORNSKIN },
};

static int CheckMobRemoveableSpellBits(P_char ch, RemoveableSpellBit *spellBits, int countSpellBits,
				       ulong *bitStore, bool nosave, int saveMod, int affectVector)
{
	int success = 0;

	if (!IS_NPC(ch))
	{
		return success;
	}

	for (int i = 0; i < countSpellBits; i++)
	{
		if (IS_SET(*bitStore, spellBits[i].bit) &&
		    !affected_by_spell(ch, spellBits[i].spell) &&
		    (nosave || !NewSaves(ch, SAVING_SPELL, saveMod)))
		{
			success = 1;
			if (!IS_ELITE(ch))
			{
				REMOVE_BIT(*bitStore, spellBits[i].bit);
			}
			else
			{
				struct affected_type *paf = NULL;
				if (!affected_by_spell(ch, TAG_SUPPRESS_PERM_BITS))
				{
					add_tag_to_char(
						ch, TAG_SUPPRESS_PERM_BITS, 0,
						AFFTYPE_SHORT | AFFTYPE_NODISPEL,
						WAIT_SEC *
							get_property(
								"timer.secs.suppressElitePermBits",
								30));
					paf = get_spell_from_char(ch, TAG_SUPPRESS_PERM_BITS);
				}
				else
				{
					paf = get_spell_from_char(ch, TAG_SUPPRESS_PERM_BITS);
				}

				ulong *paffectBits = NULL;
				switch (affectVector)
				{
				case 1:
					paffectBits = &paf->bitvector;
					break;
				case 2:
					paffectBits = &paf->bitvector2;
					break;
				case 3:
					paffectBits = &paf->bitvector3;
					break;
				case 4:
					paffectBits = &paf->bitvector4;
					break;
				case 5:
					paffectBits = &paf->bitvector5;
					break;
				default:
					logit(LOG_EXIT,
					      "Invalid affect vector in CheckMobRemoveableSpellBits");
					return success;
				}
				// add the suppressed bits
				SET_BIT(*paffectBits, spellBits[i].bit);
			}
		}
	}

	return success;
}

void spell_dispel_magic(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			P_char victim, P_obj obj)
{
	struct affected_type *af, *next_af_dude;
	int mod, success = 0, nosave = 0;
	P_obj temp_wall, next_obj;
	P_char orig;

	if (!IS_ALIVE(ch))
	{
		return;
	}

	/* victim target... */
	if (victim)
	{
		/*
		 * no save when cast or on consenting target
		 */

		if (ch == victim && level < 57 && !IS_TRUSTED(ch))
		{
			send_to_char("You dispel the very spell you are casting!\n", ch);
			return;
		}

		if (ch == victim || is_linked_to(ch, victim, LNK_CONSENT) || IS_TRUSTED(ch))
		{
			nosave = 1;
		}

		mod = GET_LEVEL(ch) - GET_LEVEL(victim);
		if (IS_NPC(ch) && IS_PC(victim) && mod > 0)
		{
			mod /= 3;
		}

		act("$n tries to dispel your magic!", FALSE, ch, 0, victim, TO_VICT);
		act("You try to dispel $N's magic.", FALSE, ch, 0, victim, TO_CHAR);
		act("$n tries to dispel $N's magic!", FALSE, ch, 0, victim, TO_NOTVICT);

		for (af = victim->affected; af; af = next_af_dude)
		{
			next_af_dude = af->next;

			// Skip over the multiple affect spells.
			while (next_af_dude && next_af_dude->type == af->type)
			{
				next_af_dude = next_af_dude->next;
			}

			if (!IS_SET(af->flags, AFFTYPE_NODISPEL) && (af->type > 0))
			{
				if (nosave ||
				    !NewSaves(victim, SAVING_SPELL, (IS_ELITE(ch) ? mod + 5 : mod)))
				{
					if (!nosave && resists_spell(ch, victim))
					{
						return;
					}

					success = 1;
					wear_off_message(victim, af);
					if ((af->type == SPELL_CALL_OF_THE_WILD) &&
					    IS_MORPH(victim))
					{
						orig = victim->only.npc->orig_char;

						act("$n suddenly changes shape, reforming into $N.",
						    FALSE, victim, NULL, orig, TO_NOTVICT);
						send_to_char(
							"You suddenly feel yourself going back to normal.\n",
							victim);

						send_to_char("and you succeed!\n", ch);
						act("and succeeds!", FALSE, ch, 0, victim,
						    TO_NOTVICT);

						un_morph(victim);
						return;
					}
					affect_from_char(victim, af->type);
				}
			}
		}

		success = CheckMobRemoveableSpellBits(victim, mobAffects, ARRAY_SIZE(mobAffects),
						      &victim->specials.affected_by, nosave, mod,
						      1) == 0 ?
				  success :
				  1;
		success = CheckMobRemoveableSpellBits(victim, mobAffects2, ARRAY_SIZE(mobAffects2),
						      &victim->specials.affected_by2, nosave, mod,
						      2) == 0 ?
				  success :
				  1;
		success = CheckMobRemoveableSpellBits(victim, mobAffects3, ARRAY_SIZE(mobAffects3),
						      &victim->specials.affected_by3, nosave, mod,
						      3) == 0 ?
				  success :
				  1;
		success = CheckMobRemoveableSpellBits(victim, mobAffects4, ARRAY_SIZE(mobAffects4),
						      &victim->specials.affected_by4, nosave, mod,
						      4) == 0 ?
				  success :
				  1;
		success = CheckMobRemoveableSpellBits(victim, mobAffects5, ARRAY_SIZE(mobAffects5),
						      &victim->specials.affected_by5, nosave, mod,
						      5) == 0 ?
				  success :
				  1;

		if (!success)
		{
			send_to_char("&+Land you fail miserably...\n", ch);
			act("&+Land fails miserably...&n", FALSE, ch, 0, victim, TO_NOTVICT);
		}
		else
		{
			send_to_char("&+Yand you have some &+Gsuccess!\r\n", ch);
			act("&+Yand $e has some &+Gsuccess!&n", FALSE, ch, 0, victim, TO_NOTVICT);
		}

		if (!nosave &&
		    (IS_AFFECTED(ch, AFF_INVISIBLE) || IS_AFFECTED2(ch, AFF2_CONCEALMENT)))
			appear(ch);

		if (!nosave && IS_NPC(victim) && CAN_SEE(victim, ch))
		{
			remember(victim, ch);
			if (!IS_FIGHTING(victim))
				MobStartFight(victim, ch);
		}
		balance_affects(victim);
		return;
	}
	/* okay.. must be an object target! */

	if (obj_index[obj->R_num].func.obj && invoke_object_special(obj, ch, CMD_DISPEL, NULL))
		return;

	/* second deal with "special" objects (conjurerer wall spells) */
	if (obj->R_num == real_object(VOBJ_WALLS))
	{
		int dispelDmg = ((obj->value[3] == WALL_OUTPOST) ? 1 : number(level / 4, level));

		if ((number(0, 5) > (obj->value[4] - level)) || IS_TRUSTED(ch) ||
		    (IS_PC(ch) && obj->value[5] == GET_PID(ch)) ||
		    (IS_NPC(ch) && obj->value[5] == GET_RNUM(ch)) || (dispelDmg >= obj->value[2]))
		{
			/* clear the other side */
			if (EXIT(ch, obj->value[1]))
			{
				for (temp_wall = world[EXIT(ch, obj->value[1])->to_room].contents;
				     temp_wall; temp_wall = next_obj)
				{
					next_obj = temp_wall->next_content;
					if ((temp_wall->R_num == obj->R_num) &&
					    (temp_wall->value[1] == rev_dir[obj->value[1]]))
					{
						Decay(temp_wall);
					}
				}
			}
			Decay(obj);
		}
		else
			obj->value[2] -= dispelDmg;
		return;
	}

	/* casting is on a "normal" object.. only has an effect if the
	   object has a MAGIC flag! */
	if (IS_SET(obj->extra2_flags, ITEM2_MAGIC))
	{
		if (IS_ARTIFACT(obj))
		{
			send_to_char("&+GIt is impossible to dispel an artifact!\r\n", ch);
			return;
		}

		/* 5% chance of totally destroying the object <Cackle> */
		if (!number(0, 4) && !IS_TRUSTED(ch))
		{
			send_to_char("&+YUh oh!&N\n", ch);

			Decay(obj);
		}
		else if ((number(25, 59) < level) || IS_TRUSTED(ch))
		{
			act("&+bThe magic aura around&n $p&+b fades.", TRUE, ch, obj, 0, TO_CHAR);
			REMOVE_BIT(obj->extra2_flags, ITEM2_MAGIC);
			obj->affected[0].location = APPLY_NONE;
			obj->affected[0].modifier = 0;
			obj->affected[1].location = APPLY_NONE;
			obj->affected[1].modifier = 0;
		}
	}
}

void spell_dispel_good(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		       P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"A &+rred aura&N surrounds you as you shoot out a &+Lblack bolt of energy&N from your hands to destroy $N.",
		"A bolt of evil energy sent by $n engulfs you, dissolving your very soul!",
		"A &+rred aura&N surrounds $n as a &+Lblack bolt of energy&N shoots out from $s hands to dispel $N.",
		"You cackle in triumph as the essence of $N is utterly destroyed by your evil power!",
		"You scream in horror as your soul is purged from existance by the evil forces of $n!",
		"$N screams in terror as $S soul is ripped to shreds by the vile power of $n's spell!"
	};

	/*  if(IS_GOOD(ch))
	      victim = ch; */
	// Removed because of bard song
	if (!IS_GOOD(victim))
	{
		act("$N basks in your ignorance.", FALSE, ch, 0, victim, TO_CHAR);
		act("$n is clueless.", FALSE, ch, 0, victim, TO_VICT);
		act("$N chuckles at $n's cluelessness.", FALSE, ch, 0, victim, TO_NOTVICT);
		return;
	}
	dam = dice(level, 8);

	if (saves_spell(victim, SAVING_SPELL))
		dam >>= 1;

	spell_damage(ch, victim, dam, SPLDAM_HOLY, SPLDAM_ALLGLOBES, &messages);
}

void spell_dispel_evil(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		       P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"$N shivers, and suffers from $S evilness!",
		"$n makes your soul hurt and suffer!",
		"$n makes $N's evil spirit shiver and suffer!",
		"$N is dissolved by your goodness.",
		"$n dissolves you, you regret having been so evil, and die...",
		"$n completely dissolves $N."
	};

	/*  if(IS_EVIL(ch))
	      victim = ch; */
	// Removed because of bard song
	if (!IS_EVIL(victim))
	{
		act("$N chuckles at your foolishness.", FALSE, ch, 0, victim, TO_CHAR);
		act("$n is clueless.", FALSE, ch, 0, victim, TO_VICT);
		act("$N chuckles at $n's cluelessness.", FALSE, ch, 0, victim, TO_NOTVICT);
		return;
	}
	dam = dice((level + 1), 5);

	if (saves_spell(victim, SAVING_SPELL))
		dam >>= 1;

	spell_damage(ch, victim, dam, SPLDAM_HOLY, SPLDAM_ALLGLOBES, &messages);
}
