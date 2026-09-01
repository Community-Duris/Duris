/*
 *  kingdom_internal.h
 *  Duris
 *
 *  Private to src/kingdom/. Nothing outside this directory may include it --
 *  the public surface is kingdom.h and it is deliberately small.
 */

#ifndef _KINGDOM_INTERNAL_H_
#define _KINGDOM_INTERNAL_H_

#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>

#include "kingdom/kingdom.h"
#include "kingdom/kingdom_geometry.h"

#define LOG_KINGDOM "logs/log/kingdom"

/* ------------------------------------------------------------------ *
 * Resources
 * ------------------------------------------------------------------ *
 * Typed, non-withdrawable quantities held by the realm. Ruled 2026-08-28:
 * once deposited these can only be SPENT on kingdom benefits -- there is
 * deliberately no withdraw verb, which is why they cannot live in the guild
 * coin treasury (Guild::withdraw is officer-accessible).
 *
 * Upkeep is charged in COIN, not in these.
 */
enum kingdom_resource {
	KRES_MINERAL = 0,
	KRES_WOOD,
	KRES_FIBRE,
	KRES_WATER,
	KRES_MAX
};

const char *kingdom_resource_name(int res);

/* ------------------------------------------------------------------ *
 * Arrears ladder
 * ------------------------------------------------------------------ *
 * Ruled 2026-08-28, in this order, halting wherever payment arrives:
 *   0  current
 *   1  guards despawn
 *   2  harvest nodes go dormant
 *   3+ one OUTER ring reverts per missed cycle
 * Reclaiming a reverted ring costs the full original price again.
 */
enum kingdom_arrears {
	KARR_CURRENT = 0,
	KARR_GUARDS_GONE,
	KARR_NODES_DORMANT,
	KARR_RINGS_REVERTING
};

/* ------------------------------------------------------------------ *
 * The realm
 * ------------------------------------------------------------------ *
 * THE TERRITORY IS ONE INTEGER. Because claims are strictly ordered and a
 * ring completes before the next opens, a realm owns exactly claim indices
 * 1..highest_claim and nothing else. That is the entire ownership record:
 * no per-square rows, no 80-row rewrite when a guard dies, and reverting a
 * ring is just lowering highest_claim to a ring boundary.
 */
struct kingdom_realm {
	int realm_id = 0;
	int assoc_id = 0;

	/* Anchor. hall_vnum is persisted; the rest is resolved at boot and after
	 * any guildhall move, and left invalid when the hall cannot be found. */
	int hall_vnum = 0;
	int hall_rnum = 0; /* 0 = unresolved, matching real_room0's sentinel */
	int zone_idx = -1;
	int hall_x = -1;
	int hall_y = -1;

	int highest_claim = 0; /* 0..KINGDOM_MAX_SQUARES */

	long resources[KRES_MAX] = {0, 0, 0, 0};

	time_t upkeep_paid_through = 0;
	int arrears = KARR_CURRENT;
	int missed_cycles = 0;

	bool dirty = false;
};

/* ------------------------------------------------------------------ *
 * Module state
 * ------------------------------------------------------------------ */

/* Realms by association id. */
extern std::unordered_map<int, kingdom_realm> kingdom_realms;

/* Owned map ROOM VNUM -> association id. Rebuilt whenever a realm's anchor or
 * highest_claim changes. This exists because kingdom_owner_of_room() is called
 * from movement and from the map renderer, where an O(realms * 80) scan would
 * be paid on every step -- the guildhall subsystem's plain vector with linear
 * finders is exactly the shape not to copy here. */
extern std::unordered_map<int, int> kingdom_square_index;

kingdom_realm *kingdom_find_realm(int assoc_id);
void kingdom_reindex_realm(const kingdom_realm &realm);
void kingdom_unindex_realm(int assoc_id);
void kingdom_reindex_all(void);

/* Resolve hall_rnum/zone/x/y from hall_vnum. False when the anchor is gone. */
bool kingdom_resolve_anchor(kingdom_realm &realm);

/* ------------------------------------------------------------------ *
 * Config (kingdom_config.c) -- lib/kingdom.cfg with compiled fallbacks
 * ------------------------------------------------------------------ */
struct kingdom_config {
	bool enabled = false;
	/* Coin cost of claim n, before any multiplier: base + per_square * n. */
	long claim_cost_base = 25000;
	long claim_cost_per_square = 5000;
	/* Coin upkeep per owned square per cycle. */
	long upkeep_per_square = 250;
	/* Real seconds between upkeep charges. */
	int upkeep_period_seconds = 3600;
	/* Guards permitted per owned square, and the ratio's denominator. */
	int guards_per_squares = 5;
	/* Distance in map squares a realm must keep from a hometown or a zone
	 * entrance. */
	int min_hometown_distance = 30;
	int min_entrance_distance = 30;
};
extern kingdom_config kingdom_cfg;
void kingdom_config_load(void);

