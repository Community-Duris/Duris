/* Currency exchange special procedure for money-changer mobiles. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"

extern const char *coin_names[];

#define RATE_TO_LOWER 10
#define RATE_TO_SILVER 15
#define RATE_TO_GOLD 25
#define RATE_TO_PLATINUM 45

int money_changer(P_char me, P_char ch, int cmd, char *arg)
{
	long amount, from, to, n, rate = 0;
	char Gbuf1[MAX_STRING_LENGTH];

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}
	if (!me || !ch || !IS_AWAKE(me) || IS_FIGHTING(me))
		return FALSE;

	if ((cmd != CMD_EXCHANGE) && (cmd != CMD_LIST))
		return FALSE;

	if (!CAN_SEE(me, ch))
	{
		mobsay(me, "How may I be of help if I cannot see you?");
		return TRUE;
	}
	if (cmd == CMD_LIST)
	{
		snprintf(Gbuf1, MAX_STRING_LENGTH,
			 "Our surcharge is %d percent for exchange to a lower coin-type,",
			 RATE_TO_LOWER);
		mobsay(me, Gbuf1);
		snprintf(Gbuf1, MAX_STRING_LENGTH,
			 "%d percent for an exchange from copper to silver,", RATE_TO_SILVER);
		mobsay(me, Gbuf1);
		snprintf(Gbuf1, MAX_STRING_LENGTH,
			 "%d percent for an exchange from copper or silver to gold,", RATE_TO_GOLD);
		mobsay(me, Gbuf1);
		snprintf(Gbuf1, MAX_STRING_LENGTH,
			 "and %d percent for an exchange from a lesser coin to platinum.",
			 RATE_TO_PLATINUM);
		mobsay(me, Gbuf1);
		return TRUE;
	}
	/*
	 * else cmd == CMD_EXCHANGE
	 */
	arg = one_argument(arg, Gbuf1);
	if (strlen(Gbuf1) > 6)
	{
		strcpy(Gbuf1,
		       "Sorry, due to weight restrictions, we can't handle such large amounts.");
		mobsay(me, Gbuf1);
		return TRUE;
	}
	amount = strtol(Gbuf1, NULL, 0);

	if (amount < 0)
	{
		act("$n laughs at $N and says, 'Sorry, I don't deal in negative change.'", TRUE, ch,
		    0, me, TO_ROOM);
		act("$N laughs at you and says, 'Sorry, I don't deal in negative change.'", TRUE,
		    ch, 0, me, TO_CHAR);
		return FALSE;
	}
	arg = one_argument(arg, Gbuf1);
	from = coin_type(Gbuf1);
	one_argument(arg, Gbuf1);
	to = coin_type(Gbuf1);

	amount = amount * pow10(from);
	if ((amount == 0) || (from == -1) || (to == -1))
	{
		act("$n tries to exchange some coins with $N.", TRUE, ch, 0, me, TO_ROOM);
		act("$N stares blankly at $n.", TRUE, ch, 0, me, TO_ROOM);
		act("$N stares blankly at you.", TRUE, ch, 0, me, TO_CHAR);
		return FALSE;
	}
	if (from > to)
	{
		/*
		 * not enough ?
		 */
		if (ch->points.cash[from] < (amount / pow10(from)))
		{
			snprintf(Gbuf1, MAX_STRING_LENGTH,
				 "You don't have enough %s coins to complete that exchange.\r\n",
				 coin_names[from]);
			send_to_char(Gbuf1, ch);
			return TRUE;
		}
		n = (amount * (100 - RATE_TO_LOWER) / 100) / pow10(to);
		ch->points.cash[from] -= (amount / pow10(from));
		ch->points.cash[to] += n;
		act("$n exchanges some coins with $N.", TRUE, ch, 0, me, TO_ROOM);
		snprintf(Gbuf1, MAX_STRING_LENGTH,
			 "You exchange %ld %s coins for %ld %s coins.\r\n", (amount / pow10(from)),
			 coin_names[from], n, coin_names[to]);
		send_to_char(Gbuf1, ch);
		return TRUE;
	}
	if (from < to)
	{
		switch (to)
		{
		case 1:
		{
			rate = RATE_TO_SILVER;
		}
		break;
		case 2:
		{
			rate = RATE_TO_GOLD;
		}
		break;
		case 3:
		{
			rate = RATE_TO_PLATINUM;
		}
		break;
		default:
		{
			send_to_char("Unknown coin-type.\r\n", ch);
			return TRUE;
		}
		break;
		}

		amount = ((amount / ((pow10(to) * (10 + (rate / 10))) / 10)) *
			  ((pow10(to) * (10 + (rate / 10))) / 10));

		if (amount == 0)
		{
			snprintf(Gbuf1, MAX_STRING_LENGTH,
				 "You need to specify more %s coins to complete that exchange.\r\n",
				 coin_names[from]);
			send_to_char(Gbuf1, ch);
			return TRUE;
		}
		if (ch->points.cash[from] < (amount / pow10(from)))
		{
			/*
			 * not enough
			 */
			snprintf(Gbuf1, MAX_STRING_LENGTH,
				 "You don't have enough %s coins to complete that exchange.\r\n",
				 coin_names[from]);
			send_to_char(Gbuf1, ch);
			return TRUE;
		}
		ch->points.cash[from] -= (amount / pow10(from));
		/*
		 * weight check?
		 */
		ch->points.cash[to] += amount / ((pow10(to) * (10 + (rate / 10))) / 10);
		act("$n exchanges some coins with $N.", TRUE, ch, 0, me, TO_ROOM);
		snprintf(Gbuf1, MAX_STRING_LENGTH,
			 "You exchange %ld %s coins for %ld %s coins.\r\n", (amount / pow10(from)),
			 coin_names[from], (amount / ((pow10(to) * (10 + (rate / 10))) / 10)),
			 coin_names[to]);
		send_to_char(Gbuf1, ch);
		return TRUE;
	}
	if (from == to)
	{
		send_to_char("What a wiseguy.\r\n", ch);
		return TRUE;
	}
	return FALSE;
}
