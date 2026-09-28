#include "core/prototypes.h"
#include "combat/defense_resolution.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "combat/grapple.h"
#include "magic/spells.h"
#include <stdio.h>
#include <strings.h>

extern bool is_dragoon_mounted(P_char ch);
extern P_char get_dragoon_mount(P_char ch);
extern bool has_dragoon_mount(P_char ch);
extern void do_point(P_char ch, P_char victim);

/* ---- DRAGOON SPELLS ---- */

void spell_draconic_apotheosis(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			       P_char /*victim*/, P_obj /*obj*/)
{
	if (!ch)
		return;
	if (!IS_ALIVE(ch))
		return;

	if (!IS_DRAGOON(ch))
	{
		send_to_char("Your &+rsoul&n can't take anymore!\n", ch);
		return;
	}

	if (!is_dragoon_mounted(ch) || !has_dragoon_mount(ch))
	{
		send_to_char(
			"Your &+rsoul&n aches as you feel untethered from the &+Gdr&+Lag&+Gon&n god's &+rpower&n!\n",
			ch);
		return;
	}

	P_char mount = get_dragoon_mount(ch);

	if (GET_OPPONENT(ch) || GET_OPPONENT(mount))
	{
		send_to_char(
			"The heat of battle prevents you from hearing the &+Gdr&+Lag&+Gon&n god's roar.\n",
			ch);
		return;
	}

	if (affected_by_spell(ch, SPELL_DRACONIC_APOTHEOSIS))
	{
		send_to_char("You couldn't possibly consume more &+rpower&n!\n", ch);
		return;
	}

	wizlog(MINLVLIMMORTAL, "%s cast draconic apotheosis in room %d.", GET_NAME(ch),
	       ch->in_room);

	struct affected_type af;

	// remove the dragon from the room and set the player to not riding
	unlink_char(ch, mount, LNK_DRAGOON_MOUNT);
	unlink_char(ch, mount, LNK_RIDING);
	unlink_char(ch, mount, LNK_PET);
	unlink_char(ch, mount, LNK_CONSENT);

	char_from_room(mount);
	char_to_room(mount, 4, -1); // send to tyrus's bone temple

	bzero(&af, sizeof(af));
	af.type = SPELL_DRACONIC_APOTHEOSIS;
	af.duration = 10;
	af.location = APPLY_NONE;
	af.flags = AFFTYPE_NODISPEL | AFFTYPE_NOSAVE;
	af.modifier = GET_RACE(ch);
	af.bitvector3 = AFF3_ENLARGE;
	affect_to_char(ch, &af);

	GET_RACE(ch) = RACE_DRAGON;
	SET_POS(ch, POS_STANDING + GET_STAT(ch));

	act("$n's rips away $s &+rflesh&n as $e becomes one with &n$N&n!", FALSE, ch, 0, mount,
	    TO_ROOM);
	send_to_char(
		"You rip away your mortal &+rflesh&n and become one with the &+Gdr&+Lag&+Gon&n god's &-L&+RPOWER&N!\n",
		ch);

	// maybe kill the mount here once it's fully unlinked
	extract_char(mount);

	// Take a little damage for your benefits
	spell_damage(ch, ch, GET_HIT(ch) * 0.20f, SPLDAM_SPIRIT, SPLDAM_GRSPIRIT | SPLDAM_NOSHRUG,
		     NULL);

	do_roar_of_heroes(ch);
}

