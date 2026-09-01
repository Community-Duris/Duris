/*
 *  kingdom_upkeep.c
 *  Duris
 *
 *  The periodic upkeep charge and the arrears ladder.
 *
 *  Every cycle a realm is billed COIN for each square it owns. Resources are
 *  deliberately not accepted here: they are non-withdrawable and spendable
 *  only on kingdom benefits (kingdom_internal.h), so letting them settle a
 *  debt would launder them back into the guild's liquid wealth.
 *
 *  THE LADDER (ruled 2026-08-28, RULINGS.md "6 -- UPKEEP DEGRADATION")
 *  ------------------------------------------------------------------
 *  A missed cycle advances one rung, in this order:
 *
 *      KARR_CURRENT        -> KARR_GUARDS_GONE     guards despawn
 *                          -> KARR_NODES_DORMANT   harvest nodes go dormant
 *                          -> KARR_RINGS_REVERTING one OUTER ring reverts,
 *                                                  and one more per further
 *                                                  missed cycle
 *
 *  Paying halts the ladder wherever it has reached and restores the guards
 *  and the nodes -- but a ring already reverted is GONE, and buying it back
 *  costs the full original price. That asymmetry is the whole point of the
 *  ruling, so kingdom_clear_arrears() never touches highest_claim.
 *
 *  THE LADDER IS STATE, NOT AN ERRAND
 *  ----------------------------------
 *  Nothing here hunts down guard mobs or switches nodes off. The realm record
 *  is the single authority -- exactly as the territory is the single integer
 *  highest_claim -- so this file only moves `arrears`, and the guard and
 *  harvest code reads it. That is the contract the rest of the module is
 *  written against:
 *
 *      nodes   yield             only while realm.arrears <  KARR_NODES_DORMANT
 *                                (kingdom_nodes_dormant(), kingdom_harvest.c,
 *                                 is exactly that test)
 *      guards  are permitted     only while realm.arrears <  KARR_GUARDS_GONE
 *                                (kingdom_guard_allowance() must apply this;
 *                                 see the note in the lane report)
 *
 *  Keeping it as state makes "restore on payment" free: clearing the rung
 *  restores both, with no bookkeeping of which mob stood where. It also means
 *  the ladder gates expansion for nothing -- kingdom_claim_next() refuses
 *  `kingdom claim` while arrears != KARR_CURRENT, so clearing the rung is
 *  what reopens buying land.
 *
 *  Sibling files inside src/kingdom/ are cited by FUNCTION NAME rather than by
 *  line: they are still being written alongside this one, so a line number
 *  into them would be stale before it was read. Engine files outside this
 *  directory are stable and are cited by line.
 *
 *  PERIODIC-JOB DISCIPLINE
 *  -----------------------
 *  kingdom_upkeep_event() is a periodic-registry callback. A callback cannot
 *  be preempted and "heavy jobs must expose bounded slices instead of hiding
 *  an unbounded sweep inside one invocation" (docs/reference/EVENTS.md:158-159),
 *  so this one is bounded by the realm count -- at most MAX_ASC (999,
 *  guild/assocs.h:23) and in practice a handful, since a realm needs an
 *  81-square footprint and Chebyshev-10 clearance from every other. Per realm
 *  the work is a hash lookup, integer arithmetic and at most one coin
 *  deduction; no world traversal and no allocation. The single 80-step path
 *  (rebuilding the square index after a ring reverts) fires only for a realm
 *  that has already missed three consecutive cycles.
 *
 *  The sweep walks a snapshot of the association ids rather than the map's
 *  iterators, which is the discipline EVENTS.md:186-188 asks of every periodic
 *  scan: "stable cursors or runtime-ID snapshots so entity removal between
 *  slices cannot invalidate traversal state".
 *
 *  It deliberately does NOT call nevent_periodic_continue_after() or
 *  nevent_periodic_next_after(): both panic via panic_corruption() when there
 *  is no periodic job running (world/nevent_periodic.c:342-358), so either
 *  would turn a non-periodic caller -- a test, an immortal command -- into a
 *  crash.
 *
 *  Registration is the wiring lane's job; this file only provides the
 *  callback.
 */

