/* World quest special procedure and payment callback. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "net/gmcp.h"
#include "world/db.h"
#include "world/difficulty.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include "world/specs.prototypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "world/world_quest.h"
#include "world/world_quest_policy_math.h"
#include "economy/currency_transaction.h"
#include "sql/sql.h"

int get_map_room(int zone_id);

enum class world_quest_payment_action : uint8_t
{
	abandon,
	map,
	quest,
};

struct world_quest_payment_context
{
	world_quest_payment_action action;
	int32_t fee;
	int32_t giver_vnum;
	// Saved per-player attempt watermark; reset preserves it and creation advances it.
	int32_t quest_started;
};

static_assert(sizeof(world_quest_payment_context) <= CURRENCY_PENDING_CONTEXT_MAX_BYTES);

static void world_quest_report_creation_failure(P_char pl, quest_creation_failure failure)
{
	if (!pl)
		return;

	if (failure == QUEST_CREATION_NO_ELIGIBLE_ZONE)
	{
		send_to_char(
			"Hmm, I can't find an eligible quest zone for you right now. Try one of my colleagues around the world.\r\n",
			pl);
	}
	else if (failure == QUEST_CREATION_NO_ELIGIBLE_TARGET)
	{
		send_to_char(
			"Hmm, I found quest zones, but no suitable quest target is available right now. Try one of my colleagues around the world.\r\n",
			pl);
	}
	else if (GET_LEVEL(pl) >= MAXLVLMORTAL)
	{
		send_to_char(
			"Hmm, I can't find a suitable quest for someone of your experience right now. Try one of my colleagues around the world.\r\n",
			pl);
	}
	else
	{
		send_to_char(
			"Hmm, I'm unable to help you right now, try one of my colleagues around the world, or grab a few levels and come back.\r\n",
			pl);
	}
}

static void world_quest_refund_payment(P_char pl, int fee)
{
	if (!pl || fee <= 0)
		return;

	send_to_char("\r\n&=LWYou get your money back.\r\n", pl);
	ADD_MONEY(pl, fee);
}

static void world_quest_payment_committed(P_char pl, bool committed,
					  const currency_command_result & /*result*/,
					  unsigned int error_code, const uint8_t *raw_context,
					  size_t context_size)
{
	world_quest_payment_context payment = {};
	if (!pl || !pl->only.pc || !raw_context || context_size != sizeof(payment))
	{
		logit(LOG_WIZ,
		      "World quest payment callback lost its player or context (committed=%d error=%u)",
		      committed, error_code);
		if (pl && !committed)
			send_to_char(
				"Your world-quest payment was rejected; your quest was not changed.\r\n",
				pl);
		return;
	}
	memcpy(&payment, raw_context, sizeof(payment));
	if (payment.fee <= 0 || payment.giver_vnum <= 0 || payment.quest_started < 0 ||
	    (payment.action != world_quest_payment_action::quest && !payment.quest_started) ||
	    (payment.action != world_quest_payment_action::abandon &&
	     payment.action != world_quest_payment_action::map &&
	     payment.action != world_quest_payment_action::quest))
	{
		logit(LOG_WIZ,
		      "World quest payment callback received invalid context for pid %d (committed=%d error=%u)",
		      GET_PID(pl), committed, error_code);
		if (!committed)
			send_to_char(
				"Your world-quest payment was rejected; your quest was not changed.\r\n",
				pl);
		return;
	}

	if (!committed)
	{
		logit(LOG_DEBUG, "World quest payment rejected for pid %d (error %u)", GET_PID(pl),
		      error_code);
		send_to_char(
			"Your world-quest payment could not be completed; your quest was not changed.\r\n",
			pl);
		return;
	}

	switch (payment.action)
	{
	case world_quest_payment_action::abandon:
		if (!pl->only.pc->quest_active || pl->only.pc->quest_accomplished ||
		    pl->only.pc->quest_started != payment.quest_started)
		{
			send_to_char(
				"Your quest changed before the payment settled, so the charge is being returned.\r\n",
				pl);
			world_quest_refund_payment(pl, payment.fee);
			return;
		}
		send_to_char("You hand over the money.\r\n", pl);
		send_to_char("You no longer have a task.\r\n", pl);
		// Make it so pl doesn't get the same quest again; once you fail, you fail.
		// This also makes the quest count as one of today's quests.
		if (pl->only.pc->quest_type == FIND_AND_KILL &&
		    pl->only.pc->quest_kill_how_many > 0)
			sql_world_quest_finished(pl, 0);
		resetQuest(pl);
		gmcp_quest_status(pl);
		return;

	case world_quest_payment_action::map:
		if (pl->only.pc->quest_active != 1 || pl->only.pc->quest_accomplished ||
		    pl->only.pc->quest_map_bought == 1 ||
		    pl->only.pc->quest_started != payment.quest_started)
		{
			send_to_char(
				"Your quest changed before the payment settled, so the charge is being returned.\r\n",
				pl);
			world_quest_refund_payment(pl, payment.fee);
			return;
		}
		send_to_char("You hand over the money.\r\n", pl);
		send_to_char("Take a quick peek at this note:\r\n", pl);
		quest_buy_map(pl);
		return;

	case world_quest_payment_action::quest:
		// A quest may have completed or another command may have changed the
		// state while the debit was in flight.  Never charge for a stale request.
		if (pl->only.pc->quest_active || pl->only.pc->quest_accomplished ||
		    sql_world_quest_can_do_another(pl) < 1)
		{
			send_to_char(
				"Your quest state changed before the payment settled, so the charge is being returned.\r\n",
				pl);
			world_quest_refund_payment(pl, payment.fee);
			return;
		}

		quest_creation_failure failure = QUEST_CREATION_NO_FAILURE;
		if (createQuestForGiverVnum(pl, payment.giver_vnum, &failure))
		{
			send_to_char("You hand over the money.\r\n", pl);
			do_quest(pl, writable_arg(""), 0);
			send_to_char(
				"Remember, you can always type 'quest' to see your current quest.\r\n",
				pl);
			gmcp_quest_status(pl);
			return;
		}

		world_quest_report_creation_failure(pl, failure);
		world_quest_refund_payment(pl, payment.fee);
		return;
	}
}

