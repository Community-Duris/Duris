#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "magic/spells.h"

// Return true for next missile if available.
// Return false to stop barrage.
static bool spell_solbeeps_single_missile(int /*level*/, P_char ch, char * /*arg*/, int /*type*/,
					  P_char victim, P_obj /*tar_obj*/)
{
	struct damage_messages fulldam_messages = {
		"A &+Yhuge&n missile of &+Wforce&n departs from your fingertips, making a loud &+Lthud&n as it hits $N.",
		"$n's &+Yhuge&n missile of &+Wforce&n impacts on your chest with a loud &+Lthud&n, causing you to reel in pain.",
		"A &+Yhuge&n bolt of &+Wforce&n sent by $n impacts on $N's chest with a loud &+Lthud&n.",
		"Your &+Wforce&n missile slams into $N, leaving nothing but a bloody mess.",
		"$n's grin is the last thing you see before their &+Wforce&n missile bashes you into a pulpy mess.",
		"$n grins as their &+Wforce&n missile bashes $N into a pulpy mess.",
		0
	};

	struct damage_messages halfdam_messages = {
		"Your missile of &+Wforce&n hits $N soundly.",
		"$n's &+Wforce&n missile slams into you.",
		"A bolt of &+Wforce&n sent by $n hits $N soundly.",
		"Your &+Wforce&n missile slams into $N, leaving nothing but a bloody mess.",
		"$n's grin is the last thing you see before their &+Wforce&n missile bashes you into a pulpy mess.",
		"$n grins as their &+Wforce&n missile bashes $N into a pulpy mess.",
		0
	};

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
		return FALSE;

	if (resists_spell(ch, victim))
		return TRUE;

	int dam = dice(
		30,
		11); // average ~45 per missile, so from 135+ at 51 to 180 at 55 plus 25% chance of 225 at 56
	// made damage level-independent, since average number of missiles grows with level

	bool saved = true;
	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_MISSILE_BARRAGE);
	if (!NewSaves(victim, SAVING_SPELL, mod))
	{
		dam = (int)(dam * 1.5);
		saved = FALSE;
	}

	if (spell_damage(ch, victim, dam, SPLDAM_GENERIC, SPLDAM_NOSHRUG,
			 saved ? &halfdam_messages : &fulldam_messages) == DAM_NONEDEAD)
	{
		/*
		 if(!saved && GET_SIZE(victim) < SIZE_HUGE &&
		   !StatSave(victim, APPLY_AGI, -2 * (SIZE_LARGE - GET_SIZE(victim))) &&
		   !IS_AFFECTED4(victim, AFF4_DEFLECT))
		 {
		   act("$N goes flying and crashes into the wall!", FALSE, ch, 0,
		       victim, TO_CHAR);
		   act("You are sent flying and crash into the wall!", FALSE, ch, 0,
		       victim, TO_VICT);
		   act("$N goes flying and crashes into the wall!", FALSE, ch, 0,
		       victim, TO_NOTVICT);
		   SET_POS(victim, POS_PRONE + GET_STAT(victim));

		   stop_fighting(victim);
		   CharWait(victim, PULSE_VIOLENCE * 1);


		     int door = number(0, 9);

		     if((CAN_GO(victim, door)) && (!check_wall(victim->in_room, door)))
		     {
		       act("$N goes flying out of the room!", FALSE, ch, 0,
		         victim, TO_CHAR);
		       act("You go flying out of the room!", FALSE, ch, 0,
		         victim, TO_VICT);
		       act("$N goes flying out of the room!", FALSE, ch, 0,
		         victim, TO_NOTVICT);
		       int target_room = world[victim->in_room].dir_option[door]->to_room;
		       char_from_room(victim);
		       if(char_to_room(victim, target_room, -1))
		       {
		         act("$n flies in, crashing on the floor!", TRUE, victim, 0, 0,
		           TO_ROOM);
		         SET_POS(victim, POS_PRONE + GET_STAT(victim));
		         update_pos(victim);
		         stop_fighting(victim);
		         CharWait(victim, PULSE_VIOLENCE * 1);
		       }
		       return FALSE;
		     }
		 }
	   */
		return TRUE;
	}
	return FALSE;
}

void spell_solbeeps_missile_barrage(int level, P_char ch, char *arg, int type, P_char victim,
				    P_obj tar_obj)
{
	if (!IS_ALIVE(ch))
		return;

	int num_missiles = 3, i = 0;

	if (level >= 56 || !number(0, 55 - level))
		num_missiles++;

	/*
	   if (level >= 56 && !number(0, 3))
	   num_missiles++;
	*/
	while (i < num_missiles && IS_ALIVE(victim) && IS_ALIVE(ch) && victim)
	{
		if (!spell_solbeeps_single_missile(level, ch, arg, type, victim, tar_obj))
			break;

		i++;
	}
}
