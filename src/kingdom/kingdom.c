/*
 *  kingdom.c
 *  Duris
 *
 *  Lifecycle, the module's two globals, and the authoritative square index.
 *
 *  THE INDEX IS THE POINT OF THIS FILE. A realm's territory is one integer --
 *  it owns claims 1..highest_claim and nothing else -- but
 *  kingdom_owner_of_room() is asked from movement and from the map renderer,
 *  so answering it by walking realms and re-deriving up to 80 offsets per step
 *  is not an option. Every mutation of a realm's anchor or of highest_claim
 *  therefore rebuilds a vnum-keyed map, and the query side is one hash lookup.
 *
 *  The index is keyed by room VNUM rather than rnum on purpose: rnums are
 *  positions in the world table and shift whenever the world is reloaded,
 *  vnums do not. kingdom_owner_of_room() converts with world[rnum].number,
 *  which is still O(1).
 *
 *  Nothing here decides WHETHER a square may be claimed. That is
 *  kingdom_judge_square() / kingdom_judge_footprint() in kingdom_placement.c,
 *  the single authority; this file only asks it. Two copies of an
 *  availability predicate drift, and the weaker one lies.
 */

#include "kingdom/kingdom_internal.h"

#include "core/structs.h"

#include "core/prototypes.h"
#include "core/utility.h"
#include "core/utils.h"
#include "guild/assocs.h"
#include "guild/guildhall.h"

#include <algorithm>
#include <vector>

extern struct room_data *world;
extern int top_of_world;

/* ------------------------------------------------------------------ *
 * The module's two globals
 * ------------------------------------------------------------------ *
 * Declared extern in kingdom_internal.h; this is their one definition.
 * kingdom_cfg belongs to kingdom_config.c, not here.
 */

std::unordered_map<int, kingdom_realm> kingdom_realms;
std::unordered_map<int, int> kingdom_square_index;

/* ------------------------------------------------------------------ *
 * Small local helpers
 * ------------------------------------------------------------------ */

/* True when rnum indexes a real room. rnum 0 is rejected on purpose: it is
 * both the first room and real_room0()'s "no such vnum" answer
 * (world/db.c:4477-4508), and every caller here means the latter. */
static bool kingdom_valid_rnum(int rnum)
{
	return rnum > 0 && rnum <= top_of_world;
}

/* Association ids are 1-based: found_asc() searches upwards from 1 for the
 * lowest free id (guild/assocs.c:350-354), so 0 is never a guild and is what
 * the public API returns for "nobody". */
static bool kingdom_valid_assoc(int assoc_id)
{
	return assoc_id > 0 && assoc_id < MAX_ASC;
}

/*
 * The guildhall a realm anchors on: the association's MAIN hall.
 *
 * Deliberately not Guildhall::find_by_assoc_id(): that one dereferences
 * gh->guild without checking it (guild/guildhall.c:140-141) and returns the
 * FIRST hall it finds for the association, which may be an outpost.
 *
 * Matched on the persisted assoc_id field rather than on gh->guild->get_id(),
 * because guild is DERIVED state -- Guildhall::reload() recomputes it as
 * get_guild_from_id(assoc_id) (guild/guildhall.c:91) and boot leaves it NULL
 * for any hall whose association is gone (guild/guildhall.c:51-56).
 *
 * GH_TYPE_MAIN only, with no outpost fallback: `kingdom convert` anchors a new
 * realm on the main hall (kingdom_claim.c:456), so an outpost that could never
 * have founded a realm must not silently become one's anchor either. Two
 * copies of a predicate drift, and the weaker one lies -- a review found
 * three copies of this one, and the copy in kingdom_cmds.c matched on
 * gh->guild->get_id(), so a hall whose guild pointer was NULL could found a
 * realm through `convert` yet show no land through `map`. This is now the
 * module's ONE finder, exported through kingdom_internal.h.
 */
Guildhall *kingdom_main_hall(int assoc_id)
{
	if (!kingdom_valid_assoc(assoc_id))
		return NULL;

	for (size_t i = 0; i < Guildhall::guildhalls.size(); i++)
	{
		Guildhall *gh = Guildhall::guildhalls[i];

		/* guildhalls[] is a plain vector that can hold NULLs. */
		if (!gh)
			continue;
		if (gh->assoc_id != assoc_id)
			continue;
		if (gh->type != GH_TYPE_MAIN)
			continue;

		return gh;
	}

	return NULL;
}

/* Are any associations loaded at all? kingdom_initialize() uses this to refuse
 * to prune "orphaned" realms when it has merely been called before the guild
 * list was read -- that mistake would delete every realm on the server. */
static bool kingdom_any_guild_loaded(void)
{
	for (int id = 1; id < MAX_ASC; id++)
		if (get_guild_from_id(id) != NULL)
			return true;
	return false;
}

/* The same shape, for the halls. "Not one hall on the server" is far more
 * likely to mean Guildhall::initialize() has not run yet than that every hall
 * was demolished, so the anchor audit in kingdom_reindex_all() is skipped in
 * that case: a too-early call must do LESS, not unanchor every realm. */
static bool kingdom_halls_loaded(void)
{
	return !Guildhall::guildhalls.empty();
}

/* Drop the derived half of the anchor. hall_vnum is the persisted half and is
 * deliberately KEPT: a realm without a hall is dormant, not dissolved, and its
 * record of where the hall stood is what an administrator reads afterwards.
 * kingdom_reindex_realm() indexes nothing without a resolved hall_rnum, so a
 * cleared anchor owns no squares. */
