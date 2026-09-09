/*
 *  kingdom_cmds.c
 *  Duris
 *
 *  The `kingdom` command: parse a verb, check that the actor may be heard at
 *  all, and hand the work to the file that owns it. EVERY verb is a subcommand
 *  because the obvious top-level names are already spoken for -- `claim` and
 *  `harvest` are live entries in the interpreter's command table bound to
 *  do_not_here, so a second binding for either would never be reached.
 *
 *  THIS FILE OWNS NO RULES. It holds no price, no distance, no eligibility
 *  test and no piece of geometry. Conversion, claiming and abandoning live in
 *  kingdom_claim.c; the status table and the ring grid live in
 *  kingdom_display.c; nodes and yields live in kingdom_harvest.c. Those files
 *  also own the permission model for the verbs they implement, which is why
 *  the mutating verbs below are called straight through rather than being
 *  pre-screened here: a second rank test in this file would be a second answer
 *  to the same question, and two copies of a predicate drift until the weaker
 *  one lies to the player. For the same reason every refusal reason a player
 *  sees comes from kingdom_verdict_text() inside those modules, never from a
 *  sentence composed here.
 *
 *  What this file does own is the small amount that is genuinely dispatch:
 *  the guild gate on the realm verbs (any MEMBER may look at their own realm;
 *  no rank is asked -- but membership is the engine's own three-part test,
 *  not a bare GET_ASSOC(), see do_kingdom), the DELIBERATE ABSENCE of that
 *  gate on `harvest` and `survey` -- nodes belong to nobody and anyone may
 *  work one, so those two
 *  verbs go straight through to kingdom_harvest.c, which speaks for itself to
 *  a guildless actor -- the confirmation step in front of an irreversible
 *  abandon, the treasury deposit that funds upkeep, and the help text.
 *
 *  PARSING -- WHY lohrr_chop() AND NOT one_argument().
 *  one_argument() (src/cmd/interp.c:2194-2230) wraps its scan in
 *  `do { ... } while (fill_word(first_arg));`, so it silently swallows the
 *  fill words "in", "from", "with", "the", "on", "at" and "to"
 *  (src/cmd/interp.c:1113). `kingdom on` would therefore hand back an EMPTY
 *  token -- and is_abbrev("", anything) returns TRUE, because its loop
 *  `for (i = 0; *(arg1 + i); i++)` never executes for an empty arg1
 *  (src/cmd/interp.c:2241-2257). An empty token matches the first arm of any
 *  abbreviation chain. lohrr_chop() (src/cmd/interp.c:3539) takes the word as
 *  typed, and every dispatch below tests the PARSED token, not the raw
 *  argument, before comparing it.
 */

#include "kingdom/kingdom_internal.h"

#include "core/structs.h"

#include "core/prototypes.h"
#include "core/utility.h"
#include "core/utils.h"
#include "guild/assocs.h"
#include "guild/guildhall.h"

#include <cstdio>
#include <cstring>
#include <string>

/* ------------------------------------------------------------------ *
 * Entry points owned by other modules
 * ------------------------------------------------------------------ *
 * The kingdom siblings this file dispatches to are all declared in
 * kingdom_internal.h. They used to be forward-declared here as well; that was
 * a second copy of each signature which the compiler cannot check across
 * translation units, and its line-number comments had already gone stale
 * once. All that is taken here against its definition is the one engine
 * function declared in no header at all.
 */

/* src/guild/assocs.c:2011, declared in no header. The association subsystem's
 * own coin-string parser ("500 gold 20 silver"); using it keeps
 * `kingdom deposit` speaking the same syntax as `society deposit` rather than
 * growing a second, subtly different one. */
extern bool str_to_money(char *string, int *pc, int *gc, int *sc, int *cc);

/* ------------------------------------------------------------------ *
 * Player-facing text
 * ------------------------------------------------------------------ */

