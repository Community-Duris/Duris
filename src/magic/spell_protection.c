#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "magic/spells.h"

extern bool has_skin_spell(P_char ch);
#include <strings.h>
#include <string.h>
void spell_protection_from_evil(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
				P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_PROTECT_FROM_EVIL))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_PROTECT_FROM_EVIL;
		af.duration = level;
		af.bitvector = AFF_PROTECT_EVIL;
		affect_to_char(victim, &af);
		send_to_char("&+YYou feel protected from the evil of the world!\n", victim);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_PROTECT_FROM_EVIL)
			{
				af1->duration = level;
			}
	}
}

void spell_protection_from_good(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
				P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_PROTECT_FROM_GOOD))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_PROTECT_FROM_GOOD;
		af.duration = level;
		af.bitvector = AFF_PROTECT_GOOD;
		affect_to_char(victim, &af);
		send_to_char("&+rYou feel able to withstand the holy goodness of the vile world!\n",
			     victim);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_PROTECT_FROM_GOOD)
			{
				af1->duration = level;
			}
	}
}

void spell_protection_from_fire(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
				P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_PROTECT_FROM_FIRE))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_PROTECT_FROM_FIRE;
		af.duration = level;
		af.bitvector = AFF_PROT_FIRE;
		affect_to_char(victim, &af);
		send_to_char("You feel protected from the &+Rfire!\n", victim);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_PROTECT_FROM_FIRE)
			{
				af1->duration = level;
			}
	}
}

void spell_protection_from_cold(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
				P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_PROTECT_FROM_COLD))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_PROTECT_FROM_COLD;
		af.duration = level;
		af.bitvector2 = AFF2_PROT_COLD;
		affect_to_char(victim, &af);
		send_to_char("You feel protected from the &+ccold!\n", victim);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_PROTECT_FROM_COLD)
			{
				af1->duration = level;
			}
	}
}

void spell_protection_from_living(int level, P_char /*ch*/, char * /*arg*/,
				  [[maybe_unused]] int type, P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_PROTECT_FROM_LIVING))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_PROTECT_FROM_LIVING;
		af.duration = level;
		af.bitvector4 = AFF4_PROT_LIVING;
		affect_to_char(victim, &af);
		send_to_char("&+LAn aura of death surrounds you!&n\n", victim);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_PROTECT_FROM_LIVING)
			{
				af1->duration = level;
			}
	}
}

void spell_protection_from_animals(int level, P_char /*ch*/, char * /*arg*/,
				   [[maybe_unused]] int type, P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_PROTECT_FROM_ANIMAL))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_PROTECT_FROM_ANIMAL;
		af.duration = level;
		af.bitvector3 = AFF3_PROT_ANIMAL;
		affect_to_char(victim, &af);
		send_to_char("You feel protected from the &+Ganimals of the world!\n", victim);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_PROTECT_FROM_ANIMAL)
			{
				af1->duration = level;
			}
	}
}

void spell_protection_from_gas(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
			       P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_PROTECT_FROM_GAS))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_PROTECT_FROM_GAS;
		af.duration = level;
		af.bitvector2 = AFF2_PROT_GAS;
		affect_to_char(victim, &af);
		send_to_char("You feel protected from the &+Gpoisonous gasses!\n", victim);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_PROTECT_FROM_GAS)
			{
				af1->duration = level;
			}
	}
}

void spell_protection_from_acid(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
				P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_PROTECT_FROM_ACID))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_PROTECT_FROM_ACID;
		af.duration = level;
		af.bitvector2 = AFF2_PROT_ACID;
		affect_to_char(victim, &af);
		send_to_char("You feel protected from &+yacid!\n", victim);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_PROTECT_FROM_ACID)
			{
				af1->duration = level;
			}
	}
}

void spell_protection_from_lightning(int level, P_char /*ch*/, char * /*arg*/,
				     [[maybe_unused]] int type, P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_PROTECT_FROM_LIGHTNING))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_PROTECT_FROM_LIGHTNING;
		af.duration = level;
		af.bitvector2 = AFF2_PROT_LIGHTNING;
		affect_to_char(victim, &af);
		send_to_char("You feel protected from the &+Clightning!\n", victim);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_PROTECT_FROM_LIGHTNING)
			{
				af1->duration = level;
			}
	}
}

