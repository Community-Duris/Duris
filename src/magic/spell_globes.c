#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utils.h"
#include "core/defines.h"
#include "magic/spells.h"
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

	if (IS_AFFECTED2(victim, AFF2_GLOBE))
		return;

	if (!affected_by_spell(victim, SPELL_GLOBE))
	{
		act("&+R$n &+Rbegins to shimmer.", TRUE, victim, 0, 0, TO_ROOM);
		act("&+RYou begin to shimmer.", TRUE, victim, 0, 0, TO_CHAR);

		bzero(&af, sizeof(af));
		af.type = SPELL_GLOBE;
		if (GET_CLASS(ch, CLASS_CONJURER))
			af.duration = 15;
		else if (GET_CLASS(ch, CLASS_SUMMONER))
			af.duration = 12;
		else
			af.duration = 8;
		af.bitvector2 = AFF2_GLOBE;
		affect_to_char(victim, &af);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_GLOBE)
			{
				af1->duration = 8;
			}
	}
}

void spell_minor_globe(int /*level*/, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
		       P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (IS_AFFECTED(victim, AFF_MINOR_GLOBE))
		return;

	if (!affected_by_spell(victim, SPELL_MINOR_GLOBE))
	{
		act("&+r$n &+rbegins to shimmer.", TRUE, victim, 0, 0, TO_ROOM);
		act("&+rYou begin to shimmer.", TRUE, victim, 0, 0, TO_CHAR);

		bzero(&af, sizeof(af));
		af.type = SPELL_MINOR_GLOBE;
		af.duration = 6;
		af.bitvector = AFF_MINOR_GLOBE;
		affect_to_char(victim, &af);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_MINOR_GLOBE)
			{
				af1->duration = 6;
			}
	}
}