static const char *KINGDOM_SYNTAX =
	"&+WKingdom&n -- guild territory, and the nodes anyone may work.\r\n"
	"  kingdom status           the realm at a glance\r\n"
	"  kingdom map              the 9x9 footprint, every claim numbered\r\n"
	"  kingdom convert          raise your guild to a kingdom       (leader)\r\n"
	"  kingdom claim            buy the next square in order        (leader)\r\n"
	"  kingdom abandon confirm  give up your highest claim, no refund (leader)\r\n"
	"  kingdom deposit <coins>  pay coin into the treasury upkeep draws on\r\n"
	"  kingdom prospect         would a realm stand here? -- no guild needed\r\n"
	"  kingdom harvest          work the node here -- no guild needed\r\n"
	"  kingdom survey           the node here: yield, richness, draws left\r\n"
	"  kingdom guards           how large a garrison the realm may field\r\n"
	"  kingdom roster           every guard: class, level, cost to promote\r\n"
	"  kingdom hire <class>     raise a new guard                    (leader)\r\n"
	"  kingdom promote <#> [class]  raise one to the next tier       (leader)\r\n"
	"  kingdom respec <#> <class>   re-school a guard, same rank    (leader)\r\n"
	"  kingdom champion <class> <class>  raise the realm's one champion (leader)\r\n"
	"  kingdom champion respec <class> <class>  re-school it        (leader)\r\n"
	"  kingdom help             the long explanation\r\n";

static const char *KINGDOM_DISABLED = "Kingdoms are not enabled on this world.\r\n";

/* Said to anyone the membership test in do_kingdom refuses: the guildless,
 * but also an applicant, a banned ex-member, an enemy and someone on parole,
 * none of whom is a member in good standing. The wording covers all of them. */
static const char *KINGDOM_NO_GUILD =
	"You are not a member in good standing of any guild, so you have no realm to\r\n"
	"speak for. You can still '&+Wkingdom harvest&n' or '&+Wkingdom survey&n' any\r\n"
	"node you find.\r\n";

static const char *KINGDOM_NO_REALM =
	"Your guild is not a kingdom. A leader must use '&+Wkingdom convert&n' first.\r\n";

/* ------------------------------------------------------------------ *
 * Small local helpers
 * ------------------------------------------------------------------ */

/* The guild's MAIN guildhall, for surveying a site before any realm exists,
 * comes from kingdom_main_hall() in kingdom.c: the module's one finder. A copy
 * here once matched on hall->guild->get_id() and skipped every hall whose guild
 * pointer was NULL, so `convert` and `map` disagreed about the same hall. */

/* The racewar a prospective site is judged against: the owning guild's own,
 * which Guild::set_racewar keeps inside 1..MAX_RACEWAR
 * (src/guild/assocs.h:202-207). The Underdark ban is absolute regardless of
 * this value (ruled 2026-08-28) and is enforced by kingdom_judge_square(). */
static int kingdom_racewar_of(P_Guild guild)
{
	if (!guild)
		return 0;
	return static_cast<int>(guild->get_racewar());
}

/* Render a copper total as coins.
 *
 * The engine's denominations are 1p = 10g = 100s = 1000c (GET_MONEY,
 * src/core/utils.h:467-469; Guild::deposit does the same arithmetic at
 * src/guild/assocs.c:2616). coins_to_string() returns a STATIC buffer
 * (src/core/utility.c:7291), so exactly ONE result may be live at a time --
 * never pass two of these into a single format string.
 *
 * The all-zero case is reachable. kingdom_upkeep_due() (kingdom_upkeep.c)
 * answers 0 for a realm that owns nothing, which is every realm between
 * `kingdom convert` and its first `kingdom claim`; for a realm whose anchor
 * no longer resolves, which is a dormant one; and for any realm at all once
 * an operator sets upkeep_per_square to 0. Cited by FUNCTION NAME and not by
 * line: that is this module's rule for its own cross-references, and the line
 * numbers this comment used to carry had already gone stale under a rewrite
 * of that file. coins_to_string renders the zero itself:
 * with no denomination set it returns "nothing" in the caller's colour
 * (src/core/utility.c:7355-7358), so no special case is needed here. */
static const char *kingdom_coin_string(long copper_total)
{
	if (copper_total < 0)
		copper_total = 0;

	/* coins_to_string takes ints; clamp so the platinum divide cannot
	 * overflow one whatever the config file says. */
	if (copper_total > 2000000000L)
		copper_total = 2000000000L;

	const int plat = static_cast<int>(copper_total / 1000);
	const int gold_part = static_cast<int>((copper_total / 100) % 10);
	const int silver_part = static_cast<int>((copper_total / 10) % 10);
	const int copper_part = static_cast<int>(copper_total % 10);

	return coins_to_string(plat, gold_part, silver_part, copper_part, "&+y");
}

