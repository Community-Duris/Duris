#include "world/zone_story_quest_feature.h"
#include <cassert>
#include <cstdlib>
#include <new>
#include <iostream>
static size_t remaining;
static bool hit;
void *operator new(size_t size)
{
	if (remaining && --remaining == 0)
	{
		hit = true;
		throw std::bad_alloc();
	}
	if (void *p = std::malloc(size ? size : 1))
		return p;
	throw std::bad_alloc();
}
void *operator new[](size_t size)
{
	return ::operator new(size);
}
void operator delete(void *p) noexcept
{
	std::free(p);
}
void operator delete[](void *p) noexcept
{
	std::free(p);
}
void operator delete(void *p, size_t) noexcept
{
	std::free(p);
}
void operator delete[](void *p, size_t) noexcept
{
	std::free(p);
}
int main()
{
	zone_story_quest_tracking::quest_definition definition{
		.definition_id = "zone-story:900:001",
		.source_system = "zone_story",
		.zone_number = 900,
		.source_area = "production-qst",
		.giver_vnum = 90001,
		.completion_key = "zone-story:900:001",
		.active = true,
		.eligible_for_zone_completion = true,
		.repeatable = true,
		.content_revision = 7,
		.display_name = "Quest",
		.giver_name = "Giver",
		.zone_name = "Zone",
		.objective = "Objective"
	};
	zone_story_quest_catalog::catalog catalog{ .content_revision = 7,
						   .definitions = { definition } };
	zone_story_quest_feature::service state(catalog);
	for (unsigned pid = 1; pid < 30; ++pid)
		state.remember_character(1, pid, std::string(128, 'x'));
	const auto healthy = state.serialize_state();
	assert(healthy.starts_with("ZSQF|1\n") && healthy.size() > 29 * 128);
	size_t injected_failures = 0;
	size_t stream_failure = 0;
	bool completed = false;
	for (size_t index = 1; index < 500; ++index)
	{
		remaining = index;
		hit = false;
		std::string encoded;
		try
		{
			encoded = state.serialize_state();
		}
		catch (const std::bad_alloc &)
		{
			remaining = 0;
			++injected_failures;
			continue;
		}
		remaining = 0;
		if (hit)
		{
			++injected_failures;
			if (encoded.empty() && !stream_failure)
				stream_failure = index;
		}
		if (hit && !encoded.empty() && encoded != healthy)
		{
			std::cerr
				<< "RED: production zone-story serializer returned partial personal state after allocation failure\n";
			return 1;
		}
		if (!hit)
		{
			assert(encoded == healthy);
			completed = true;
			break;
		}
	}
	assert(injected_failures > 0 && completed && stream_failure);
	// A refused rollback capture must not begin a quest completion mutation.
	zone_story_quest_feature::completion_event event;
	event.transaction = {
		.schema_version =
			zone_story_quest_tracking::ZONE_STORY_QUEST_TRACKING_SCHEMA_VERSION,
		.transaction_id = "tx-allocation",
		.quest_definition_id = definition.definition_id,
		.zone_number = 900,
		.direct_completer_pid = 1,
		.credited_pids = { 1 },
		.room_vnum = 90010,
		.completed_at = 172800100,
		.season_id = 1,
		.content_revision = 7
	};
	event.character_name = "Player";
	remaining = stream_failure;
	hit = false;
	const auto refused = state.record_completion(event);
	remaining = 0;
	assert(hit && refused == zone_story_quest_feature::result::rejected);
	assert(state.serialize_state() == healthy);
	assert(state.record_completion(event) == zone_story_quest_feature::result::applied);
	assert(state.serialize_state() != healthy);
	std::cout << "PASS: serialization allocation failures cannot return partial state\n";
}