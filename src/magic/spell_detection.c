#include "core/prototypes.h"
#include "core/structs.h"
#include "cmd/interp.h"
#include "world/db.h"
#include "net/comm.h"
#include "core/utils.h"
#include "core/defines.h"
#include "classes/disguise.h"
#include "magic/spells.h"
#include <stdio.h>
#include <strings.h>

extern P_desc descriptor_list;

void spell_detect_evil(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		       P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (affected_by_spell(victim, SPELL_DETECT_EVIL))
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_DETECT_EVIL)
			{
				af1->duration = KludgeDuration(ch, level, level);
			}
		return;
	}
	bzero(&af, sizeof(af));

	af.type = SPELL_DETECT_EVIL;
	af.duration = KludgeDuration(ch, level, level);
	af.bitvector2 = AFF2_DETECT_EVIL;

	affect_to_char(victim, &af);

	send_to_char("&+rYour eyes tingle.\n", victim);
}

void spell_detect_good(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		       P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (affected_by_spell(victim, SPELL_DETECT_GOOD))
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_DETECT_GOOD)
			{
				af1->duration = KludgeDuration(ch, level, level);
			}
		return;
	}
	bzero(&af, sizeof(af));

	af.type = SPELL_DETECT_GOOD;
	af.duration = KludgeDuration(ch, level, level);
	af.bitvector2 = AFF2_DETECT_GOOD;

	affect_to_char(victim, &af);

	send_to_char("&+YYour eyes tingle.\n", victim);
}

void spell_detect_magic(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (affected_by_spell(victim, SPELL_DETECT_MAGIC))
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_DETECT_MAGIC)
			{
				af1->duration = KludgeDuration(ch, level, level);
			}
		send_to_char("&+bYour sight of magical auras is renewed.&n\n", victim);
		return;
	}
	bzero(&af, sizeof(af));
	af.type = SPELL_DETECT_MAGIC;
	af.duration = KludgeDuration(ch, level, level);
	af.bitvector2 = AFF2_DETECT_MAGIC;

	affect_to_char(victim, &af);
	send_to_char("&+bYour vision sharpens; magical auras become visible.&n\n", victim);
}

void spell_detect_invisibility(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
			       P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (affected_by_spell(victim, SPELL_DETECT_INVISIBLE))
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_DETECT_INVISIBLE)
			{
				af1->duration = MAX(level, 10);
			}
		return;
	}
	bzero(&af, sizeof(af));
	af.type = SPELL_DETECT_INVISIBLE;
	af.duration = MAX(level, 10);
	af.bitvector = AFF_DETECT_INVISIBLE;

	affect_to_char(victim, &af);

	send_to_char("&+WYour eyes tingle.\n", victim);
}

void spell_detect_poison(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			 P_char victim, P_obj obj)
{
	if (victim)
	{
		if (victim == ch)
			if (IS_SET(ch->specials.affected_by2, AFF2_POISONED))
				send_to_char("&+GYou can sense poison in your blood.\n", ch);
			else
				send_to_char("&+WYou feel healthy.\n", ch);
		else if (IS_SET(victim->specials.affected_by2, AFF2_POISONED))
		{
			act("&+GYou sense that $E is poisoned.", FALSE, ch, 0, victim, TO_CHAR);
		}
		else
		{
			act("&+WYou sense that $E is healthy.", FALSE, ch, 0, victim, TO_CHAR);
		}
	}
	else
	{ /*
	   * It's an object
	   */
		if ((obj->type == ITEM_DRINKCON) || (obj->type == ITEM_FOOD))
		{
			if (obj->value[3])
				act("&+GPoisonous fumes are revealed.", FALSE, ch, 0, 0, TO_CHAR);
			else
				send_to_char("&+WIt looks very delicious.\n", ch);
		}
	}
}

void spell_sense_holiness(int /*level*/, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
			  P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_SENSE_HOLINESS))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_SENSE_HOLINESS;
		af.duration = 25;
		af.bitvector4 = AFF4_SENSE_HOLINESS;
		affect_to_char(victim, &af);
		send_to_char("&+WYou can now sense holy auras!\n", victim);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_SENSE_HOLINESS)
			{
				af1->duration = 25;
			}
	}
}

void spell_sense_life(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		      P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_SENSE_LIFE))
	{
		send_to_char("&+LYour feel your awareness improve.\n", ch);

		bzero(&af, sizeof(af));
		af.type = SPELL_SENSE_LIFE;
		af.duration = level;
		af.bitvector = AFF_SENSE_LIFE;
		affect_to_char(victim, &af);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_SENSE_LIFE)
			{
				af1->duration = level;
			}
	}
}

