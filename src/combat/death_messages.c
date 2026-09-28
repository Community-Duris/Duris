/* Death cries and rattles sent to nearby rooms. */
#include "cmd/track.h"
#include "core/random.h"
#include "core/structs.h"
#include "core/utils.h"
#include "combat/death_messages.h"
#include "net/comm.h"
#include <stdio.h>

void death_cry(P_char ch)
{
	int door, was_in, room;
	char buf[MAX_INPUT_LENGTH];

	switch (number(1, 5))
	{
	case 1:
		act("&+rYou feel the bloodlust in your heart as you hear the death cry of&N $n.&n",
		    FALSE, ch, 0, 0, TO_ROOM);
		break;
	case 2:
		act("$n&N&+r's death cry reverberates in your head as $e falls to the ground.&n",
		    FALSE, ch, 0, 0, TO_ROOM);
		break;
	case 3:
		act("&+rThe last gasps of&n $n &n&+rcause a sickening chill to run up your spine.&n",
		    FALSE, ch, 0, 0, TO_ROOM);
		break;
	case 4:
		act("&+rThe unmistakable scent of fresh blood can be smelled as&N $n &N&+rdies in agony.&n",
		    FALSE, ch, 0, 0, TO_ROOM);
		break;
	case 5:
		act("&+rA look of horror and a silent scream are&n $n&N&+r's last actions in this world.&n",
		    FALSE, ch, 0, 0, TO_ROOM);
		break;
	}
	was_in = ch->in_room;

	add_track(ch, NUM_EXITS);

	if (was_in != NOWHERE)
		for (door = 0; door <= (NUM_EXITS - 1); door++)
		{
			if (VIRTUAL_CAN_GO(was_in, door))
			{
				room = world[ch->in_room].dir_option[door]->to_room;
				switch (number(1, 3))
				{
				case 1:
					snprintf(
						buf, MAX_INPUT_LENGTH,
						"&+rThe unmistakable sound of something dying reverberates from nearby.\r\n");
					break;
				case 2:
					snprintf(
						buf, MAX_INPUT_LENGTH,
						"&+rYour spine tingles as a rattling death cry reaches your senses from nearby.\r\n");
					break;
				case 3:
					snprintf(
						buf, MAX_INPUT_LENGTH,
						"&+rA nearby death cry rings out loudly, heightening your bloodlust.\r\n");
					break;
				}
				send_to_room(buf, room);
			}
		}
}

void death_rattle(P_char ch)
{
	int door, was_in, room;
	char buf[MAX_INPUT_LENGTH];

	act("&+rYou feel a carnal satisfaction as $n&+r's gurgling and choking signals $s demise.&n",
	    FALSE, ch, 0, 0, TO_ROOM);
	was_in = ch->in_room;

	add_track(ch, NUM_EXITS);

	if (was_in != NOWHERE)
		for (door = 0; door <= (NUM_EXITS - 1); door++)
		{
			if (VIRTUAL_CAN_GO(was_in, door))
			{
				room = world[ch->in_room].dir_option[door]->to_room;
				snprintf(buf, MAX_INPUT_LENGTH,
					 "&+rYou hear a shrill death rattle nearby!\r\n");
				send_to_room(buf, room);
			}
		}
}
