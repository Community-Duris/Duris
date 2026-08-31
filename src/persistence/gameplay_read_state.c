#include "persistence/gameplay_read_state.h"

#include <algorithm>
#include <climits>
#include <cstring>

namespace
{
bool death_before(const gameplay_recent_death &left, const gameplay_recent_death &right)
{
	if (left.occurred_at != right.occurred_at)
		return left.occurred_at > right.occurred_at;
	return left.token > right.token;
}

void prune_durable(gameplay_read_state *state)
{
	if (!state)
		return;
	size_t durable = 0;
	size_t output = 0;
	for (size_t index = 0; index < state->recent_death_count; ++index)
	{
		const gameplay_recent_death entry = state->recent_deaths[index];
		if (!entry.provisional && durable++ >= GAMEPLAY_READ_RECENT_DURABLE_MAX)
			continue;
		state->recent_deaths[output++] = entry;
	}
	memset(state->recent_deaths + output, 0,
	       (GAMEPLAY_READ_RECENT_MAX - output) * sizeof(state->recent_deaths[0]));
	state->recent_death_count = output;
}
}

void gameplay_read_state_reset(gameplay_read_state *state)
{
	if (state)
		memset(state, 0, sizeof(*state));
}

bool gameplay_read_state_publish(gameplay_read_state *state, const int64_t *recent_deaths,
				 size_t recent_death_count, const int32_t *completed_zones,
				 size_t completed_zone_count)
{
	if (!state || recent_death_count > GAMEPLAY_READ_RECENT_DURABLE_MAX ||
	    completed_zone_count > GAMEPLAY_READ_COMPLETED_ZONE_MAX ||
	    (recent_death_count && !recent_deaths) || (completed_zone_count && !completed_zones))
		return false;
	gameplay_read_state candidate = {};
	candidate.status = gameplay_read_status::ready;
	candidate.next_token = 1;
	for (size_t index = 0; index < recent_death_count; ++index)
	{
		if (recent_deaths[index] <= 0 ||
		    (index && recent_deaths[index - 1] < recent_deaths[index]))
			return false;
		candidate.recent_deaths[index] = { recent_deaths[index], 0, false };
	}
	candidate.recent_death_count = recent_death_count;
	for (size_t index = 0; index < completed_zone_count; ++index)
	{
		if (completed_zones[index] <= 0 ||
		    (index && completed_zones[index - 1] >= completed_zones[index]))
			return false;
		candidate.completed_zones[index] = completed_zones[index];
	}
	candidate.completed_zone_count = completed_zone_count;
	*state = candidate;
	return true;
}

size_t gameplay_read_state_recent_count(const gameplay_read_state *state, int64_t now)
{
	if (!state || state->status != gameplay_read_status::ready || now <= 0)
		return 0;
	size_t inspected = 0;
	size_t recent = 0;
	for (size_t index = 0;
	     index < state->recent_death_count && inspected < GAMEPLAY_READ_RECENT_DURABLE_MAX;
	     ++index, ++inspected)
		if (state->recent_deaths[index].occurred_at <= now &&
		    now - state->recent_deaths[index].occurred_at <
			    GAMEPLAY_READ_RECENT_WINDOW_SECONDS)
			++recent;
	return recent;
}

bool gameplay_read_state_heaven_seconds(const gameplay_read_state *state, int64_t now,
					int base_seconds, int *seconds)
{
	if (!state || state->status != gameplay_read_status::ready || now <= 0 ||
	    base_seconds <= 0 || !seconds)
		return false;
	int64_t duration = base_seconds;
	const size_t recent_deaths = gameplay_read_state_recent_count(state, now);
	for (size_t count = 1; count < recent_deaths; ++count)
	{
		if (duration > INT_MAX / 2)
			return false;
		duration *= 2;
	}
	if (duration > 30 * 60)
	{
		if (duration > INT_MAX / 24)
			return false;
		duration *= 24;
	}
	*seconds = static_cast<int>(duration);
	return true;
}

uint64_t gameplay_read_state_add_provisional(gameplay_read_state *state, int64_t occurred_at)
{
	if (!state || state->status != gameplay_read_status::ready || occurred_at <= 0 ||
	    state->recent_death_count >= GAMEPLAY_READ_RECENT_MAX)
		return 0;
	uint64_t token = state->next_token++;
	if (!token)
		token = state->next_token++;
	gameplay_recent_death entry = { occurred_at, token, true };
	size_t at = 0;
	while (at < state->recent_death_count && death_before(state->recent_deaths[at], entry))
		++at;
	memmove(state->recent_deaths + at + 1, state->recent_deaths + at,
		(state->recent_death_count - at) * sizeof(state->recent_deaths[0]));
	state->recent_deaths[at] = entry;
	++state->recent_death_count;
	return token;
}

bool gameplay_read_state_finish_provisional(gameplay_read_state *state, uint64_t token,
					    bool committed)
{
	if (!state || state->status != gameplay_read_status::ready || !token)
		return false;
	for (size_t index = 0; index < state->recent_death_count; ++index)
		if (state->recent_deaths[index].provisional &&
		    state->recent_deaths[index].token == token)
		{
			if (committed)
			{
				state->recent_deaths[index].provisional = false;
				prune_durable(state);
			}
			else
			{
				memmove(state->recent_deaths + index,
					state->recent_deaths + index + 1,
					(state->recent_death_count - index - 1) *
						sizeof(state->recent_deaths[0]));
				--state->recent_death_count;
				state->recent_deaths[state->recent_death_count] = {};
			}
			return true;
		}
	return false;
}

bool gameplay_read_state_zone_completed(const gameplay_read_state *state, int32_t zone_number)
{
	if (!state || state->status != gameplay_read_status::ready || zone_number <= 0)
		return false;
	return std::binary_search(state->completed_zones,
				  state->completed_zones + state->completed_zone_count,
				  zone_number);
}

bool gameplay_read_state_add_completed_zone(gameplay_read_state *state, int32_t zone_number)
{
	if (!state || state->status != gameplay_read_status::ready || zone_number <= 0)
		return false;
	auto *begin = state->completed_zones;
	auto *end = begin + state->completed_zone_count;
	auto *at = std::lower_bound(begin, end, zone_number);
	if (at != end && *at == zone_number)
		return true;
	if (state->completed_zone_count >= GAMEPLAY_READ_COMPLETED_ZONE_MAX)
		return false;
	memmove(at + 1, at, static_cast<size_t>(end - at) * sizeof(*at));
	*at = zone_number;
	++state->completed_zone_count;
	return true;
}
