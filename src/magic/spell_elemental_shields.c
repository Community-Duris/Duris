#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utils.h"
#include "core/defines.h"
#include "magic/spells.h"
#include <strings.h>

extern P_room world;
extern const int top_of_world;

void spell_fireshield(int /*level*/, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
		      P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!IS_AFFECTED2(victim, AFF2_FIRESHIELD) && !IS_AFFECTED3(victim, AFF3_COLDSHIELD) &&
	    !IS_AFFECTED3(victim, AFF3_LIGHTNINGSHIELD))
	{
		act("&+R$n &+Ris surrounded by burning flames!", TRUE, victim, 0, 0, TO_ROOM);
		act("&+RYou are surrounded by an aura of burning flames!&n", TRUE, victim, 0, 0,
		    TO_CHAR);
		bzero(&af, sizeof(af));
		af.type = SPELL_FIRESHIELD;
		af.duration = 6;
		af.bitvector2 = AFF2_FIRESHIELD;
		affect_to_char(victim, &af);
	}
}

void spell_coldshield(int /*level*/, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
		      P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!IS_AFFECTED2(victim, AFF2_FIRESHIELD) && !IS_AFFECTED3(victim, AFF3_COLDSHIELD) &&
	    !IS_AFFECTED3(victim, AFF3_LIGHTNINGSHIELD))
	{
		act("&+B$n &+Bis surrounded by an aura of deadly cold!&n", TRUE, victim, 0, 0,
		    TO_ROOM);
		act("&+BYou are surrounded by freezing cold!&n", TRUE, victim, 0, 0, TO_CHAR);
		bzero(&af, sizeof(af));
		af.type = SPELL_COLDSHIELD;
		af.duration = 6;
		af.bitvector3 = AFF3_COLDSHIELD;
		affect_to_char(victim, &af);
	}
}

void spell_lightning_shield(int /*level*/, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
			    P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!IS_AFFECTED2(victim, AFF2_FIRESHIELD) && !IS_AFFECTED3(victim, AFF3_COLDSHIELD) &&
	    !IS_AFFECTED3(victim, AFF3_LIGHTNINGSHIELD))
	{
		act("&+Y$n &+Yis surrounded by a crackling maelstrom of energy!", TRUE, victim, 0,
		    0, TO_ROOM);
		act("&+YYou are surrounded by an crackling maelstrom of energy!&n", TRUE, victim, 0,
		    0, TO_CHAR);

		bzero(&af, sizeof(af));
		af.type = SPELL_LIGHTNINGSHIELD;
		af.duration = 6;
		af.bitvector3 = AFF3_LIGHTNINGSHIELD;
		affect_to_char(victim, &af);
	}
}

const char *elemental_aura_failure_message(P_char ch)
{
	if (!ch || ch->in_room < 0 || ch->in_room > top_of_world)
		return "There is no elemental planar essence here to draw upon.\n";

	if (affected_by_spell(ch, SPELL_ELEMENTAL_AURA) || IS_AFFECTED2(ch, AFF2_EARTH_AURA) ||
	    IS_AFFECTED2(ch, AFF2_WATER_AURA) || IS_AFFECTED2(ch, AFF2_FIRE_AURA) ||
	    IS_AFFECTED2(ch, AFF2_AIR_AURA) || IS_AFFECTED4(ch, AFF4_ICE_AURA))
	{
		return "An elemental aura already surrounds you.\n";
	}

	switch (world[ch->in_room].sector_type)
	{
	case SECT_FIREPLANE:
	case SECT_WATER_PLANE:
	case SECT_AIR_PLANE:
	case SECT_EARTH_PLANE:
		return NULL;
	default:
		return "There is no elemental planar essence here to draw upon.\n";
	}
}