/* The reader's realm, with its anchor refreshed. NULL after explaining why.
 * Looking at your own realm is not a leader's privilege: the callers ask only
 * that the actor is a member in good standing of a guild (the gate in
 * do_kingdom), deliberately NOT the leader-tier gate the mutating verbs carry
 * inside kingdom_claim.c. */
static kingdom_realm *kingdom_reader_realm(P_char ch, P_Guild guild)
{
	kingdom_realm *realm = kingdom_find_realm(static_cast<int>(guild->get_id()));

	if (!realm)
	{
		send_to_char(KINGDOM_NO_REALM, ch);
		return NULL;
	}

	/* hall_rnum is 0 until this succeeds, and 0 is real_room0's answer for
	 * "no such vnum" as well as for the first room, so an unresolved anchor
	 * must be reported rather than used.
	 *
	 * The wording names the HALL, not the map: resolving now also fails when
	 * the association's main hall no longer stands on the realm's seat --
	 * destroyed, demoted to an outpost, or moved -- and in those cases the
	 * map room itself is still perfectly findable. */
	if (!kingdom_resolve_anchor(*realm))
	{
		send_to_char("No main guildhall of yours stands on the realm's seat, so it has "
			     "no centre.\r\n",
			     ch);
		return NULL;
	}
	return realm;
}

/* ------------------------------------------------------------------ *
 * kingdom status
 * ------------------------------------------------------------------ */

/* Print the realm's status table (kingdom_display.c) for the reader's own
 * realm; kingdom_reader_realm() has already explained any refusal. */
static void kingdom_cmd_status(P_char ch, P_Guild guild)
{
	kingdom_realm *realm = kingdom_reader_realm(ch, guild);

	if (!realm)
		return;

	kingdom_show_status(ch, *realm);
}

/* ------------------------------------------------------------------ *
 * kingdom map
 * ------------------------------------------------------------------ */

/* Draw the 9x9 ring grid: the realm's own footprint when one exists, or else
 * the ground around the guild's main hall so a leader can judge a site before
 * converting. Refuses when the guild has no hall or the hall is off the map. */
static void kingdom_cmd_map(P_char ch, P_Guild guild)
{
	const int assoc_id = static_cast<int>(guild->get_id());
	kingdom_realm *realm = kingdom_find_realm(assoc_id);

	if (realm)
	{
		kingdom_realm *ready = kingdom_reader_realm(ch, guild);

		if (!ready)
			return;

		kingdom_show_map(ch, *ready);
		return;
	}

	/* No realm yet. Survey the ground the guildhall stands on, so a leader
	 * can see whether `kingdom convert` will be accepted before trying it.
	 * highest_claim 0 means "nothing held"; the grid judges the rest. */
	Guildhall *hall = kingdom_main_hall(assoc_id);

	if (!hall)
	{
		send_to_char("Your guild has no guildhall, so there is no land to show.\r\n", ch);
		return;
	}

	/* real_room0() answers 0 for BOTH "the first room" and "no such vnum"
	 * (src/world/db.c), so 0 means absent here. */
	const int hall_rnum = real_room0(hall->outside_vnum);

	if (hall_rnum <= 0)
	{
		send_to_char("Your guildhall does not stand on the map.\r\n", ch);
		return;
	}

	kingdom_show_grid(ch, hall_rnum, kingdom_racewar_of(guild), assoc_id, 0,
			  "&+CThe ground your hall stands on&n");
}

/* ------------------------------------------------------------------ *
 * kingdom abandon
 * ------------------------------------------------------------------ */

/* The act itself, its rank test and its refusals all belong to
 * kingdom_abandon_last(). What is added here is the confirmation: releasing a
 * square pays nothing back and retaking it costs the full original price
 * again (ruled 2026-08-28), so the player says so out loud first. */
