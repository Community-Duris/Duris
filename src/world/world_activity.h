/*
 * World activity policy for ordinary mobile work.
 *
 * The policy deliberately lives outside mobact.c so that player/corpse
 * lifecycle events can promote a zone without scanning the whole character
 * list.  Combat, special procedures, patrols, and other timing-sensitive
 * work remain on their existing schedules.
 */

#ifndef DURIS_WORLD_ACTIVITY_H
#define DURIS_WORLD_ACTIVITY_H

#include "core/structs.h"

#include <cstdint>

enum class world_activity_tier : uint8_t
{
	active,
	nearby,
	distant
};

struct world_activity_health
{
	bool enabled;
	bool ready;
	uint64_t indexed_npcs;
	uint64_t player_reasons;
	uint64_t corpse_reasons;
	uint64_t adjacent_reasons;
	uint64_t wake_promotions;
	uint64_t wake_events;
	uint64_t stale_wake_handles;
};

void world_activity_reload();
void world_activity_rebuild();
void world_activity_log_diagnostics();
world_activity_health world_activity_get_health();

bool world_activity_is_enabled();
world_activity_tier world_activity_tier_for_room(int room);

/* Character-room hooks.  These are game-thread-only lifecycle notifications. */
void world_activity_character_enter(P_char ch);
void world_activity_character_leave(P_char ch);
void world_activity_player_enter(P_char ch);
void world_activity_player_leave(P_char ch);
void world_activity_promote_character(P_char ch);

/* The ordinary mobile event is cached on the character with a scheduler
 * sequence so zone wakeups do not scan the full character event list. */
void world_activity_record_mundane_event(P_char ch, nevent_handle event);
nevent_handle world_activity_mundane_event(P_char ch);

/* Object lifecycle hooks cover nested PC corpses as one committed subtree. */
void world_activity_object_enter(P_obj object);
void world_activity_object_leave(P_obj object);

/* The mundane callback uses these helpers for its normal/quick reschedules. */
bool world_activity_mob_is_timing_sensitive(P_char ch);
int world_activity_mundane_delay(P_char ch, bool quick_retry, bool legacy_zone_occupied);
void world_activity_schedule_mundane(P_char ch, bool quick_retry, bool legacy_zone_occupied);
void world_activity_schedule_mundane_after(P_char ch, int delay);

#endif /* DURIS_WORLD_ACTIVITY_H */