void spell_animae_cicatrix(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			   P_char /*victim*/, P_obj /*obj*/)
{
	struct affected_type af;

	if (!ch || !IS_ALIVE(ch))
	{
		return;
	}

	if (affected_by_spell(ch, SPELL_ANIMAE_CICATRIX))
	{
		send_to_char(
			"Your soul is already scarred by the &+Gdr&+Lag&+Gon&n god's &+rember&n-&+Ldark&n magic!\n",
			ch);
		return;
	}
	act("The &+Gdr&+Lag&+Gon&n god's &+rpower&n scars your soul, leaving you torn!", FALSE, ch,
	    0, 0, TO_CHAR);

	bzero(&af, sizeof(af));
	af.type = SPELL_ANIMAE_CICATRIX;
	af.duration = (int)(5 + GET_CHAR_SKILL(ch, SKILL_SPELL_KNOWLEDGE_SHAMAN) / 5);
	af.modifier = 10;
	af.location = APPLY_POW;
	affect_to_char(ch, &af);
	af.location = APPLY_FIRE_PROT;
	affect_to_char(ch, &af);
	af.modifier = 10;
	af.location = APPLY_HITROLL;
	affect_to_char(ch, &af);
	af.modifier = 10;
	af.location = APPLY_DAMROLL;
	affect_to_char(ch, &af);
	af.modifier = 10;
	af.location = APPLY_WIS;
	affect_to_char(ch, &af);
	af.modifier = 10;
	af.location = APPLY_AGI;
	affect_to_char(ch, &af);
	af.modifier = 10;
	af.location = APPLY_STR;
	affect_to_char(ch, &af);
	af.modifier = 5;
	af.location = APPLY_POW_MAX;
	affect_to_char(ch, &af);

	if (GET_SPEC(ch, CLASS_DRAGOON, SPEC_DRAGON_PRIEST))
	{
		af.modifier = 5;
		af.location = APPLY_WIS_MAX;
		affect_to_char(ch, &af);
	}

	if (GET_SPEC(ch, CLASS_DRAGOON, SPEC_DRAGON_HUNTER))
	{
		af.modifier = 5;
		af.location = APPLY_AGI_MAX;
		affect_to_char(ch, &af);
	}

	if (GET_SPEC(ch, CLASS_DRAGOON, SPEC_DRAGON_LANCER))
	{
		af.modifier = 5;
		af.location = APPLY_STR_MAX;
		affect_to_char(ch, &af);
	}

	// Take a little damage for your benefits
	spell_damage(ch, ch, GET_HIT(ch) * 0.10f, SPLDAM_SPIRIT, SPLDAM_GRSPIRIT | SPLDAM_NOSHRUG,
		     NULL);
}

void spell_stigmata_draconica(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			      P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	int duration_i = (int)(15 + GET_CHAR_SKILL(ch, SKILL_SPELL_KNOWLEDGE_SHAMAN) / 5);

	if (!ch || !IS_ALIVE(ch))
	{
		return;
	}

	if (affected_by_spell(ch, SPELL_STIGMATA_DRACONICA))
	{
		act("Your hands are still bloodied from the &+rstigmata&n", TRUE, ch, 0, victim,
		    TO_CHAR);
		return;
	}

	bzero(&af, sizeof(af));
	af.type = SPELL_STIGMATA_DRACONICA;
	af.location = APPLY_NONE;
	af.modifier = level;
	af.duration = BOUNDED(2, duration_i, 10);
	affect_to_char(ch, &af);

	act("Open &+rsores&n erupt on the plams of $n's hands..", FALSE, ch, 0, 0, TO_ROOM);
	act("Open &+rsores&n erput on the palms of your hands..", FALSE, ch, 0, 0, TO_CHAR);

	spell_damage(ch, ch, GET_HIT(ch) * 0.10f, SPLDAM_SPIRIT, SPLDAM_GRSPIRIT | SPLDAM_NOSHRUG,
		     NULL);
}

void spell_judicium_fidei(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			  P_char /*victim*/, P_obj /*obj*/)
{
	struct affected_type af;

	if (!ch || !IS_ALIVE(ch))
	{
		return;
	}

	if (!IS_AFFECTED5(ch, AFF5_JUDICIUM_FIDEI))
	{
		act("$n's flesh &+rsears&n as the &+Gdr&+Lag&+Gon&n god's power eminates around $m!",
		    TRUE, ch, 0, 0, TO_ROOM);
		act("Your flesh &+rsears&n as the &+Gdr&+Lag&+Gon&n god's power eminates around you!",
		    TRUE, ch, 0, 0, TO_CHAR);

		bzero(&af, sizeof(af));
		af.type = SPELL_JUDICIUM_FIDEI;
		af.duration = (int)(5 + GET_CHAR_SKILL(ch, SKILL_SPELL_KNOWLEDGE_SHAMAN) / 5);
		af.bitvector5 = AFF5_JUDICIUM_FIDEI;
		affect_to_char(ch, &af);

		// Take a little damage for your benefits
		spell_damage(ch, ch, GET_HIT(ch) * 0.10f, SPLDAM_SPIRIT,
			     SPLDAM_GRSPIRIT | SPLDAM_NOSHRUG, NULL);
	}
}

