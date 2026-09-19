#ifndef DURIS_SHOPKEEPER_SAVE_POLICY_H
#define DURIS_SHOPKEEPER_SAVE_POLICY_H

#include <limits.h>
#include <time.h>

/* Keep failed dirty-shopkeeper saves observable without retrying on every pulse. */
#define SHOPKEEPER_SAVE_RETRY_BASE_SECONDS 60U
#define SHOPKEEPER_SAVE_RETRY_MAX_SECONDS 900U

struct shopkeeper_save_retry_state
{
	unsigned int failure_count;
	time_t next_retry_at;
};

static inline void shopkeeper_save_retry_reset(struct shopkeeper_save_retry_state *state)
{
	if (!state)
		return;
	state->failure_count = 0;
	state->next_retry_at = 0;
}

static inline unsigned int shopkeeper_save_retry_delay_seconds(unsigned int failure_count)
{
	if (failure_count == 0)
		return 0;

	unsigned int delay = SHOPKEEPER_SAVE_RETRY_BASE_SECONDS;
	for (unsigned int step = 1; step < failure_count; ++step)
	{
		if (delay >= SHOPKEEPER_SAVE_RETRY_MAX_SECONDS / 2)
			return SHOPKEEPER_SAVE_RETRY_MAX_SECONDS;
		delay *= 2;
	}
	return delay;
}

static inline bool shopkeeper_save_retry_due(const struct shopkeeper_save_retry_state *state,
					     time_t now, bool force)
{
	if (!state || force)
		return true;
	return state->next_retry_at <= now;
}

static inline void shopkeeper_save_retry_record_failure(struct shopkeeper_save_retry_state *state,
							time_t now)
{
	if (!state)
		return;
	if (state->failure_count < UINT_MAX)
		++state->failure_count;
	if (now < 0)
		now = 0;
	state->next_retry_at =
		now + (time_t)shopkeeper_save_retry_delay_seconds(state->failure_count);
}

#endif