void spell_negative_energy_barrier(int /*level*/, P_char ch, char * /*arg*/,
				   [[maybe_unused]] int type, P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (IS_AFFECTED2(victim, AFF2_SOULSHIELD))
	{
		send_to_char(
			"&+LUsing this in conjuction with the putrid &+Wholy&+L energy is absurd.&n\n",
			ch);
		return;
	}

	if (!IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
	{
		act("&+L$n&+L is surrounded by a deadly aura of &n&+bnegative energy!", TRUE,
		    victim, 0, 0, TO_ROOM);
		act("&+LYou are surrounded by a deadly aura of &n&+bnegative energy!", TRUE, victim,
		    0, 0, TO_CHAR);
		bzero(&af, sizeof(af));
		af.type = SPELL_NEG_ENERGY_BARRIER;
		af.duration = 10;
		af.bitvector4 = AFF4_NEG_SHIELD;
		affect_to_char(victim, &af);
	}
}

void spell_soulshield(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		      P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	if (!ch || !IS_ALIVE(ch) || !victim || !IS_ALIVE(victim))
	{
		return;
	}
	if ((GET_ALIGNMENT(ch) > -950) && (GET_ALIGNMENT(ch) < 950) && !IS_MULTICLASS_PC(ch))
	{
		send_to_char("Your beliefs aren't strong enough!\n", ch);
		return;
	}

	if (IS_AFFECTED4(victim, AFF4_NEG_SHIELD))
	{
		send_to_char(
			"&+WHoly &+Lenergies do not mix so well with those wrought of the Negative Plane.&n\n",
			ch);
		return;
	}
	if (!IS_AFFECTED2(victim, AFF2_SOULSHIELD))
	{
		char buf1[500], buf2[500];

		if (IS_EVIL(ch))
		{
			strcpy(buf1, "&+rAn aura of malevolency forms around&n $n!");
			strcpy(buf2, "&+rAn aura of malevolency forms around you!&n");
		}
		else
		{
			strcpy(buf1, "&+WA holy aura forms around&n $n!");
			strcpy(buf2, "&+WA holy aura forms around you!&n");
		}
		act(buf1, TRUE, victim, 0, 0, TO_ROOM);
		act(buf2, TRUE, victim, 0, 0, TO_CHAR);

		bzero(&af, sizeof(af));
		af.type = SPELL_SOULSHIELD;
		af.duration = (int)(15 + GET_CHAR_SKILL(ch, SKILL_DEVOTION) / 5);
		af.bitvector2 = AFF2_SOULSHIELD;
		affect_to_char(victim, &af);
	}
}

void spell_divine_warding(int level, P_char ch, char * /*arg*/, int type, P_char victim,
			  P_obj /*obj*/)
{
	if (!IS_ALIVE(ch) || !IS_ALIVE(victim) || ch->in_room == NOWHERE ||
	    ch->in_room != victim->in_room)
		return;

	// Keep the individual protections' duration, refresh, and dispel behavior.
	spell_protection_from_fire(level, ch, nullptr, type, victim, nullptr);
	spell_protection_from_cold(level, ch, nullptr, type, victim, nullptr);
	spell_protection_from_acid(level, ch, nullptr, type, victim, nullptr);
	spell_protection_from_gas(level, ch, nullptr, type, victim, nullptr);
	spell_protection_from_lightning(level, ch, nullptr, type, victim, nullptr);
	spell_protection_from_good(level, ch, nullptr, type, victim, nullptr);
	spell_protection_from_evil(level, ch, nullptr, type, victim, nullptr);
}

void spell_deflect(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		   P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (IS_AFFECTED4(victim, AFF4_DEFLECT))
		return;

	if (IS_NPC(ch) && GET_VNUM(ch) == 250)
	{
		act("&+cA &+Ctranslucent&n&+c field surrounds&n $n &+cthen the field crumbles into &+Rsparks!&n",
		    TRUE, victim, 0, 0, TO_ROOM);
		return;
	}

	if (!affected_by_spell(victim, SPELL_DEFLECT))
	{
		act("&+cA &+Ctranslucent&n&+c field flashes around $n&n&+c, then vanishes.", TRUE,
		    victim, 0, 0, TO_ROOM);
		act("&+cA &+Ctranslucent&n&+c field briefly flashes around your body, then fades.",
		    TRUE, victim, 0, 0, TO_CHAR);

		bzero(&af, sizeof(af));
		af.type = SPELL_DEFLECT;
		af.duration = -1;
		af.bitvector4 = AFF4_DEFLECT;
		affect_to_char(victim, &af);
	}
}

void spell_sanctuary(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		     P_char /*victim*/, P_obj /*obj*/)
{
	struct affected_type af;

	if (IS_AFFECTED4(ch, AFF4_SANCTUARY))
	{
		act("You are already blessed with sanctuary!", FALSE, ch, 0, ch, TO_CHAR);
		return;
	}

	bzero(&af, sizeof(af));
	af.type = SPELL_SANCTUARY;
	af.location = APPLY_NONE;
	af.duration = 15;
	af.bitvector4 = AFF4_SANCTUARY;
	affect_to_char(ch, &af);

	act("&+W$n's&+W is encased in a solid white aura!", FALSE, ch, 0, 0, TO_ROOM);
	act("&+WYou are encased in a solid white aura!", FALSE, ch, 0, 0, TO_CHAR);
}

void spell_stornogs_spheres(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			    P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}
	if (IS_AFFECTED4(victim, AFF4_STORNOGS_SPHERES))
		return;

	memset(&af, 0, sizeof(af));
	af.type = SPELL_STORNOGS_SPHERES;
	af.bitvector4 = AFF4_STORNOGS_SPHERES;
	af.duration = -1;
	af.flags = AFFTYPE_NOSHOW;

	if (!IS_AFFECTED4(victim, AFF4_STORNOGS_SPHERES) && (GET_CLASS(victim, CLASS_CONJURER)))
	{
		act("&+RSpheres begin rotating around $n.", TRUE, victim, 0, 0, TO_ROOM);
		act("&+RSpheres begin rotating around you.", TRUE, victim, 0, 0, TO_CHAR);
		af.modifier = MAX(1, (GET_LEVEL(victim) - 50)) / 2 + 1;
	}
	else
	{
		act("&+RSpheres begin rotating around $n.", TRUE, victim, 0, 0, TO_ROOM);
		act("&+RSpheres begin rotating around you.", TRUE, victim, 0, 0, TO_CHAR);
		af.modifier = 1;
	}
	affect_to_char(victim, &af);
}

