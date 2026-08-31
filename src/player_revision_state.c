#include "player_revision_state.h"

#include <array>
#include <limits>
#include <new>
#include <unordered_map>

namespace
{
constexpr size_t MAX_PLAYER_REVISION_STATES = 8192;
constexpr size_t CHECKPOINT_COMPONENT_COUNT = 14;

struct player_revision_entry
{
	player_revision_t current_revision = 0;
	player_revision_t acknowledged_revision = 0;
	player_revision_t queued_revision = 0;
	player_revision_t inflight_revision = 0;
	player_component_mask_t dirty_components = 0;
	player_component_mask_t unacknowledged_components = 0;
	player_component_mask_t queued_components = 0;
	player_component_mask_t inflight_components = 0;
	std::array<player_revision_t, CHECKPOINT_COMPONENT_COUNT> component_revisions = {};
	bool overflowed = false;
};

std::unordered_map<int, player_revision_entry> revision_states;

bool valid_components(player_component_mask_t components)
{
	return components && !(components & ~PLAYER_CHECKPOINT_COMPONENT_ALL);
}

player_revision_entry *find_state(int pid)
{
	auto found = revision_states.find(pid);
	return found == revision_states.end() ? nullptr : &found->second;
}

void clear_acknowledged_components(player_revision_entry &state, player_revision_t revision,
				   player_component_mask_t components)
{
	for (size_t index = 0; index < CHECKPOINT_COMPONENT_COUNT; ++index)
	{
		const player_component_mask_t component = UINT64_C(1) << index;
		if ((components & component) && state.component_revisions[index] <= revision)
		{
			state.unacknowledged_components &= ~component;
			state.dirty_components &= ~component;
			state.queued_components &= ~component;
		}
	}
}
} // namespace

bool player_revision_hydrate(int pid, player_revision_t durable_revision)
{
	if (pid <= 0)
		return false;

	player_revision_entry *state = find_state(pid);
	if (state)
	{
		if (state->unacknowledged_components || state->queued_components ||
		    state->inflight_components)
			return durable_revision == state->acknowledged_revision;
		if (durable_revision < state->acknowledged_revision)
			return false;
		state->current_revision = durable_revision;
		state->acknowledged_revision = durable_revision;
		state->component_revisions.fill(durable_revision);
		return true;
	}

	if (revision_states.size() >= MAX_PLAYER_REVISION_STATES)
		return false;

	try
	{
		player_revision_entry entry;
		entry.current_revision = durable_revision;
		entry.acknowledged_revision = durable_revision;
		entry.component_revisions.fill(durable_revision);
		revision_states.emplace(pid, entry);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool player_revision_mark(int pid, player_component_mask_t components,
			  player_revision_t *revision_out)
{
	player_revision_entry *state = find_state(pid);
	if (!state || !valid_components(components) || state->overflowed)
		return false;
	if (state->current_revision == std::numeric_limits<player_revision_t>::max())
	{
		state->overflowed = true;
		return false;
	}

	++state->current_revision;
	state->dirty_components |= components;
	state->unacknowledged_components |= components;
	for (size_t index = 0; index < CHECKPOINT_COMPONENT_COUNT; ++index)
	{
		if (components & (UINT64_C(1) << index))
			state->component_revisions[index] = state->current_revision;
	}
	if (revision_out)
		*revision_out = state->current_revision;
	return true;
}

bool player_revision_queue(int pid, player_revision_t *revision_out,
			   player_component_mask_t *components_out)
{
	player_revision_entry *state = find_state(pid);
	if (!state || !state->unacknowledged_components)
		return false;

	state->queued_revision = state->current_revision;
	state->queued_components = state->unacknowledged_components;
	state->dirty_components = 0;
	if (revision_out)
		*revision_out = state->queued_revision;
	if (components_out)
		*components_out = state->queued_components;
	return true;
}

bool player_revision_begin_inflight(int pid, player_revision_t revision,
				    player_component_mask_t components)
{
	player_revision_entry *state = find_state(pid);
	if (!state || state->inflight_components || revision != state->queued_revision ||
	    components != state->queued_components || !valid_components(components))
		return false;

	state->inflight_revision = revision;
	state->inflight_components = components;
	state->queued_revision = 0;
	state->queued_components = 0;
	return true;
}

bool player_revision_acknowledge(int pid, player_revision_t revision,
				 player_component_mask_t components)
{
	player_revision_entry *state = find_state(pid);
	if (!state || revision != state->inflight_revision ||
	    components != state->inflight_components || !valid_components(components))
		return false;

	clear_acknowledged_components(*state, revision, components);
	if (revision > state->acknowledged_revision)
		state->acknowledged_revision = revision;
	state->inflight_revision = 0;
	state->inflight_components = 0;
	return true;
}

bool player_revision_acknowledge_durable(int pid, player_revision_t revision,
					 player_component_mask_t components)
{
	player_revision_entry *state = find_state(pid);
	if (!state || !valid_components(components) || revision < state->acknowledged_revision ||
	    revision > state->current_revision)
		return false;

	const player_component_mask_t before = state->unacknowledged_components;
	clear_acknowledged_components(*state, revision, components);
	const player_component_mask_t cleared = before & ~state->unacknowledged_components;
	state->inflight_components &= ~cleared;
	if (!state->queued_components)
		state->queued_revision = 0;
	if (!state->inflight_components)
		state->inflight_revision = 0;
	if (revision > state->acknowledged_revision)
		state->acknowledged_revision = revision;
	return true;
}

bool player_revision_fail_inflight(int pid, player_revision_t revision,
				   player_component_mask_t components)
{
	player_revision_entry *state = find_state(pid);
	if (!state || revision != state->inflight_revision ||
	    components != state->inflight_components || !valid_components(components))
		return false;

	state->inflight_revision = 0;
	state->inflight_components = 0;
	state->queued_revision = state->current_revision;
	state->queued_components = state->unacknowledged_components;
	return true;
}

bool player_revision_snapshot_copy(int pid, struct player_revision_snapshot *snapshot_out)
{
	const player_revision_entry *state = find_state(pid);
	if (!state || !snapshot_out)
		return false;

	*snapshot_out = {
		.pid = pid,
		.current_revision = state->current_revision,
		.acknowledged_revision = state->acknowledged_revision,
		.queued_revision = state->queued_revision,
		.inflight_revision = state->inflight_revision,
		.dirty_components = state->dirty_components,
		.unacknowledged_components = state->unacknowledged_components,
		.queued_components = state->queued_components,
		.inflight_components = state->inflight_components,
		.overflowed = state->overflowed,
	};
	return true;
}

void player_revision_forget(int pid)
{
	if (pid > 0)
		revision_states.erase(pid);
}

void player_revision_reset_for_tests(void)
{
	revision_states.clear();
}

size_t player_revision_state_count(void)
{
	return revision_states.size();
}

size_t player_revision_dirty_count(void)
{
	size_t count = 0;
	for (const auto &[pid, state] : revision_states)
	{
		(void)pid;
		if (state.unacknowledged_components)
			++count;
	}
	return count;
}