void spell_elemental_aura(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			  P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	const char *failure;

	if (!ch)
		return;

	failure = elemental_aura_failure_message(ch);
	if (failure)
	{
		send_to_char(failure, ch);
		return;
	}

	bzero(&af, sizeof(af));
	af.type = SPELL_ELEMENTAL_AURA;
	af.duration = (SECS_PER_MUD_DAY / 60);

	/* We do not want it going off in a non-plane room
	  af.location = APPLY_AC;
	  affect_to_char(victim, &af);*/

	switch (world[ch->in_room].sector_type)
	{
	case SECT_FIREPLANE:
		act("&+rYour body glows with an aura of fire!", FALSE, ch, 0, 0, TO_CHAR);
		act("&+r$n's&+r body glows with an aura of fire!", FALSE, ch, 0, 0, TO_ROOM);
		af.location = APPLY_STR_MAX;
		af.modifier = 50;
		affect_to_char(victim, &af);

		af.location = APPLY_DEX_MAX;
		af.modifier = -20;
		affect_to_char(victim, &af);

		af.location = APPLY_AGI_MAX;
		af.modifier = -10;
		affect_to_char(victim, &af);

		af.location = APPLY_CON_MAX;
		af.modifier = 25;
		affect_to_char(victim, &af);

		if (NewSaves(ch, SAVING_FEAR, 4) || IS_NPC(ch))
		{
			act("&+RYour body bursts into flames as you achieve full form!", FALSE, ch,
			    0, 0, TO_CHAR);
			act("&+R$n's&+R body bursts into flames as $e achieves full form!", FALSE,
			    ch, 0, 0, TO_ROOM);
			af.location = 0;
			af.modifier = 0;
			af.bitvector2 = AFF2_FIRE_AURA;
			affect_to_char(victim, &af);

			af.bitvector2 = AFF2_FIRESHIELD;
			affect_to_char(victim, &af);
		}
		break;
	case SECT_WATER_PLANE:
		act("&+bYour body flows with an aura of water!", FALSE, ch, 0, 0, TO_CHAR);
		act("&+b$n's&+b body flows with an aura of water!", FALSE, ch, 0, 0, TO_ROOM);
		af.location = APPLY_STR_MAX;
		af.modifier = -20;
		affect_to_char(victim, &af);

		af.location = APPLY_DEX_MAX;
		af.modifier = 50;
		affect_to_char(victim, &af);

		af.location = APPLY_AGI_MAX;
		af.modifier = 25;
		affect_to_char(victim, &af);

		af.location = APPLY_CON_MAX;
		af.modifier = -10;
		affect_to_char(victim, &af);

		if (NewSaves(ch, SAVING_FEAR, 4) || IS_NPC(ch))
		{
			act("&+BYour body &+bliquifies&+B as you achieve full form!", FALSE, ch, 0,
			    0, TO_CHAR);
			act("&+B$n's&+B body &+bliquifies&+B as $e achieves full form!", FALSE, ch,
			    0, 0, TO_ROOM);
			af.location = 0;
			af.modifier = 0;
			af.bitvector2 = AFF2_WATER_AURA;
			affect_to_char(victim, &af);
		}
		break;
	case SECT_AIR_PLANE:
		act("&+cYour body fumes with an aura of air!", FALSE, ch, 0, 0, TO_CHAR);
		act("&+c$n's&+c body fumes with an aura of air!", FALSE, ch, 0, 0, TO_ROOM);
		af.location = APPLY_STR_MAX;
		af.modifier = -25;
		affect_to_char(victim, &af);

		af.location = APPLY_DEX_MAX;
		af.modifier = 25;
		affect_to_char(victim, &af);

		af.location = APPLY_AGI_MAX;
		af.modifier = 100;
		affect_to_char(victim, &af);

		af.location = APPLY_CON_MAX;
		af.modifier = -20;
		affect_to_char(victim, &af);

		if (NewSaves(ch, SAVING_FEAR, 4) || IS_NPC(ch))
		{
			act("&+CYour body vaporizes as you achieve full form!", FALSE, ch, 0, 0,
			    TO_CHAR);
			act("&+C$n's&+C body vaporizes as $e achieves full form!", FALSE, ch, 0, 0,
			    TO_ROOM);
			af.location = 0;
			af.modifier = 0;
			af.bitvector2 = AFF2_AIR_AURA;
			affect_to_char(victim, &af);
		}
		break;
	case SECT_EARTH_PLANE:
		act("&+yYour body hardens with an aura of earth!", FALSE, ch, 0, 0, TO_CHAR);
		act("&+y$n's&+y body hardens with an aura of earth!", FALSE, ch, 0, 0, TO_ROOM);
		af.location = APPLY_STR_MAX;
		af.modifier = 75;
		affect_to_char(victim, &af);

		af.location = APPLY_DEX_MAX;
		af.modifier = -25;
		affect_to_char(victim, &af);

		af.location = APPLY_AGI_MAX;
		af.modifier = -20;
		affect_to_char(victim, &af);

		af.location = APPLY_CON_MAX;
		af.modifier = 75;
		affect_to_char(victim, &af);

		if (NewSaves(ch, SAVING_FEAR, 4) || IS_NPC(ch))
		{
			act("&+yYour body turns to &+Lstone&+y as you achieve full form!", FALSE,
			    ch, 0, 0, TO_CHAR);
			act("&+y$n's&+y body turns to &+Lstone&+y as $e achieve full form!", FALSE,
			    ch, 0, 0, TO_ROOM);
			af.location = 0;
			af.modifier = 0;
			af.bitvector2 = AFF2_EARTH_AURA;
			affect_to_char(victim, &af);
		}
		break;

	default:
		send_to_char("There is no elemental planar essence here to draw upon.\n", ch);
	}
}