int world_quest(P_char ch, P_char pl, int cmd, char *arg)
{
	char buf[MAX_INPUT_LENGTH];
	char name[MAX_INPUT_LENGTH], what[MAX_STRING_LENGTH];
	char money_string[MAX_INPUT_LENGTH];

	int temp = 0;
	float timediff, costmod;

	if (cmd == CMD_SET_PERIODIC)
	{
		return FALSE;
	}

	if (!IS_ALIVE(ch) || !arg || !IS_ALIVE(pl) || IS_NPC(pl))
	{
		return FALSE;
	}

	if ((cmd == CMD_ASK) && pl)
	{
		half_chop(arg, name, what);

		// ask <bartender> <quest|abandon|resign> ...
		if (ch != ParseTarget(ch, name))
		{
			return FALSE;
		}

		if (isname("abandon", what) || isname("resign", what))
		{
			if (pl->only.pc->quest_accomplished)
			{
				mobsay(ch, "Why would you like to resign when you done?!");
				return TRUE;
			}
			if (!pl->only.pc->quest_active)
			{
				mobsay(ch, "Abandon what quest, hmmmm!");
				return TRUE;
			}

			/* Can now abandon your quest at any bartender.
			if(pl->only.pc->quest_giver != GET_VNUM(ch))
			{
			  mobsay(ch, "Abandon that quest at same place as you got it!");
			  return TRUE;
			}
			*/

			temp = (int)((get_property("world.quest.abandon.mod", 1.0) * GET_LEVEL(pl) *
				      GET_LEVEL(pl) * GET_LEVEL(pl)));

			timediff = time(NULL) - pl->only.pc->quest_started;
			// debug("timediff: %f", timediff);
			costmod = 1.0 - (timediff / 60.0 / 60.0 /
					 get_property("world.quest.cost.abandon.time", 24.000));
			costmod *= 100;
			// debug("costmod: %f", costmod);
			// debug("temp: %d", temp);
			temp = temp * BOUNDED(1, costmod, 100) / 100;
			// debug("temp: %d", temp);
			// debug("timediff: %f, hrsdiff: %f, costmod: %f, temp: %d, cost: %s", timediff, timediff / 60 / 60, costmod, temp, coin_stringv(temp));

			if (pl->only.pc->quest_type == FIND_AND_KILL &&
			    pl->only.pc->quest_kill_how_many > 0)
			{
				// Allowing them to abandon at the cost of 1 quest (cost is done during the call to sql_world_quest_finished).
				if (!isname("confirm", what))
				{
					send_to_char(
						"You must be &+Wcrazy&N!  Try asking &+yabandon confirm&n if you really want to do that.\r\n",
						pl);
					return TRUE;
				}
			}

			snprintf(
				money_string, MAX_INPUT_LENGTH,
				"OH NO, you've cost me alot of time and money, but toss me %s and I'll take care of your task!",
				coin_stringv(temp));

			mobsay(ch, money_string);
			if (GET_MONEY(pl) < temp)
			{
				send_to_char("You dont have that much money...\r\n", pl);
				return (TRUE);
			}

			const world_quest_payment_context payment = {
				world_quest_payment_action::abandon, temp, GET_VNUM(ch),
				pl->only.pc->quest_started
			};
			if (!currency_transaction_submit_wallet_value(
				    pl, -static_cast<int64_t>(temp),
				    currency_reason_type::wallet_spend, GET_VNUM(ch),
				    critical_source_site::command,
				    critical_deadline_class::interactive,
				    world_quest_payment_committed, &payment, sizeof(payment)))
			{
				send_to_char(
					"The bartender's payment service is busy; your quest was not changed. Please try again.\r\n",
					pl);
			}
			return TRUE;
		}

		if (isname(what, "map") || isname(arg, "m"))
		{
			if (pl->only.pc->quest_active != 1)
			{
				send_to_char("Maybe try getting a quest first?\r\n", pl);
				return TRUE;
			}

			int map_room = get_map_room(real_zone(pl->only.pc->quest_zone_number));
			// debug("do_quest(): quest_zone_number: %d, real_zone: %d, map_room: %d", pl->only.pc->quest_zone_number, real_zone(pl->only.pc->quest_zone_number), map_room);

			if (map_room <= 0)
			{
				mobsay(ch, "Sorry, but I don't have any maps to that zone.");
				return TRUE;
			}
			if (pl->only.pc->quest_map_bought == 1)
			{
				send_to_char(
					"Your memory is bad, you dont have to pay me for this, you can just type 'quest'.\r\n",
					pl);
				return TRUE;
			}

			temp = 10 * GET_LEVEL(pl);

			snprintf(
				money_string, sizeof money_string,
				"Hmmmm, yeah, I might have a additional information for you, but I'm not giving it away for free! It'll cost you %s.",
				coin_stringv(temp));

			mobsay(ch, money_string);
			if (GET_MONEY(pl) < temp)
			{
				send_to_char(
					"You dont have the money, so you go can't get any additional information.\r\n",
					pl);
				return (TRUE);
			}

			const world_quest_payment_context payment = {
				world_quest_payment_action::map, temp, GET_VNUM(ch),
				pl->only.pc->quest_started
			};
			if (!currency_transaction_submit_wallet_value(
				    pl, -static_cast<int64_t>(temp),
				    currency_reason_type::wallet_spend, GET_VNUM(ch),
				    critical_source_site::command,
				    critical_deadline_class::interactive,
				    world_quest_payment_committed, &payment, sizeof(payment)))
			{
				send_to_char(
					"The bartender's payment service is busy; your map was not changed. Please try again.\r\n",
					pl);
			}
			return TRUE;
		}

		if (!isname(what, "quest") && !isname(arg, "q"))
			return FALSE;

		if (GET_LEVEL(pl) < WORLD_QUEST_MIN_LEVEL)
		{
			mobsay(ch, "You need to reach level 11 before taking world quests.");
			return TRUE;
		}

		if (sql_world_quest_can_do_another(pl) < 1)
		{
			act("$n says, 'Sorry, I don't have any more quests for right now.'", TRUE,
			    ch, 0, pl, TO_VICT);

			return TRUE;
		}

		/*
		   if(pl->only.pc->quest_accomplished != 1 &&
		   pl->only.pc->quest_active == 1)
		   {
		   mobsay(ch, "Oh hmm unable to finish your quest? Why would you want another one then!");
		   return TRUE;
		   }
		 */
		if (pl->only.pc->quest_accomplished && pl->only.pc->quest_giver == GET_VNUM(ch))
		{
			act("$n says, 'Woah, nice work! Congratulations!'", TRUE, ch, 0, pl,
			    TO_VICT);
			act("$N says to $n, 'Well done!'", TRUE, pl, 0, ch, TO_NOTVICT);
			quest_full_reward(pl, ch, 1);
			return TRUE;
		}

		if (pl->only.pc->quest_accomplished)
		{
			act("$n says, 'Woah, nice work! But i didt give you this quest! Go find the real quest master'",
			    TRUE, ch, 0, pl, TO_VICT);
			return TRUE;
		}

		if (pl->only.pc->quest_active == 1)
		{
			mobsay(ch,
			       "&+LBaaaaaah! Finish the quest that you're already on first, then come back!&n");
			send_to_char(
				"&+LIf you unable to finish it, go to the quest master and ask him to take you of duty!\r\n",
				pl);
			return -1;
		}

		temp = MAX(0, static_cast<int>(get_property("world.quest.cost.per.level", 20.000) *
					       GET_LEVEL(pl)));
		temp = difficulty_scale_world_quest_fee(temp);

		snprintf(
			money_string, sizeof money_string,
			"Hmmmm, yeah, I might have a tip for you, but I'm not giving it away for free! It'll cost you %s.",
			coin_stringv(temp));

		mobsay(ch, money_string);

		if (GET_MONEY(pl) < temp)
		{
			send_to_char("You dont have the money, so you go sulk in the corner.\r\n",
				     pl);
			return (TRUE);
		}

		const world_quest_payment_context payment = { world_quest_payment_action::quest,
							      temp, GET_VNUM(ch),
							      pl->only.pc->quest_started };
		if (!currency_transaction_submit_wallet_value(
			    pl, -static_cast<int64_t>(temp), currency_reason_type::wallet_spend,
			    GET_VNUM(ch), critical_source_site::command,
			    critical_deadline_class::interactive, world_quest_payment_committed,
			    &payment, sizeof(payment)))
		{
			send_to_char(
				"The bartender's payment service is busy; your quest was not changed. Please try again.\r\n",
				pl);
		}
		return TRUE;
	}

	if (pl)
	{
		return (0);
	}

	switch (number(1, 15))
	{
	case 1:
		act("$n smells the fresh air.", TRUE, ch, 0, 0, TO_ROOM);
		break;

	case 2:
		strcpy(buf, "Come talk to me, I got some hot tips on quests for you!");
		do_yell(ch, buf, 0);
		break;
	}

	return FALSE;
}
