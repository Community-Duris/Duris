/* Special procedures for Wildland Trails. */

#include <strings.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "world/db.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"

extern P_room world;

int barbarian_spiritist(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict;
	struct affected_type af;

	/*
	 * check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || cmd)
		return FALSE;

	switch (number(0, 5))
	{
	case 0:
		act("$n gestures wildly, calling upon the spirits of his long dead ancestors to protect him in his time of need!  Luckily for you, they don't seem to have answered him this time.",
		    TRUE, ch, 0, 0, TO_ROOM);
		return FALSE;
	case 1:
		act("Tossing a handful of strange smelling herbs into the air, $n calls upon the spirit forces of nature to protect him from the heathen outsiders!",
		    TRUE, ch, 0, 0, TO_ROOM);
		vict = char_in_room(ch->in_room);
		if (vict && CAN_SEE(ch, vict) && !IS_NPC(vict))
		{
			bzero(&af, sizeof(af));
			af.type = SPELL_CURSE;
			af.duration = 5;
			af.modifier = -3;
			af.location = APPLY_HITROLL;
			affect_to_char(vict, &af);
			af.modifier = -3;
			af.location = APPLY_DAMROLL;
			affect_to_char(vict, &af);
			send_to_char(
				"\r\n&+BThe summoned spirits&N interfere with your battle ability!.\r\n",
				vict);
		}
		return TRUE;
	case 2:
		act("Rubbing his bone totem in his hands, $n chants an ancient mantra of  summoning!  &+BA shimmering aura of pale blue&N suddenly appears and surrounds you!",
		    TRUE, ch, 0, 0, TO_ROOM);
		vict = char_in_room(ch->in_room);
		if (vict && CAN_SEE(ch, vict) && !IS_NPC(vict))
		{
			if (vict->equipment[PRIMARY_WEAPON] != NULL)
			{
				SET_BIT(vict->equipment[PRIMARY_WEAPON]->extra_flags, ITEM_SECRET);
				send_to_char(
					"\r\n&+BThe summoned spirits&N tug at your weapon, causing it to fly from your grip!\r\n",
					vict);
				obj_to_room(unequip_char(vict, PRIMARY_WEAPON), vict->in_room);
			}
			else if (vict->equipment[SECONDARY_WEAPON] != NULL)
			{
				SET_BIT(vict->equipment[SECONDARY_WEAPON]->extra_flags,
					ITEM_SECRET);
				send_to_char(
					"\r\n&+BThe summoned spirits&N tug at your weapon, causing it to fly from your grip!\r\n",
					vict);
				obj_to_room(unequip_char(vict, SECONDARY_WEAPON), vict->in_room);
			}
			act("\r\n&+BThe spirits&N swarm $N, causing $M to flail about blindly!\r\n",
			    FALSE, ch, 0, vict, TO_NOTVICT);
		}
		return TRUE;
	case 3:
		act("Waving his arms wildly, $n calls down the might of the great spirits to harm his enemies, those who have dared attack him and his village!\r\n",
		    TRUE, ch, 0, 0, TO_ROOM);
		vict = char_in_room(ch->in_room);
		if (vict && CAN_SEE(ch, vict) && !IS_NPC(vict))
		{
			spell_cyclone(50, ch, 0, SPELL_TYPE_SPELL, vict, 0);
			do_action(ch, 0, CMD_CACKLE);
		}
		return TRUE;
	default:
		return FALSE;
	}
}

int plant_attacks_poison(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	P_char vict, next_ch;
	int flag = FALSE;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || pl || cmd)
		return FALSE;

	/*  LOOP_THRU_PEOPLE(vict, ch)*/
	for (vict = world[ch->in_room].people; vict; vict = next_ch)
	{
		next_ch = vict->next_in_room;

		if ((vict != ch) && (IS_PC(vict) || IS_PC_PET(vict)) && !number(0, 2) &&
		    !saves_spell(vict, SAVING_PARA) && CAN_SEE(ch, vict))
		{
			act("$n launches a volley of barbed red thorns!", TRUE, ch, 0, 0, TO_ROOM);
			act("You fling thorns towards your victims", FALSE, ch, 0, 0, TO_CHAR);
			act("One of $n's thorns pierces your skin!", FALSE, ch, 0, vict, TO_VICT);
			act("$N is hit by one of the thorns!", TRUE, ch, 0, vict, TO_ROOM);
			poison_neurotoxin(GET_LEVEL(ch), ch, 0, 0, vict, 0);
			flag = TRUE;
		}
	}

	return flag;
}

