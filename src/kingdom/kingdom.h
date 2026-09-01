/*
 *  kingdom.h
 *  Duris
 *
 *  THE SEAM. This is the only kingdom header any file outside src/kingdom/
 *  may include, and everything below is the whole surface the rest of the
 *  server is allowed to touch. Internals live in kingdom_internal.h.
 *
 *  WHAT A KINGDOM IS
 *  -----------------
 *  A guild that has converted to a kingdom owns map squares in four
 *  concentric rings around its guildhall entrance -- 8, 16, 24 then 32
 *  squares, 80 in total. A ring must be completed before the next opens, and
 *  within a ring the squares are claimed in a fixed clockwise order starting
 *  due north. The geometry lives in kingdom_geometry.h.
 *
 *  BECAUSE THAT ORDER IS FIXED, a realm's territory is described by a SINGLE
 *  INTEGER: the highest claim index it holds. Squares 1..highest_claim are
 *  owned and nothing else is. There is no per-square ownership table, no
 *  80-row write per realm, and reverting a ring for unpaid upkeep is just
 *  lowering that integer to a ring boundary.
 *
 *  ENABLEMENT
 *  ----------
 *  The subsystem is gated at runtime by lib/kingdom.cfg (kingdom.enabled),
 *  not by a compile flag. With it off, kingdom_enabled() is false, every
 *  command answers "kingdoms are not enabled", and -- importantly --
 *  kingdom_footprint_check() permits everything, so guildhall placement
 *  behaves exactly as it did before this module existed.
 */

#ifndef _KINGDOM_H_
#define _KINGDOM_H_

#include <cstddef> /* size_t, for the refusal-reason buffer below */

/* Forward declarations only: this header must stay cheap to include and must
 * not drag the guild or world headers into unrelated translation units. */
struct char_data;

/* ------------------------------------------------------------------ *
 * Lifecycle -- called from the boot and shutdown paths
 * ------------------------------------------------------------------ */

/* Load config and persisted realms, and build the in-memory index. Safe to
 * call when the feature is disabled: it reads the config, logs, and returns. */
void kingdom_initialize(void);

/* Release everything. Idempotent. */
void kingdom_shutdown(void);

/* True when lib/kingdom.cfg enabled the subsystem. Every public entry point
 * below is a no-op or a permissive answer when this is false. */
bool kingdom_enabled(void);

/* ------------------------------------------------------------------ *
 * Queries the rest of the server asks
 * ------------------------------------------------------------------ */

/* The association id owning the map square this room is, or 0 for none.
 * O(1): the module keeps a vnum-keyed index rather than scanning realms,
 * because this is called from movement and from the map renderer. */
int kingdom_owner_of_room(int rnum);

/* True when `ch` belongs to the guild that owns this room's square. */
bool kingdom_char_owns_room(struct char_data *ch, int rnum);

/* One line for the map legend / room description, or NULL when the square is
 * unowned. The returned pointer is owned by the module and must not be freed
 * or stored across a tick. */
const char *kingdom_room_banner(int rnum);

/* ------------------------------------------------------------------ *
 * The guildhall placement gate
 * ------------------------------------------------------------------ */

/* May a guildhall be sited on this map room, given the whole 9x9 footprint it
 * would need as a kingdom?
 *
 * Ruled 2026-08-28: ALL 80 squares must be eligible AT PLACEMENT, so a hall
 * can only be built where a complete realm could later exist. Returns true to
 * permit. On refusal, `why` (if given) receives a player-facing reason; the
 * buffer must be at least KINGDOM_WHY_LEN bytes.
 *
 * Returns TRUE unconditionally when the subsystem is disabled, so existing
 * guildhall behaviour is untouched unless kingdoms are switched on. */
#define KINGDOM_WHY_LEN 256
bool kingdom_footprint_check(int hall_rnum, int racewar, char *why, size_t why_len);

/* Minimum separation between two guildhall centres, in map squares, so that
 * two 9x9 footprints cannot even touch. Exposed for the guildhall subsystem's
 * own messaging. */
int kingdom_min_hall_separation(void);

/* ------------------------------------------------------------------ *
 * Hooks the rest of the server must call
 * ------------------------------------------------------------------ */

/* A guild is being deleted: drop its realm, despawn its guards, and release
 * its squares from the index. Safe to call for a guild that has no realm.
 *
 * MUST be called before the Guild object is freed. Guild ids are reused --
 * found_asc() hands out the lowest free id -- so a realm left behind would be
 * silently inherited by the next guild to take that id. */
void kingdom_on_guild_deleted(int assoc_id);

/* A guildhall moved or was destroyed; the realm's anchor is no longer valid. */
void kingdom_on_guildhall_changed(int assoc_id);

/* The periodic upkeep charge and the arrears ladder. Registered once from
 * ne_init_events(); not called from anywhere else. */
void kingdom_upkeep_event(void);

/* ------------------------------------------------------------------ *
 * The command
 * ------------------------------------------------------------------ */

/* `kingdom <subcommand>`. Registered in the interpreter's command table.
 * Every verb is a subcommand because `claim` and `harvest` are already taken
 * as dead CMD_TRIG stubs bound to do_not_here. */
void do_kingdom(struct char_data *ch, char *argument, int cmd);

#endif /* _KINGDOM_H_ */
