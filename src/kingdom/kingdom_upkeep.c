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
 *                          -> KARR_LAND_REVERTING  the OUTERMOST SQUARE
 *                                                  reverts, and one more per
 *                                                  further missed cycle
 *
 *  One square, not one ring (ruled 2026-09-05). A ring took eight to
 *  thirty-two squares for a single missed payment, which stripped a realm
 *  faster than any debt warrants; a square makes the ladder a slope the guild
 *  can feel and still climb back.
 *
 *  Paying halts the ladder wherever it has reached and restores the guards
 *  and the nodes -- but a square already reverted is GONE, and buying it back
 *  costs the full original price. That asymmetry is the whole point of the
 *  ruling, so kingdom_clear_arrears() never touches highest_claim. Losing a
 *  square also UNMAKES the champion, which answers only to a realm holding
 *  every square; retaking the eightieth lets the guild buy a new one.
 *
 *  The one place the rung clears WITHOUT a payment is the bottom of the
 *  ladder: when the last square reverts and highest_claim reaches 0 the realm
 *  owes nothing -- upkeep scales with squares held -- and no payment could
 *  ever clear the rung, so kingdom_apply_arrears() clears it itself. Leaving
 *  it set would have `kingdom claim` refuse with "settle the debt" over a debt
 *  of zero, wedging the realm out of the game for good.
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
 *                                (kingdom_guard_allowance(), kingdom_guards.c,
 *                                 applies it)
 *
 *  Keeping it as state makes "restore on payment" free: clearing the rung
 *  restores both, with no bookkeeping of which mob stood where. It also means
 *  the ladder gates expansion for nothing -- kingdom_claim_next() refuses
 *  `kingdom claim` while arrears != KARR_CURRENT and something is owed, so
 *  clearing the rung is what reopens buying land.
 *
 *  DORMANT REALMS ARE NOT BILLED
 *  -----------------------------
 *  A realm is DORMANT when its anchor will not resolve (hall_rnum 0). Since
 *  kingdom_resolve_anchor() requires the association's own MAIN guildhall to
 *  stand on the persisted hall_vnum, that covers a hall destroyed, demoted to
 *  outposts or moved elsewhere, as well as a vnum that no longer names a map
 *  room and a row inherited by a reused association id and not yet audited.
 *  Dormancy is sticky: nothing re-anchors a realm until a main hall stands on
 *  its square again.
 *
 *  A dormant realm owns no squares in the index, cannot claim and cannot
 *  abandon. kingdom_upkeep_due() answers 0 for it, and the sweep skips it --
 *  telling the guild once per cycle that the realm lies dormant -- WHATEVER
 *  it holds on paper, including nothing at all. Nothing about its state is
 *  touched: not the rung, not missed_cycles, not upkeep_paid_through. A debt
 *  outstanding when the hall fell is still owed when one stands again, and a
 *  landless dormant realm keeps whatever rung it was on rather than being
 *  quietly returned to KARR_CURRENT by the owes-nothing branch below.
 *
 *  Because upkeep_paid_through is left where it was, the first sweep after the
 *  hall stands again finds the paid period long expired and bills ONE cycle's
 *  upkeep straight away -- one charge, not one per period of dormancy -- and
 *  re-stamps the paid mark to that moment. A realm that cannot pay it takes a
 *  single rung, counted from wherever the ladder already stood.
 *
 *  CADENCE AND CRASHES
 *  -------------------
 *  Two clocks matter here, and both survive a reboot:
 *
 *    WHEN a realm is next billed derives from its PERSISTED
 *    upkeep_paid_through -- a realm is due once a full period has passed
 *    since it last paid. The in-memory cycle stamp below only throttles the
 *    sweep; it decides nothing about who owes. So the first sweep after a
 *    boot does not re-bill realms that paid shortly before the restart.
 *
 *    WHAT a payment took is written when it is taken, by
 *    kingdom_persist_payment(), and the guarantee differs by build mode:
 *
 *      MariaDB    the debited guild and the realm's paid mark are written
 *                 inside ONE transaction, or neither is written. No crash
 *                 can keep one without the other. When the transaction
 *                 cannot be opened, or either write or the commit fails,
 *                 nothing is left written: the realm is marked
 *                 payment_pending, the debit stands in memory, the generic
 *                 flush withholds the record and the sweep stops billing
 *                 the realm until kingdom_upkeep_retry_pending() lands the
 *                 pair. A crash while pending loses the in-memory debit, so
 *                 the realm is billed again for the same period -- the safe
 *                 direction.
 *
 *      flat-file  the guild and realm catalogues are after-images in one
 *                 recovery journal. The journal lands before either image;
 *                 an interruption after one is installed is completed at the
 *                 next authority access. A failed commit marks the realm
 *                 payment_pending for retry.
 *
 *    THE PENDING RULE. A paid mark must never reach disk without its debit,
 *    and kingdom_db_save_realm() is a primitive that cannot enforce that for
 *    itself: kingdom_persist_payment() is precisely the caller that must be
 *    allowed to publish a pending record, because it writes the guild in the
 *    same breath. So the obligation falls on every OTHER caller. Each
 *    kingdom_db_save_realm() outside kingdom_db.c and kingdom_persist_payment()
 *    -- the default write at the end of the sweep below, kingdom_rehome_realm()
 *    in kingdom.c after a hall change, the abandon path in kingdom_claim.c --
 *    has to test realm.payment_pending first and, when it is set, write
 *    nothing and leave the record DIRTY. The change it wanted to save is in
 *    the record either way, and kingdom_upkeep_retry_pending() carries the
 *    record to disk with its guild when the pair lands. An unguarded writer
 *    would publish the pending realm's paid mark (and any raised
 *    highest_claim) over a treasury that still shows the coin -- the exact
 *    half-state payment_pending exists to prevent. Both backends'
 *    kingdom_db_flush_dirty() skip pending records for the same reason, and
 *    the flat-file realm write is never attempted before the guild's has
 *    succeeded.
 *
 *    A DEFAULT (a rung advanced, a ring reverted) moves no coin, so it is
 *    written on its own the moment it is applied, and a failed write leaves
 *    the record dirty for the next flush. Deposits and the other non-payment
 *    changes that only set `dirty` are flushed on EVERY tick, not just at the
 *    sweep -- see kingdom_upkeep_event() -- so they wait at most one tick
 *    rather than one billing period to reach disk.
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
 *  iterators, which is the discipline EVENTS.md:185-186 asks of every periodic
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
#include "sql/sql_player.h" /* transaction helpers; stubs under __NO_MYSQL__ */