void spell_group_stornog(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
			 P_obj /*obj*/)
{
	struct group_list *gl;

	if (ch && ch->group)
	{
		gl = ch->group;
		/* leader first */
		if (gl->ch->in_room == ch->in_room)
			spell_stornogs_spheres(level, ch, 0, 0, gl->ch, 0);
		/* followers */
		for (gl = gl->next; gl; gl = gl->next)
		{
			if (gl->ch->in_room == ch->in_room)
				spell_stornogs_spheres(level, ch, 0, 0, gl->ch, 0);
		}
	}
}

void spell_natures_blessing(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			    P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (affected_by_spell(ch, SPELL_NATURES_BLESSING))
	{
		struct affected_type *af1;
		for (af1 = victim->affected; af1; af1 = af1->next)
		{
			if (af1->type == SPELL_NATURES_BLESSING)
			{
				af1->duration = 72;
			}
		}
		return;
	}

	bzero(&af, sizeof(af));
	af.type = SPELL_NATURES_BLESSING;
	af.duration = 72;
	switch (world[ch->in_room].sector_type)
	{
	case SECT_FOREST:
		af.bitvector = AFF_PROT_FIRE;
		af.bitvector2 = AFF2_PROT_GAS | AFF2_PROT_ACID | AFF2_PROT_COLD |
				AFF2_PROT_LIGHTNING;
		break;
	case SECT_HILLS:
	case SECT_FIELD:
	case SECT_MOUNTAIN:
		af.bitvector2 = AFF2_PROT_COLD | AFF2_PROT_LIGHTNING;
		break;
	case SECT_SWAMP:
	case SECT_DESERT:
		af.bitvector = AFF_PROT_FIRE;
		af.bitvector2 = AFF2_PROT_ACID | AFF2_PROT_GAS;
		break;
	default:
		af.bitvector = AFF_PROTECT_GOOD | AFF_PROTECT_EVIL;
		break;
	}

	affect_to_char(ch, &af);
	send_to_char("You feel overwhelmed with the &+gwarmth &+Gof &+gnature&n.\n", ch);
	act("$n's skin &+Gflashes green&n for a moment, and then returns to normal.\n", FALSE, ch,
	    0, 0, TO_ROOM);
}