void spell_igneus_vitae(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
			P_obj obj)
{
	int healpoints;

	if (!IS_ALIVE(ch))
		return;

	if (!is_dragoon_mounted(ch))
	{
		act("$n's ophidian eyes go dull as $s inner &+Weyelids&n slowly close for a moment.",
		    TRUE, ch, 0, 0, TO_ROOM);
		act("Your soul feels hollow as you ache for the &+Gdr&+Lag&+Gon&n god's &+rpower&n...",
		    TRUE, ch, 0, 0, TO_CHAR);
		return;
	}

	P_char mount = get_dragoon_mount(ch);

	if (mount == NULL)
		return;
	if (!IS_ALIVE(mount))
		return;

	spell_cure_blind(level, ch, NULL, SPELL_TYPE_SPELL, mount, obj);

	grapple_heal(mount);

	healpoints = number(150, (GET_LEVEL(ch) * 5));

	heal(mount, ch, healpoints, GET_MAX_HIT(mount));

	if (healpoints)
	{
		act("$n's ophidian eyes glow for a moment as $e infuses $N with &+rpower&n.", FALSE,
		    ch, 0, mount, TO_NOTVICT);
		act("Your ophidian eyes glow as you infuse $N with &+rpower&n.", TRUE, ch, 0, mount,
		    TO_CHAR);
		act("$n roars as the as $e is infused with the &+Gdr&+Lag&+Lon&n god's &+rpower&n!",
		    FALSE, mount, 0, 0, TO_ROOM);
	}

	update_pos(mount);

	spell_damage(ch, ch, GET_HIT(ch) * 0.10f, SPLDAM_SPIRIT, SPLDAM_GRSPIRIT | SPLDAM_NOSHRUG,
		     NULL);
}

void spell_sanguinis_ignis(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			   P_char /*victim*/, P_obj /*obj*/)
{
	struct affected_type af;
	char Gbuf1[100];
	int skl_lvl;

	if (!IS_ALIVE(ch))
		return;

	if (affected_by_spell(ch, SPELL_SANGUINIS_IGNIS))
	{
		send_to_char("Your &+rblood&n is already boiling with power.\n", ch);
		return;
	}

	skl_lvl = MAX(4, (level / 5));

	snprintf(Gbuf1, 100, "Your blood burns with &+Gdr&+Lag&+Gon&n god's &+rpower&n.\n");

	bzero(&af, sizeof(af));
	af.type = SPELL_SANGUINIS_IGNIS;
	af.duration = skl_lvl;
	send_to_char(Gbuf1, ch);
	affect_to_char(ch, &af);

	// Take a little damage for your benefits
	spell_damage(ch, ch, GET_HIT(ch) * 0.10f, SPLDAM_SPIRIT, SPLDAM_GRSPIRIT | SPLDAM_NOSHRUG,
		     NULL);
}

void spell_sanctum_draconis(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			    P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	char Gbuf1[100];
	int skl_lvl;

	if (!IS_ALIVE(ch))
	{
		return;
	}

	if (affected_by_spell(ch, SPELL_SANCTUM_DRACONIS))
	{
		act("You are already protected by the &+Gdr&+Lag&+Gon&n &+Lgod&n's &+rpower&n!",
		    FALSE, ch, 0, ch, TO_CHAR);
		return;
	}

	snprintf(Gbuf1, 100, "Your bones burn with &+Gdr&+Lag&+Gon&n god's &+rpower&n.\n");

	skl_lvl = MAX(4, (level / 5));

	bzero(&af, sizeof(af));
	af.type = SPELL_SANCTUM_DRACONIS;
	af.location = APPLY_NONE;
	af.duration = skl_lvl;
	af.bitvector = AFF_SANCTUM_DRACONIS;
	send_to_char(Gbuf1, victim);
	affect_to_char(ch, &af);

	spell_damage(ch, ch, GET_HIT(ch) * 0.10f, SPLDAM_SPIRIT, SPLDAM_GRSPIRIT | SPLDAM_NOSHRUG,
		     NULL);
}

