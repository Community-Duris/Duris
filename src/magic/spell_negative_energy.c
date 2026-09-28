#include "core/prototypes.h"
#include "combat/defense_resolution.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "world/db.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include <strings.h>
void spell_invoke_negative_energy(int /*level*/, P_char ch, char * /*arg*/, int /*type*/,
				  P_char victim, P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"&+LYou unleash the negative material powers on $N.",
		"&+L$n&+L unleashes the negative material powers on you.",
		"&+L$n&+L unleash the negative material powers on $N.",
		"&+L$N&+L howls in pain as $S essence is unmade!",
		"&+LYou howl as $n's&+L spell unmakes you!",
		"&+L$N&+L howls in pain as $n's&+L unmakes $m!"
	};

	dam = dice(5, 10) + GET_LEVEL(ch);
	spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, SPLDAM_GLOBE, &messages);
}

void spell_channel_negative_energy(int /*level*/, P_char ch, char * /*arg*/, int /*type*/,
				   P_char victim, P_obj /*obj*/)
{
	struct damage_messages messages = {
		"&+rYou unleash a torrent of &n&+braw negative energy&+r on $N&n&+r, who begins to &+Bdissolve!",
		"&+r$n&n&+r unleashes a torrent of &n&+braw negative energy&+r on you, you begin to &+Bdissolve!",
		"&+r$n&n&+r unleashes a torrent of &n&+braw negative energy&+r on $N&n&+r, who begins to &+Bdissolve!",
		"&+L$N's&+L body completely &+REXPLODES&+L upon contact with too much &n&+bnegative energy!",
		"&+LYour body completely &+REXPLODES&+L upon contact with too much &n&+bnegative energy!",
		"&+L$N's&+L body completely &+REXPLODES&+L upon contact with too much &n&+bnegative energy!"
	};
	int dam;

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_CHANNEL_NEG_ENERGY);
	if (NewSaves(victim, SAVING_SPELL, mod))
		dam = 150;
	else
		dam = 300;

	spell_damage(ch, victim, dam, SPLDAM_NEGATIVE, RAWDAM_NOKILL, &messages);
}

void spell_negative_energy_vortex(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
				  P_obj /*obj*/)
{
	LOOP_THRU_PEOPLE(victim, ch)
	{
		if (ch->specials.z_cord == victim->specials.z_cord)
			spell_heal(GET_CLASS(ch, CLASS_WARLOCK) ? level : -1, ch, 0, 0, victim, 0);
	}
}

void spell_entropy_storm(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
			 P_obj /*obj*/)
{
	int healpoints;
	P_char tch, next;

	act("&+LAs you open a rift to the &n&+bnegative material plane&+L, black vapors drift through, engulfing all nearby!",
	    FALSE, ch, 0, 0, TO_CHAR);
	act("&+LAs $n&+L opens a rift to the &n&+bnegative material plane&+L, black vapors drift through, engulfing all nearby!",
	    FALSE, ch, 0, 0, TO_ROOM);

	for (tch = world[ch->in_room].people; tch != NULL; tch = next)
	{
		next = tch->next_in_room;

		if (tch == ch)
			continue;

		if (IS_NPC(ch) && IS_NPC(tch))
			continue;

		if (IS_UNDEADRACE(tch))
		{
			healpoints = 70;
			heal(tch, ch, healpoints, GET_MAX_HIT(tch));
			update_pos(tch);
			send_to_char(
				"&+LYou feel the black vapors infusing you with negative energy!\n",
				tch);
		}
		else if (!(IS_DRAGON(tch) || (GET_RACE(tch) == RACE_GOLEM) || IS_TRUSTED(tch)) &&
			 !NewSaves(tch, SAVING_FEAR, 5))
		{
			if (fear_check(tch))
			{
				continue;
			}
			else
			{
				send_to_char("&+LArgh, those vapors are reaching right for you!",
					     tch);
				do_flee(tch, 0, 1);
			}
		}
	}
}

void spell_vitalize_undead(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			   P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	int healpoints = 2 * level;

	// Modied 02/05/15 - was returning false positives.
	if (!IS_UNDEADRACE(victim) && !IS_ANGEL(victim) && GET_RACE(victim) != RACE_GOLEM &&
	    !GET_CLASS(victim, CLASS_NECROMANCER))
	{
		send_to_char("Nothing seems to happen.\n", ch);
		return;
	}
	if (IS_PC(victim))
		healpoints = healpoints;

	if (!affected_by_spell(victim, SPELL_VITALIZE_UNDEAD))
	{
		if (GET_CLASS(ch, CLASS_THEURGIST))
		{
			act("The &+Wholy powers&n of &+WH&+Yei&+Wr&+Yo&+Rn&+Yiou&+Rs&n strengthens $n's physical being.",
			    FALSE, victim, 0, 0, TO_ROOM);
			send_to_char("&+WYou feel your spirit gain strength.\n", victim);
		}
		else
		{
			act("&+LDarkness seems to encompass $n&+L, and begins to permeate $s rotting flesh!&n",
			    FALSE, victim, 0, 0, TO_ROOM);
			send_to_char("&+WYou feel your bones gain strength.\n", victim);
		}

		bzero(&af, sizeof(af));
		af.type = SPELL_VITALIZE_UNDEAD;
		af.duration = KludgeDuration(ch, 25, 10);
		af.modifier = healpoints;
		af.location = APPLY_HIT;

		affect_to_char(victim, &af);

		update_pos(victim);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_VITALIZE_UNDEAD)
			{
				af1->duration = KludgeDuration(ch, 25, 10);
			}
	}
}