#include "kingdom/kingdom_internal.h"

#include "core/structs.h"

#include "core/prototypes.h"
#include "core/utility.h"
#include "guild/assocs.h"

#include <climits>
#include <cstdio>
#include <ctime>
#include <vector>

/* net/comm.c:333 defines this as a plain `long` and world/db.c:426 sets it to
 * time(0) at boot. world/db.c:60 and world/epic.c:40 redeclare it unqualified,
 * which is the spelling matched here (world/outposts.c:65 says
 * `extern const long`, and the mismatch is only survivable because plain
 * globals are not type-mangled). */
extern long boot_time;

/* Who the treasury notices come from. */
static const char KINGDOM_STEWARD[] = "The Kingdom Steward";

/* Coin denominations are decimal, and the guild treasury never carries between
 * them: Guild::deposit() folds a member's purse into copper as
 * p*1000 + g*100 + s*10 + c (guild/assocs.c:2562, and again at 2587) but then
 * adds each coin to its own counter untouched (guild/assocs.c:2567-2570). */
#define KINGDOM_COPPER_PER_PLATINUM 1000
#define KINGDOM_COPPER_PER_GOLD 100
#define KINGDOM_COPPER_PER_SILVER 10

/* An hour of grace after boot before the ladder may advance, copying the
 * intent of the outpost job (world/outposts.c:1675-1676): officers who were
 * offline through a restart get one warning before anything is taken.
 *
 * The outposts spell this as `real_time_passed(time(0), boot_time).hour < 1 &&
 * .day < 1`, which is wrong for a long-lived process -- those fields are
 * hour-within-day and day-within-month, so an uptime of exactly one month
 * reads as zero hours and zero days and silently re-opens the grace window.
 * A plain difference cannot do that. */
#define KINGDOM_UPKEEP_BOOT_GRACE_SECONDS 3600

/* Comfortably larger than any message below; every unbounded %s is bounded
 * with an explicit precision so nothing can be truncated. */
#define KINGDOM_UPKEEP_MSG_LEN 512

/* ------------------------------------------------------------------ *
 * What is owed
 * ------------------------------------------------------------------ */

long kingdom_upkeep_due(const kingdom_realm &realm)
{
	if (realm.highest_claim <= 0)
		return 0; /* a realm that owns nothing owes nothing */

	const long per_square = kingdom_cfg.upkeep_per_square;
	if (per_square <= 0)
		return 0; /* operator has switched upkeep off */

	int squares = realm.highest_claim;
	if (squares > KINGDOM_MAX_SQUARES)
		squares = KINGDOM_MAX_SQUARES; /* clamp a corrupt persisted value */

	/* kingdom_config.c's KINGDOM_UPKEEP_MAX already caps upkeep_per_square so
	 * that a full 80-square realm cannot exceed 32 bits. This saturation is
	 * the belt to that braces: the value reaches us from lib/kingdom.cfg, and
	 * the only coin API available takes ints (Guild::sub_money,
	 * guild/assocs.h:282), so a config path that ever stopped clamping must
	 * saturate here rather than wrap into a negative charge. */
	if (per_square > static_cast<long>(INT_MAX) / squares)
		return static_cast<long>(INT_MAX);

	return per_square * squares;
}

/* ------------------------------------------------------------------ *
 * The ladder
 * ------------------------------------------------------------------ */

/* Take the outermost ring back: highest_claim drops to the last index of the
 * ring below, which is what "one ring reverts" means when territory is a
 * single integer. A part-built ring counts as that realm's outer ring, since
 * kingdom_claim.c claims one square at a time (highest_claim + 1), so a realm
 * caught mid-ring loses the squares it had bought of it. Returns false when
 * there is nothing left to take. */