/* ------------------------------------------------------------------ *
 * Placement (kingdom_placement.c)
 * ------------------------------------------------------------------ *
 * THE SINGLE AUTHORITY on "may this square be part of a realm". Every
 * display, every command and the guildhall gate ask THIS -- none of them
 * re-derives the rule, because two copies of an availability predicate drift
 * and the weaker one lies.
 */
enum kingdom_square_verdict {
	KSQ_OK = 0,
	KSQ_NO_ROOM,        /* sparse map: nothing behind the square */
	KSQ_OFF_GRID,       /* would leave the map zone */
	KSQ_BAD_SECTOR,     /* impassable / water / not settleable */
	KSQ_UNDERDARK,      /* absolute ban, ruled 2026-08-28 */
	KSQ_NEAR_HOMETOWN,
	KSQ_NEAR_ENTRANCE,
	KSQ_OTHER_REALM,    /* inside another realm's footprint */
	KSQ_HAS_GUILDHALL
};

const char *kingdom_verdict_text(int verdict);

/* Judge one square of a prospective realm anchored at hall_rnum.
 * `ignore_assoc` excludes a realm from the overlap test so a realm does not
 * collide with itself. */
int kingdom_judge_square(int hall_rnum, int index, int racewar, int ignore_assoc);

/* Judge the whole 80-square footprint. Returns KSQ_OK when every square
 * passes; otherwise the first failing verdict, with *bad_index set. */
int kingdom_judge_footprint(int hall_rnum, int racewar, int ignore_assoc, int *bad_index);

/* ------------------------------------------------------------------ *
 * Cross-file surface within src/kingdom/
 * ------------------------------------------------------------------ *
 * Every function one kingdom file calls in another is declared HERE. Callers
 * must not forward-declare a sibling locally: a local `extern` is a second
 * copy of the signature that drifts silently when the definition changes,
 * and the compiler cannot catch the mismatch across translation units.
 *
 * These carry no file:line references on purpose. Three lanes reported their
 * own line citations had already gone stale during authoring; a line number
 * in a comment is a second copy of a moving target.
 */

/* --- kingdom_claim.c : conversion and the claim/abandon state machine --- */
long kingdom_claim_cost(int index);
long kingdom_ring_cost(int ring);
bool kingdom_convert_guild(struct char_data *ch);
bool kingdom_claim_next(struct char_data *ch);
bool kingdom_abandon_last(struct char_data *ch);

/* --- kingdom_upkeep.c : the periodic charge and the arrears ladder --- */
long kingdom_upkeep_due(const kingdom_realm &realm);
void kingdom_apply_arrears(kingdom_realm &realm);
void kingdom_clear_arrears(kingdom_realm &realm);

/* --- kingdom_guards.c : the garrison --- */
int kingdom_guard_allowance(const kingdom_realm &realm);
int kingdom_guard_garrison(const kingdom_realm &realm);
int kingdom_guards_count(int assoc_id);
int kingdom_guards_despawn(int assoc_id);
int kingdom_guards_despawn_all(void);
int kingdom_guards_refresh(const kingdom_realm &realm);
void kingdom_guards_refresh_all(void);

/* --- kingdom_harvest.c : resource nodes on owned squares --- */
int kingdom_resource_for_room(int rnum);
bool kingdom_nodes_dormant(const kingdom_realm &realm);
long kingdom_resource_deposit(kingdom_realm &realm, int res, long amount);
void kingdom_harvest_command(struct char_data *ch, char *argument);
void kingdom_harvest_survey(struct char_data *ch);
void kingdom_harvest_prune(const kingdom_realm &realm);
void kingdom_harvest_release(int assoc_id);
void kingdom_harvest_shutdown(void);

/* --- kingdom_display.c : status, map overlay and the 9x9 ring grid --- *
 * kingdom_show_grid() takes a bare anchor rather than a realm so a prospective
 * site can be surveyed before any realm exists; pass highest_claim 0 then. */
void kingdom_show_status(struct char_data *ch, const kingdom_realm &realm);
void kingdom_show_map(struct char_data *ch, const kingdom_realm &realm);
void kingdom_show_grid(struct char_data *ch, int hall_rnum, int racewar, int ignore_assoc,
		       int highest_claim, const char *heading);

/* ------------------------------------------------------------------ *
 * Persistence (kingdom_db.c + flatfile_kingdom_repository.c)
 * ------------------------------------------------------------------ *
 * Dual backend on purpose: under the default mariadb build
 * persistence_mode_flatfile_root() returns NULL and no flat-file directories
 * are provisioned, so a flat-file-only store would silently not persist in
 * production.
 */
bool kingdom_db_load_all(void);
bool kingdom_db_save_realm(const kingdom_realm &realm);
bool kingdom_db_delete_realm(int assoc_id);
void kingdom_db_flush_dirty(void);

#endif /* _KINGDOM_INTERNAL_H_ */