static void kingdom_cmd_abandon(P_char ch, P_Guild guild, const char *token)
{
	/* An exact word, not is_abbrev: is_abbrev would accept a bare "c",
	 * which is far too cheap a gesture for an act with no refund. str_cmp is
	 * the case-insensitive compare macro at src/core/utils.h:46. */
	if (token && *token && str_cmp(token, "confirm") == 0)
	{
		kingdom_abandon_last(ch);
		return;
	}

	/* Preview. A realm may be absent, or hold nothing -- say the general
	 * warning in that case and let the verb itself produce the specific
	 * refusal when the player confirms. */
	const kingdom_realm *realm = kingdom_find_realm(static_cast<int>(guild->get_id()));

	if (realm && realm->highest_claim >= 1)
		send_to_char_f(
			ch,
			"Abandoning square &+W#%d&n pays nothing back, and taking it again later\r\n"
			"costs the full %s.\r\n",
			realm->highest_claim,
			kingdom_coin_string(kingdom_claim_cost(realm->highest_claim)));
	else
		send_to_char(
			"Abandoning land pays nothing back, and retaking it costs full price.\r\n",
			ch);

	send_to_char("Type '&+Wkingdom abandon confirm&n' if you mean it.\r\n", ch);
}

/* ------------------------------------------------------------------ *
 * kingdom deposit
 * ------------------------------------------------------------------ */

/* Upkeep is charged in COIN, and coin lives in the guild treasury -- the
 * realm's own stores are typed resources that can never be withdrawn (ruled
 * 2026-08-28), which is exactly why they are not a treasury. So this verb
 * funds the pot the upkeep event draws on, through the association's own
 * deposit path and under the association's own bank rule
 * (src/guild/assocs.c:2280-2284) rather than a second one invented here.
 *
 * No rank test: paying money in is not a privilege, and `society deposit` does
 * not gate it either. */
static void kingdom_cmd_deposit(P_char ch, P_Guild guild, char *rest)
{
	kingdom_realm *realm = kingdom_find_realm(static_cast<int>(guild->get_id()));

	if (!realm)
	{
		send_to_char(KINGDOM_NO_REALM, ch);
		return;
	}

	if (!test_atm_present(ch))
	{
		send_to_char("I don't see a bank around here.\r\n", ch);
		return;
	}

	int plat_coins = 0;
	int gold_coins = 0;
	int silver_coins = 0;
	int copper_coins = 0;

	/* str_to_money dereferences its argument immediately (assocs.c:2019), so
	 * it must never be handed a null. */
	if (!rest || !*rest ||
	    !str_to_money(rest, &plat_coins, &gold_coins, &silver_coins, &copper_coins))
	{
		send_to_char("A coin string looks like '&+w500 gold 20 silver&n'.\r\n", ch);
		return;
	}

	/* str_to_money accepts a leading '-', because is_number does
	 * (src/cmd/interp.c:2142-2153). A negative component would underflow the
	 * treasury's unsigned counters inside Guild::deposit, so refuse here. */
	if (plat_coins < 0 || gold_coins < 0 || silver_coins < 0 || copper_coins < 0)
	{
		send_to_char("You cannot deposit a negative number of coins.\r\n", ch);
		return;
	}

	if (plat_coins == 0 && gold_coins == 0 && silver_coins == 0 && copper_coins == 0)
	{
		send_to_char("Don't be silly.\r\n", ch);
		return;
	}

	/* Say the kingdom part BEFORE handing off. Guild::deposit prints its own
	 * confirmation, saves the character and saves the guild; nothing here may
	 * touch ch afterwards, so it is the last thing this function does. */
	send_to_char_f(ch, "Upkeep for your realm is %s each cycle.\r\n",
		       kingdom_coin_string(kingdom_upkeep_due(*realm)));

	guild->deposit(ch, plat_coins, gold_coins, silver_coins, copper_coins);
}

/* ------------------------------------------------------------------ *
 * kingdom guards
 * ------------------------------------------------------------------ */

/* Report the realm's guard allowance against the guards actually standing,
 * and point at `kingdom status` when arrears have taken the garrison. */
