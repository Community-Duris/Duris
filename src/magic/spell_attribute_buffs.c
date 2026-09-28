#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include <strings.h>

extern Skill skills[];
extern P_room world;

void spell_fortitude(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		     P_obj /*obj*/)
{
	struct affected_type af;
	int temp;

	temp = (int)(level / 10);
	if (!affected_by_spell(victim, SPELL_FORTITUDE))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_FORTITUDE;
		af.duration = 2 * (1 + temp);
		af.location = APPLY_CON;
		af.modifier = BOUNDED(1, GET_LEVEL(ch) / 4, 5);
		affect_to_char(victim, &af);
		send_to_char("&+WYour will to live grows stronger!\n", victim);
	}
}

void spell_dexterity(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
		     P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (affected_by_spell(victim, SPELL_DEXTERITY))
	{
		send_to_char("&+CYou can't get more dexterous than this!\n", victim);
		return;
	}
	act("&+gYou feel more dexterous.", FALSE, victim, 0, 0, TO_CHAR);

	bzero(&af, sizeof(af));

	af.type = SPELL_DEXTERITY;
	af.duration = level;
	af.modifier = 10;
	af.location = APPLY_DEX;

	affect_join(victim, &af, TRUE, FALSE);
}

void spell_agility(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		   P_obj /*obj*/)
{
	struct affected_type af;

	if (!IS_ALIVE(ch))
		return;

	if (affected_by_spell(victim, SPELL_AGILITY))
	{
		send_to_char("&+BYou can't get more agile than this!\n", victim);
		return;
	}

	act("&+BYou feel more agile.&n", FALSE, victim, 0, 0, TO_CHAR);

	bzero(&af, sizeof(af));

	af.type = SPELL_AGILITY;
	af.duration = level;
	af.modifier = 10;
	af.location = APPLY_AGI;

	affect_join(victim, &af, TRUE, FALSE);
}

void spell_strength(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
		    P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (affected_by_spell(victim, SPELL_STRENGTH))
	{
		send_to_char("&+CYou can't get much stronger than this!\n", victim);
		return;
	}
	act("&+CYou feel stronger.", FALSE, victim, 0, 0, TO_CHAR);

	bzero(&af, sizeof(af));

	af.type = SPELL_STRENGTH;
	af.duration = level;
	af.modifier = 10;
	af.location = APPLY_STR;

	affect_join(victim, &af, TRUE, FALSE);
}

void spell_pulchritude(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
		       P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	int temp;

	temp = (int)(level / 10);
	if (affected_by_spell(victim, SPELL_PULCHRITUDE))
	{
		send_to_char("&+CYou can't get much prettier than this!\n", victim);
		return;
	}
	act("&+CYour features soften a bit, you feel friendlier.", FALSE, victim, 0, 0, TO_CHAR);

	bzero(&af, sizeof(af));

	af.type = SPELL_PULCHRITUDE;
	af.duration = level;
	af.modifier = 5 * (1 + temp);
	af.location = APPLY_CHA;

	affect_join(victim, &af, TRUE, FALSE);
}
void spell_serendipity(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
		       P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	int skl_lvl;

	skl_lvl = (int)(level / 10);

	if (affected_by_spell(victim, SPELL_SERENDIPITY))
	{
		send_to_char("&+CYou can't get much luckier than this!\n", victim);
		return;
	}
	send_to_char("&+CYou feel much luckier.\n", victim);

	bzero(&af, sizeof(af));

	af.type = SPELL_SERENDIPITY;
	af.duration = level;
	af.modifier = 3 * (1 + skl_lvl);
	af.location = APPLY_LUCK;

	affect_join(victim, &af, TRUE, FALSE);
}

void spell_warring_zeal(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	struct affected_type *paf = NULL;

	if (!SanityCheck(ch, "spell_warring_zeal"))
		return;

	if ((paf = get_spell_from_char(ch, SPELL_WARRING_ZEAL)) != NULL)
	{
		// refresh
		paf->duration = 2;
		paf->location = APPLY_HITROLL;
		paf->modifier = level / 5;
		send_to_char("&+WThe &+Rbeat &+Wof the &+rdrums &+Wis renewed!&n\n", ch);
	}
	else
	{
		send_to_char("&+WThe sound of &+rwa&+Rrd&+rru&+Rms &+Wfill your head!&n\n", ch);
		act("$n &+Wsuddendly becomes &+rba&+Rtt&+rle &+Rcr&+raz&+Red!", TRUE, victim, 0, 0,
		    TO_ROOM);
		bzero(&af, sizeof(af));
		af.type = SPELL_WARRING_ZEAL;
		af.duration = 2;
		af.location = APPLY_HITROLL;
		af.modifier = level / 5;
		affect_to_char(ch, &af);
	}
}

