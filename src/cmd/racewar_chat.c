/*
 * Racewar chat commands and their private helpers.
 */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/safe_format.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "net/gmcp.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

extern P_desc descriptor_list;
extern const racewar_struct racewar_color[MAX_RACEWAR + 2];

static void choronize(char *argument);
static int SpammingNchat(P_char ch);

void do_nchat(P_char ch, char *argument, int /*cmd*/)
{
	P_desc i;
	bool good, evil, undead, neutral, all;
	char Gbuf1[MAX_STRING_LENGTH];
	char Gbuf2[MAX_STRING_LENGTH];
	static char LastNchat1[MAX_INPUT_LENGTH], LastNchat2[MAX_INPUT_LENGTH];
	P_char to;

	if (!IS_ALIVE(ch))
	{
		return;
	}

	if (IS_NPC(ch))
	{
		send_to_char(
			"You try, but you just can't figure out how.. Maybe this requires being a PC.\n\r",
			ch);
		return;
	}

	if (!IS_SET(ch->specials.act2, PLR2_NCHAT))
	{
		send_to_char("Newbie chat is turned off, type \"tog nchat\" to turn it on.\n", ch);
		return;
	}

	if (IS_ILLITHID(ch) && !IS_TRUSTED(ch))
	{
		send_to_char("If you need this channel, you shouldn't be playing this character.\n",
			     ch);
		return;
	}

	if (IS_AFFECTED2(ch, AFF2_SILENCED))
	{
		send_to_char("You move your lips, but no sound comes forth!\n", ch);
		return;
	}

	if (is_silent(ch, TRUE))
	{
		return;
	}
	/*
	  if((GET_LEVEL(ch) > 31) &&
	     (GET_LEVEL(ch) < 57) &&
	     !IS_SET(PLR2_FLAGS(ch), PLR2_NEWBIE_GUIDE))
	  {
	   send_to_char("Your level no longer qualifies you for newbie-chat, sorry.\r\n", ch);
	   return;
	  }
	*/
	if (IS_DISGUISE_PC(ch) || IS_DISGUISE_ILLUSION(ch) || IS_DISGUISE_SHAPE(ch))
	{
		send_to_char("&+WYou are not in your true shape!\r\n", ch);
		return;
	}

	while (*argument == ' ' && *argument != '\0')
	{
		argument++;
	}

	all = good = evil = undead = neutral = FALSE;

	if (!*argument)
	{
		send_to_char(
			"Thats right nchat and then add something else, for example: \"nchat how do I kill things?\"\n",
			ch);
		return;
	}

	choronize(argument);
	if (is_abbrev(
		    "PANIC!  You couldn't escape! PANIC!  You couldn't escape! PANIC!  You couldn't escape!",
		    argument))
	{
		if (SpammingNchat(ch) > 3)
		{
			send_to_char("You have temporarily lost nchat privledges due to spam.\n",
				     ch);
			return;
		}
	}
	if (!strcmp(argument, LastNchat1))
	{
		if (SpammingNchat(ch) > 5)
		{
			send_to_char("You have temporarily lost nchat privledges due to spam.\n",
				     ch);
			return;
		}
	}
	if (!strcmp(argument, LastNchat2))
	{
		if (SpammingNchat(ch) > 5)
		{
			send_to_char("You have temporarily lost nchat privledges due to spam.\n",
				     ch);
			return;
		}
	}
	else
	{
		snprintf(LastNchat2, MAX_INPUT_LENGTH, "%s", LastNchat1);
		snprintf(LastNchat1, MAX_INPUT_LENGTH, "%s", argument);
	}

	if (ch->desc)
	{
		if (IS_TRUSTED(ch))
		{
			if (((*argument == 'g') || (*argument == 'G')) && (*(argument + 1) == ' '))
				good = TRUE;
			else if (((*argument == 'e') || (*argument == 'E')) &&
				 (*(argument + 1) == ' '))
				evil = TRUE;
			else if (((*argument == 'u') || (*argument == 'U')) &&
				 (*(argument + 1) == ' '))
				undead = TRUE;
			else if (((*argument == 'n') || (*argument == 'N')) &&
				 (*(argument + 1) == ' '))
				neutral = TRUE;
			else if (((*argument == 'a') || (*argument == 'A')) &&
				 (*(argument + 1) == ' '))
			{
				all = good = evil = undead = TRUE;
			}
			else
			{
				send_to_char(
					"&+YMake up your mind first which side do you want to help. Use nchat 'e', 'u', 'g', 'n' or 'a'. &n\n",
					ch);
				return;
			}
			argument += 2;

			if (all)
				snprintf(Gbuf2, MAX_STRING_LENGTH, "&+W*all*&n");
			else if (good)
				snprintf(Gbuf2, MAX_STRING_LENGTH, "&+%c%s&N",
					 racewar_color[RACEWAR_GOOD].color,
					 racewar_color[RACEWAR_GOOD].name);
			else if (evil)
				snprintf(Gbuf2, MAX_STRING_LENGTH, "&+%c%s&N",
					 racewar_color[RACEWAR_EVIL].color,
					 racewar_color[RACEWAR_EVIL].name);
			else if (undead)
				snprintf(Gbuf2, MAX_STRING_LENGTH, "&+%c%s&N",
					 racewar_color[RACEWAR_UNDEAD].color,
					 racewar_color[RACEWAR_UNDEAD].name);
			else if (neutral)
				snprintf(Gbuf2, MAX_STRING_LENGTH, "&+%c%s&N",
					 racewar_color[RACEWAR_NEUTRAL].color,
					 racewar_color[RACEWAR_NEUTRAL].name);
			else
				snprintf(Gbuf2, MAX_STRING_LENGTH, "&+Cundefined&n");

			checked_snprintf(Gbuf1, MAX_STRING_LENGTH,
					 "&+mYou racewar chat to &n(%s): '&+w%s&n&+w'\n", Gbuf2,
					 argument);
			send_to_char(Gbuf1, ch, LOG_PRIVATE);
		}
		else if (IS_SET(ch->specials.act, PLR_ECHO))
		{
			snprintf(Gbuf1, MAX_STRING_LENGTH,
				 "&+mYou tell your racewar '&+W%s&n&+w'\n", argument);
			send_to_char(Gbuf1, ch, LOG_PRIVATE);
		}
		else
			send_to_char("Ok.\n", ch);
	}

	if (!IS_TRUSTED(ch))
	{
		if (IS_RACEWAR_GOOD(ch))
		{
			snprintf(Gbuf2, MAX_STRING_LENGTH, "&+%c%s&N",
				 racewar_color[RACEWAR_GOOD].color,
				 racewar_color[RACEWAR_GOOD].name);
			good = TRUE;
		}
		else if (IS_RACEWAR_EVIL(ch))
		{
			snprintf(Gbuf2, MAX_STRING_LENGTH, "&+%c%s&N",
				 racewar_color[RACEWAR_EVIL].color,
				 racewar_color[RACEWAR_EVIL].name);
			evil = TRUE;
		}
		else if (IS_RACEWAR_UNDEAD(ch))
		{
			snprintf(Gbuf2, MAX_STRING_LENGTH, "&+%c%s&N",
				 racewar_color[RACEWAR_UNDEAD].color,
				 racewar_color[RACEWAR_UNDEAD].name);
			undead = TRUE;
		}
		else if (IS_RACEWAR_NEUTRAL(ch))
		{
			snprintf(Gbuf2, MAX_STRING_LENGTH, "&+%c%s&N",
				 racewar_color[RACEWAR_NEUTRAL].color,
				 racewar_color[RACEWAR_NEUTRAL].name);
			neutral = TRUE;
		}
		else
		{
			snprintf(Gbuf2, MAX_STRING_LENGTH, "&-Rundefined&n");
		}
	}

	for (i = descriptor_list; i; i = i->next)
	{
		if (i->connected || !(to = i->character) || to == ch)
		{
			continue;
		}
		// If mortal && racewar side doesn't match.  (Immortals see all nchats).
		if (!IS_TRUSTED(to) && !all &&
		    ((evil && !IS_RACEWAR_EVIL(to)) || (undead && !IS_RACEWAR_UNDEAD(to)) ||
		     (good && !IS_RACEWAR_GOOD(to)) || (neutral && !IS_RACEWAR_NEUTRAL(to))))
		{
			continue;
		}
		// NPCs need to not hear nchat so that the pc only flags can be checked in peace.
		if (IS_NPC(to) || !PLR2_FLAGGED(to, PLR2_NCHAT))
		{
			continue;
		}
		// Skip if to is ignoring ch.
		if (to->only.pc->ignored == ch)
		{
			continue;
		}
		/* Allowing disguised people to hear nchat
		 * Just a FYI, this doesn't allow people to nchat across racewars.
		if( IS_DISGUISE(to) || IS_DISGUISE_ILLUSION(to) || IS_DISGUISE_SHAPE(to) )
		{
		  continue;
		}
		*/
		// Mortals do not see the undefined racewar sides.
		if (!IS_TRUSTED(to) && (!good && !evil && !undead && !neutral))
			continue;
		if (IS_TRUSTED(to))
		{
			checked_snprintf(Gbuf1, MAX_STRING_LENGTH,
					 "&+W%s&n&+m racewar-chats &+w(%s&+w): '&+Y%s&n&+w'\n",
					 PERS(ch, to, FALSE), Gbuf2,
					 language_CRYPT(ch, to, argument));
		}
		else
		{
			snprintf(Gbuf1, MAX_STRING_LENGTH,
				 "&+W%s&n&+m tells your racewar: &+w'&+Y%s&n&+w'\n",
				 PERS(ch, to, FALSE), language_CRYPT(ch, to, argument));
		}
		send_to_char(Gbuf1, to, LOG_PRIVATE);

		/* Send to web client via GMCP with alignment */
		{
			const char *alignment = "neutral";
			if (good)
				alignment = "good";
			else if (evil)
				alignment = "evil";
			else if (undead)
				alignment = "undead";
			gmcp_comm_channel_ex(to, "nchat", PERS(ch, to, FALSE), argument, alignment);
		}
	}

	/* Send to sender's web client too */
	{
		const char *alignment = "neutral";
		if (good)
			alignment = "good";
		else if (evil)
			alignment = "evil";
		else if (undead)
			alignment = "undead";
		gmcp_comm_channel_ex(ch, "nchat", GET_NAME(ch), argument, alignment);
	}

	if (get_property("logs.chat.status", 0.000))
	{
		logit(LOG_CHAT, "%s newb chat's (%s) '%s'", GET_NAME(ch), Gbuf2, argument);
	}
}