static void kingdom_clear_anchor(kingdom_realm &realm)
{
	realm.hall_rnum = 0;
	realm.zone_idx = -1;
	realm.hall_x = -1;
	realm.hall_y = -1;
}

/* The MAIN guildhall this realm is anchored on, or NULL when no such hall
 * stands at realm.hall_vnum. THE ONE test for "does this realm still have a
 * seat", asked by kingdom_resolve_anchor() and by the boot audit in
 * kingdom_reindex_all() so the two cannot drift apart.
 *
 * "The vnum resolves to a map room" is NOT that test: destroy_guildhall()
 * erases the hall and frees the object (guild/guildhall_cmds.c:1304-1312,
 * guild/guildhall.c:96-115) while the map room it stood on carries on
 * existing, so a realm whose hall is gone would go on resolving forever.
 *
 * `moved_to` is optional and only for the caller's log: it is set to 0 when
 * the association has no main hall at all, and to the hall's own outside_vnum
 * when a main hall stands somewhere other than realm.hall_vnum. It is written
 * on every call, including the successful one (0 then), so a caller never
 * reads a stale value. */
static Guildhall *kingdom_anchor_hall(const kingdom_realm &realm, int *moved_to)
{
	Guildhall *hall = kingdom_main_hall(realm.assoc_id);

	if (moved_to)
		*moved_to = 0;

	if (!hall)
		return NULL;

	if (hall->outside_vnum != realm.hall_vnum)
	{
		if (moved_to)
			*moved_to = hall->outside_vnum;
		return NULL;
	}

	return hall;
}

/* ------------------------------------------------------------------ *
 * Failed realm-row deletes, remembered for retry
 * ------------------------------------------------------------------ *
 * Association ids are REUSED -- found_asc() hands out the lowest free id
 * (guild/assocs.c:350-354) -- so a realm row whose DELETE failed is not a
 * leak: it is territory waiting to be inherited by the next guild to take
 * that id at the next boot's kingdom_db_load_all(). Logging the failure once
 * does not change that outcome; retrying the delete does. Failed ids wait
 * here and are retried from this file's DB-touching entry points (the two
 * hooks and kingdom_shutdown()).
 *
 * The list is in-memory only, deliberately: a row that outlives a crash is
 * re-loaded at the next boot and re-swept by kingdom_initialize()'s orphan
 * pass, which is the durable backstop. That backstop has one hole -- an id
 * re-issued to a NEW guild before the retry lands makes the orphan sweep see
 * a live association and keep the row -- which is exactly why the runtime
 * retry exists rather than leaving it all to boot.
 */
static std::vector<int> kingdom_failed_deletes;

/* Delete a realm's DB row, remembering the id for retry when the backend
 * refuses. Every realm-row delete in this file goes through here so no
 * failure path can forget to remember. */
static void kingdom_delete_realm_row(int assoc_id)
{
	if (kingdom_db_delete_realm(assoc_id))
		return;

	/* No duplicates: a second failure of the same id IS the retry. */
	if (std::find(kingdom_failed_deletes.begin(), kingdom_failed_deletes.end(), assoc_id) ==
	    kingdom_failed_deletes.end())
		kingdom_failed_deletes.push_back(assoc_id);

	logit(LOG_KINGDOM,
	      "realm row for association %d could not be deleted; remembered for retry.", assoc_id);
}

/* Retry every remembered delete, keeping the ones that fail again. Cheap when
 * the list is empty, which is always except while the backend is refusing
 * deletes. Gated on enabled because the kingdom tables are only guaranteed to
 * exist while the feature is on (see kingdom_on_guild_deleted()). */
static void kingdom_retry_failed_deletes(void)
{
	std::vector<int> still_failed;

	if (kingdom_failed_deletes.empty())
		return;
	if (!kingdom_cfg.enabled)
		return;

	for (size_t i = 0; i < kingdom_failed_deletes.size(); i++)
	{
		const int assoc_id = kingdom_failed_deletes[i];

		if (kingdom_db_delete_realm(assoc_id))
			logit(LOG_KINGDOM, "realm row for association %d deleted on retry.",
			      assoc_id);
		else
			still_failed.push_back(assoc_id);
	}

	kingdom_failed_deletes.swap(still_failed);
}

/* ------------------------------------------------------------------ *
 * The realm table
 * ------------------------------------------------------------------ */

/*
 * The returned pointer stays valid until THIS realm is erased: unordered_map
 * never invalidates references to elements it did not remove, not even when it
 * rehashes. Callers may therefore hold it across an insert of another realm.
 */
kingdom_realm *kingdom_find_realm(int assoc_id)
{
	if (!kingdom_valid_assoc(assoc_id))
		return NULL;

	std::unordered_map<int, kingdom_realm>::iterator it = kingdom_realms.find(assoc_id);
	if (it == kingdom_realms.end())
		return NULL;

	return &it->second;
}

/* Re-derive hall_rnum/zone_idx/hall_x/hall_y from the persisted hall_vnum,
 * clearing them first. False -- with the anchor left cleared, so the realm is
 * dormant -- when the vnum is unset, when the association's main guildhall no
 * longer stands on it, when it names no room, or when that room is not a map
 * room.
 *
 * DORMANCY IS STICKY, and the hall test is what makes it so: it is asked on
 * every re-resolution, not only at boot, so a realm the boot audit found
 * hall-less cannot be quietly re-anchored by the next verb that calls this. */