void spell_infernal_fury(int /*level*/, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
			 P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!IS_AFFECTED(victim, AFF_INFERNAL_FURY))
	{
		act("$n &+Yis surrounded by &+Rinf&+rer&+Lnal f&+rla&+Rmes&+Y!&n", TRUE, victim, 0,
		    0, TO_ROOM);
		act("You &+Yare surrounded by &+Rinf&+rer&+Lnal f&+rla&+Rmes&+Y!&n", TRUE, victim,
		    0, 0, TO_CHAR);
		bzero(&af, sizeof(af));
		af.type = SPELL_INFERNAL_FURY;
		af.duration = 6;
		af.bitvector = AFF_INFERNAL_FURY;
		affect_to_char(victim, &af);
	}
}

void spell_divine_fury(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		       P_char /*victim*/, P_obj /*obj*/)
{
	struct affected_type af;
	char strn[256];

	if (!IS_ALIVE(ch) || ch->in_room == NOWHERE)
		return;

	if (affected_by_spell(ch, SPELL_DIVINE_FURY))
	{
		send_to_char("You are already &+Wdivinely furious!\n", ch);
		return;
	}

	bzero(&af, sizeof(af));
	af.type = SPELL_DIVINE_FURY;
	af.duration = 2;
	af.flags = AFFTYPE_NODISPEL | AFFTYPE_NOMSG;

	af.modifier = level / 5;
	af.location = APPLY_DAMROLL;
	affect_to_char(ch, &af);

	af.modifier = level / 5;
	af.location = APPLY_HITROLL;
	affect_to_char(ch, &af);

	send_to_char("&+RYour fury rages as you call upon your deity for power!\n", ch);
	snprintf(strn, 256, "%s$n %sis briefly surrounded by a%s glow!",
		 IS_EVIL(ch) ? "&+R" : "&+W", IS_EVIL(ch) ? "&+R" : "&+W",
		 IS_EVIL(ch) ? "n unholy" : " holy");
	act(strn, TRUE, ch, 0, 0, TO_ROOM);
}

void spell_battle_ecstasy(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			  P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!IS_ALIVE(ch))
	{
		return;
	}

	if (!IS_AFFECTED4(victim, AFF4_BATTLE_ECSTASY))
	{
		char buf1[500], buf2[500];

		strcpy(buf1, "$n &+rbegins to drool, a bloodthirsty look passing over $s face.&n");
		strcpy(buf2, "&+rYour blood begins to boil-- where's the fight?&n");
		act(buf1, TRUE, victim, 0, 0, TO_ROOM);
		act(buf2, TRUE, victim, 0, 0, TO_CHAR);

		bzero(&af, sizeof(af));
		af.type = SPELL_BATTLE_ECSTASY;
		af.duration = (int)(15 + GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 10);
		af.bitvector4 = AFF4_BATTLE_ECSTASY;
		affect_to_char(victim, &af);
	}
}

void spell_miracle(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		   P_obj /*obj*/)
{
	struct group_list *gl;

	if (ch && ch->group)
	{
		gl = ch->group;
		/* leader first */
		if (gl->ch->in_room == ch->in_room)
			spell_vitality(level, ch, NULL, 0, gl->ch, 0);
		spell_armor(level, ch, 0, 0, gl->ch, 0);
		spell_bless(level, ch, 0, 0, gl->ch, 0);
		/* followers */
		for (gl = gl->next; gl; gl = gl->next)
		{
			if (gl->ch->in_room == ch->in_room)
				spell_vitality(level, ch, NULL, 0, gl->ch, 0);
			spell_armor(level, ch, 0, 0, gl->ch, 0);
			spell_bless(level, ch, 0, 0, gl->ch, 0);
		}
	}
}