#include <climits>
#include <cstdio>
#include <ctime>
#include <vector>

/* net/comm.c:340 defines this as a plain `long` and world/db.c:426 sets it to
 * time(0) at boot. world/db.c:60 and world/epic.c:40 redeclare it unqualified,
 * which is the spelling matched here (world/outposts.c:65 says
 * `extern const long`, and the mismatch is only survivable because plain
 * globals are not type-mangled). */
extern long boot_time;

/* Who the treasury notices come from. */
static const char KINGDOM_STEWARD[] = "The Kingdom Steward";

/* Coin denominations are decimal, and the guild treasury never carries between
 * them: Guild::deposit() folds a member's purse into copper as
 * p*1000 + g*100 + s*10 + c (guild/assocs.c:2616, and again at 2641) but then
 * adds each coin to its own counter untouched (guild/assocs.c:2621-2624). */
#define KINGDOM_COPPER_PER_PLATINUM 1000
#define KINGDOM_COPPER_PER_GOLD 100
#define KINGDOM_COPPER_PER_SILVER 10

/* An hour of grace after boot before the ladder may advance, copying the
 * intent of the outpost job (world/outposts.c:1675-1676): officers who were
 * offline through a restart get one warning before anything is taken.
 *
 * The grace is for realms that fell due DURING the downtime -- ones whose
 * paid period had already run out by boot_time. A realm that falls due for
 * the first time after boot has had its whole period of uptime to deposit and
 * gets none; otherwise every restart would forgive one rung to every realm
 * that happened to come due in the following hour.
 *
 * ONE WARNING PER REALM PER BOOT, not "free until the hour is out": the window
 * is wall-clock but the excuse is counted, in kingdom_boot_grace_used below.
 * With a billing period shorter than this window -- an operator may set
 * upkeep_period_seconds to anything -- an insolvent realm would otherwise be
 * swept, warned and forgiven again on every cycle inside the hour, and the
 * ladder would not advance at all until the window closed.
 *
 * The outposts spell the window as `real_time_passed(time(0), boot_time).hour
 * < 1 && .day < 1`, which is wrong for a long-lived process -- those fields
 * are hour-within-day and day-within-month, so an uptime of exactly one month
 * reads as zero hours and zero days and silently re-opens the grace window.
 * A plain difference cannot do that. */