void spell_lodestone_vision(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			    P_char victim, P_obj /*obj*/)
{
	if (IS_AFFECTED5(ch, AFF5_MINE))
	{
		if (ch == victim)
			send_to_char("You are already under the influence of miners sight.\r\n",
				     ch);
		act("$N is already under the influence of miners sight.", TRUE, ch, 0, victim,
		    TO_CHAR);
		return;
	}

	if (!affected_by_spell(victim, SPELL_LODESTONE))
	{
		act("A faint glimmer passes across your eyes as you feel your ability to detect minerals improve.",
		    FALSE, ch, 0, victim, TO_CHAR);
		struct affected_type af;
		bzero(&af, sizeof(af));
		af.type = SPELL_LODESTONE;
		af.bitvector5 = AFF5_MINE;
		af.duration = number(5, 7);
		affect_to_char(victim, &af);
	}
}

void spell_farsee(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
		  P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_FARSEE))
	{
		act("&+CYour eyes begin to tingle.", TRUE, victim, 0, 0, TO_CHAR);

		bzero(&af, sizeof(af));
		af.type = SPELL_FARSEE;
		af.duration = level * 2;
		af.bitvector = AFF_FARSEE;
		affect_to_char(victim, &af);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_FARSEE)
			{
				af1->duration = level * 2;
			}
	}
}

void spell_true_seeing(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		       P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (affected_by_spell(ch, SPELL_TRUE_SEEING))
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_TRUE_SEEING)
			{
				af1->duration = 72;
			}
		return;
	}
	bzero(&af, sizeof(af));

	af.type = SPELL_TRUE_SEEING;
	af.duration = 72;

	af.bitvector4 = AFF4_SENSE_HOLINESS;
	af.bitvector2 = AFF2_DETECT_MAGIC | AFF2_DETECT_GOOD | AFF2_DETECT_EVIL;
	af.bitvector = AFF_SENSE_LIFE | AFF_DETECT_INVISIBLE;
	affect_to_char(ch, &af);

	send_to_char("&+MYour vision sharpens considerably.&n\n", ch);
}

void spell_animal_vision(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			 P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (affected_by_spell(ch, SPELL_ANIMAL_VISION))
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_ANIMAL_VISION)
			{
				af1->duration = 72;
			}
		return;
	}
	bzero(&af, sizeof(af));
	af.type = SPELL_ANIMAL_VISION;
	af.duration = 72;
	switch (world[ch->in_room].sector_type)
	{
	case SECT_FOREST:
		if (GET_LEVEL(ch) > 45)
		{
			af.bitvector = AFF_SENSE_LIFE | AFF_FARSEE | AFF_DETECT_INVISIBLE;
		}
		else
		{
			af.bitvector = AFF_SENSE_LIFE | AFF_FARSEE;
		}
		break;
	case SECT_MOUNTAIN:
		af.bitvector = AFF_SENSE_LIFE;
		af.bitvector2 = AFF2_DETECT_MAGIC;
		break;
	case SECT_FIELD:
	case SECT_HILLS:
		af.bitvector = AFF_FARSEE;
		af.bitvector2 = AFF2_DETECT_MAGIC | AFF2_DETECT_GOOD | AFF2_DETECT_EVIL;
		break;
	case SECT_SWAMP:
		af.bitvector = AFF_SENSE_LIFE;
		af.bitvector2 = AFF2_DETECT_EVIL | AFF2_DETECT_GOOD;
		break;
	case SECT_UNDRWLD_WILD:
	case SECT_UNDRWLD_CITY:
	case SECT_UNDRWLD_MOUNTAIN:
	case SECT_UNDRWLD_SLIME:
	case SECT_UNDRWLD_LOWCEIL:
	case SECT_UNDRWLD_LIQMITH:
	case SECT_UNDRWLD_MUSHROOM:
		af.bitvector = AFF_INFRAVISION;
		break;
	default:
		af.bitvector2 = AFF2_DETECT_MAGIC | AFF2_DETECT_EVIL | AFF2_DETECT_GOOD;
		break;
	}
	affect_to_char(ch, &af);
	send_to_char("Your eyes turn &+Yj&n&+ye&+Yw&n&+ye&+Yl&n&+ye&+Yd&n like an &+yanimal&n.\n",
		     ch);
	act("$n's eyes turn &+Yj&n&+ye&+Yw&n&+ye&+Yl&n&+ye&+Yd&n like an &+yanimal&n!\n", FALSE, ch,
	    0, 0, TO_ROOM);
}

