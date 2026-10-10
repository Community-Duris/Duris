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
	uint64_t repaired_mundane_handles;
	uint64_t topology_changes;
};

void world_activity_reload();
void world_activity_rebuild();
void world_activity_log_diagnostics();
world_activity_health world_activity_get_health();

bool world_activity_is_enabled();
world_activity_tier world_activity_tier_for_room(int room);

/* Character-room hooks.  These are game-thread-only lifecycle notifications. */
// Private projection reconstruction; original birth/room owners supply authority.
class quest_mobile_native_room_restore_owner;
class world_activity_native_birth_restore_owner
{
	static bool enter(P_char) noexcept;
	static bool enter_bounded(P_char, bool (*)(size_t, void *) noexcept, void *,
				  size_t) noexcept;
	friend class quest_mobile_native_room_restore_owner;
};

void world_activity_character_enter(P_char ch);
void world_activity_character_leave(P_char ch);
void world_activity_player_enter(P_char ch);
void world_activity_player_leave(P_char ch);
void world_activity_promote_character(P_char ch);
/* Call after publishing a live exit change. Regions stay stable until rebuild. */
void world_activity_room_exits_changed(int room);

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

// Serialized actual owning activity registries/configuration/counters and all
// current nested heap requests under GCC13/C++11 ABI. World/game objects and
// scheduler/output storage excluded. Strong output, no allocation or authority.
// Active native caller includes this once and refreshes CURRENT on every return;
// observer alone never admits an original allocating activity notification.
bool world_activity_storage_bytes(size_t *) noexcept;

// Complete original object-connection/subtree/corpse/topology/wakeup provider.
// Outer includes CURRENT activity/pending-deferred/output storage exactly once,
// plus live caller inputs. Refresh all CURRENT owners on EVERY return, including
// partial mutation/refusal. Native effect must be marked started before invocation;
// never retry already-effected placement or RNG/advances on a false return.
bool world_activity_object_enter_bounded(P_obj, bool (*)(size_t, void *) noexcept, void *,
					 size_t outer_live) noexcept;

// Full original character NPC/control/corpse activity; same actual global
// allocation helpers and freshly observed activity/pending/output retention.
// No substitute object-connection notification. Partial refusal never retries
// an already effected room or activity action. Strong preaction markers.
bool world_activity_character_enter_bounded(P_char, bool *, bool *,
					    bool (*)(size_t, void *) noexcept, void *,
					    size_t) noexcept;
// Original validated cached/fallback event, original existing-event reschedule
// or actual character scheduler. Actual returned handoff survives diagnostics.
bool world_activity_schedule_mundane_after_bounded(P_char, int, bool *, bool *,
						   bool (*)(size_t, void *) noexcept, void *,
						   size_t) noexcept;

#endif /* DURIS_WORLD_ACTIVITY_H */