bool kingdom_resolve_anchor(kingdom_realm &realm)
{
	int zone_idx = -1, hall_x = -1, hall_y = -1;

	/* Everything but hall_vnum is derived, so clear it first: a realm whose
	 * hall has gone must end up unanchored rather than keep a stale square,
	 * because kingdom_reindex_realm() indexes nothing for hall_rnum 0 and
	 * the realm's territory then simply disappears until the hall is found. */
	kingdom_clear_anchor(realm);

	if (realm.hall_vnum <= 0)
		return false;

	/* A destroyed hall leaves its map room standing, so the room resolving
	 * below is no evidence of a seat: require the association's own MAIN
	 * hall to stand on hall_vnum. Without this, a realm made dormant because
	 * its hall was razed, demoted to an outpost or moved would be re-anchored
	 * by any verb that re-resolves -- and with the anchor back would come its
	 * billing, its garrison and its right to claim.
	 *
	 * Gated on kingdom_halls_loaded() for the same reason the boot audit in
	 * kingdom_reindex_all() is: an empty hall list means Guildhall::initialize()
	 * has almost certainly not run yet rather than that every hall on the
	 * server was demolished, and a too-early call must do LESS, not unanchor
	 * every realm. */
	if (kingdom_halls_loaded() && !kingdom_anchor_hall(realm, NULL))
		return false;

	const int rnum = real_room0(realm.hall_vnum);
	if (!kingdom_valid_rnum(rnum))
		return false;

	/* Refuses non-map zones, so a hall dragged into a non-map room leaves the
	 * realm unanchored instead of indexing nonsense. */
	if (!kingdom_square_of_room(rnum, &zone_idx, &hall_x, &hall_y))
		return false;

	realm.hall_rnum = rnum;
	realm.zone_idx = zone_idx;
	realm.hall_x = hall_x;
	realm.hall_y = hall_y;
	return true;
}

/* ------------------------------------------------------------------ *
 * The square index
 * ------------------------------------------------------------------ */

/* Write this realm's squares into the index. ADD-ONLY: the caller guarantees
 * the realm has no rows left in the index, either by having just cleared the
 * whole thing (kingdom_reindex_all) or by having unindexed this realm
 * (kingdom_reindex_realm). Kept separate so the boot path does not pay an
 * O(index) scan per realm for rows it knows are not there. */
static void kingdom_index_realm_squares(const kingdom_realm &realm)
{
	if (!kingdom_valid_assoc(realm.assoc_id))
		return;

	/* An unresolved anchor owns nothing. See kingdom_resolve_anchor(). */
	if (!kingdom_valid_rnum(realm.hall_rnum))
		return;

	int claim_top = realm.highest_claim;
	if (claim_top > KINGDOM_MAX_SQUARES)
	{
		logit(LOG_KINGDOM, "realm %d: highest_claim %d exceeds %d; clamping.",
		      realm.assoc_id, realm.highest_claim, KINGDOM_MAX_SQUARES);
		claim_top = KINGDOM_MAX_SQUARES;
	}
	if (claim_top < 0)
		claim_top = 0;

	/* Index 0 is the SEAT: the hall's own map square. It is not a claim --
	 * kingdom_offset_for_index() rejects index 0 by design -- but the square
	 * belongs to the realm all the same, and leaving it out made
	 * kingdom_owner_of_room() answer 0 there: a resource node could load ON
	 * the guildhall, and banner/ownership queries for the hall square said
	 * "unclaimed". kingdom_unindex_realm() erases by assoc_id VALUE, so the
	 * seat row leaves the index together with the claims. */
	for (int index = 0; index <= claim_top; index++)
	{
		/* kingdom_room_for_claim() refuses to leave the map grid, unlike
		 * calculate_relative_room(), which wraps toroidally. The seat needs no
		 * geometry: kingdom_resolve_anchor() has already proven hall_rnum sits
		 * on the map grid. */
		const int rnum = (index == 0) ? realm.hall_rnum :
						kingdom_room_for_claim(realm.hall_rnum, index);
		if (!kingdom_valid_rnum(rnum))
			continue;

		const int vnum = world[rnum].number;
		std::unordered_map<int, int>::iterator held = kingdom_square_index.find(vnum);

		if (held != kingdom_square_index.end())
		{
			/* Cannot happen while placement enforces
			 * KINGDOM_MIN_HALL_SEPARATION, so it means the stored data
			 * disagrees with the rule. Keep the first claimant and say so
			 * rather than letting ownership depend on hash order. */
			if (held->second != realm.assoc_id)
				logit(LOG_KINGDOM,
				      "room %d is claimed by realms %d and %d; keeping %d.", vnum,
				      held->second, realm.assoc_id, held->second);
			continue;
		}

		kingdom_square_index[vnum] = realm.assoc_id;
	}
}

/*
 * A REBUILD, which is what kingdom_internal.h calls it: the realm's existing
 * rows go FIRST.
 *
 * highest_claim SHRINKS as well as grows -- `kingdom abandon` releases the
 * single last-claimed square (kingdom_abandon_last, kingdom_claim.c) and the
 * arrears ladder reverts one outer ring per missed cycle (revert_outer_ring,
 * kingdom_upkeep.c) -- and an add-only rebuild would leave every released
 * square still resolving to this realm in kingdom_owner_of_room(). Callers
 * that already unindex first are unharmed: erasing rows that are gone is free.
 */