static bool revert_outer_ring(kingdom_realm &realm)
{
	const int ring = kingdom_ring_for_index(realm.highest_claim);
	if (ring <= 0)
		return false; /* already stripped to the guildhall square */

	/* kingdom_ring_first_index(r) - 1 == kingdom_ring_last_index(r - 1), and
	 * is 0 for ring 1, so the realm can be stripped to nothing. */
	realm.highest_claim = kingdom_ring_first_index(ring) - 1;

	/* kingdom_square_index maps owned room vnums to the association, so it
	 * MUST be rebuilt the moment highest_claim moves (kingdom_internal.h) --
	 * otherwise kingdom_owner_of_room() keeps answering with squares the
	 * realm no longer holds. Unindex first: that is correct whether or not
	 * kingdom_reindex_realm() drops the realm's stale rows itself, and
	 * removing rows that are already gone is free.
	 *
	 * Neither call can invalidate the caller's `realm` reference:
	 * kingdom_unindex_realm() and kingdom_reindex_realm() touch only
	 * kingdom_square_index and never kingdom_realms. */
	kingdom_unindex_realm(realm.assoc_id);
	kingdom_reindex_realm(realm);
	return true;
}

void kingdom_apply_arrears(kingdom_realm &realm)
{
	/* A persisted rung outside the enum would otherwise skip the reversion
	 * test below and leave the realm permanently un-punishable. */
	if (realm.arrears < KARR_CURRENT || realm.arrears > KARR_RINGS_REVERTING)
		realm.arrears = KARR_CURRENT;
	if (realm.missed_cycles < 0)
		realm.missed_cycles = 0;

	if (realm.missed_cycles < INT_MAX)
		realm.missed_cycles++;

	if (realm.arrears < KARR_RINGS_REVERTING)
		realm.arrears++;

	/* Reaching the bottom rung reverts a ring, and so does every miss after
	 * it -- "one outer ring reverts per missed cycle". Rungs 1 and 2 cost the
	 * realm its guards and its nodes but no land. */
	if (realm.arrears == KARR_RINGS_REVERTING)
		(void)revert_outer_ring(realm);

	realm.dirty = true;
}

void kingdom_clear_arrears(kingdom_realm &realm)
{
	if (realm.arrears == KARR_CURRENT && realm.missed_cycles == 0)
		return; /* already current; do not dirty the record for nothing */

	realm.arrears = KARR_CURRENT;
	realm.missed_cycles = 0;
	realm.dirty = true;

	/* highest_claim is NOT restored, and that is deliberate: RULINGS.md 6
	 * says a reverted ring must be re-claimed at the full original price.
	 * Guards and nodes come back for free because they are keyed off the
	 * rung we just cleared, not off any record of what was despawned. */
}

/* ------------------------------------------------------------------ *
 * Charging the treasury
 * ------------------------------------------------------------------ */

/* Spend `copper_total` worth of coin out of the guild treasury.
 *
 * This used to try eight different decompositions of the same total, because
 * Guild::sub_money(p,g,s,c) refuses unless EVERY denomination separately covers
 * its share (guild/assocs.c:217-219) -- a treasury holding 20 platinum and no
 * copper could not pay 5 copper. That workaround was still incomplete: a purse
 * of 5 gold and 20 silver holds 700 copper but matches none of the eight
 * spellings of 700, because a short count of one coin topped up by a long count
 * of the next is outside that family.
 *
 * Guild::sub_copper() (RULINGS.md 3) settles it properly: it values the whole
 * purse, checks it once, and makes change across the denominations. Upkeep is
 * the case that needs it, because the charge scales with territory and so lands
 * on arbitrary amounts rather than authored ones. */
static bool charge_treasury(P_Guild guild, long copper_total)
{
	if (!guild)
		return false;
	if (copper_total <= 0)
		return true; /* nothing owed is trivially paid */

	return guild->sub_copper(copper_total);
}

/* ------------------------------------------------------------------ *
 * Player-facing text for a rung
 * ------------------------------------------------------------------ */

static const char *arrears_text(int arrears)
{
	switch (arrears)
	{
	case KARR_GUARDS_GONE:
		return "The garrison goes unpaid and melts away into the countryside.";
	case KARR_NODES_DORMANT:
		return "Work stops across the realm; the harvest nodes lie dormant.";
	case KARR_RINGS_REVERTING:
		return "The realm's grip fails at its edge.";
	default:
		return "The treasury is in arrears.";
	}
}

/* ------------------------------------------------------------------ *
 * The periodic callback
 * ------------------------------------------------------------------ */

