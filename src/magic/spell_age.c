#include "core/prototypes.h"
#include "core/structs.h"
#include "world/db.h"
#include "net/comm.h"
#include "core/utils.h"
#include "core/defines.h"
#include "magic/spells.h"
#include <stdio.h>
#include <strings.h>
#include <time.h>

void AgeChar(P_char ch, int years)
{
	long played, secs;
	time_t curr_time = time(NULL);

	if (IS_NPC(ch))
		return;

	secs = years * SECS_PER_MUD_YEAR;
	played = curr_time - ch->player.time.birth;
	ch->player.time.birth = curr_time - (played + secs);
	if (ch->player.time.birth > curr_time)
		ch->player.time.birth = curr_time;
}

void spell_age(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim, P_obj /*obj*/)
{
	char Gbuf1[MAX_STRING_LENGTH];

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
		return;

	if (IS_NPC(victim))
		return;

	if ((IS_PC(ch) || IS_PC_PET(ch)) && IS_PC(victim))
	{
		if (!is_linked_to(ch, victim, LNK_CONSENT) && (ch != victim) && !IS_TRUSTED(ch))
		{
			snprintf(Gbuf1, MAX_STRING_LENGTH, "%s has not given %s consent to you.\n",
				 GET_NAME(victim), HSHR(victim));
			send_to_char(Gbuf1, ch);
			return;
		}
	}

	AgeChar(victim, dice(2, 8));
	send_to_char("You feel a bit older all of a sudden!\n", victim);
}

void spell_rejuvenate_major(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			    P_obj /*tar_obj*/)
{
	char Gbuf1[MAX_STRING_LENGTH];

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
		return;

	if (IS_NPC(victim))
		return;

	if (GET_RACE(victim) == RACE_ILLITHID)
		return;

	if (!is_linked_to(ch, victim, LNK_CONSENT) && (ch != victim) && !IS_TRUSTED(ch))
	{
		snprintf(Gbuf1, MAX_STRING_LENGTH, "%s has not given %s consent to you.\n",
			 GET_NAME(victim), HSHR(victim));
		send_to_char(Gbuf1, ch);
		return;
	}
	AgeChar(victim, -(dice(2, 4)));
	send_to_char("You feel a bit younger all of a sudden!\n", victim);
}

void spell_rejuvenate_minor(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			    P_char victim, P_obj /*tar_obj*/)
{
	struct affected_type af;
	int t_age;

	act("You feel younger.", FALSE, victim, 0, 0, TO_CHAR);
	t_age = dice(2, level) / 2;

	bzero(&af, sizeof(af));

	af.type = SPELL_REJUVENATE_MINOR;
	af.duration = t_age;
	af.modifier = -t_age;

	af.location = APPLY_AGE;

	affect_join(victim, &af, TRUE, FALSE);

	af.location = APPLY_MOVE_REG;
	af.modifier = BOUNDED(1, GET_LEVEL(ch) / 10, 5);
	affect_to_char(victim, &af);

	af.location = APPLY_HIT_REG;
	af.modifier = GET_LEVEL(ch) / 5;
	affect_to_char_with_messages(victim, &af, "You feel older again.",
				     "$n suddenly looks a little older than a moment before.");
}