static void kingdom_cmd_guards(P_char ch, P_Guild guild)
{
	kingdom_realm *realm = kingdom_reader_realm(ch, guild);

	if (!realm)
		return;

	/* kingdom_guard_allowance() owns the scaling and kingdom_guards_count()
	 * owns the head-count (one walk of character_list matching this realm's
	 * stamped guards; kingdom_guards.c, cited by function name as this module
	 * cites its own). This reports the numbers they return and never
	 * recomputes either from the square count. */
	const int allowance = kingdom_guard_allowance(*realm);

	send_to_char_f(ch,
		       "&+WGarrison&n\r\n"
		       "  Your realm holds %d of %d squares and may field up to %d guard(s).\r\n"
		       "  Standing right now: &+G%d&n of %d.\r\n",
		       realm->highest_claim, KINGDOM_MAX_SQUARES, allowance,
		       kingdom_guards_count(realm->assoc_id), allowance);

	/* The rung the realm is on, and the words for it, belong to the status
	 * table; point at it rather than keeping a second vocabulary here. */
	if (realm->arrears != KARR_CURRENT)
		send_to_char(
			"  &+RUnpaid upkeep takes the guards first. See '&+Wkingdom status&+R'.&n\r\n",
			ch);
}

/* ------------------------------------------------------------------ *
 * kingdom help
 * ------------------------------------------------------------------ */

/* Every number below is read from the module that owns it, so the help can
 * never quietly disagree with the code. */