void spell_shadow_vision(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			 P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (affected_by_spell(ch, SPELL_SHADOW_VISION))
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_SHADOW_VISION)
			{
				af1->duration = 72;
			}
		return;
	}
	bzero(&af, sizeof(af));

	af.type = SPELL_SHADOW_VISION;
	af.duration = 72;

	af.bitvector2 = AFF2_DETECT_MAGIC | AFF2_DETECT_GOOD | AFF2_DETECT_EVIL;
	af.bitvector = AFF_SENSE_LIFE | AFF_DETECT_INVISIBLE;
	affect_to_char(ch, &af);

	send_to_char("&+LA dark tint covers your vision.&n\n", ch);
	act("&+m$n's&+M eyes turn &+Lpitch black!", FALSE, ch, 0, 0, TO_ROOM);
}

void spell_sense_follower(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
			  P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!IS_AFFECTED4(victim, AFF4_SENSE_FOLLOWER))
	{
		send_to_char("&+LYou feel your awareness improve.\n", victim);

		bzero(&af, sizeof(af));
		af.type = SPELL_SENSE_FOLLOWER;
		af.duration = level;
		af.bitvector4 = AFF4_SENSE_FOLLOWER;
		affect_to_char(victim, &af);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_SENSE_FOLLOWER)
			{
				af1->duration = level;
			}
	}
}

void spell_wizard_eye(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		      P_obj /*obj*/)
{
	int target, distance;
	char Gbuf1[MAX_STRING_LENGTH];

	if (!ch)
		return;

	if (!victim)
	{
		send_to_char("&+CYou failed.\n", ch);
		return;
	}

	target = victim->in_room;

	distance = MAX(level * 2, (int)get_property("spell.wizardEye.maxDistance", 100));

	if (IS_TRUSTED(victim) || racewar(ch, victim) ||
	    !(world[ch->in_room].zone == world[victim->in_room].zone) || IS_NPC(victim) ||
	    (IS_MAP_ROOM(ch->in_room) && IS_MAP_ROOM(victim->in_room) &&
	     (calculate_map_distance(ch->in_room, victim->in_room) > (distance * distance))))
	{
		send_to_char("&+CYou failed.\n", ch);
		return;
	}
	strcpy(Gbuf1, "&+WYou cast your sights far out into the zone...\n");
	send_to_char(Gbuf1, ch);
	new_look(ch, NULL, CMD_LOOKAFAR, target);

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_WIZARD_EYE);
	if (NewSaves(victim, SAVING_SPELL, mod))
		send_to_char("&+LYou feel strangely like you are being watched..\n", victim);
}

void spell_clairvoyance(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			P_obj /*obj*/)
{
	int target, where;
	char Gbuf1[MAX_STRING_LENGTH];

	if (!ch)
		return;

	if (!victim)
	{
		send_to_char("&+CYou failed.\n", ch);
		return;
	}

	// Disabling this spell to stop ppl from cheating.
	if (TRUE)
	{
		send_to_char("&+CYou failed.\n", ch);
		return;
	}

	where = world[victim->in_room].number;
	target = victim->in_room;
	if (number(0, 3))
		target = real_room0(where + number(-2, 2));

	if (!target)
	{
		send_to_char("&+CYou failed.\n", ch);
		return;
	}
	if ((GET_RACE(ch) != RACE_ILLITHID) && !IS_NPC(victim))
	{
		if (racewar(ch, victim))
		{
			send_to_char("&+CYou failed.\n", ch);
			return;
		}
	}
	if (IS_DISGUISE(victim))
	{
		send_to_char("&+CYou failed.\n", ch);
		return;
	}
	if (IS_TRUSTED(victim) || (IS_NPC(victim)))
	{
		send_to_char("&+CYou failed.\n", ch);
		return;
	}
	if (IS_AFFECTED3(victim, AFF3_NON_DETECTION))
	{
		send_to_char("&+CYou failed.\n", ch);
		return;
	}
	strcpy(Gbuf1, "&+WYou cast your sights far out into the realms...\n");
	send_to_char(Gbuf1, ch);
	new_look(ch, NULL, CMD_LOOKAFAR, target);

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_CLAIRVOYANCE);
	if (NewSaves(victim, SAVING_SPELL, mod) && victim->in_room == target)
		send_to_char("&+LYou feel strangely like you are being watched..\n", victim);
}