void kingdom_reindex_realm(const kingdom_realm &realm)
{
	if (!kingdom_valid_assoc(realm.assoc_id))
		return;

	kingdom_unindex_realm(realm.assoc_id);
	kingdom_index_realm_squares(realm);
}

/*
 * Erase by VALUE, not by recomputing the realm's squares.
 *
 * The entries were written from whatever anchor the realm had at the time. If
 * the hall has since moved, recomputing from the current anchor would erase
 * the wrong squares and leave the old ones behind -- pointing at an
 * association that no longer owns them.
 */
void kingdom_unindex_realm(int assoc_id)
{
	if (!kingdom_valid_assoc(assoc_id))
		return;

	std::unordered_map<int, int>::iterator it = kingdom_square_index.begin();
	while (it != kingdom_square_index.end())
	{
		if (it->second == assoc_id)
			it = kingdom_square_index.erase(it);
		else
			++it;
	}
}

/* Rebuild the whole square index from scratch: clear it, then for every realm
 * in ascending assoc_id order audit the anchor against the guild's main hall
 * (when halls are loaded), re-resolve it, and index its squares. A realm whose
 * hall is missing, moved or unresolvable is left unanchored and logged. */
void kingdom_reindex_all(void)
{
	std::vector<int> ids;
	const bool halls_loaded = kingdom_halls_loaded();

	kingdom_square_index.clear();

	/* Ascending association id, not hash order: if two realms ever do collide
	 * on a square, the winner must be the same across reboots. */
	ids.reserve(kingdom_realms.size());
	for (std::unordered_map<int, kingdom_realm>::const_iterator it = kingdom_realms.begin();
	     it != kingdom_realms.end(); ++it)
		ids.push_back(it->first);
	std::sort(ids.begin(), ids.end());

	for (size_t i = 0; i < ids.size(); i++)
	{
		kingdom_realm *realm = kingdom_find_realm(ids[i]);
		if (!realm)
			continue;

		/*
		 * AUDIT THE ANCHOR, do not trust the stored vnum. A realm is
		 * territory only because a guildhall stands at its centre, and a
		 * hall can be destroyed permanently -- destroy_guildhall() erases
		 * the row and frees the object (guild/guildhall_cmds.c:1304-1312,
		 * guild/guildhall.c:96-115) -- or moved, while kingdoms are
		 * switched off and no hook of ours runs at all. Without this
		 * check the map room the hall stood on still resolves, and a
		 * hall-less realm quietly re-owns eighty squares at the next boot.
		 *
		 * Deliberately does NOT adopt a hall that has moved: taking new
		 * ground means judging it, and kingdom_judge_footprint() is the
		 * placement authority's call, not a boot loop's. Dormant and
		 * loudly logged is recoverable; misplaced territory is not.
		 *
		 * kingdom_resolve_anchor() below asks kingdom_anchor_hall() the
		 * same question and would refuse on its own; this block is here to
		 * SAY WHICH of the two things happened, which is what an
		 * administrator reads afterwards. One predicate, asked twice --
		 * not two predicates.
		 */
		if (halls_loaded)
		{
			int moved_to = 0;

			if (!kingdom_anchor_hall(*realm, &moved_to))
			{
				kingdom_clear_anchor(*realm);
				if (moved_to)
					logit(LOG_KINGDOM,
					      "realm %d: anchored on vnum %d but its main hall "
					      "stands on %d; realm dormant.",
					      realm->assoc_id, realm->hall_vnum, moved_to);
				else
					logit(LOG_KINGDOM,
					      "realm %d: no main guildhall; realm dormant.",
					      realm->assoc_id);
				continue;
			}
		}

		/* Re-derive the anchor rather than trusting the cached rnum: rnums
		 * are world-table positions and shift on any world reload, while
		 * hall_vnum does not. Resolution is pure derived state, so redoing
		 * it is never wrong and makes this function idempotent. */
		if (!kingdom_resolve_anchor(*realm))
		{
			logit(LOG_KINGDOM,
			      "realm %d: guildhall vnum %d not a map room; realm dormant.",
			      realm->assoc_id, realm->hall_vnum);
			continue;
		}

		/* The index was cleared above, so this realm has no rows to drop
		 * and the add-only path is both correct and O(claims) rather than
		 * O(index) per realm. */
		kingdom_index_realm_squares(*realm);
	}
}

/* ------------------------------------------------------------------ *
 * Lifecycle
 * ------------------------------------------------------------------ */

/*
 * WIRING NOTE: this must be called AFTER the world, the associations and the
 * guildhalls are loaded. It resolves anchors through real_room0(), prunes
 * realms whose association is gone through get_guild_from_id(), and reads
 * Guildhall::guildhalls. Both of the lookups it depends on are guarded below
 * so that a too-early call degrades to "do less" rather than to data loss.
 */