static void kingdom_cmd_help(P_char ch)
{
	std::string out;
	char line[MAX_INPUT_LENGTH];

	out += "&+WKINGDOMS&n\r\n\r\n";
	out += "A guild that converts to a kingdom takes the ground outside its guildhall as\r\n";
	out += "the seat of a realm, then buys the land around it in four concentric rings.\r\n\r\n";

	for (int ring = 1; ring <= KINGDOM_MAX_RING; ring++)
	{
		snprintf(line, sizeof(line),
			 "  ring %d   %2d squares, claims #%d to #%d, %s to complete\r\n", ring,
			 kingdom_ring_size(ring), kingdom_ring_first_index(ring),
			 kingdom_ring_last_index(ring),
			 kingdom_coin_string(kingdom_ring_cost(ring)));
		out += line;
	}

	snprintf(line, sizeof(line), "\r\n%d squares in all, %d counting the seat itself.\r\n",
		 KINGDOM_MAX_SQUARES, KINGDOM_MAX_SQUARES + 1);
	out += line;

	out += "\r\nThe order is FIXED. Within a ring, land is taken clockwise starting due\r\n";
	out += "north of the hall, and a ring must be finished before the next one opens.\r\n";
	out += "Because the order never varies, a realm's territory is one number: it holds\r\n";
	out += "claims 1 to its highest and nothing else. '&+Wkingdom map&n' numbers them all.\r\n";

	out += "\r\n&+WWhere a realm may stand&n\r\n";
	out += "  All eighty squares must be claimable when the guildhall is built, so a hall\r\n";
	out += "  can only be sited where a whole realm could later exist. The Underdark is\r\n";
	out += "  barred outright, whatever your racewar.\r\n";

	snprintf(line, sizeof(line),
		 "  Two realms may not even touch: guildhall seats must be at least %d squares\r\n"
		 "  apart, leaving open ground between their footprints.\r\n",
		 kingdom_min_hall_separation());
	out += line;

	out += "  '&+Wkingdom prospect&n' judges the square you stand on and, when it will not\r\n";
	out += "  serve, points at the nearest ground that would. It needs no guild.\r\n";

	out += "\r\n&+WWhat land costs&n\r\n";
	out += "  Coin AND worked material, on the same compounding curve.\r\n";
	/* ONE kingdom_coin_string() PER FORMAT CALL. It ends in coins_to_string(),
	 * which returns a STATIC buffer (core/utility.c), so two of them in one
	 * snprintf would both render whichever value was formatted last -- the
	 * first square and the eightieth would print the same price. */
	snprintf(line, sizeof(line), "  Square 1 costs %s and %ld of EVERY resource.\r\n",
		 kingdom_coin_string(kingdom_claim_cost(1)), kingdom_claim_material_cost(1));
	out += line;
	snprintf(line, sizeof(line), "  Square %d costs %s and %ld of each.\r\n",
		 KINGDOM_MAX_SQUARES, kingdom_coin_string(kingdom_claim_cost(KINGDOM_MAX_SQUARES)),
		 kingdom_claim_material_cost(KINGDOM_MAX_SQUARES));
	out += line;
	out += "  A realm short of even one resource claims nothing, so expansion cannot run\r\n";
	out += "  on one kind of ground alone.\r\n";

	out += "\r\n&+WThe garrison&n\r\n";
	snprintf(line, sizeof(line),
		 "  Guards are BOUGHT, one at a time: %s each, at level %d. The land sets how\r\n"
		 "  many ('&+Wkingdom guards&n') and how high they may rise -- level 50 after\r\n"
		 "  ring one, then two per ring to %d. '&+Wkingdom roster&n' lists them.\r\n",
		 kingdom_coin_string(kingdom_cfg.guard_cost_base), KINGDOM_GUARD_BASE_LEVEL,
		 KINGDOM_GUARD_TOP_LEVEL);
	out += line;
	char champion_classes[512];

	kingdom_champion_class_list(champion_classes, sizeof(champion_classes));
	snprintf(line, sizeof(line),
		 "  A realm holding every square may raise ONE champion for %s: level %d,\r\n"
		 "  multiclass, and it plants a banner that buffs or heals the whole garrison\r\n"
		 "  until someone tears it down. Losing a square UNMAKES it; retake the\r\n"
		 "  eightieth and a new one may be raised, at the full price. It may take\r\n"
		 "  callings no guard may: %s.\r\n",
		 kingdom_coin_string(KINGDOM_CHAMPION_COST), KINGDOM_CHAMPION_LEVEL,
		 champion_classes);
	out += line;
	snprintf(line, sizeof(line),
		 "  A guard is re-schooled at its present rank for %lu prestige\r\n"
		 "  ('&+Wkingdom respec <#> <class>&n'), the champion for %lu\r\n"
		 "  ('&+Wkingdom champion respec <class> <class>&n'). Levels never move.\r\n",
		 (unsigned long)KINGDOM_GUARD_RESPEC_PRESTIGE,
		 (unsigned long)KINGDOM_CHAMPION_RESPEC_PRESTIGE);
	out += line;

	out += "\r\n&+WUpkeep&n\r\n";
	snprintf(line, sizeof(line),
		 "  Every %d seconds -- one real week as shipped -- the realm is charged coin\r\n"
		 "  for the land it holds. The coin comes from the guild treasury;\r\n"
		 "  '&+Wkingdom deposit&n' pays into it.\r\n",
		 kingdom_cfg.upkeep_period_seconds);
	out += line;
	out += "  Unpaid upkeep walks a ladder, one rung per missed cycle, and stops wherever\r\n";
	out += "  payment arrives:\r\n";
	out += "    1  the guards disperse\r\n";
	out += "    2  the realm stops banking what its people gather\r\n";
	out += "    3  the OUTERMOST SQUARE reverts, and another every cycle after that\r\n";
	out += "  Land that reverts costs the full original price to take back, and losing a\r\n";
	out += "  square unmakes the champion: only a realm holding all eighty may have one.\r\n";

	out += "\r\n&+WNodes&n\r\n";
	out += "  Resource nodes load at random across the whole world, the Underdark\r\n";
	out += "  included, and never on ground a realm holds. ANYONE may work one, guild or\r\n";
	out += "  no guild. A kingdom's advantage is the YIELD: the land a realm holds makes\r\n";
	out += "  matching nodes give more, everywhere. A node worked but not finished rots\r\n";
	out += "  away after a time. Nodes draw on the overhead map as coloured letters.\r\n";

	out += "\r\n&+WStores&n\r\n";
	out += "  Harvested resources are held by the realm, not by the treasury, and are\r\n";
	out += "  spent on the realm's own works. They cannot be withdrawn by anyone.\r\n";

	out += "\r\n'&+Whelp kingdoms&n' holds the full rules.\r\n\r\n";
	out += KINGDOM_SYNTAX;

	send_to_char(out.c_str(), ch);
}

/* ------------------------------------------------------------------ *
 * The command
 * ------------------------------------------------------------------ */

/* `kingdom <verb> [rest]`: parse the verb as typed, refuse the three
 * ambiguous single letters, dispatch help/survey/harvest to anyone, then
 * apply the membership gate and dispatch the realm verbs. A bare `kingdom`
 * shows the status table to a realm's member and the syntax to anyone else. */