void spell_vivernae_concordia(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			      P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	struct affected_type *af1;

	if (!IS_ALIVE(ch))
	{
		return;
	}

	if (!affected_by_spell(ch, SPELL_VIVERNAE_CONCORDIA))
	{
		send_to_char(
			"Your nerves burn as your soul touches the &+Gdr&+Lag&+Gon&n god's &+rpower&n.\n",
			ch);
		act("$n's ophidian eyes narrow as they shriek in &+rpain&n!", TRUE, ch, 0, 0,
		    TO_ROOM);

		bzero(&af, sizeof(af));
		af.type = SPELL_VIVERNAE_CONCORDIA;
		af.duration = 10;
		af.bitvector3 = AFF3_VIVERNAE_CONCORDIA;
		affect_to_char(victim, &af);
	}
	else if (affected_by_spell(ch, SPELL_VIVERNAE_CONCORDIA))
	{
		for (af1 = ch->affected; af1; af1 = af1->next)
		{
			if (af1->type == SPELL_VIVERNAE_CONCORDIA)
			{
				send_to_char(
					"Your soul &+rrekindles&n its concord with the &+Gdr&+Lag&+Gon&n god.&n\n",
					ch);
				af1->duration = 10;
			}
		}
	}

	// Take a little damage for your benefits
	spell_damage(ch, ch, GET_HIT(ch) * 0.10f, SPLDAM_SPIRIT, SPLDAM_GRSPIRIT | SPLDAM_NOSHRUG,
		     NULL);
}

void spell_pactum_serpentis(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			    P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	char Gbuf1[100];
	int skl_lvl;

	if (!IS_ALIVE(ch))
		return;

	if (affected_by_spell(ch, SPELL_ACCEL_HEALING) || affected_by_spell(ch, SKILL_REGENERATE) ||
	    affected_by_spell(ch, SPELL_REGENERATION) ||
	    affected_by_spell(ch, SPELL_PACTUM_SERPENTIS) || IS_AFFECTED3(ch, AFF3_GR_SPIRIT_WARD))
	{
		send_to_char("Your &+rsoul&n can't take anymore!\n", ch);
		return;
	}

	skl_lvl = MAX(4, (level / 5));

	snprintf(Gbuf1, 100, "Your soul burns with &+Gdr&+Lag&+Gon&n god's &+rpower&n.\n");

	bzero(&af, sizeof(af));
	af.type = SPELL_PACTUM_SERPENTIS;
	af.duration = skl_lvl;
	af.bitvector3 = AFF3_GR_SPIRIT_WARD;
	af.bitvector4 = AFF4_REGENERATION;
	send_to_char(Gbuf1, victim);
	affect_to_char(victim, &af);

	// Take a little damage for your benefits
	spell_damage(ch, ch, GET_HIT(ch) * 0.10f, SPLDAM_SPIRIT, SPLDAM_GRSPIRIT | SPLDAM_NOSHRUG,
		     NULL);
}

void spell_ritus_draconum(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
			  P_obj /*obj*/)
{
	send_to_char("You cast Pyroclastar's ritus draconum. -- not finished", ch);
}