void kingdom_initialize(void)
{
	std::vector<int> orphans;

	kingdom_config_load();

	/*
	 * Cleared BEFORE the enabled gate, not after it. A re-initialise that
	 * finds the feature switched off must leave nothing behind, or
	 * kingdom_enabled() would answer false while kingdom_owner_of_room()
	 * went on naming an owner from the previous configuration. It is also
	 * what makes the function idempotent: a second call (copyover, a reload
	 * command) starts clean instead of loading a second copy of every realm
	 * on top of the first.
	 */
	kingdom_square_index.clear();
	kingdom_realms.clear();

	/* Stale after any re-initialise: a row whose delete failed is re-loaded
	 * below and, if still orphaned, re-detected by the sweep -- which
	 * repopulates this list on a second failure. */
	kingdom_failed_deletes.clear();

	if (!kingdom_cfg.enabled)
	{
		logit(LOG_KINGDOM, "kingdoms are disabled in lib/kingdom.cfg; no realms loaded.");
		return;
	}

	if (!kingdom_db_load_all())
	{
		/*
		 * REFUSE TO RUN OVER A TABLE WE COULD NOT READ. Both backends
		 * return false only for genuine failure -- a failed or truncated
		 * SELECT, schema drift, a missing state root, a corrupt authority
		 * file -- never for "no kingdoms yet" (kingdom_db.c documents each
		 * case). Running live anyway would treat every unloaded realm's
		 * territory as unowned: claimable by others, buildable over, and
		 * the orphan sweep below would DELETE rows it merely failed to
		 * read. A disabled subsystem is honest; a half-loaded one corrupts.
		 *
		 * Self-disabling flips kingdom_cfg.enabled in memory only --
		 * lib/kingdom.cfg is untouched -- so the next boot tries again.
		 */
		logit(LOG_KINGDOM,
		      "kingdom_db_load_all() FAILED; disabling kingdoms for this boot "
		      "rather than running over an empty or partial table (%zu realm(s) "
		      "partially loaded, now dropped). Likely cause: the kingdom_realms "
		      "table is missing -- it is not in the boot schema contract yet.",
		      kingdom_realms.size());
		kingdom_square_index.clear();
		kingdom_realms.clear();
		kingdom_cfg.enabled = false;
		return;
	}

	/*
	 * Association ids are REUSED -- found_asc() hands out the lowest free id
	 * (guild/assocs.c:350-354). A realm left behind by a guild that was
	 * deleted while kingdoms were switched off would therefore be inherited,
	 * silently, by the next guild to take that id. kingdom_on_guild_deleted()
	 * cannot have run in that case, so reconcile here as well.
	 */
	if (kingdom_any_guild_loaded())
	{
		for (std::unordered_map<int, kingdom_realm>::const_iterator it =
			     kingdom_realms.begin();
		     it != kingdom_realms.end(); ++it)
			if (get_guild_from_id(it->first) == NULL)
				orphans.push_back(it->first);

		/* Collected first, erased second: kingdom_db_delete_realm() is
		 * another module's code and must not be able to invalidate an
		 * iterator into kingdom_realms while we are still holding one.
		 *
		 * The DB row goes too, not just the memory, and through the
		 * remember-on-failure path: a row merely dropped from memory would
		 * be re-loaded next boot, and by then the id may belong to a new
		 * guild -- at which point this sweep would see a live association
		 * and hand the new guild the dead realm's territory. */
		for (size_t i = 0; i < orphans.size(); i++)
		{
			logit(LOG_KINGDOM, "realm for association %d has no association; deleting.",
			      orphans[i]);
			kingdom_realms.erase(orphans[i]);
			kingdom_delete_realm_row(orphans[i]);
		}
	}
	else if (!kingdom_realms.empty())
	{
		/* No guild exists at all. Almost certainly a boot-order mistake
		 * rather than a server that deleted every association, and pruning
		 * on that assumption would destroy every realm. Refuse and shout. */
		logit(LOG_KINGDOM,
		      "no associations are loaded; skipping the orphan sweep for %zu realm(s). "
		      "kingdom_initialize() is being called too early.",
		      kingdom_realms.size());
	}

	kingdom_reindex_all();

	/* Nodes last, and unconditionally: they are scattered across the WHOLE
	 * world rather than over kingdom land, so their sweep does not depend on
	 * any realm existing -- but it does need the index built first, because
	 * placement refuses to load a node on a square a realm controls
	 * (kingdom_node_invalid_room(), ruling 1). The credit is the digger's,
	 * not the ground's: ANYONE may harvest, and a member's yield bonus comes
	 * from the TYPE of land their own realm controls. It self-gates on
	 * kingdom_enabled().
	 *
	 * Without this call the whole harvest subsystem is inert: the spec proc
	 * is never bound to the eight node prototypes (477-484: four surface,
	 * four Underdark, world/vnum.obj.h), the spawn windows stay zeroed and
	 * the reload never starts. An adversarial review caught exactly that. */
	kingdom_harvest_initialize();

	/* And post the garrisons. After the index, because a guard is placed on
	 * a square the realm owns, and after the anchors are resolved. Without
	 * this the guard prototype is never read_mobile()'d and no garrison ever
	 * appears -- the same inert-because-uncalled defect the review found in
	 * the harvest lane. */
	kingdom_guards_refresh_all();

	logit(LOG_KINGDOM, "kingdoms enabled: %zu realm(s) holding %zu square(s).",
	      kingdom_realms.size(), kingdom_square_index.size());
}

/* The copyover flush: retry any payment pair still pending, then write every
 * dirty realm that is not held for a pending payment. Tears nothing down --
 * a failed copyover resumes the game loop over this state. */