void spell_air_form(int /*level*/, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
		    P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (IS_AFFECTED2(victim, AFF2_AIR_AURA))
		return;

	if (!affected_by_spell(victim, SPELL_AIR_FORM))
	{
		act("&+CThe image of &+W$n &+Wb&+cl&+wu&+Cr&+ws &+Cand starts to &+cfade &+Win &+Cand &+Lout &+Cof e&+wx&+ci&+Ws&+ct&+we&+Cn&+wc&+Ce.&N",
		    TRUE, victim, 0, 0, TO_ROOM);
		act("&+CYou feel as if your &+cm&+Wo&+wl&+We&+cc&+Wu&+wl&+ce&+Ws &+Cstart dr&+cift&+Ling.&N",
		    TRUE, victim, 0, 0, TO_CHAR);

		bzero(&af, sizeof(af));
		af.type = SPELL_AIR_FORM;
		af.duration = 8;
		af.bitvector2 = AFF2_AIR_AURA;
		affect_to_char(victim, &af);
	}
}

void spell_prot_undead(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		       P_char victim, P_obj obj)
{
	struct affected_type af;

	if (!IS_UNDEADRACE(victim) && !IS_ANGEL(victim) && GET_RACE(victim) != RACE_GOLEM &&
	    !GET_CLASS(victim, CLASS_NECROMANCER))
	{
		send_to_char("The target is not undead!\r\n", ch);
		return;
	}

	if (level > 45 && !IS_AFFECTED2(victim, AFF2_GLOBE))
	{
		spell_globe(level, ch, 0, 0, victim, obj);
	}

	/*
	if(has_skin_spell(victim) && IS_PC(victim))
	{
	  act("$N is already protected by a &+cmagical barrier.&n",
	    TRUE, ch, 0, victim, TO_CHAR);
	  return;
	}
	*/

	bzero(&af, sizeof(af));

	/* so that the spell actually wears off properly, set it to stone skin */

	af.type = SPELL_STONE_SKIN;
	af.modifier = (level / 4) + number(1, 4);
	af.duration = BOUNDED(1, GET_LEVEL(ch) / 5, 10);
	affect_to_char(victim, &af);

	if (GET_CLASS(ch, CLASS_THEURGIST))
	{
		act("With a quick &+Cge&+cs&+bt&+cu&+Cre&n, $N's skin hardens to the &+ydensity&n of &+Lstone&n.",
		    FALSE, ch, 0, victim, TO_CHAR);
		act("With a quick &+Cge&+cs&+bt&+cu&+Cre&n from $n, $N's skin hardens to the &+ydensity&n of &+Lstone&n.",
		    FALSE, ch, 0, victim, TO_ROOM);
		act("With a quick &+Cge&+cs&+bt&+cu&+Cre&n from $n, your skin hardens to the &+ydensity&n of &+Lstone&n.",
		    FALSE, ch, 0, victim, TO_VICT);
	}
	else
	{
		act("A dark chill from beyond the &+Lgrave&n permeates the air around $n.&n", FALSE,
		    victim, 0, 0, TO_ROOM);
		send_to_char("You feel protected by powers from beyond the &+Lgrave.\r\n", victim);
	}
}

void spell_prot_from_undead(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			    P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (affected_by_spell(victim, SPELL_PROT_FROM_UNDEAD))
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_PROT_FROM_UNDEAD)
			{
				af1->duration = BOUNDED(2, GET_LEVEL(ch) / 3, 20);
			}
		return;
	}
	if (IS_UNDEADRACE(victim))
	{
		send_to_char("The target is undead! DUH!\n", ch);
		return;
	}
	bzero(&af, sizeof(af));
	af.type = SPELL_PROT_FROM_UNDEAD;
	af.modifier = GET_LEVEL(ch);
	af.duration = BOUNDED(2, GET_LEVEL(ch) / 3, 20);
	af.bitvector5 = AFF5_PROT_UNDEAD;
	affect_to_char(ch, &af);
	act("&+WA field of &+Yliving energy&+W slowly forms around $N.", TRUE, ch, 0, victim,
	    TO_CHAR);
}

