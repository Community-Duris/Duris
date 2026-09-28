#include "core/prototypes.h"
#include "core/structs.h"
#include "core/defines.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/safe_format.h"
#include "cmd/interp.h"
#include "net/comm.h"
#include "net/gmcp.h"
#include "net/listen.h"
#include <stdio.h>
#include <string.h>

extern P_desc descriptor_list;
void do_ptell(P_char ch, char *arg, int /*cmd*/)
{
	struct descriptor_data *d;
	P_char vict = NULL;

	//  P_desc d;

	char name[MAX_INPUT_LENGTH], msg[MAX_STRING_LENGTH], Gbuf1[MAX_STRING_LENGTH];

	if (!(SanityCheck(ch, "do_ptell")))
	{
		logit(LOG_DEBUG, "do_ptell failed SanityCheck");
		return;
	}
	if (IS_NPC(ch))
	{
		send_to_char("You try. . . . but you fail miserably.\n", ch);
		return;
	}
	half_chop(arg, name, msg);

	if (!*name || !*msg)
	{
		send_to_char("To whom are you responding? And what are you telling them?\n", ch);
		return;
	}
	/*
	  if(!(vict = get_char(name))) {
	    send_to_char("Nobody by that name seems available.\n", ch);
	    return;
	  }
	*/
	for (d = descriptor_list; d; d = d->next)
	{
		if (!d->character || d->connected || !d->character->player.name)
			continue;
		if (!isname(d->character->player.name, name))
			continue;
		if (!CAN_SEE_Z_CORD(ch, d->character))
			continue;
		vict = d->character;
		break;
	}

	if (!d || !vict)
	{
		send_to_char("Nobody by that name seems available.\n", ch);
		return;
	}
	if (IS_TRUSTED(vict) && (GET_WIZINVIS(vict) >= MIN(62, GET_LEVEL(ch))))
	{
		send_to_char("Nobody by that name seems available.\n", ch);
		return;
	}
	if (ch == vict)
	{
		send_to_char("You try to tell yourself something. Everybody cares. Really.\n", ch);
		return;
	}
	if (IS_NPC(vict))
	{
		send_to_char("What's the point?\n", ch);
		return;
	}
	if (!vict->desc)
	{
		send_to_char("That person can't hear you.\n", ch);
		return;
	}
	if (ch->desc)
	{
		if (IS_SET(ch->specials.act, PLR_ECHO))
		{
			checked_snprintf(Gbuf1, MAX_STRING_LENGTH, "&+rYou ptell %s '&+R%s&n&+r'\n",
					 GET_NAME(vict), msg);
			send_to_char(Gbuf1, ch, LOG_PRIVATE);
		}
		else
			send_to_char("Ok.\n", ch);
	}
	checked_snprintf(Gbuf1, MAX_STRING_LENGTH,
			 "&+r%s responds to your petition with '&+R%s&n&+r'\n",
			 (CAN_SEE(vict, ch) && IS_TRUSTED(ch)) ? ch->player.name :
			 IS_TRUSTED(vict)		       ? ch->player.name :
								 "Someone",
			 msg);
	send_to_char(Gbuf1, vict, LOG_PRIVATE);

	/* Send GMCP to victim (player receiving the reply) */
	gmcp_comm_channel(vict, "petition", GET_NAME(ch), msg);

	logit(LOG_PETITION, "(%s) ptells (%s): (%s).", GET_NAME(ch), GET_NAME(vict), msg);

	if (get_property("logs.chat.status", 0.000) && IS_PC(ch) && IS_PC(vict))
		logit(LOG_CHAT, "%s ptells %s '%s'", GET_NAME(ch), GET_NAME(vict), msg);

	/* Send GMCP to god who sent the ptell (so they see their own reply) */
	{
		char sender_label[MAX_NAME_LENGTH + MAX_NAME_LENGTH + 8];
		snprintf(sender_label, sizeof(sender_label), "%s -> %s", GET_NAME(ch),
			 GET_NAME(vict));
		gmcp_comm_channel(ch, "petition", sender_label, msg);
	}

	for (d = descriptor_list; d; d = d->next)
	{
		if ((STATE(d) == CON_PLAYING) && IS_TRUSTED(d->character) &&
		    IS_SET(d->character->specials.act, PLR_PETITION) && (d->character != vict) &&
		    (d->character != ch))
		{
			checked_snprintf(Gbuf1, MAX_STRING_LENGTH,
					 "&+rPetition: %s responds to %s with '&+R%s&n&+r'.\n",
					 CAN_SEE(d->character, ch) ? GET_NAME(ch) : "Someone",
					 GET_NAME(vict), msg);
			send_to_char(Gbuf1, d->character, LOG_PRIVATE);

			/* Send GMCP to other gods monitoring petitions */
			{
				char sender_label[MAX_NAME_LENGTH + MAX_NAME_LENGTH + 8];
				snprintf(sender_label, sizeof(sender_label), "%s -> %s",
					 CAN_SEE(d->character, ch) ? GET_NAME(ch) : "Someone",
					 GET_NAME(vict));
				gmcp_comm_channel(d->character, "petition", sender_label, msg);
			}
		}
	}

	return;
}

void do_petition_block(P_char ch, char *argument, int /*cmd*/)
{
	P_char vict;
	P_obj dummy;
	char buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch) || !IS_TRUSTED(ch))
		return;

	one_argument(argument, buf);

	if (!*buf)
		send_to_char("Usage: petition_block <player>\r\n", ch);
	else if (!generic_find(argument, FIND_CHAR_WORLD, ch, &vict, &dummy))
		send_to_char("Cannot find target.\r\n", ch);
	else if (IS_NPC(vict))
		send_to_char("Can't do that to a mobile.\n", ch);
	else if (GET_LEVEL(vict) >= GET_LEVEL(ch))
		act("Not a chance.", 0, ch, 0, vict, TO_CHAR);
	else if (IS_SET(vict->specials.act2, PLR2_B_PETITION))
	{
		send_to_char("The petition block was removed.\r\n", vict);
		send_to_char("Petition block removed.\n", ch);

		REMOVE_BIT(vict->specials.act2, PLR2_B_PETITION);

		wizlog(GET_LEVEL(ch), "%s just removed the petition block on %s.", GET_NAME(ch),
		       GET_NAME(vict));
		logit(LOG_WIZ, "%s just removed the petition block on %s.", GET_NAME(ch),
		      GET_NAME(vict));
		// sql_log(ch, WIZLOG, "Removed petition block on %s", GET_NAME(vict));
	}
	else
	{
		send_to_char(
			"&+WThe gods take away your ability to use the &+Rpetition &+Wchannel.\r\n",
			vict);
		send_to_char("Petition block set.\r\n", ch);

		SET_BIT(vict->specials.act2, PLR2_B_PETITION);

		wizlog(GET_LEVEL(ch), "%s was just PETITION BLOCKED by %s.", GET_NAME(vict),
		       GET_NAME(ch));
		logit(LOG_WIZ, "%s was just PETITION BLOCKED by %s.", GET_NAME(vict), GET_NAME(ch));
		// sql_log(ch, WIZLOG, "Petition blocked %s", GET_NAME(vict));
	}
}
