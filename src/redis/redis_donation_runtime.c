#include "redis/redis_donation_runtime.h"

#include "economy/donation_event.h"
#include "core/prototypes.h"
#include "redis/redis_donation_worker.h"
#include "core/utils.h"

#include <cstdio>

extern P_desc descriptor_list;

namespace
{
constexpr int REDIS_DONATION_MAX_MESSAGES_PER_PULSE = 8;
bool donation_enabled = false;

void broadcast_donation_nchat(const struct donation_event *event)
{
	char buffer[MAX_STRING_LENGTH];
	const double amount = static_cast<double>(event->amount_cents) / 100.0;

	if (event->is_public)
	{
		if (event->message[0])
			snprintf(buffer, sizeof(buffer),
				 "&+Y%s&n&+m donated &+W%.2f %s&n&+m: &+w'%s'&n\n",
				 event->character_name, amount, event->currency, event->message);
		else
			snprintf(buffer, sizeof(buffer), "&+Y%s&n&+m donated &+W%.2f %s&n&+m!&n\n",
				 event->character_name, amount, event->currency);
	}
	else
	{
		if (event->message[0])
			snprintf(buffer, sizeof(buffer),
				 "&+Yan anonymous donor&n&+m gave &+W%.2f %s&n&+m: &+w'%s'&n\n",
				 amount, event->currency, event->message);
		else
			snprintf(buffer, sizeof(buffer),
				 "&+Yan anonymous donor&n&+m gave &+W%.2f %s&n&+m!&n\n", amount,
				 event->currency);
	}

	for (P_desc descriptor = descriptor_list; descriptor; descriptor = descriptor->next)
	{
		P_char recipient = descriptor->character;
		if (descriptor->connected || !recipient || IS_NPC(recipient) ||
		    !PLR2_FLAGGED(recipient, PLR2_NCHAT))
			continue;
		send_to_char(buffer, recipient);
	}

	logit(LOG_SYS, "donation: event=%s donor=%s amount=%.2f currency=%s", event->event_id,
	      event->is_public ? event->character_name : "anonymous", amount, event->currency);
}

void check_donation_messages(void)
{
#ifndef __NO_REDIS__
	if (!donation_enabled)
		return;

	/* DurisWeb hook gate. Read on the game thread, not the worker thread, so
	   the properties array is never read concurrently with a properties set.
	   When disabled we still drain the queue and drop, rather than leaving
	   events to accumulate to the queue cap and then flood on re-enable. */
	const bool hook_enabled = durisweb_hook_enabled("donation_delivery");
	int dropped_while_disabled = 0;

	for (int handled = 0; handled < REDIS_DONATION_MAX_MESSAGES_PER_PULSE; ++handled)
	{
		donation_event event = {};
		if (!redis_donation_worker_take(&event))
			break;
		if (!hook_enabled)
		{
			++dropped_while_disabled;
			continue;
		}
		broadcast_donation_nchat(&event);
	}

	/* One line per pulse, not per event, so a disabled hook cannot spam the log. */
	if (dropped_while_disabled > 0)
		logit(LOG_SYS,
		      "donation: dropped %d event(s); durisweb.hook.donation_delivery is disabled",
		      dropped_while_disabled);
#endif
}
} // namespace

bool redis_donation_runtime_enabled(void)
{
	return donation_enabled;
}

void redis_donation_runtime_set_enabled(bool enabled)
{
	donation_enabled = enabled;
}

void event_check_donation_messages(P_char /*ch*/, P_char /*victim*/, P_obj /*obj*/, void * /*data*/)
{
	check_donation_messages();
}