void kingdom_flush_persistent_state(void)
{
	if (kingdom_cfg.enabled && !kingdom_realms.empty())
	{
		/* Pairs first: a pending payment is a debit that lives only in
		 * memory, and the flush below deliberately will not publish its
		 * realm alone. */
		kingdom_upkeep_retry_pending();
		kingdom_db_flush_dirty();
	}
}

/* Final write-out and teardown: retry pending payment pairs, flush dirty
 * realms, retry failed row deletes, despawn guards and nodes, then empty
 * every container and the upkeep module's state. Idempotent. */
void kingdom_shutdown(void)
{
	/* Idempotent: clearing an already-empty map is free, and flushing an
	 * empty realm table writes nothing, so a second call is a no-op. */
	if (kingdom_cfg.enabled && !kingdom_realms.empty())
	{
		/* Last chance for a payment pair that has not landed; the flush
		 * skips a realm still pending after this, and its debit dies with
		 * the process (the realm is billed again next boot). */
		kingdom_upkeep_retry_pending();
		kingdom_db_flush_dirty();
	}

	/* Last chance before the process goes: a delete the backend refused
	 * earlier is retried here, or its row survives to the next boot's
	 * orphan sweep. Self-gates on enabled, and on an empty list. */
	kingdom_retry_failed_deletes();
	kingdom_failed_deletes.clear();

	/* Reap the placed nodes and stop the reload before the maps go, or the
	 * sweep would keep running against a cleared index. */
	kingdom_guards_despawn_all();
	kingdom_harvest_shutdown();

	kingdom_square_index.clear();
	kingdom_realms.clear();

	/* After the flush and the clears: the pending list and the cycle stamp
	 * are meaningless once no realm exists. */
	kingdom_upkeep_reset();
}

/* The feature switch as currently loaded, including a self-disable after a
 * failed kingdom_db_load_all(). */
bool kingdom_enabled(void)
{
	return kingdom_cfg.enabled;
}

/* ------------------------------------------------------------------ *
 * Queries
 * ------------------------------------------------------------------ */

/* Association id owning the room at rnum, or 0: one hash lookup keyed on the
 * room's vnum, with an empty-index fast path for the common case. */
int kingdom_owner_of_room(int rnum)
{
	/* The hot path. With kingdoms off, or on but with no territory claimed
	 * anywhere, this is the whole cost of the call -- which matters, because
	 * movement and the map renderer ask it constantly. */
	if (kingdom_square_index.empty())
		return 0;

	if (!kingdom_valid_rnum(rnum))
		return 0;

	std::unordered_map<int, int>::const_iterator it =
		kingdom_square_index.find(world[rnum].number);
	if (it == kingdom_square_index.end())
		return 0;

	return it->second;
}

/* Backs Guild::is_kingdom(). O(1): one hash find against the realm table.
 * The enabled gate is explicit rather than an inference from the table being
 * empty, because the header promises "false when disabled" and an inference
 * would quietly break if the load/clear choreography above ever changed. */
bool kingdom_guild_has_realm(int assoc_id)
{
	if (!kingdom_cfg.enabled)
		return false;

	return kingdom_find_realm(assoc_id) != NULL;
}

/*
 * NO ENGINE CALLER YET. The movement lane is wiring the realm-banner path
 * (cmd/actmove.c already asks kingdom_owner_of_room()) and may consume this
 * next; it is kept exported because the membership subtlety below is easy to
 * get wrong at a call site.
 */
bool kingdom_char_owns_room(struct char_data *ch, int rnum)
{
	if (!ch)
		return false;

	const int owner = kingdom_owner_of_room(rnum);
	if (owner <= 0)
		return false;

	/*
	 * GET_ASSOC() alone is NOT membership: Guild::apply() points an
	 * applicant's assoc pointer at the guild it is applying to
	 * (guild/assocs.c:1353-1354), and a banned character keeps the pointer
	 * too. Use the same three-part test the engine calls membership,
	 * IS_ASSOC_MEMBER (guild/assocs.h:97-98), so an applicant, a banned
	 * character, an enemy and someone on parole all answer false.
	 */
	if (!GET_ASSOC(ch))
		return false;
	if (GET_ASSOC(ch)->get_id() != static_cast<unsigned int>(owner))
		return false;

	return IS_MEMBER(GET_A_BITS(ch)) && GT_PAROLE(GET_A_BITS(ch));
}

/* ------------------------------------------------------------------ *
 * The guildhall placement gate
 * ------------------------------------------------------------------ */

/* The compiled Chebyshev distance two main halls must keep apart, for the
 * guildhall placement code. */
int kingdom_min_hall_separation(void)
{
	return KINGDOM_MIN_HALL_SEPARATION;
}

/* The guildhall placement gate: true when kingdoms are off or every one of
 * the 80 squares around hall_rnum passes kingdom_judge_footprint(); false
 * otherwise, with a player-readable reason written to `why` when the buffer
 * is at least KINGDOM_WHY_LEN bytes. */