void spell_teleimage(int /*level*/, P_char ch, P_char victim, P_obj /*obj*/)
{
	int target;
	P_char tmp_victim;
	char Gbuf1[MAX_STRING_LENGTH], Gbuf2[MAX_STRING_LENGTH];

	if (!victim)
	{
		send_to_char("And just who are you trying to look for?\n", ch);
		return;
	}
	target = world[victim->in_room].number;
	strcpy(Gbuf1, "$n conjures up a cloud which thickens, solidifies, shimmers, and\n"
		      "then ... suddenly goes transparent and shows ...\n");
	act(Gbuf1, FALSE, ch, 0, 0, TO_CHAR);
	act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);
	snprintf(Gbuf2, MAX_STRING_LENGTH, "%d look", target);
	for (tmp_victim = world[ch->in_room].people; tmp_victim;
	     tmp_victim = tmp_victim->next_in_room)
	{
		if (IS_PC(tmp_victim))
			do_at(tmp_victim, Gbuf2, -4);
	}
}

void spell_infravision(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
		       P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (IS_AFFECTED(victim, AFF_INFRAVISION) || has_innate(victim, INNATE_OPHIDIAN_EYES))
		return;

	if (!affected_by_spell(victim, SPELL_INFRAVISION))
	{
		send_to_char("Your feel your infrared vision improve.\n", victim);
		act("$n's eyes start to glow &+rred&n!", FALSE, victim, 0, 0, TO_ROOM);

		bzero(&af, sizeof(af));
		af.type = SPELL_INFRAVISION;
		af.duration = level;
		af.bitvector = AFF_INFRAVISION;
		affect_to_char(victim, &af);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
		{
			if (af1->type == SPELL_INFRAVISION)
			{
				af1->duration = level;
			}
		}
	}
}

void spell_comprehend_languages(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
				P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_COMPREHEND_LANGUAGES))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_COMPREHEND_LANGUAGES;
		af.duration = level * 2;
		affect_to_char(victim, &af);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_COMPREHEND_LANGUAGES)
			{
				af1->duration = level * 2;
			}
	}

	send_to_char("&+WYou feel your understanding of the languages of Duris improve!\n", victim);
}

void spell_ether_sense(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*vict*/,
		       P_obj /*obj*/)
{
	int elevel, ilevel, glevel;
	P_desc d;
	char buf[256];

	if (!IS_ILLITHID(ch) && !IS_PILLITHID(ch))
	{
		send_to_char(
			"A flood of strange images stream into your brain at a mind-numbing pace..  Woah man, the colors.  Alas, you can't make sense of any of it.\n",
			ch);
		return;
	}

	d = descriptor_list;
	elevel = ilevel = glevel = 0;
	while (d)
	{
		if (!d->connected &&
		    (world[ch->in_room].zone == world[d->character->in_room].zone) &&
		    !(IS_ROOM(ch->in_room, ROOM_GUILD)))
		{
			/*found char in same zone */
			if ((GET_LEVEL(d->character) > 25) && !IS_TRUSTED(d->character))
			{
				if (IS_ILLITHID(d->character) || IS_PILLITHID(d->character))
				{
					ilevel += GET_LEVEL(d->character);
				}
				else
				{
					if (EVIL_RACE(d->character))
					{
						elevel += GET_LEVEL(d->character);
					}
					else
						glevel += GET_LEVEL(d->character);
				}
			}
		}
		d = d->next;
	}

	/* don't show them exact level.. */
	/*
	  if(elevel) elevel = 230;
	  if(glevel) glevel = 230;
	  if(ilevel) ilevel = 230;
	*/
	if (EVIL_RACE(ch))
	{
		if (glevel == 0)
		{
			snprintf(buf, 256,
				 "&+WYou detect no good presence in the ether around you.\n");
		}
		else
		{
			snprintf(buf, 256,
				 "&+WYou detect a good presence in the ether around you.\n");
		}
		send_to_char(buf, ch);
	}
	if (!IS_ILLITHID(ch) && !IS_PILLITHID(ch))
	{
		if (ilevel == 0)
		{
			snprintf(buf, 256,
				 "&+mYou detect no planar presence in the ether around you.\n");
		}
		else
		{
			snprintf(buf, 256,
				 "&+mYou detect planar presence in the ether around you.\n");
		}
		send_to_char(buf, ch);
	}
	if ((!EVIL_RACE(ch)) || IS_ILLITHID(ch))
	{ /* show evils */
		if (elevel == 0)
		{
			snprintf(buf, 256,
				 "&+rYou detect no evil presence in the ether around you.\n");
		}
		else
		{
			snprintf(buf, sizeof buf,
				 "&+rYou detect a evil presence in the ether around you.\n");
		}
		send_to_char(buf, ch);
	}
}