void do_kingdom(P_char ch, char *argument, int /*cmd*/)
{
	if (!ch || IS_NPC(ch))
		return;

	if (!kingdom_enabled())
	{
		send_to_char(KINGDOM_DISABLED, ch);
		return;
	}

	/* lohrr_chop copies into arg1 with no bound of its own
	 * (src/cmd/interp.c:3539-3593), so the destination is sized here.
	 *
	 * IT CAN WRITE ROUGHLY TWICE THE INPUT. On an opening quote with no
	 * closing quote it copies to the end of the string, then discovers the
	 * quote was never closed, resets `index = string` (interp.c:3578-3579)
	 * and copies the WHOLE string a second time WITHOUT rewinding arg1, so a
	 * line of n characters yields 2n-1 bytes plus a terminator. Player input
	 * reaches a command already capped at MAX_INPUT_LENGTH-1 by the reader
	 * (src/net/comm.c:3670, :3718-3723), so `kingdom 'aaaa...` with ~600 a's
	 * would overrun a MAX_INPUT_LENGTH buffer and smash this frame. Two times
	 * the cap is the worst case, so that is what it gets. The engine's own
	 * callers (str_to_money at src/guild/assocs.c:2013, and others) size
	 * theirs at MAX_INPUT_LENGTH and are exposed; fixing lohrr_chop is not
	 * this file's to make, but wearing its bug is. */
	char token[2 * MAX_INPUT_LENGTH];
	char *rest = lohrr_chop(argument, token);

	if (!rest)
		rest = token + strlen(token); /* lohrr_chop returns NULL only for null input */

	/* GUARD THE PARSED TOKEN. An empty token makes is_abbrev return TRUE
	 * against the first candidate it is offered, so nothing below may be
	 * reached with one. */

	/* "h" is a prefix of both `help` and `harvest`, and help is dispatched
	 * first below, so a bare "h" would silently become help. Refuse it the
	 * way the c/ and s/ collisions are refused. It must sit AHEAD of the
	 * help arm to be reached at all, and neither verb needs a guild, so no
	 * gate is jumped. str_cmp on an empty token is safe -- "" equals
	 * neither word. */
	if (str_cmp(token, "h") == 0)
	{
		send_to_char("Do you mean '&+Wkingdom help&n', '&+Wkingdom harvest&n' or "
			     "'&+Wkingdom hire&n'?\r\n",
			     ch);
		return;
	}
	/* `p`, `pr` and `pro` are shared by prospect and promote, and the two sit on opposite
	 * sides of the guild gate -- prospect ahead of it, promote behind --
	 * so an unresolved prefix would silently become the one that dispatches
	 * first. Refuse it here, ahead of both. */
	if (str_cmp(token, "p") == 0 || str_cmp(token, "pr") == 0 || str_cmp(token, "pro") == 0)
	{
		send_to_char("Do you mean '&+Wkingdom prospect&n' or '&+Wkingdom promote&n'?\r\n",
			     ch);
		return;
	}

	/* help sits ahead of the guild test on purpose: someone deciding whether
	 * to found a kingdom has to be able to read the rules first. */
	if (*token && is_abbrev(token, "help"))
	{
		kingdom_cmd_help(ch);
		return;
	}

	/* "c" is a prefix of both `claim` and `convert`, and is_abbrev would hand
	 * it to whichever arm came first. Both are leader-tier acts that move
	 * money, so refuse the ambiguity instead of guessing. `s` is likewise
	 * shared by `status` and `survey`; both refusals sit ahead of the guild
	 * gate because `survey` itself does. */
	if (str_cmp(token, "c") == 0)
	{
		send_to_char("Do you mean '&+Wkingdom claim&n' or '&+Wkingdom convert&n'?\r\n", ch);
		return;
	}
	if (str_cmp(token, "s") == 0)
	{
		send_to_char("Do you mean '&+Wkingdom status&n' or '&+Wkingdom survey&n'?\r\n", ch);
		return;
	}
	/* `r`, `re` and `res` are shared by `roster` and `respec`: one lists the
	 * garrison and the other spends a thousand prestige on it. */
	if (str_cmp(token, "r") == 0 || str_cmp(token, "re") == 0 || str_cmp(token, "res") == 0)
	{
		send_to_char("Do you mean '&+Wkingdom roster&n' or '&+Wkingdom respec&n'?\r\n", ch);
		return;
	}

	/* harvest and survey are NOT guild verbs, so they dispatch AHEAD of the
	 * guild gate below. Nodes load anywhere except on land a realm controls,
	 * and ANYONE may work one, guild or no guild -- a kingdom's benefit is a
	 * yield bonus from the type of land it holds, not access to the node.
	 * Both entry points were written for a guildless actor:
	 * kingdom_realm_of_char() inside kingdom_harvest.c answers NULL for one,
	 * and each says where the yield would (or would not) go. Every realm verb
	 * below keeps the gate. */
	if (*token && is_abbrev(token, "survey"))
	{
		kingdom_harvest_survey(ch);
		return;
	}
	if (*token && is_abbrev(token, "harvest"))
	{
		kingdom_harvest_command(ch, rest);
		return;
	}
	/* prospect joins them ahead of the gate, for the same reason and a
	 * stronger one: it is the verb someone uses to decide whether founding a
	 * guild is worth it at all, so demanding the guild would make it useless
	 * exactly when it is wanted. */
	if (*token && is_abbrev(token, "prospect"))
	{
		kingdom_prospect(ch);
		return;
	}

	P_Guild guild = GET_ASSOC(ch);

	/* GET_ASSOC() alone is NOT membership: Guild::apply() points an
	 * applicant's assoc pointer at the guild it is applying to, and a banned
	 * character keeps the pointer too. The realm verbs below read the realm's
	 * arrears rung, its stores and its guard count, none of which an
	 * applicant, a banned ex-member, an enemy or someone on parole has any
	 * business seeing -- so this is the module's own membership rule, the
	 * one kingdom_char_owns_room() (kingdom.c) and IS_ASSOC_MEMBER
	 * (guild/assocs.h) spell out, not a bare null test. */
	if (!guild || !IS_MEMBER(GET_A_BITS(ch)) || !GT_PAROLE(GET_A_BITS(ch)))
	{
		send_to_char(KINGDOM_NO_GUILD, ch);
		return;
	}

	if (!*token)
	{
		if (kingdom_find_realm(static_cast<int>(guild->get_id())))
			kingdom_cmd_status(ch, guild);
		else
			send_to_char(KINGDOM_SYNTAX, ch);
		return;
	}

	if (is_abbrev(token, "status"))
	{
		kingdom_cmd_status(ch, guild);
	}
	else if (is_abbrev(token, "map"))
	{
		kingdom_cmd_map(ch, guild);
	}
	else if (is_abbrev(token, "convert"))
	{
		kingdom_convert_guild(ch);
	}
	else if (is_abbrev(token, "claim"))
	{
		kingdom_claim_next(ch);
	}
	else if (is_abbrev(token, "abandon"))
	{
		/* Sized like `token` above: `rest` still points into the raw input,
		 * so the same unclosed-quote doubling applies. */
		char confirm[2 * MAX_INPUT_LENGTH];

		lohrr_chop(rest, confirm);
		kingdom_cmd_abandon(ch, guild, confirm);
	}
	else if (is_abbrev(token, "deposit"))
	{
		kingdom_cmd_deposit(ch, guild, rest);
	}
	else if (is_abbrev(token, "guards"))
	{
		kingdom_cmd_guards(ch, guild);
	}
	else if (is_abbrev(token, "roster"))
	{
		kingdom_roster_show(ch);
	}
	else if (is_abbrev(token, "hire"))
	{
		kingdom_roster_hire(ch, rest);
	}
	else if (is_abbrev(token, "promote"))
	{
		kingdom_roster_upgrade(ch, rest);
	}
	else if (is_abbrev(token, "champion"))
	{
		/* `kingdom champion respec <class> <class>` re-schools the one the
		 * realm already has; anything else raises a new one. */
		char second[MAX_INPUT_LENGTH];
		char *after = one_argument(rest, second);

		if (*second && is_abbrev(second, "respec"))
		{
			kingdom_roster_champion_respec(ch, after);
		}
		else
		{
			kingdom_roster_champion(ch, rest);
		}
	}
	else if (is_abbrev(token, "respec"))
	{
		kingdom_roster_respec(ch, rest);
	}
	else
	{
		send_to_char(KINGDOM_SYNTAX, ch);
	}
}