void spell_edictum_cineris(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			   P_char victim, P_obj obj)
{
	struct affected_type af;
	int save = 2;

	if (!(victim && ch))
	{
		return;
	}

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	appear(ch);
	do_point(ch, victim);

	if (resists_spell(ch, victim))
	{
		act("The &+Gdr&+Lag&+Gon&n god holds no &+rpower&n over $n&n.", FALSE, victim, 0, 0,
		    TO_ROOM);
		send_to_char("The &+Gdr&+Lag&+Gon&n god holds no &+rpower&n over you.\r\n", victim);
		return;
	}

	if (affected_by_spell(victim, SPELL_FEAR))
	{
		act("The &+Gdr&+Lag&+Gon&n god holds no &+rpower&n over $n&n.", FALSE, victim, 0, 0,
		    TO_ROOM);
		send_to_char("The &+Gdr&+Lag&+Gon&n god holds no &+rpower&n over you.\r\n", victim);
		return;
	}

	if (IS_ELITE(victim) || IS_UNDEAD(victim) || IS_ANGEL(victim) || IS_TRUSTED(victim))
	{
		act("The &+Gdr&+Lag&+Gon&n god holds no &+rpower&n over $n&n.", FALSE, victim, 0, 0,
		    TO_ROOM);
		send_to_char("The &+Gdr&+Lag&+Gon&n god holds no &+rpower&n over you.\r\n", victim);
		return;
	}

	// same save as beholder fear
	save = (int)(GET_LEVEL(ch) / 10);

	if (!NewSaves(ch, SAVING_FEAR, save))
	{
		bzero(&af, sizeof(af));

		if (affected_by_spell(victim, SKILL_BERSERK))
		{
			act("You feel a strange sensation overcome you...", TRUE, ch, obj, victim,
			    TO_VICT);

			CharWait(victim, PULSE_VIOLENCE);
			affect_from_char(victim, SKILL_BERSERK);

			send_to_char(
				"Your blood cools, and you no longer see targets everywhere.\r\n",
				victim);
			act("$n seems to have overcome $s battle madness.", TRUE, victim, 0, 0,
			    TO_ROOM);
			return;
		}

		if (fear_check(victim, true))
		{
			act("The &+Gdr&+Lag&+Gon&n god holds no &+rpower&n over $n&n", FALSE,
			    victim, 0, 0, TO_ROOM);
			return;
		}

		af.type = SPELL_FEAR;
		af.location = APPLY_DAMROLL;
		af.modifier = -2 - (level / 10);
		if (-af.modifier > GET_DAMROLL(victim))
			af.modifier = -GET_DAMROLL(victim);
		af.duration = 3 + (level / 10);
		affect_to_char(victim, &af);

		af.location = APPLY_HITROLL;
		af.modifier = -2 - (level / 10);
		if (-af.modifier > GET_HITROLL(victim))
			af.modifier = -GET_HITROLL(victim);
		affect_to_char(victim, &af);

		send_to_char("The &+Gdr&+Lag&+Gon&n god's &+Lterror&n overcomes you!\r\n", victim);
		do_flee(victim, 0, 2);
	}
	else
	{
		act("The &+Gdr&+Lag&+Gon&n god holds no &+rpower&n over $n&n", FALSE, victim, 0, 0,
		    TO_ROOM);
		send_to_char("The &+Gdr&+Lag&+Gon&n god holds no &+rpower&n over you.\r\n", victim);
	}

	if (ch->in_room == victim->in_room)
	{
		/*
		 * they didn't flee, know what happens when you corner a scared
		 * rat?
		 */
		if (IS_NPC(victim) && CAN_SEE(victim, ch))
		{
			remember(victim, ch);
			if (!IS_FIGHTING(victim))
			{
				MobStartFight(victim, ch);
			}
		}
	}
}

void spell_sigillum_negati(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			   P_char /*victim*/, P_obj /*obj*/)
{
	struct affected_type af;

	if (!IS_ALIVE(ch))
		return;

	bzero(&af, sizeof(af));
	af.type = SPELL_SIGILLUM_NEGATI;
	af.duration = (int)(5 + GET_CHAR_SKILL(ch, SKILL_SPELL_KNOWLEDGE_SHAMAN) / 5);
	af.bitvector4 = AFF4_NOFEAR;

	if (!affected_by_spell(ch, SPELL_SIGILLUM_NEGATI) && !IS_AFFECTED4(ch, AFF4_NOFEAR))
	{
		send_to_char(
			"A wave of &+rpower&n washes over you as a &+Lnegation&n &+ysigil&n is burned into your &+rflesh&n!\n",
			ch);
		affect_to_char(ch, &af);
	}

	if (ch->group)
	{
		for (struct group_list *gl = ch->group; gl; gl = gl->next)
		{
			if (ch != gl->ch && gl->ch->in_room == ch->in_room)
			{
				if (IS_ALIVE(gl->ch))
				{
					if (!affected_by_spell(gl->ch, SPELL_SIGILLUM_NEGATI) &&
					    !IS_AFFECTED4(gl->ch, AFF4_NOFEAR))
					{
						send_to_char(
							"A wave of &+rpower&n washes over you as a &+Lnegation&n &+ysigil&n is burned into your &+rflesh&n!\n",
							gl->ch);
						affect_to_char(gl->ch, &af);
					}
				}
			}
		}
	}
}
