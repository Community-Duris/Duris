#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utils.h"
#include "core/defines.h"
#include "magic/spells.h"
#include "combat/spell_wards.h"
#include <strings.h>

void spell_group_globe(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		       P_obj /*obj*/)
{
	struct group_list *gl;

	if (ch && ch->group)
	{
		gl = ch->group;
		/* leader first */
		if (gl->ch->in_room == ch->in_room)
			spell_globe((level / 3) * 2, ch, 0, 0, gl->ch, 0);
		/* followers */
		for (gl = gl->next; gl; gl = gl->next)
		{
			if (gl->ch->in_room == ch->in_room)
				spell_globe((level / 3) * 2, ch, 0, 0, gl->ch, 0);
		}
	}
}

void spell_globe(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		 P_obj /*obj*/)
{
	struct affected_type af;
	/* NPC protection bits are native state, not finite player equipment. */
	if (IS_NPC(victim) && IS_AFFECTED2(victim, AFF2_GLOBE))
		return;
	const bool already_active = affected_by_spell(victim, SPELL_GLOBE);
	int duration;

	if (GET_CLASS(ch, CLASS_CONJURER))
		duration = 15;
	else if (GET_CLASS(ch, CLASS_SUMMONER))
		duration = 12;
	else
		duration = 8;

	if (!already_active)
	{
		act("&+R$n &+Rbegins to shimmer.", TRUE, victim, 0, 0, TO_ROOM);
		act("&+RYou begin to shimmer.", TRUE, victim, 0, 0, TO_CHAR);
	}

	bzero(&af, sizeof(af));
	af.type = SPELL_GLOBE;
	af.bitvector2 = AFF2_GLOBE;
	spell_ward_apply_cast(victim, &af, duration);
}

void spell_minor_globe(int /*level*/, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
		       P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	/* NPC protection bits are native state, not finite player equipment. */
	if (IS_NPC(victim) && IS_AFFECTED(victim, AFF_MINOR_GLOBE))
		return;
	const bool already_active = affected_by_spell(victim, SPELL_MINOR_GLOBE);

	if (!already_active)
	{
		act("&+r$n &+rbegins to shimmer.", TRUE, victim, 0, 0, TO_ROOM);
		act("&+rYou begin to shimmer.", TRUE, victim, 0, 0, TO_CHAR);
	}

	bzero(&af, sizeof(af));
	af.type = SPELL_MINOR_GLOBE;
	af.bitvector = AFF_MINOR_GLOBE;
	spell_ward_apply_cast(victim, &af, 6);
}
