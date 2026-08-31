#include "world/epic_bonus_state.h"

#include <limits.h>
#include <math.h>
#include <string.h>

static int64_t saturating_add(int64_t left, int64_t right)
{
	if (right > 0 && left > INT64_MAX - right)
		return INT64_MAX;
	if (right < 0 && left < INT64_MIN - right)
		return INT64_MIN;
	return left + right;
}

void epic_bonus_state_reset(struct EpicBonusState *state)
{
	if (!state)
		return;
	memset(state, 0, sizeof(*state));
}

void epic_bonus_state_mark_unavailable(struct EpicBonusState *state)
{
	epic_bonus_state_reset(state);
	if (state)
		state->status = EPIC_BONUS_STATE_UNAVAILABLE;
}

void epic_bonus_state_expire(struct EpicBonusState *state, time_t now)
{
	if (!state || state->status != EPIC_BONUS_STATE_READY || now < 0)
		return;

	int first_active = 0;
	while (first_active < state->bucket_count && state->buckets[first_active].expires_at <= now)
	{
		state->total_contributions = saturating_add(state->total_contributions,
							    -state->buckets[first_active].amount);
		first_active++;
	}

	if (first_active > 0)
	{
		state->bucket_count -= first_active;
		memmove(state->buckets, state->buckets + first_active,
			state->bucket_count * sizeof(state->buckets[0]));
		memset(state->buckets + state->bucket_count, 0,
		       first_active * sizeof(state->buckets[0]));
	}

	if (state->total_contributions < 0)
		state->total_contributions = 0;
	state->next_expiry = state->bucket_count > 0 ? state->buckets[0].expires_at : 0;
}

bool epic_bonus_state_publish(struct EpicBonusState *state, int selected_type, time_t selected_at,
			      double contribution_cap, double maximum_modifier,
			      const struct EpicBonusContributionBucket *buckets, int bucket_count,
			      time_t now)
{
	if (!state || selected_type < 0 || selected_type > EPIC_BONUS_STATE_MAX_TYPE ||
	    selected_at < 0 || !isfinite(contribution_cap) || contribution_cap <= 0.0 ||
	    !isfinite(maximum_modifier) || maximum_modifier < 0.0 || bucket_count < 0 ||
	    bucket_count > EPIC_BONUS_STATE_MAX_BUCKETS || (bucket_count > 0 && !buckets))
		return false;

	struct EpicBonusState candidate = {};
	candidate.status = EPIC_BONUS_STATE_READY;
	candidate.selected_type = selected_type;
	candidate.selected_at = selected_at;
	candidate.contribution_cap = contribution_cap;
	candidate.maximum_modifier = maximum_modifier;

	for (int i = 0; i < bucket_count; i++)
	{
		if (buckets[i].amount <= 0 || buckets[i].expires_at <= 0 ||
		    (i > 0 && buckets[i - 1].expires_at >= buckets[i].expires_at))
			return false;
		candidate.buckets[candidate.bucket_count++] = buckets[i];
		candidate.total_contributions =
			saturating_add(candidate.total_contributions, buckets[i].amount);
	}

	epic_bonus_state_expire(&candidate, now);
	*state = candidate;
	return true;
}

bool epic_bonus_state_select(struct EpicBonusState *state, int selected_type, time_t selected_at,
			     double contribution_cap, double maximum_modifier)
{
	return epic_bonus_state_publish(state, selected_type, selected_at, contribution_cap,
					maximum_modifier, NULL, 0, selected_at);
}

bool epic_bonus_state_add(struct EpicBonusState *state, int64_t amount, time_t expires_at,
			  time_t now)
{
	if (!state || state->status != EPIC_BONUS_STATE_READY || state->selected_type == 0 ||
	    amount <= 0 || expires_at <= now)
		return false;

	epic_bonus_state_expire(state, now);
	int insert_at = 0;
	while (insert_at < state->bucket_count && state->buckets[insert_at].expires_at < expires_at)
		insert_at++;

	if (insert_at < state->bucket_count && state->buckets[insert_at].expires_at == expires_at)
	{
		const int64_t previous = state->buckets[insert_at].amount;
		state->buckets[insert_at].amount = saturating_add(previous, amount);
		state->total_contributions = saturating_add(
			state->total_contributions, state->buckets[insert_at].amount - previous);
		return true;
	}

	if (state->bucket_count >= EPIC_BONUS_STATE_MAX_BUCKETS)
		return false;

	memmove(state->buckets + insert_at + 1, state->buckets + insert_at,
		(state->bucket_count - insert_at) * sizeof(state->buckets[0]));
	state->buckets[insert_at].expires_at = expires_at;
	state->buckets[insert_at].amount = amount;
	state->bucket_count++;
	state->total_contributions = saturating_add(state->total_contributions, amount);
	state->next_expiry = state->buckets[0].expires_at;
	return true;
}

double epic_bonus_state_modifier(struct EpicBonusState *state, int requested_type, time_t now)
{
	if (!state || state->status != EPIC_BONUS_STATE_READY || requested_type <= 0)
		return 0.0;

	epic_bonus_state_expire(state, now);
	if (requested_type != state->selected_type)
		return 0.0;
	if (state->total_contributions <= 0 || state->contribution_cap <= 0.0 ||
	    state->maximum_modifier <= 0.0)
		return 0.0;

	double contributions = (double)state->total_contributions;
	if (contributions > state->contribution_cap)
		contributions = state->contribution_cap;
	return (contributions / state->contribution_cap) * state->maximum_modifier;
}