#define KINGDOM_UPKEEP_BOOT_GRACE_SECONDS 3600

/* Comfortably larger than any message below; every unbounded %s is bounded
 * with an explicit precision so nothing can be truncated. */
#define KINGDOM_UPKEEP_MSG_LEN 512

/* ------------------------------------------------------------------ *
 * What is owed
 * ------------------------------------------------------------------ */

/* Coin owed for one cycle: upkeep_per_square times the squares held,
 * saturating at INT_MAX. Zero for a realm holding nothing, for one whose
 * anchor is unresolved (dormant: it cannot use the land), and when the
 * operator has switched upkeep off. */
long kingdom_upkeep_due(const kingdom_realm &realm)
{
	if (realm.highest_claim <= 0)
		return 0; /* a realm that owns nothing owes nothing */

	/* No hall to rule from: the land is on paper only, the index holds none
	 * of its squares, and claim/abandon both refuse. Not billed. */
	if (realm.hall_rnum <= 0)
		return 0;

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

/* Take the outermost SQUARE back: highest_claim drops by one, which is what
 * "one square reverts" means when territory is a single integer. Claims are
 * bought one at a time in a fixed order (highest_claim + 1), so the square
 * lost is always the last one bought. Returns false when there is nothing left
 * to take.
 *
 * Ruled 2026-09-05: this used to revert a whole ring, costing between eight
 * and thirty-two squares for one missed payment. */
static bool revert_outer_square(kingdom_realm &realm)
{
	if (realm.highest_claim <= 0)
		return false; /* already stripped to the guildhall square */

	realm.highest_claim--;

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

/* Record one missed cycle: normalise a corrupt rung, count the miss, advance
 * one rung (stopping at KARR_LAND_REVERTING), and on the bottom rung revert
 * the outermost square. When that reversion leaves highest_claim at 0 the rung
 * is cleared again, because a realm holding nothing owes nothing and could
 * never pay its way off the ladder. Always dirties the record. */
void kingdom_apply_arrears(kingdom_realm &realm)
{
	/* A persisted rung outside the enum would otherwise skip the reversion
	 * test below and leave the realm permanently un-punishable. */
	if (realm.arrears < KARR_CURRENT || realm.arrears > KARR_LAND_REVERTING)
		realm.arrears = KARR_CURRENT;
	if (realm.missed_cycles < 0)
		realm.missed_cycles = 0;

	if (realm.missed_cycles < INT_MAX)
		realm.missed_cycles++;

	if (realm.arrears < KARR_LAND_REVERTING)
		realm.arrears++;

	/* Reaching the bottom rung reverts a square, and so does every miss after
	 * it -- "one square reverts per missed cycle". Rungs 1 and 2 cost the
	 * realm its guards and its nodes but no land.
	 *
	 * A realm below eighty squares has no champion, so the loss unmakes it:
	 * the guild must buy a new one once the eightieth square is retaken. */
	if (realm.arrears == KARR_LAND_REVERTING && revert_outer_square(realm))
		(void)kingdom_champion_destroy(realm);

	realm.dirty = true;

	/* Nothing left to take and nothing left to owe: the debt dies with the
	 * land. kingdom_clear_arrears() dirties the record itself. */
	if (realm.highest_claim <= 0)
		kingdom_clear_arrears(realm);
}

/* Return the realm to KARR_CURRENT with no missed cycles, dirtying the record
 * only when something actually changed. highest_claim is untouched. */
void kingdom_clear_arrears(kingdom_realm &realm)
{
	if (realm.arrears == KARR_CURRENT && realm.missed_cycles == 0)
		return; /* already current; do not dirty the record for nothing */

	realm.arrears = KARR_CURRENT;
	realm.missed_cycles = 0;
	realm.dirty = true;

	/* highest_claim is NOT restored, and that is deliberate: RULINGS.md 6
	 * says a reverted square must be re-claimed at the full original price.
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
 * on arbitrary amounts rather than authored ones.
 *
 * The debit is IN-MEMORY ONLY: sub_copper() moves the counters and writes the
 * ledger line, but neither it nor sub_money() runs Guild::save()
 * (guild/assocs.c:655) -- the engine's one path, sql and flatfile both, that
 * writes the counters out. kingdom_persist_payment() writes the guild and the
 * realm together right after the debit; a caller that skipped that would see
 * the payment undone at the next boot. */
static bool charge_treasury(P_Guild guild, long copper_total)
{
	if (!guild)
		return false;
	if (copper_total <= 0)
		return true; /* nothing owed is trivially paid */

	return guild->sub_copper(copper_total);
}

/* ------------------------------------------------------------------ *
 * Making a payment durable
 * ------------------------------------------------------------------ */

/* Association ids of realms whose payment pair has not landed. Each is
 * retried as a PAIR by kingdom_upkeep_retry_pending(); while listed, its realm
 * carries payment_pending, which keeps every flush from publishing the record
 * and the sweep from billing it again. In-memory only, on purpose: the debit
 * it stands for is in-memory too, so a crash loses both together and the
 * realm is simply billed again for the same period. */
static std::vector<int> kingdom_pending_payments;

/* Add an association id to the pending list, once. */
static void remember_pending(int assoc_id)
{
	for (const int id : kingdom_pending_payments)
		if (id == assoc_id)
			return;
	kingdom_pending_payments.push_back(assoc_id);
}

/* Remove an association id from the pending list; a no-op when absent. */
static void forget_pending(int assoc_id)
{
	for (size_t i = 0; i < kingdom_pending_payments.size(); i++)
	{
		if (kingdom_pending_payments[i] == assoc_id)
		{
			kingdom_pending_payments.erase(kingdom_pending_payments.begin() +
						       static_cast<long>(i));
			return;
		}
	}
}

/* Mark a realm's payment as not yet durable: set payment_pending, list the id
 * for kingdom_upkeep_retry_pending(), and log why. The debit stays in memory,
 * so the realm is not charged twice within this process. */
static void hold_payment(kingdom_realm &realm, const char *why)
{
	realm.payment_pending = true;
	remember_pending(realm.assoc_id);
	logit(LOG_KINGDOM,
	      "upkeep: payment for realm %d (association %d) is NOT durable: %.128s; the "
	      "record is held back from every flush and the pair will be retried",
	      realm.realm_id, realm.assoc_id, why);
}

/* Write the debited guild and this realm's record as ONE unit and report
 * whether they are durable. What each build mode guarantees:
 *
 *   MariaDB    If a transaction is already open the two writes JOIN it and
 *              true means both ran; commit or rollback is the owner's, and
 *              its rollback undoes both together. Otherwise this function
 *              owns one: guild, then realm, then commit, rolling back on any
 *              failure. If the transaction cannot be opened NOTHING is
 *              written -- never an unpaired write -- and the realm is held.
 *              On success the record is clean; no flush is needed.
 *
 *   flat-file  The guild and realm catalogue after-images share one recovery
 *              journal. The journal is durable before either image, so an
 *              interrupted commit completes on recovery. Success means both
 *              are durable; failure holds the realm for a retry.
 *
 * False in either mode means hold_payment() ran: payment_pending is set, the
 * id is listed for retry, and the debit stands in memory. */
bool kingdom_persist_payment(Guild *guild, kingdom_realm &realm)
{
	if (!guild)
	{
		hold_payment(realm, "no guild object to write");
		return false;
	}

#ifndef __NO_MYSQL__
	bool ok;

	if (sql_in_transaction())
	{
		/* Joining: Guild::save() -> sql_save_guild() joins the same open
		 * transaction. A failed write here leaves the owner's transaction
		 * half-applied, which is why it must roll back on our false. */
		ok = guild->save() && kingdom_db_save_realm(realm);
	}
	else
	{
		if (!sql_begin_transaction())
		{
			hold_payment(realm, "could not open a transaction; nothing written");
			return false;
		}

		ok = guild->save() && kingdom_db_save_realm(realm);
		if (ok)
			ok = sql_commit();
		if (!ok)
			sql_rollback(); /* sql_commit() keeps ownership on failure */
	}

	if (!ok)
	{
		hold_payment(realm, "guild or realm write failed; nothing committed");
		return false;
	}

	realm.dirty = false;
	realm.payment_pending = false;
	return true;
#else
	if (!guild->save_with_kingdom(realm))
	{
		hold_payment(realm, "recoverable guild/realm commit failed");
		return false;
	}

	realm.dirty = false;
	realm.payment_pending = false;
	return true;
#endif
}

/* Retry every held payment as a PAIR through kingdom_persist_payment(). An id
 * whose realm is gone is dropped; one whose guild is gone has its realm's
 * payment_pending cleared and is dropped with a log, since the debit died
 * with the guild and there is nothing left to pair; anything else leaves the
 * list only when the pair lands. */
void kingdom_upkeep_retry_pending(void)
{
	size_t i = 0;

	while (i < kingdom_pending_payments.size())
	{
		const int assoc_id = kingdom_pending_payments[i];
		kingdom_realm *realm = kingdom_find_realm(assoc_id);
		bool settled;

		if (!realm)
		{
			settled = true; /* realm erased; nothing left to write */
		}
		else
		{
			P_Guild guild = get_guild_from_id(assoc_id);

			if (!guild)
			{
				realm->payment_pending = false;
				logit(LOG_KINGDOM,
				      "upkeep: realm %d (association %d) had a payment pending but "
				      "its guild is gone; the pending payment is dropped",
				      realm->realm_id, assoc_id);
				settled = true;
			}
			else
			{
				/* On failure this re-lists the same id, which the
				 * dedup in remember_pending() makes a no-op. */
				settled = kingdom_persist_payment(guild, *realm);
				if (settled)
					logit(LOG_KINGDOM,
					      "upkeep: pending payment for realm %d (association %d) "
					      "is now durable",
					      realm->realm_id, assoc_id);
			}
		}

		if (settled)
			kingdom_pending_payments.erase(kingdom_pending_payments.begin() +
						       static_cast<long>(i));
		else
			i++;
	}
}

/* Drop any pending retry keyed on this association id. The realm record
 * itself is the caller's to erase; this only stops a reused id from
 * inheriting the dead guild's retry. */
void kingdom_upkeep_forget_guild(int assoc_id)
{
	forget_pending(assoc_id);
}

/* ------------------------------------------------------------------ *
 * Player-facing text for a rung
 * ------------------------------------------------------------------ */

/* The steward's notice for the rung a default has just reached. */
static const char *arrears_text(int arrears)
{
	switch (arrears)
	{
	case KARR_GUARDS_GONE:
		return "The garrison goes unpaid and melts away into the countryside.";
	case KARR_NODES_DORMANT:
		return "Work stops across the realm; the harvest nodes lie dormant.";
	case KARR_LAND_REVERTING:
		return "The realm's grip fails at its edge.";
	default:
		return "The treasury is in arrears.";
	}
}

/* ------------------------------------------------------------------ *
 * The periodic callback
 * ------------------------------------------------------------------ */

/* Association ids that have already spent their post-boot grace this process.
 * In-memory only and never persisted: the window it belongs to ends with this
 * boot, and kingdom_upkeep_reset() empties it. */
static std::vector<int> kingdom_boot_grace_used;

/* Take this association's ONE post-boot grace: true the first time it is asked
 * for in this process, false on every later ask. Called only from the branch
 * that has already established the realm is inside the grace window and fell
 * due during the downtime, so an id reaches this list only when it is actually
 * being excused. */
static bool take_boot_grace(int assoc_id)
{
	for (const int id : kingdom_boot_grace_used)
		if (id == assoc_id)
			return false;

	kingdom_boot_grace_used.push_back(assoc_id);
	return true;
}

/* When the last sweep ran: the throttle that turns a fast tick into the
 * configured billing period.
 *
 * The registered interval is fixed at boot and counted in PULSES (WAIT_SEC is
 * 4 of them to the second, core/config.h:105), while
 * kingdom_cfg.upkeep_period_seconds is config and can be reloaded, so the
 * wiring ticks this callback much faster than realms are billed -- once a
 * minute against a default period of an hour -- and the stamp here throttles
 * the sweep down to at most one per configured period. That also makes the
 * billing rate independent of the registered interval: a tick registered too
 * fast (or with the pulse conversion forgotten) cannot over-bill or walk the
 * arrears ladder early, it just burns a cheap early return.
 *
 * Being called before the period is up is therefore the NORMAL case -- the
 * overwhelming majority of ticks -- and the not-yet-due return is deliberately
 * SILENT: logging it would write a line a minute, forever. Actual charges,
 * arrears, and the end-of-cycle summary are still logged. The one thing a
 * not-yet-due tick does first is flush dirty records, so a deposit does not
 * wait for the sweep.
 *
 * The stamp is in-memory, so on the first tick after a boot it throttles
 * nothing. Reboot cadence is honoured anyway, because the sweep derives each
 * realm's due time from the PERSISTED upkeep_paid_through and skips realms
 * whose paid period has not yet run out. */
static time_t kingdom_upkeep_last_cycle = 0;

/* Clear the pending-payment list, the spent boot graces and the cycle stamp.
 * For kingdom_shutdown(); a pending payment dropped here is lost with the
 * process, and the realm is billed again for the period at the next boot. */
void kingdom_upkeep_reset(void)
{
	kingdom_pending_payments.clear();
	kingdom_boot_grace_used.clear();
	kingdom_upkeep_last_cycle = 0;
}

/* True when some realm is dirty and NOT held back for a pending payment --
 * the only records kingdom_db_flush_dirty() would write. */
static bool any_flushable_realm(void)
{
	for (const auto &entry : kingdom_realms)
		if (entry.second.dirty && !entry.second.payment_pending)
			return true;
	return false;
}

/* The periodic callback. Every tick: flush dirty, non-pending realms. Once
 * per configured period: retry held payment pairs, then sweep every realm --
 * skip those inside their paid period, held for a pending payment, or
 * dormant; bill the rest, advancing the arrears ladder for any that cannot
 * pay -- flush what the sweep dirtied, and reconcile every garrison. */
void kingdom_upkeep_event(void)
{
	if (!kingdom_enabled())
		return;
	if (kingdom_realms.empty())
		return;

	const time_t now = time(0);
	const time_t period = (kingdom_cfg.upkeep_period_seconds > 0) ?
				      static_cast<time_t>(kingdom_cfg.upkeep_period_seconds) :
				      0;

	/* Deposits and the other bookkeeping that only sets `dirty` would
	 * otherwise sit in memory until the next DUE sweep -- an hour by
	 * default. Under MariaDB this is one upsert per dirty realm; under the
	 * flat-file build it is one catalogue rewrite, and only when something
	 * is dirty. Records held for a pending payment are never written here:
	 * kingdom_db_flush_dirty() skips them, and this gate keeps the call
	 * (and its log line) off the routine path when only those are dirty. */
	if (any_flushable_realm())
		kingdom_db_flush_dirty();

	if (kingdom_upkeep_last_cycle > now)
	{
		/* The wall clock stepped backwards. Re-arm from here rather than
		 * refusing to charge until the old stamp is overtaken. */
		kingdom_upkeep_last_cycle = 0;
	}
	else if (kingdom_upkeep_last_cycle != 0 && period > 0 &&
		 (now - kingdom_upkeep_last_cycle) < period)
	{
		/* Not yet due. The tick outruns the period by design, so this
		 * is the routine path -- silent, per the note on the stamp. */
		return;
	}
	kingdom_upkeep_last_cycle = now;

	/* A payment whose pair did not land last cycle is still only in memory;
	 * try again before charging anyone. */
	kingdom_upkeep_retry_pending();

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
	 * leave that write sitting in memory until the next tick's flush. */
	int paid = 0, defaulted = 0, touched = 0;

	for (const int assoc_id : assoc_ids)
	{
		char msg[KINGDOM_UPKEEP_MSG_LEN];

		kingdom_realm *realm = kingdom_find_realm(assoc_id);
		if (!realm)
			continue;

		if (realm->payment_pending)
		{
			/* Its last payment is not yet durable, and the retry above
			 * could not land it. Billing again would debit a treasury
			 * the store may never accept, once per cycle, forever. */
			logit(LOG_KINGDOM,
			      "upkeep: realm %d (association %d) has a payment pending and "
			      "is not billed this cycle",
			      realm->realm_id, realm->assoc_id);
			continue;
		}

		if (realm->upkeep_paid_through > now)
		{
			/* The wall clock stepped backwards past the last payment.
			 * Re-arm from here -- exactly as the cycle stamp does --
			 * so the realm owes one period from NOW instead of
			 * sitting unbillable until the clock overtakes the old
			 * stamp. */
			realm->upkeep_paid_through = now;
			realm->dirty = true;
			touched++;
		}

		/* THE CADENCE IS THE REALM'S, NOT THE PROCESS'S. A realm is due
		 * only once a full period has passed since its PERSISTED
		 * upkeep_paid_through, which is what makes cadence survive a
		 * reboot: a realm that paid ten minutes before a restart is not
		 * re-billed on the first post-boot sweep, and a realm founded
		 * mid-cycle (kingdom_claim_next() stamps a fresh realm's
		 * paid_through) is not billed seconds later. Silent: a realm
		 * inside its paid period is the unremarkable case, not an
		 * anomaly. */
		if (period > 0 && (now - realm->upkeep_paid_through) < period)
			continue;

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

		if (realm->hall_rnum <= 0)
		{
			/* DORMANT: no hall to rule from, whatever the realm holds on
			 * paper. Not billed, and NOTHING about its state moves --
			 * not the rung, not missed_cycles, not upkeep_paid_through --
			 * so a debt outstanding when the hall fell is still owed when
			 * one stands again, and the realm is due again the moment it
			 * is anchored.
			 *
			 * The test deliberately does not ask for highest_claim > 0.
			 * A dormant realm holding nothing would otherwise fall
			 * through to the owes-nothing branch below, which moves the
			 * paid mark and clears the rung -- forgiving a debt and
			 * re-stamping a clock for a realm the header promises is
			 * left untouched. It cannot wedge itself out of the game by
			 * keeping a rung it can never pay off, either: it cannot
			 * claim while dormant in any case, and once anchored the
			 * owes-nothing branch clears the rung on the first sweep.
			 *
			 * Once per cycle, because only a due realm reaches here. */
			send_to_guild(guild, KINGDOM_STEWARD,
				      "The realm lies dormant: with no hall to rule from, the "
				      "crown asks no upkeep of it until one stands again.");
			logit(LOG_KINGDOM,
			      "upkeep: realm %d (association %d, %.64s) is dormant (anchor "
			      "unresolved, hall vnum %d); not billed, rung %d kept",
			      realm->realm_id, realm->assoc_id, guild->get_name().c_str(),
			      realm->hall_vnum, realm->arrears);
			continue;
		}

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

		if (charge_treasury(guild, due))
		{
			const bool was_behind = (realm->arrears != KARR_CURRENT);

			realm->upkeep_paid_through = now;
			realm->dirty = true;
			kingdom_clear_arrears(*realm);
			/* False has already held the realm (payment_pending) and
			 * logged; the coin stays taken in memory. */
			(void)kingdom_persist_payment(guild, *realm);
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
		if (in_boot_grace &&
		    realm->upkeep_paid_through + period <= static_cast<time_t>(boot_time) &&
		    take_boot_grace(realm->assoc_id))
		{
			/* This realm fell due while the server was down, so its
			 * officers have had no chance to deposit: one free warning.
			 * The charge was still attempted, so a solvent realm still
			 * pays. A realm that only came due after boot is outside this
			 * branch and takes the rung.
			 *
			 * take_boot_grace() is LAST in the condition on purpose: it
			 * consumes the excuse, and only a realm that would actually
			 * be forgiven may spend it. It is also what makes this "this
			 * once" rather than "free for an hour" -- with a billing
			 * period shorter than the window, the same realm would
			 * otherwise be excused on every sweep inside it. */
			send_to_guild(guild, KINGDOM_STEWARD,
				      "The stewards will hold their hand this once, the realm "
				      "having only just stirred. Deposit coin before the next "
				      "reckoning.");
			logit(LOG_KINGDOM,
			      "upkeep: realm %d (association %d, %.64s) could not pay %ld; "
			      "fell due during downtime, boot grace, ladder not advanced",
			      realm->realm_id, realm->assoc_id, guild->get_name().c_str(), due);
			continue;
		}

		const int claim_before = realm->highest_claim;
		const bool had_champion = realm->champion_class != 0;

		kingdom_apply_arrears(*realm);
		defaulted++;
		touched++;

		if (realm->highest_claim != claim_before)
		{
			/* Land was lost, so the rung reached was the bottom one --
			 * even if kingdom_apply_arrears() has already cleared it
			 * because nothing is left to owe. */
			send_to_guild(guild, KINGDOM_STEWARD, arrears_text(KARR_LAND_REVERTING));
			snprintf(msg, sizeof(msg),
				 "The outermost square slips from your grasp; %d of the realm's "
				 "%d remain. Reclaiming it will cost the full price.",
				 realm->highest_claim, KINGDOM_MAX_SQUARES);
			send_to_guild(guild, KINGDOM_STEWARD, msg);

			if (had_champion && realm->champion_class == 0)
				send_to_guild(guild, KINGDOM_STEWARD,
					      "The realm no longer holds every square, and its "
					      "champion is unmade. A new one may be raised once "
					      "the eightieth square is taken again.");

			if (realm->highest_claim <= 0)
				send_to_guild(guild, KINGDOM_STEWARD,
					      "Nothing remains of the realm but the hall itself. "
					      "No debt is owed on it; land may be claimed again "
					      "at full price.");
		}
		else
		{
			send_to_guild(guild, KINGDOM_STEWARD, arrears_text(realm->arrears));
		}

		logit(LOG_KINGDOM,
		      "upkeep: realm %d (association %d, %.64s) missed %ld; rung %d, "
		      "missed_cycles %d, claims %d -> %d",
		      realm->realm_id, realm->assoc_id, guild->get_name().c_str(), due,
		      realm->arrears, realm->missed_cycles, claim_before, realm->highest_claim);

		/* A default moves no coin, so it needs no pairing. Make the rung
		 * -- and any reverted square -- durable NOW rather than at the
		 * sweep's end: a crash before the batched flush would otherwise
		 * forgive the missed cycle. Failure leaves the record dirty for
		 * the flush below to retry.
		 *
		 * The payment_pending test is THE PENDING RULE spelled out: this
		 * realm cannot in fact be pending here -- the sweep skipped every
		 * pending realm before billing, and the failed charge above
		 * attempted no write -- but every direct writer in the module
		 * reads the same, and a later edit that reordered the sweep would
		 * find the guard already in place. */
		if (!realm->payment_pending && kingdom_db_save_realm(*realm))
			realm->dirty = false;
	}

	if (touched)
	{
		/* Under MariaDB every PAYMENT is already durable in its own
		 * transaction and every DEFAULT was written as it was applied, so
		 * what remains dirty here is the cheap bookkeeping -- realms
		 * owing nothing, clock re-arms -- plus any write that failed
		 * above. Under the flat-file build the paid realms are dirty too,
		 * by design: this is the one catalogue rewrite that publishes
		 * their paid marks, after their guilds were written. Records held
		 * for a pending payment are skipped by both backends. Neither
		 * backend erases from kingdom_realms, so flushing after the sweep
		 * costs nothing. */
		kingdom_db_flush_dirty();
		logit(LOG_KINGDOM,
		      "upkeep: cycle complete; %d realm(s) paid, %d in default, %d written", paid,
		      defaulted, touched);
	}

	/* Reconcile every garrison against what the realms now hold. This tick is
	 * where arrears rungs move -- guards go at KARR_GUARDS_GONE and come back
	 * when the debt clears -- and where territory lost to a reverted square
	 * changes the allowance, so it is the right place to settle up. Cheap
	 * when nothing changed: refresh is idempotent. */
	kingdom_guards_refresh_all();
}