bool kingdom_footprint_check(int hall_rnum, int racewar, char *why, size_t why_len)
{
	int bad_index = 0;

	if (why && why_len > 0)
		why[0] = '\0';

	/*
	 * FIRST, and before anything else is touched: with kingdoms off this
	 * gate permits everything, so guildhall_map_check()
	 * (guild/guildhall_cmds.c:1421) behaves exactly as it did before this
	 * module existed.
	 */
	if (!kingdom_cfg.enabled)
		return true;

	/* Ruled 2026-08-28: all 80 squares must be eligible AT PLACEMENT, so a
	 * hall may only be sited where a complete realm could later exist. There
	 * is no realm to exclude from the overlap test yet, hence ignore_assoc 0. */
	const int verdict = kingdom_judge_footprint(hall_rnum, racewar, 0, &bad_index);
	if (verdict == KSQ_OK)
		return true;

	/* kingdom.h requires at least KINGDOM_WHY_LEN bytes, and the check is
	 * real rather than decorative: an undersized buffer gets the empty string
	 * set above instead of half a sentence. It also gives the compiler the
	 * lower bound it needs, without which -Wformat-truncation=2 (correctly)
	 * objects that the literal prefix alone would not fit a 1-byte buffer. */
	if (why && why_len >= KINGDOM_WHY_LEN)
	{
		/* The return value is used rather than discarded: snprintf() is
		 * negative on an output error, which leaves the buffer's contents
		 * unspecified, and a caller printing it would show garbage.
		 * Truncation is harmless by comparison -- the result is still
		 * NUL-terminated. */
		int written;

		if (bad_index >= 1)
			written = snprintf(why, why_len,
					   "A kingdom's %dx%d domain will not fit here "
					   "(square %d of %d: %s).\r\n",
					   KINGDOM_FOOTPRINT_SIDE, KINGDOM_FOOTPRINT_SIDE,
					   bad_index, KINGDOM_MAX_SQUARES,
					   kingdom_verdict_text(verdict));
		else
			written = snprintf(why, why_len,
					   "A kingdom's %dx%d domain will not fit here (%s).\r\n",
					   KINGDOM_FOOTPRINT_SIDE, KINGDOM_FOOTPRINT_SIDE,
					   kingdom_verdict_text(verdict));

		if (written < 0)
			why[0] = '\0';
	}

	return false;
}

/* ------------------------------------------------------------------ *
 * Hooks
 * ------------------------------------------------------------------ */

/*
 * MUST run before the Guild object is freed, and the erase must be total.
 * Association ids are reused (guild/assocs.c:350-354), so a realm or an index
 * entry left behind here is not a leak -- it is territory handed to whichever
 * guild is founded next.
 */
void kingdom_on_guild_deleted(int assoc_id)
{
	if (!kingdom_valid_assoc(assoc_id))
		return;

	/* An earlier delete the backend refused is retried on every pass through
	 * here, BEFORE this deletion's own work: if the id being deleted is a
	 * reuse of one still on the failed list, its stale row goes first. */
	kingdom_retry_failed_deletes();

	/* Unconditional, not gated on kingdom_cfg.enabled: with the feature off
	 * both containers are empty and this costs nothing, and gating it would
	 * mean an id reused during a disabled period inherited a live index. */
	kingdom_unindex_realm(assoc_id);

	/* And any payment pair still waiting to be retried under this id: a
	 * reused id must not inherit it, and the debit it stood for dies with
	 * the guild. Unconditional for the same reason as the unindex. */
	kingdom_upkeep_forget_guild(assoc_id);

	/* Same reasoning for the cached terrain tally: ids are reused, and a
	 * recycled id must not inherit the dead realm's yield-bonus tally.
	 * Erasing a row that is not there is free. */
	kingdom_harvest_release(assoc_id);

	/* And the garrison: guards are matched on the association id STAMPED into
	 * them, never on GET_ASSOC()'s pointer (kingdom_guards.c), so this works
	 * before or after the Guild object dies and despawning zero guards is a
	 * single cheap sweep. Left standing, a dead guild's guards would fight on
	 * for nobody -- or for the next guild to be founded on this id. */
	kingdom_guards_despawn(assoc_id);

	if (kingdom_realms.erase(assoc_id) == 0)
		return; /* no realm: an ordinary guild being deleted */

	logit(LOG_KINGDOM, "association %d deleted; its realm is gone.", assoc_id);

	/*
	 * Only touch the database when the feature is on, because the kingdom
	 * tables are only guaranteed to exist then. A row orphaned while the
	 * feature is off is swept up by kingdom_initialize() the next time it is
	 * switched on, before any realm is indexed.
	 *
	 * Through the remember-on-failure path: a DELETE the backend refuses is
	 * retried at the next pass through this file's DB-touching entry points
	 * rather than logged once and left for the next holder of the id.
	 */
	if (kingdom_cfg.enabled)
		kingdom_delete_realm_row(assoc_id);
}

/*
 * Re-anchor a realm on its guild's current main hall after a hall change,
 * and say whether it is anchored afterwards. The realm's old index rows must
 * already be gone. A missing hall, an unresolvable vnum, or a moved hall
 * whose new footprint fails the placement authority all leave the realm
 * DORMANT (anchor cleared, highest_claim and hall_vnum kept) and return
 * false. A hall that did not move is re-resolved and re-indexed without
 * re-judging; one that did move is judged, adopted, re-indexed, and written --
 * unless the realm is holding a payment that is not yet durable, in which case
 * the record is left dirty for the paired retry instead of being published.
 */