void do_jestros(P_char ch, char *argument, int /*cmd*/)
{
	P_desc i;
	bool good, evil, undead, neutral, all;
	char Gbuf1[MAX_STRING_LENGTH];
	char Gbuf2[MAX_STRING_LENGTH];
	P_char to;

	if (!IS_ALIVE(ch))
	{
		return;
	}

	if (IS_NPC(ch))
	{
		send_to_char(
			"You try, but you just can't figure out how.. Maybe this requires being a PC.\n\r",
			ch);
		return;
	}

	if (PLR3_FLAGGED(ch, PLR3_JESTROS))
	{
		send_to_char("Jchat channel is turned off, type \"tog jchat\" to turn it on.\n",
			     ch);
		return;
	}

	if (IS_AFFECTED2(ch, AFF2_SILENCED))
	{
		send_to_char("You move your lips, but no sound comes forth!\n", ch);
		return;
	}

	if (is_silent(ch, TRUE))
	{
		return;
	}

	while (*argument == ' ' && *argument != '\0')
	{
		argument++;
	}

	all = good = evil = undead = neutral = FALSE;

	if (!*argument)
	{
		send_to_char("Usage: jc <e|g|u|n|a> <message>\n", ch);
		return;
	}

	if (ch->desc)
	{
		if (IS_TRUSTED(ch))
		{
			if (((*argument == 'g') || (*argument == 'G')) && (*(argument + 1) == ' '))
				good = TRUE;
			else if (((*argument == 'e') || (*argument == 'E')) &&
				 (*(argument + 1) == ' '))
				evil = TRUE;
			else if (((*argument == 'u') || (*argument == 'U')) &&
				 (*(argument + 1) == ' '))
				undead = TRUE;
			else if (((*argument == 'n') || (*argument == 'N')) &&
				 (*(argument + 1) == ' '))
				neutral = TRUE;
			else if (((*argument == 'a') || (*argument == 'A')) &&
				 (*(argument + 1) == ' '))
			{
				all = good = evil = undead = TRUE;
			}
			else
			{
				send_to_char("&+YUse jc 'e', 'u', 'g', 'n' or 'a'. &n\n", ch);
				return;
			}
			argument += 2;

			if (all)
				snprintf(Gbuf2, MAX_STRING_LENGTH, "&+W*all*&n");
			else if (good)
				snprintf(Gbuf2, MAX_STRING_LENGTH, "&+%c%s&N",
					 racewar_color[RACEWAR_GOOD].color,
					 racewar_color[RACEWAR_GOOD].name);
			else if (evil)
				snprintf(Gbuf2, MAX_STRING_LENGTH, "&+%c%s&N",
					 racewar_color[RACEWAR_EVIL].color,
					 racewar_color[RACEWAR_EVIL].name);
			else if (undead)
				snprintf(Gbuf2, MAX_STRING_LENGTH, "&+%c%s&N",
					 racewar_color[RACEWAR_UNDEAD].color,
					 racewar_color[RACEWAR_UNDEAD].name);
			else if (neutral)
				snprintf(Gbuf2, MAX_STRING_LENGTH, "&+%c%s&N",
					 racewar_color[RACEWAR_NEUTRAL].color,
					 racewar_color[RACEWAR_NEUTRAL].name);
			else
				snprintf(Gbuf2, MAX_STRING_LENGTH, "&+Cundefined&n");

			checked_snprintf(Gbuf1, MAX_STRING_LENGTH,
					 "&+GYou jchat to &n(%s): '&+w%s&n&+w'\n", Gbuf2, argument);
			send_to_char(Gbuf1, ch, LOG_PRIVATE);
		}
		else if (IS_SET(ch->specials.act, PLR_ECHO))
		{
			snprintf(Gbuf1, MAX_STRING_LENGTH, "&+GYou jchat '&+W%s&n&+w'\n", argument);
			send_to_char(Gbuf1, ch, LOG_PRIVATE);
		}
		else
			send_to_char("Ok.\n", ch);
	}

	if (!IS_TRUSTED(ch))
	{
		if (IS_RACEWAR_GOOD(ch))
		{
			snprintf(Gbuf2, MAX_STRING_LENGTH, "&+%c%s&N",
				 racewar_color[RACEWAR_GOOD].color,
				 racewar_color[RACEWAR_GOOD].name);
			good = TRUE;
		}
		else if (IS_RACEWAR_EVIL(ch))
		{
			snprintf(Gbuf2, MAX_STRING_LENGTH, "&+%c%s&N",
				 racewar_color[RACEWAR_EVIL].color,
				 racewar_color[RACEWAR_EVIL].name);
			evil = TRUE;
		}
		else if (IS_RACEWAR_UNDEAD(ch))
		{
			snprintf(Gbuf2, MAX_STRING_LENGTH, "&+%c%s&N",
				 racewar_color[RACEWAR_UNDEAD].color,
				 racewar_color[RACEWAR_UNDEAD].name);
			undead = TRUE;
		}
		else if (IS_RACEWAR_NEUTRAL(ch))
		{
			snprintf(Gbuf2, MAX_STRING_LENGTH, "&+%c%s&N",
				 racewar_color[RACEWAR_NEUTRAL].color,
				 racewar_color[RACEWAR_NEUTRAL].name);
			neutral = TRUE;
		}
		else
		{
			snprintf(Gbuf2, MAX_STRING_LENGTH, "&-Rundefined&n");
		}
	}

	for (i = descriptor_list; i; i = i->next)
	{
		if (i->connected || !(to = i->character) || to == ch)
		{
			continue;
		}
		if (!IS_TRUSTED(to) && !all &&
		    ((evil && !IS_RACEWAR_EVIL(to)) || (undead && !IS_RACEWAR_UNDEAD(to)) ||
		     (good && !IS_RACEWAR_GOOD(to)) || (neutral && !IS_RACEWAR_NEUTRAL(to))))
		{
			continue;
		}
		if (IS_NPC(to) || PLR3_FLAGGED(to, PLR3_JESTROS))
		{
			continue;
		}
		if (to->only.pc->ignored == ch)
		{
			continue;
		}
		if (!IS_TRUSTED(to) && (!good && !evil && !undead && !neutral))
			continue;
		if (IS_TRUSTED(to))
		{
			checked_snprintf(Gbuf1, MAX_STRING_LENGTH,
					 "&+W%s&n&+G jchat &+w(%s&+w): '&+Y%s&n&+w'\n",
					 PERS(ch, to, FALSE), Gbuf2,
					 language_CRYPT(ch, to, argument));
		}
		else
		{
			snprintf(Gbuf1, MAX_STRING_LENGTH, "&+W%s&n&+G jchat: &+w'&+Y%s&n&+w'\n",
				 PERS(ch, to, FALSE), language_CRYPT(ch, to, argument));
		}
		send_to_char(Gbuf1, to, LOG_PRIVATE);

		gmcp_comm_channel_ex(to, "jchat", PERS(ch, to, FALSE), argument,
				     good   ? "good" :
				     evil   ? "evil" :
				     undead ? "undead" :
					      "neutral");
	}

	gmcp_comm_channel_ex(ch, "jchat", GET_NAME(ch), argument,
			     good   ? "good" :
			     evil   ? "evil" :
			     undead ? "undead" :
				      "neutral");

	if (get_property("logs.chat.status", 0.000))
	{
		logit(LOG_CHAT, "%s jchat (%s) '%s'", GET_NAME(ch), Gbuf2, argument);
	}
}

static void choronize(char *argument)
{
	char *index;

	while ((index = strcasestr(argument, "fucking")) != NULL)
	{
		snprintf(index, MAX_STRING_LENGTH, "Choron");
		index += strlen("Choron");
		*index = ' ';
		while (*index != '\0')
		{
			*index = index[1];
			index++;
		}
	}
}

static int SpammingNchat(P_char ch)
{
	struct affected_type *afp, af;

	if ((afp = get_spell_from_char(ch, TAG_NCHATSPAMMER)))
	{
		afp->duration = 10;
		return ++afp->modifier;
	}
	else
	{
		bzero(&af, sizeof(af));
		af.type = TAG_NCHATSPAMMER;
		af.modifier = 1;
		af.duration = 10;
		af.flags = AFFTYPE_NOSHOW | AFFTYPE_NODISPEL | AFFTYPE_NOMSG;
		affect_to_char(ch, &af);
		return 1;
	}
}