/* When the last cycle actually charged, as a guard against billing twice.
 *
 * The periodic registry owns the cadence, but the interval it is registered
 * with is fixed at boot while kingdom_cfg.upkeep_period_seconds can be
 * reloaded, and the registry counts PULSES rather than seconds -- WAIT_SEC is
 * 4 of them to the second (core/config.h:105). A registration that forgot that
 * conversion would bill every realm four times as often and walk the arrears
 * ladder four times as fast. Coin taken cannot be un-taken and a reverted ring
 * is gone for good, so refuse to run twice inside one configured period and
 * say so in the log rather than failing silently.
 *
 * This never suppresses a legitimate cycle. Under either periodic policy the
 * real gap between callbacks is at least the registered interval, so an
 * interval that matches the configured period always clears the test:
 * fixed_delay adds the interval to the callback's completion
 * (world/nevent_periodic.c:313), and fixed_rate does NOT replay a backlog --
 * periodic_fixed_rate_due() advances the deadline past the current tick and
 * merely counts what it skipped into missed_runs
 * (world/nevent_periodic.c:132-146). */
static time_t kingdom_upkeep_last_cycle = 0;

void kingdom_upkeep_event(void)
{
	if (!kingdom_enabled())
		return;
	if (kingdom_realms.empty())
		return;

	const time_t now = time(0);

	if (kingdom_upkeep_last_cycle > now)
	{
		/* The wall clock stepped backwards. Re-arm from here rather than
		 * refusing to charge until the old stamp is overtaken. */
		kingdom_upkeep_last_cycle = 0;
	}
	else if (kingdom_upkeep_last_cycle != 0 && kingdom_cfg.upkeep_period_seconds > 0 &&
		 (now - kingdom_upkeep_last_cycle) <
			 static_cast<time_t>(kingdom_cfg.upkeep_period_seconds))
	{
		logit(LOG_KINGDOM,
		      "upkeep: called %ld seconds after the last cycle but the configured "
		      "period is %d; skipping so nobody is billed twice",
		      static_cast<long>(now - kingdom_upkeep_last_cycle),
		      kingdom_cfg.upkeep_period_seconds);
		return;
	}
	kingdom_upkeep_last_cycle = now;

	/* Sweep a snapshot of the KEYS, not the map's iterators. Re-looking-up
	 * each realm means nothing here holds a pointer into kingdom_realms
	 * across a call that could rehash or erase it, and a realm dropped
	 * mid-sweep is simply skipped instead of dereferenced. */
	std::vector<int> assoc_ids;
	assoc_ids.reserve(kingdom_realms.size());
	for (const auto &entry : kingdom_realms)
		assoc_ids.push_back(entry.first);

	const bool in_boot_grace =
		(now >= static_cast<time_t>(boot_time)) &&
		((now - static_cast<time_t>(boot_time)) < KINGDOM_UPKEEP_BOOT_GRACE_SECONDS);

	/* `touched` counts every realm whose record was dirtied, not just the
	 * two interesting outcomes -- a realm that owes nothing still has its
	 * upkeep_paid_through moved, and flushing only on paid/defaulted would
	 * leave that write sitting in memory until some unrelated code flushed. */
	int paid = 0, defaulted = 0, touched = 0;

	for (const int assoc_id : assoc_ids)
	{
		char msg[KINGDOM_UPKEEP_MSG_LEN];

		kingdom_realm *realm = kingdom_find_realm(assoc_id);
		if (!realm)
			continue;

		const long due = kingdom_upkeep_due(*realm);
		if (due <= 0)
		{
			/* Owns nothing, or upkeep is switched off. Either way the
			 * realm is not in default, and there is no rung left worth
			 * holding it on -- its guard allowance is already zero. */
			realm->upkeep_paid_through = now;
			realm->dirty = true;
			kingdom_clear_arrears(*realm);
			touched++;
			continue;
		}

		P_Guild guild = get_guild_from_id(realm->assoc_id);
		if (!guild)
		{
			/* kingdom_on_guild_deleted() should have taken this realm
			 * with the guild. Guild ids are reused, so an orphan left
			 * indexed would be inherited by the next guild to take the
			 * id -- log loudly and charge nobody. */
			logit(LOG_KINGDOM,
			      "upkeep: realm %d is orphaned (association %d is gone); "
			      "kingdom_on_guild_deleted was not called",
			      realm->realm_id, realm->assoc_id);
			continue;
		}

		if (charge_treasury(guild, due))
		{
			const bool was_behind = (realm->arrears != KARR_CURRENT);

			realm->upkeep_paid_through = now;
			realm->dirty = true;
			kingdom_clear_arrears(*realm);
			paid++;
			touched++;

			/* coin_stringv() hands back a STATIC buffer
			 * (core/utility.c:2827-2830), so it may appear at most once
			 * in any one format. */
			snprintf(msg, sizeof(msg),
				 "The crown's upkeep of %.128s has been paid from the treasury.",
				 coin_stringv(static_cast<int>(due)));
			send_to_guild(guild, KINGDOM_STEWARD, msg);

			if (was_behind)
				send_to_guild(
					guild, KINGDOM_STEWARD,
					"The garrison returns to its posts and the realm's nodes "
					"stir back to life. Land already lost stays lost: it must "
					"be claimed again at full price.");
			continue;
		}

		snprintf(msg, sizeof(msg), "The treasury cannot meet the crown's upkeep of %.128s.",
			 coin_stringv(static_cast<int>(due)));
		send_to_guild(guild, KINGDOM_STEWARD, msg);

		/* get_name() returns a std::string BY VALUE (guild/assocs.h:196);
		 * the temporary outlives each logit() call it is spelled in. */
		if (in_boot_grace)
		{
			/* One free warning after a restart: officers who were
			 * offline through the downtime have not had a chance to
			 * deposit. The charge was still attempted, so a solvent
			 * realm still pays. */
			send_to_guild(guild, KINGDOM_STEWARD,
				      "The stewards will hold their hand this once, the realm "
				      "having only just stirred. Deposit coin before the next "
				      "reckoning.");
			logit(LOG_KINGDOM,
			      "upkeep: realm %d (association %d, %.64s) could not pay %ld; "
			      "boot grace, ladder not advanced",
			      realm->realm_id, realm->assoc_id, guild->get_name().c_str(), due);
			continue;
		}

		const int claim_before = realm->highest_claim;
		kingdom_apply_arrears(*realm);
		defaulted++;
		touched++;

		send_to_guild(guild, KINGDOM_STEWARD, arrears_text(realm->arrears));

		if (realm->highest_claim != claim_before)
		{
			snprintf(msg, sizeof(msg),
				 "The outermost ring slips from your grasp; %d of the realm's "
				 "%d squares remain. Reclaiming it will cost the full price.",
				 realm->highest_claim, KINGDOM_MAX_SQUARES);
			send_to_guild(guild, KINGDOM_STEWARD, msg);
		}
		else if (realm->arrears == KARR_RINGS_REVERTING)
		{
			/* Bottom rung with nothing left to take: the realm holds
			 * only its guildhall square and cannot be reduced further. */
			send_to_guild(guild, KINGDOM_STEWARD,
				      "Nothing remains of the realm but the hall itself.");
		}

		logit(LOG_KINGDOM,
		      "upkeep: realm %d (association %d, %.64s) missed %ld; rung %d, "
		      "missed_cycles %d, claims %d -> %d",
		      realm->realm_id, realm->assoc_id, guild->get_name().c_str(), due,
		      realm->arrears, realm->missed_cycles, claim_before, realm->highest_claim);
	}

	if (touched)
	{
		/* One flush for the whole sweep: the record writes are what make
		 * this job expensive, and batching keeps the callback's cost off
		 * the per-realm path. Both kingdom_db_flush_dirty() backends walk
		 * kingdom_realms by reference and never erase from it, so doing it
		 * after the sweep rather than inside it costs nothing. */
		kingdom_db_flush_dirty();
		logit(LOG_KINGDOM,
		      "upkeep: cycle complete; %d realm(s) paid, %d in default, %d written", paid,
		      defaulted, touched);
	}
}