static bool kingdom_rehome_realm(kingdom_realm &realm)
{
	const int assoc_id = realm.assoc_id;

	Guildhall *hall = kingdom_main_hall(assoc_id);
	if (!hall)
	{
		/* Destroyed -- or demoted to outposts, which cannot anchor a realm.
		 * Keep highest_claim and hall_vnum: the realm is dormant, not
		 * dissolved, and owns nothing until a main hall exists again.
		 * kingdom_reindex_all() re-audits this at every boot, so dormancy
		 * survives a reboot even if this hook is never called. */
		kingdom_clear_anchor(realm);
		logit(LOG_KINGDOM, "realm %d has no main guildhall; realm dormant.", assoc_id);
		return false;
	}

	if (hall->outside_vnum == realm.hall_vnum)
	{
		/* The hall did not actually move: a room was added, or it was
		 * reloaded. Re-resolve and re-index, and do NOT re-judge the
		 * footprint -- a realm that already exists must not be dissolved
		 * because the world changed around it. */
		if (!kingdom_resolve_anchor(realm))
		{
			logit(LOG_KINGDOM, "realm %d: guildhall vnum %d unresolvable; dormant.",
			      assoc_id, realm.hall_vnum);
			return false;
		}
		kingdom_reindex_realm(realm);
		return true;
	}

	/*
	 * The anchor really moved, so the same highest_claim now describes a
	 * different 80 squares. Ask the single authority whether those squares
	 * are a legal realm BEFORE adopting them: stamping ownership onto
	 * unvetted ground could put a realm in the Underdark, on top of a
	 * hometown, or against another realm's footprint. If it fails, the realm
	 * goes dormant and a human can look at the log -- which is recoverable,
	 * where silently misplaced territory is not.
	 */
	{
		kingdom_realm probe = realm;
		int bad_index = 0;

		probe.hall_vnum = hall->outside_vnum;
		if (!kingdom_resolve_anchor(probe))
		{
			logit(LOG_KINGDOM,
			      "realm %d: new guildhall vnum %d is not a map room; dormant.",
			      assoc_id, hall->outside_vnum);
			kingdom_clear_anchor(realm);
			return false;
		}

		const int verdict = kingdom_judge_footprint(probe.hall_rnum, hall->racewar,
							    assoc_id, &bad_index);
		if (verdict != KSQ_OK)
		{
			logit(LOG_KINGDOM,
			      "realm %d: guildhall moved to vnum %d, but square %d of %d is "
			      "ineligible (%s); realm dormant.",
			      assoc_id, hall->outside_vnum, bad_index, KINGDOM_MAX_SQUARES,
			      kingdom_verdict_text(verdict));
			kingdom_clear_anchor(realm);
			return false;
		}

		logit(LOG_KINGDOM,
		      "realm %d: guildhall moved from vnum %d to %d; %d claim(s) "
		      "follow the hall.",
		      assoc_id, realm.hall_vnum, hall->outside_vnum, realm.highest_claim);

		realm = probe;
	}

	kingdom_reindex_realm(realm);

	/*
	 * THE PENDING RULE. Every kingdom_db_save_realm() outside kingdom_db.c
	 * and kingdom_persist_payment() must be guarded by !payment_pending.
	 *
	 * A realm carrying payment_pending has had coin taken from its guild in
	 * memory only. Its record already holds the paid mark and the rung that
	 * debit bought, so publishing it here -- for a reason as innocent as a
	 * moved hall -- would put a paid mark on disk over a treasury that still
	 * shows the coin: exactly the half-state payment_pending exists to
	 * prevent. The new anchor is in the record and the record stays DIRTY,
	 * so kingdom_upkeep_retry_pending() carries the anchor to disk together
	 * with the guild whenever the pair lands.
	 *
	 * When nothing is pending no coin moved here either, so this is a plain
	 * realm write rather than a paired one, and a refused write leaves the
	 * record dirty for the next flush.
	 */
	realm.dirty = true;

	if (realm.payment_pending)
	{
		logit(LOG_KINGDOM,
		      "realm %d: re-anchored on vnum %d, but a payment is not yet durable; the "
		      "record is held for the paired retry.",
		      assoc_id, realm.hall_vnum);
		return true;
	}

	if (kingdom_db_save_realm(realm))
		realm.dirty = false;
	return true;
}

/*
 * A hall moved or was destroyed.
 *
 * The realm's whole territory is expressed relative to the hall square, so the
 * old index entries are wrong the instant this is called and are dropped
 * first, whatever happens afterwards. The garrison is settled before
 * returning as well: a realm made dormant here has its guards despawned at
 * once, and a re-anchored one has them re-posted, rather than either standing
 * on the old squares until the next upkeep tick reconciles them.
 */
void kingdom_on_guildhall_changed(int assoc_id)
{
	if (!kingdom_valid_assoc(assoc_id))
		return;

	/* Same retry as kingdom_on_guild_deleted(): this hook writes the
	 * database anyway, so it is a natural moment to settle old debts. */
	kingdom_retry_failed_deletes();

	kingdom_realm *realm = kingdom_find_realm(assoc_id);
	if (!realm)
		return;

	kingdom_unindex_realm(assoc_id);

	/* The terrain tally is derived from the anchor and the claim shape, and
	 * both may be about to change. Drop the cached one with the index rows;
	 * the next harvest recomputes it from whatever this hook decides. */
	kingdom_harvest_prune(*realm);

	/* Neither branch can invalidate `realm`: kingdom_rehome_realm() touches
	 * kingdom_square_index and the record itself, never kingdom_realms, and
	 * the guard calls only read the realm table. */
	if (kingdom_rehome_realm(*realm))
		(void)kingdom_guards_refresh(*realm);
	else
		(void)kingdom_guards_despawn(assoc_id);
}