void spell_holy_sacrifice(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			  P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!IS_ALIVE(ch))
	{
		return;
	}

	if (!affected_by_spell(victim, SPELL_HOLY_SACRIFICE) &&
	    !IS_SET(ch->specials.affected_by4, AFF4_HOLY_SACRIFICE))
	{
		char buf1[500], buf2[500];

		/*
		    if(victim == ch){
		*/
		strcpy(buf1, "&+WA feeling of peace emanates from&n $n!");
		strcpy(buf2, "&+WYou feel as if your pain might do some good!&n");
		act(buf1, TRUE, victim, 0, 0, TO_ROOM);
		act(buf2, TRUE, victim, 0, 0, TO_CHAR);
		/*
		    }else{
		  strcpy(buf1, "&+WA beam of light links $n to $N, and then fades away&N.");
		  strcpy(buf2, "&+WYou feel linked to $n.&N");
		  act(buf1, TRUE, ch, 0, victim, TO_NOTVICT);
		  act(buf2, TRUE, ch, 0, victim, TO_VICT);
		  act(buf2, TRUE, victim, 0, ch, TO_VICT);
		    }
		*/
		bzero(&af, sizeof(af));
		af.type = SPELL_HOLY_SACRIFICE;
		af.duration = (int)(15 + GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 10);
		/*    af.modifier = GET_PID(ch); */
		af.bitvector4 = /*(ch == victim)? */ AFF4_HOLY_SACRIFICE /*:0 */;
		affect_to_char(victim, &af);
	}
	else
	{
		send_to_char("&+YYou are already holy enough.&n\n\r", ch);
	}
}

static bool area_divine_blessing_check(P_char ch, P_char caster)
{
	struct group_list *gl;
	struct affected_type af;

	if (!ch->group || !affected_by_spell(ch, SPELL_DIVINE_BLESSING) ||
	    GET_ALIGNMENT(ch) < get_property("spell.divineBlessing.areaShrugAlign", 970) ||
	    !ch->equipment[WIELD])
		return false;

	memset(&af, 0, sizeof(af));
	af.type = TAG_IMMUNE_AREA;
	af.duration = 10;
	af.flags = AFFTYPE_SHORT;

	act("&+WA mighty rush of &+Ccourage&+W sweeps over you as your $q goes forth "
	    "to absorb the mighty spell!",
	    FALSE, ch, ch->equipment[WIELD], 0, TO_CHAR);
	act("$n &+Wbrandishes $s $q, and dives forth in combat, absorbing the brunt "
	    "of $N's spell!",
	    FALSE, ch, ch->equipment[WIELD], caster, TO_NOTVICT);
	act("$n &+Wbrandishes $s $q, and dives forth in combat, absorbing the brunt "
	    "of your spell!",
	    FALSE, ch, ch->equipment[WIELD], caster, TO_VICT);

	for (gl = ch->group; gl; gl = gl->next)
	{
		affect_to_char(gl->ch, &af);
	}

	return true;
}

bool divine_blessing_check(P_char ch, P_char tar_char, int spl)
{
	if ((IS_SET(skills[spl].targets, TAR_IGNORE) || IS_SET(skills[spl].targets, TAR_AREA)) &&
	    number(0, 99) < get_property("spell.divineBlessing.areaShrugChance", 5))
	{
		P_char tch;

		for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
		{
			if (should_area_hit(ch, tch) && area_divine_blessing_check(tch, ch))
				break;
		}
	}
	else if (tar_char && affected_by_spell(tar_char, SPELL_DIVINE_BLESSING) &&
		 tar_char->equipment[WIELD] &&
		 number(0, 99) < get_property("spell.divineBlessing.singleShrugChance", 10))
	{
		act("&+wYou feel a &+Cf&+ci&+Wer&+cc&+Ce &+Wdetermination &+was your "
		    "weapon thrusts itself upward, and absorbs $N's onslaught!",
		    FALSE, tar_char, tar_char->equipment[WIELD], ch, TO_CHAR);
		act("&+wWith a &+Wsomber &+Cg&+cr&+Wa&+cc&+Ce&+w, $n holds $s $q high, absorbing $N's assault!",
		    FALSE, tar_char, tar_char->equipment[WIELD], ch, TO_NOTVICT);
		act("&+wWith a &+Wsomber &+Cg&+cr&+Wa&+cc&+Ce&+w, $n holds $s $q high, absorbing your assault!",
		    FALSE, tar_char, tar_char->equipment[WIELD], ch, TO_VICT);
		return TRUE;
	}

	return FALSE;
}