void spell_death_blessing(int /*level*/, P_char ch, char * /*args*/, int /*type*/,
			  P_char /*victim*/, P_obj /*obj*/)
{
	struct group_list *gl;

	if (ch && ch->group)
	{
		gl = ch->group;
		/* leader first */
		if (gl->ch->in_room == ch->in_room)
			spell_vitalize_undead(56, ch, NULL, 0, gl->ch, 0);
		spell_protection_from_living(50, ch, 0, 0, gl->ch, 0);
		/* followers */
		for (gl = gl->next; gl; gl = gl->next)
		{
			if (gl->ch->in_room == ch->in_room)
				spell_vitalize_undead(50, ch, NULL, 0, gl->ch, 0);
			spell_protection_from_living(50, ch, 0, 0, gl->ch, 0);
		}
	}
}

void spell_waterbreath(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
		       P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (affected_by_spell(victim, SPELL_WATERBREATH))
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_WATERBREATH)
			{
				af1->duration = level;
			}
		return;
	}
	bzero(&af, sizeof(af));
	af.type = SPELL_WATERBREATH;
	af.duration = level;
	af.bitvector = AFF_WATERBREATH;

	if (!IS_AFFECTED(victim, AFF_WATERBREATH))
	{
		act("You suddenly grow gills!", FALSE, victim, 0, 0, TO_CHAR);
		act("$n suddenly grows gills!", TRUE, victim, 0, 0, TO_ROOM);
	}
	affect_to_char(victim, &af);
}

void spell_starshell(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		     P_char /*vict*/, P_obj /*obj*/)
{
	struct room_affect af;

	if (!OUTSIDE(ch) || !NORMAL_PLANE(ch->in_room))
	{
		send_to_char("This magic won't work here!\n", ch);
		return;
	}

	if (get_spell_from_room(&world[ch->in_room], SPELL_STARSHELL))
	{
		send_to_room("&+WThe &+Yblazing&N&+W shell of light overhead &+REXPLODES&n!\n",
			     ch->in_room);
		af.duration = (120 + ((level - 46) * 8));
		return;
	}

	memset(&af, 0, sizeof(af));
	af.type = SPELL_STARSHELL;
	af.duration = (120 + ((level - 46) * 8));
	affect_to_room(ch->in_room, &af);

	send_to_room("&+WA &+Yblazing&N&+W shell of light appears overhead!\n", ch->in_room);
	return;
}

void spell_drakescale_aegis(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			    P_char /*victim*/, P_obj /*obj*/)
{
	struct affected_type af;
	int absorb = (level / 4) + number(1, 4);

	if (!ch || !IS_ALIVE(ch))
	{
		return;
	}

	if (!has_skin_spell(ch))
	{
		absorb = (int)(absorb * .8);
		act("Scales erupt from under $n's &+rflesh&n covering them in thick &+Gdrak&+Les&+Gcale&n armor.",
		    TRUE, ch, 0, 0, TO_ROOM);
		act("Scales erupt from under your flesh as the &+Gdr&+Lag&+Gon&n god's &+rpower&n protects you.",
		    TRUE, ch, 0, 0, TO_CHAR);
	}
	else
	{
		send_to_char("&+GDrak&+Les&+Gcale&n already pentrates your &nflesh&n!\n", ch);
		return;
	}

	bzero(&af, sizeof(af));
	af.type = SPELL_DRAKESCALE_AEGIS;
	af.duration = (int)(10 + GET_CHAR_SKILL(ch, SKILL_SPELL_KNOWLEDGE_SHAMAN) / 5);
	af.modifier = absorb;
	affect_to_char(ch, &af);

	// Take a little damage for your benefits
	spell_damage(ch, ch, GET_HIT(ch) * 0.10f, SPLDAM_SPIRIT, SPLDAM_GRSPIRIT | SPLDAM_NOSHRUG,
		     NULL);
}