int plant_attacks_blindness(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	struct affected_type af;
	P_char vict;
	int duration_factor, eyewear_value, flag = FALSE;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || pl || cmd)
		return FALSE;

	LOOP_THRU_PEOPLE(vict, ch)
		if ((vict != ch) && (IS_PC(vict) || IS_PC_PET(vict)) && !number(0, 2) &&
		    !IS_AFFECTED(vict, AFF_BLIND) && !saves_spell(vict, SAVING_BREATH) &&
		    CAN_SEE(ch, vict))
		{
			if (vict->equipment[WEAR_EYES])
				eyewear_value = (vict->equipment[WEAR_EYES]->value[0]);
			else
				eyewear_value = 0;

			duration_factor = (10 - eyewear_value + GET_LEVEL(ch));
			if (duration_factor < 1)
				duration_factor = 1;

			if (!flag)
			{
				act("The bright red flowers along the vines of $n puff out a dense cloud of pollen!",
				    TRUE, ch, 0, 0, TO_ROOM);
				act("You spray pollen towards the eyes of your victims", FALSE, ch,
				    0, 0, TO_CHAR);
			}
			act("$n's pollen makes your eyes burn and water!  You can't see!", FALSE,
			    ch, 0, vict, TO_VICT);
			act("$N staggers about blindly!", TRUE, ch, 0, vict, TO_NOTVICT);
			bzero(&af, sizeof(af));
			af.type = SPELL_BLINDNESS;
			af.duration = duration_factor;
			af.bitvector = AFF_BLIND;
			affect_join(vict, &af, FALSE, FALSE);
			flag = TRUE;
		}
	return flag;
}

int plant_attacks_paralysis(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	P_char vict;
	int duration_factor, flag = FALSE;
	struct affected_type af;

	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || pl || cmd)
		return FALSE;

	if (cmd == CMD_DEATH) /*
	                       * unwrap vines upon death of vine
	                       */
		LOOP_THRU_PEOPLE(vict, ch)
			if ((vict != ch) && affected_by_spell(vict, SPELL_MAJOR_PARALYSIS))
				affect_from_char(vict, SPELL_MAJOR_PARALYSIS);

	duration_factor = 10;

	LOOP_THRU_PEOPLE(vict, ch)
		if ((vict != ch) && (IS_PC(vict) || IS_PC_PET(vict)) && !number(0, 2) &&
		    !IS_AFFECTED2(vict, AFF2_MAJOR_PARALYSIS) && !saves_spell(vict, SAVING_PARA) &&
		    !check_freedom_of_movement(vict, false))
		{
			act("$n reach up and wrap themselves about you, making it difficult to move, or even breathe!",
			    FALSE, ch, 0, vict, TO_VICT);
			act("$n reaches up and wraps about $N!", TRUE, ch, 0, vict, TO_NOTVICT);
			act("You reach up and wrap about $N!", TRUE, ch, 0, vict, TO_CHAR);
			bzero(&af, sizeof(af));
			af.type = SPELL_MAJOR_PARALYSIS;
			af.flags = AFFTYPE_SHORT;
			af.duration = duration_factor * WAIT_SEC;
			af.bitvector2 = AFF2_MAJOR_PARALYSIS;
			affect_join(vict, &af, FALSE, FALSE);
			flag = TRUE;
		}
	return flag;
}