bool divine_blessing_parry(P_char ch, P_char victim)
{
	P_obj weapon = ch->equipment[WIELD];

	if (affected_by_spell(ch, SPELL_DIVINE_BLESSING) && weapon &&
	    GET_ALIGNMENT(ch) > get_property("spell.divineBlessing.parryAlign", 990) &&
	    number(0, 99) < get_property("spell.divineBlessing.parryChance", 10))
	{
		act("$n's $q moves with a blinding speed and successfully repels your attack!",
		    FALSE, ch, weapon, victim, TO_VICT);
		act("$n's $q moves with a blinding speed and successfully repels $N's attack!",
		    FALSE, ch, weapon, victim, TO_NOTVICT);
		act("Your $q moves with a blinding speed and successfully repels $N's attack!",
		    FALSE, ch, weapon, victim, TO_CHAR);
		return true;
	}
	else
		return false;
}

void spell_divine_blessing(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			   P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	P_obj weapon = victim->equipment[WIELD];
	struct damage_messages messages = {
		"&+wYou incite the &+rR&+RA&+rG&+RE &+wof the &+CGods&+w as you attempt to invoke a blessing of &+WP&+Curi&+Wty",
		"&+wYou incite the &+rR&+RA&+rG&+RE &+wof the &+CGods&+w as you attempt to invoke a blessing of &+WP&+Curi&+Wty",
		"&+L$N screams in &+Rp&+rAi&+RN&+L as the &+CGods&+L punish $M for his arrogance!"
	};

	if (GET_ALIGNMENT(ch) < 950)
	{
		spell_damage(ch, ch, 600, SPLDAM_HOLY,
			     SPLDAM_NODEFLECT | SPLDAM_NOSHRUG | RAWDAM_NOKILL, &messages);
		return;
	}

	if (affected_by_spell(victim, SPELL_DIVINE_BLESSING))
	{
		send_to_char("Your gods are with you already!", victim);
		return;
	}
	else if (!weapon)
	{
		send_to_char("You need a weapon.\n", victim);
	}
	else
	{
		send_to_char(
			"&+wYou thrust your weapon skywards, and infuse it with your &+Wvirtue &+wand &+wm&+Wi&+bg&+wh&+Wt&+w!\n",
			victim);
		act("$n holds $p high, and it begins to &+Yb&+Wur&+Yn &+wwith a &+Ch&+Wol&+Cy &+Wlight&+w!",
		    TRUE, victim, weapon, 0, TO_ROOM);
		memset(&af, 0, sizeof(af));
		af.type = SPELL_DIVINE_BLESSING;
		af.duration = 15;
		affect_to_char(victim, &af);
	}
}

void spell_bless(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		 P_obj obj)
{
	struct affected_type af;

	if (obj)
	{
		if ((5 * level > (GET_OBJ_WEIGHT(obj) / 4)) && (!IS_FIGHTING(ch)))
		{
			act("&+W$p briefly glows.", FALSE, ch, obj, 0, TO_CHAR);
			set_obj_affected_extra(obj, -1, SPELL_BLESS, 50, ITEM2_BLESS);
			if (obj->type == ITEM_DRINKCON)
			{
				if (IS_RACEWAR_GOOD(ch))
					obj->value[2] = LIQ_HOLYWATER;
				else if (IS_RACEWAR_EVIL(ch))
					obj->value[2] = LIQ_UNHOLYWAT;
			}
		}
	}
	else
	{
		if (!affected_by_spell(victim, SPELL_BLESS))
		{
			bzero(&af, sizeof(af));
			send_to_char("&+WYou suddenly feel blessed!\n", victim);
			af.type = SPELL_BLESS;
			af.duration = MAX(5, level / 2);
			af.modifier = ((int)(level / 20)) + 1;
			af.location = APPLY_HITROLL;
			affect_to_char(victim, &af);

			af.location = APPLY_SAVING_SPELL;
			af.modifier = -((int)(level / 30) + 1); /* Make better */
			affect_to_char(victim, &af);
		}
		else
		{
			struct affected_type *af1;

			for (af1 = victim->affected; af1; af1 = af1->next)
				if (af1->type == SPELL_BLESS)
				{
					af1->duration = MAX(5, level / 2);
				}
		}
	}
}
