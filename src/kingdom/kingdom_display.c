/*
 *  kingdom_display.c
 *  Duris
 *
 *  Everything the kingdom code shows a player: the realm status table, the
 *  9x9 territory grid, and the one-line banner the map legend and the room
 *  description hang off.
 *
 *  THIS FILE OWNS NO RULES.
 *
 *  What a realm holds is the single integer highest_claim -- claims
 *  1..highest_claim and nothing else (kingdom_internal.h). Whether a square
 *  may be part of a realm is decided by kingdom_judge_square() and by nothing
 *  else. The display asks both and renders the answers. It never re-derives
 *  either, because two copies of an availability predicate drift apart and it
 *  is always the weaker copy that lies -- here, to the player who is about to
 *  spend six figures of coin on a square the claim code will then refuse.
 *
 *  Text is accumulated with APPENDF()/checked_appendf() (core/safe_format.h),
 *  never with the legacy
 *      snprintf(buf + strlen(buf), CAPACITY - strlen(buf), ...)
 *  idiom. That idiom restates the capacity at every call site (and this tree
 *  historically restated it wrongly), and -Wformat-truncation=2 is fatal in
 *  this build. APPENDF deduces the array's real capacity from its type, tracks
 *  the used offset itself, and truncates rather than overflowing.
 */

#include "kingdom/kingdom_internal.h"

#include "core/structs.h"

#include "core/prototypes.h"
#include "core/utils.h"
#include "guild/assocs.h"

extern struct room_data *world;
extern int top_of_world;

/* One display costs well under 2 KB; this leaves generous headroom, and
 * APPENDF truncates rather than overflowing if a guild name is pathological. */
#define KINGDOM_DISPLAY_BUF 4096

/* Verdict codes run KSQ_OK..KSQ_HAS_GUILDHALL (kingdom_internal.h). Used to
 * size the tally of why squares are refused. */
#define KINGDOM_VERDICT_COUNT (KSQ_HAS_GUILDHALL + 1)

/* ------------------------------------------------------------------ *
 * Small local helpers
 * ------------------------------------------------------------------ */

/* True when rnum indexes a real room. rnum 0 is rejected on purpose: it is
 * both the first room and real_room0()'s "no such vnum" answer
 * (src/world/db.c:4477-4508), and every caller here means the latter --
 * kingdom_realm::hall_rnum uses 0 for "anchor unresolved". */
static bool valid_rnum(int rnum)
{
	return rnum > 0 && rnum <= top_of_world;
}

/* A room's title, or a placeholder. room_data::name is a plain char * and the
 * tree's FREE() macro NULLs what it frees (core/utils.h:104-108), so a name
 * can genuinely be absent -- core/files.c:3086-3088 and
 * item/storage_lockers.c:185-188 both guard it the same way. A NULL handed to
 * %s is undefined behaviour rather than a guaranteed "(null)", so the room
 * title never reaches a format unchecked. */
static const char *room_title(int rnum)
{
	if (!valid_rnum(rnum) || !world[rnum].name)
		return "<unnamed>";
	return world[rnum].name;
}

/* The owning guild's name, or a placeholder. Guild::get_name() returns a
 * std::string BY VALUE (guild/assocs.h:196), so the caller must hold the
 * returned string alive for as long as it uses .c_str(). */
static std::string realm_name(int assoc_id)
{
	P_Guild guild = get_guild_from_id(assoc_id);

	/* get_guild_from_id() walks guild_list and answers NULL for an unknown
	 * id (guild/assocs.c:76-87). A realm whose guild is gone is a bug
	 * elsewhere -- kingdom_on_guild_deleted() should have dropped it -- but
	 * the display must still render rather than dereference NULL. */
	if (!guild)
		return std::string("an abandoned realm");

	std::string name = guild->get_name();
	if (name.empty())
		return std::string("an unnamed realm");
	return name;
}

/* Racewar side of the owning guild, or 0 when the guild is gone. Passed
 * through to kingdom_judge_square(), which carries the parameter on its seam
 * but -- the Underdark ban being absolute (RULINGS.md, answer 5) -- consults
 * no racewar-sensitive rule, so a 0 here changes no verdict today. */
static int realm_racewar(int assoc_id)
{
	P_Guild guild = get_guild_from_id(assoc_id);

	if (!guild)
		return 0;
	return static_cast<int>(guild->get_racewar());
}

/* "3 north, 2 east" for a claim offset. dy is NEGATIVE to the north: origin
 * (0,0) of a map zone is its north-west corner and +y runs south
 * (kingdom_geometry.h, proven from src/cmd/testcmd.c:182-189). */
static void describe_offset(int dx, int dy, char *out, size_t capacity)
{
	if (!out || !capacity)
		return;
	out[0] = '\0';

	if (dy < 0)
		checked_appendf(out, capacity, "%d north", -dy);
	else if (dy > 0)
		checked_appendf(out, capacity, "%d south", dy);

	if (dx != 0 && dy != 0)
		checked_appendf(out, capacity, ", ");

	if (dx > 0)
		checked_appendf(out, capacity, "%d east", dx);
	else if (dx < 0)
		checked_appendf(out, capacity, "%d west", -dx);

	if (out[0] == '\0')
		checked_appendf(out, capacity, "the seat itself");
}

/* Local time, or "never" for an unset stamp.
 *
 * Returns a pointer into a single static buffer, so a caller must not use two
 * of these in one format call -- both arguments would show the later time. */
static const char *stamp(time_t when)
{
	static char text[32];
	struct tm local = {};

	if (when <= 0 || !localtime_r(&when, &local) ||
	    strftime(text, sizeof(text), "%Y-%m-%d %H:%M", &local) == 0)
		return "never";
	return text;
}

/* Player-facing name of a rung on the arrears ladder. This renders the rung
 * the upkeep code has already decided (kingdom_realm::arrears); it does not
 * work out which rung applies. Order ruled 2026-08-28: guards, then nodes,
 * then one outer ring per missed cycle, halting wherever payment arrives. */
static const char *arrears_text(int arrears)
{
	switch (arrears)
	{
	case KARR_CURRENT:
		return "&+Gpaid up&n";
	case KARR_GUARDS_GONE:
		return "&+Yin arrears -- the guards have gone home&n";
	case KARR_NODES_DORMANT:
		return "&+Yin arrears -- guards gone, harvest nodes dormant&n";
	case KARR_LAND_REVERTING:
		return "&+Rin arrears -- a square reverts every missed cycle&n";
	default:
		return "&+Runknown&n";
	}
}

/* kingdom_verdict_text() is another file's; a %s of NULL is undefined
 * behaviour, so never hand its answer straight to a format. */
static const char *verdict_text(int verdict)
{
	const char *text = kingdom_verdict_text(verdict);

	return text ? text : "the square cannot be claimed";
}

/* The grid's frame. Takes an explicit capacity because a char * parameter has
 * lost its array bound -- which is exactly why APPENDF() is a template that
 * refuses to compile on a pointer instead of silently using sizeof(char *). */
static void append_border(char *out, size_t capacity)
{
	checked_appendf(out, capacity, "    &+w+");
	for (int i = 0; i < KINGDOM_FOOTPRINT_SIDE * 3 + 1; i++)
		checked_appendf(out, capacity, "-");
	checked_appendf(out, capacity, "+&n\r\n");
}

/* ------------------------------------------------------------------ *
 * The 9x9 territory grid
 * ------------------------------------------------------------------ *
 * Every cell's state comes from exactly two questions:
 *
 *   owned?      index <= highest_claim   -- the whole ownership record
 *   claimable?  kingdom_judge_square()   -- the single placement authority
 *
 * Nothing here second-guesses either. A square that is owned but no longer
 * judged eligible is shown as such rather than quietly painted green: that is
 * a real state (a guildhall raised inside the footprint, a sector retyped) and
 * hiding it would leave the player wondering why a ring will not come back
 * after they pay their arrears.
 *
 * `heading` is the caption line, already coloured, without a newline.
 * `ignore_assoc` is passed straight through so a realm does not collide with
 * itself in the overlap test; pass 0 when surveying a site for a realm that
 * does not exist yet. `highest_claim` may be 0 for such a survey.
 */
void kingdom_show_grid(struct char_data *ch, int hall_rnum, int racewar, int ignore_assoc,
		       int highest_claim, const char *heading)
{
	char out[KINGDOM_DISPLAY_BUF] = "";
	int tally[KINGDOM_VERDICT_COUNT] = {};
	int blocked_total = 0;

	if (!ch)
		return;

	if (!valid_rnum(hall_rnum))
	{
		send_to_char("That realm has no seat on the map, so there is nothing to draw.\r\n",
			     ch);
		return;
	}

	if (heading && *heading)
		APPENDF(out, "%s\r\n", heading);

	/* Column ruler, then the frame. The row prefix below is five visible
	 * characters ("%3d |"), so the ruler is indented by five and each cell
	 * is three wide; that keeps every column under its own label. */
	APPENDF(out, "                &+wnorth&n\r\n");
	APPENDF(out, "     ");
	for (int dx = -KINGDOM_FOOTPRINT_RADIUS; dx <= KINGDOM_FOOTPRINT_RADIUS; dx++)
		APPENDF(out, "&+w%3d&n", dx);
	APPENDF(out, "\r\n");
	append_border(out, sizeof(out));

	for (int dy = -KINGDOM_FOOTPRINT_RADIUS; dy <= KINGDOM_FOOTPRINT_RADIUS; dy++)
	{
		APPENDF(out, "&+w%3d |&n", dy);

		for (int dx = -KINGDOM_FOOTPRINT_RADIUS; dx <= KINGDOM_FOOTPRINT_RADIUS; dx++)
		{
			const int index = kingdom_index_for_offset(dx, dy);

			/* Inside the 9x9 box the only offset that is not a claim
			 * is (0,0), the hall's own square -- so index 0 here
			 * means the seat, never "off the footprint". */
			if (index == 0)
			{
				APPENDF(out, " &+C[]&n");
				continue;
			}

			const int verdict =
				kingdom_judge_square(hall_rnum, index, racewar, ignore_assoc);

			if (verdict != KSQ_OK)
			{
				blocked_total++;
				if (verdict > KSQ_OK && verdict < KINGDOM_VERDICT_COUNT)
					tally[verdict]++;
			}

			/* Held squares are drawn from highest_claim, not from the
			 * verdict; magenta only says the ground under a held
			 * square would no longer be granted today. The format
			 * strings stay literal because -Wformat=2 rejects a
			 * non-literal format even when it is a ternary of two. */
			if (index <= highest_claim && verdict == KSQ_OK)
				APPENDF(out, " &+G%2d&n", index);
			else if (index <= highest_claim)
				APPENDF(out, " &+M%2d&n", index);
			else if (verdict != KSQ_OK)
				APPENDF(out, " &+RXX&n");
			else if (index == highest_claim + 1)
				APPENDF(out, " &+Y%2d&n", index);
			else
				APPENDF(out, " &+w%2d&n", index);
		}

		APPENDF(out, " &+w|&n\r\n");
	}

	append_border(out, sizeof(out));
	APPENDF(out, "                &+wsouth&n     &+w(west is left, east is right)&n\r\n");
	APPENDF(out, "\r\n");
	APPENDF(out, "  &+C[]&n seat  &+G##&n held  &+Y##&n next to claim"
		     "  &+w##&n awaiting its turn  &+RXX&n barred\r\n");
	APPENDF(out, "  &+M##&n held, but the ground would no longer be granted today\r\n");
	APPENDF(out, "  &+wThe numbers are claim order: a ring completes before the next opens.&n"
		     "\r\n");

	if (blocked_total > 0)
	{
		int named = 0;

		APPENDF(out, "\r\n  &+RBarred squares:&n\r\n");
		for (int verdict = KSQ_OK + 1; verdict < KINGDOM_VERDICT_COUNT; verdict++)
		{
			if (tally[verdict] <= 0)
				continue;
			named += tally[verdict];
			APPENDF(out, "    &+R%2d&n %s -- %s\r\n", tally[verdict],
				tally[verdict] == 1 ? "square " : "squares", verdict_text(verdict));
		}

		/* A verdict outside KSQ_OK..KSQ_HAS_GUILDHALL cannot happen while
		 * the enum and this file are compiled together, but if the enum
		 * ever grows without this tally growing with it, say so rather
		 * than draw XX squares the reason list silently omits. */
		if (blocked_total > named)
			APPENDF(out, "    &+R%2d&n squares -- reason not recognised\r\n",
				blocked_total - named);
	}

	send_to_char(out, ch);
}

/* The grid for an existing realm. */
void kingdom_show_map(struct char_data *ch, const kingdom_realm &realm)
{
	char heading[ASC_MAX_STR + 64] = "";

	if (!ch)
		return;

	/* An unresolved anchor means no main hall stands on the seat (destroyed,
	 * demoted or moved) as often as it means the room itself is gone, so the
	 * refusal names the seat rather than the map. */
	if (!valid_rnum(realm.hall_rnum))
	{
		send_to_char("Your realm has no seat to draw its lands around.\r\n", ch);
		return;
	}

	const std::string name = realm_name(realm.assoc_id);
	APPENDF(heading, "&+CThe lands of &n%s&n", name.c_str());

	kingdom_show_grid(ch, realm.hall_rnum, realm_racewar(realm.assoc_id), realm.assoc_id,
			  realm.highest_claim, heading);
}

/* ------------------------------------------------------------------ *
 * The status table
 * ------------------------------------------------------------------ */

/* The realm status table: seat, territory by ring, the next claim with its
 * price or the reason it is barred, guards, upkeep, standing and resources.
 * Reads the record and asks the placement authority; decides nothing itself. */
void kingdom_show_status(struct char_data *ch, const kingdom_realm &realm)
{
	char out[KINGDOM_DISPLAY_BUF] = "";

	if (!ch)
		return;

	const std::string name = realm_name(realm.assoc_id);
	const int racewar = realm_racewar(realm.assoc_id);

	APPENDF(out, "&+CThe realm of &n%s&n\r\n", name.c_str());
	APPENDF(out, "&+w----------------------------------------------------------------&n\r\n");

	/* Seat. hall_rnum is 0 until kingdom_resolve_anchor() succeeds, and a
	 * guildhall that has moved or been destroyed leaves it that way. */
	if (valid_rnum(realm.hall_rnum))
	{
		int zone_idx = -1, hx = -1, hy = -1;

		if (kingdom_square_of_room(realm.hall_rnum, &zone_idx, &hx, &hy))
			APPENDF(out, " &+wSeat       &n: %s&n &+w(room %d, map %d,%d)&n\r\n",
				room_title(realm.hall_rnum), world[realm.hall_rnum].number, hx, hy);
		else
			APPENDF(out, " &+wSeat       &n: %s&n &+w(room %d)&n\r\n",
				room_title(realm.hall_rnum), world[realm.hall_rnum].number);
	}
	else
	{
		APPENDF(out, " &+wSeat       &n: &+Rlost -- the guildhall is gone or has moved&n"
			     "\r\n");
	}

	/* Territory. One integer: claims 1..highest_claim, nothing else. */
	const int held = realm.highest_claim;
	APPENDF(out, " &+wTerritory  &n: &+G%d&n of %d squares\r\n", held, KINGDOM_MAX_SQUARES);

	APPENDF(out, " &+wRings      &n:");
	for (int ring = 1; ring <= KINGDOM_MAX_RING; ring++)
	{
		const int first = kingdom_ring_first_index(ring);
		const int size = kingdom_ring_size(ring);
		int in_ring = held - first + 1;

		if (in_ring < 0)
			in_ring = 0;
		if (in_ring > size)
			in_ring = size;

		APPENDF(out, "  %s%d: %2d/%2d&n",
			in_ring == size ? "&+G" :
			in_ring > 0	? "&+Y" :
					  "&+w",
			ring, in_ring, size);
	}
	APPENDF(out, "\r\n");

	/* Next claim. The one question that matters, asked of the one authority:
	 * the next index is highest_claim + 1 because the order is fixed, and
	 * whether it may be taken is kingdom_judge_square()'s answer alone. */
	if (held >= KINGDOM_MAX_SQUARES)
	{
		APPENDF(out, " &+wNext claim &n: &+Gnone -- all %d squares are held&n\r\n",
			KINGDOM_MAX_SQUARES);
	}
	else if (!valid_rnum(realm.hall_rnum))
	{
		APPENDF(out, " &+wNext claim &n: &+Rnone until the seat is restored&n\r\n");
	}
	else
	{
		const int next = held + 1;
		const int ring = kingdom_ring_for_index(next);
		int ndx = 0, ndy = 0;
		char where[96] = "";

		kingdom_offset_for_index(next, &ndx, &ndy);
		describe_offset(ndx, ndy, where, sizeof(where));

		const int verdict =
			kingdom_judge_square(realm.hall_rnum, next, racewar, realm.assoc_id);

		if (verdict == KSQ_OK)
			APPENDF(out,
				" &+wNext claim &n: &+Y#%d&n (ring %d), %s"
				" -- &+Y%ld&n coin and &+Y%ld&n of each resource\r\n",
				next, ring, where, kingdom_claim_cost(next),
				kingdom_claim_material_cost(next));
		else
			APPENDF(out,
				" &+wNext claim &n: &+R#%d&n (ring %d), %s"
				" -- &+Rbarred: %s&n\r\n",
				next, ring, where, verdict_text(verdict));

		APPENDF(out, " &+wRing %d     &n: %d squares, &+Y%ld&n coin for the whole ring\r\n",
			ring, kingdom_ring_size(ring), kingdom_ring_cost(ring));
	}

	APPENDF(out, " &+wGuards     &n: &+G%d&n permitted &+w(1 per %d squares held)&n\r\n",
		kingdom_guard_allowance(realm), kingdom_cfg.guards_per_squares);

	APPENDF(out,
		" &+wUpkeep     &n: &+Y%ld&n coin due &+w(%ld per square per cycle, a cycle is"
		" %d minutes)&n\r\n",
		kingdom_upkeep_due(realm), kingdom_cfg.upkeep_per_square,
		kingdom_cfg.upkeep_period_seconds / 60);

	/* Two stamp() calls in one APPENDF would both show the later time --
	 * it returns a single static buffer -- so paid-through gets its own. */
	APPENDF(out, " &+wPaid until &n: %s\r\n", stamp(realm.upkeep_paid_through));
	APPENDF(out, " &+wStanding   &n: %s", arrears_text(realm.arrears));
	if (realm.missed_cycles > 0)
		APPENDF(out, " &+w(%d cycle%s missed)&n", realm.missed_cycles,
			realm.missed_cycles == 1 ? "" : "s");
	APPENDF(out, "\r\n");

	APPENDF(out, " &+wResources  &n:");
	for (int res = 0; res < KRES_MAX; res++)
	{
		/* Same NULL-into-%s caution as verdict_text(): the names come
		 * from another file in the module. */
		const char *res_name = kingdom_resource_name(res);

		APPENDF(out, "  %s &+Y%ld&n", res_name ? res_name : "?", realm.resources[res]);
	}
	APPENDF(out, "\r\n");
	APPENDF(out, " &+w             held by the realm: spendable on its works, never"
		     " withdrawable.&n\r\n");

	send_to_char(out, ch);
}

/* ------------------------------------------------------------------ *
 * The realm's lines inside the society display
 * ------------------------------------------------------------------ */

/* The realm block appended to `soc` (Guild::display(), guild/assocs.c), which
 * before this knew nothing about realms: a guild could hold eighty squares and
 * a full materials store and its society sheet would show only coin.
 *
 * THE FALSE RETURN IS THE GATE. Nothing at the call site tests kingdom_enabled()
 * or asks whether the guild has a realm -- this does, and writes nothing when
 * either answer is no, so the society display is byte-identical on a server
 * running the shipped kingdom.enabled = 0 and for every unconverted guild.
 *
 * The vocabulary is deliberately kingdom_show_status()'s: the same
 * arrears_text() rungs, the same kingdom_resource_name() names, the same
 * "never withdrawable" clause. Two wordings for one state is the defect here.
 *
 * The idiom is Guild::display()'s, not the status table's: labels padded to 21
 * characters so values line up under its Cash line, &+W for values, a full stop
 * at the end, and a bare \n -- that display uses \n throughout, not \r\n.
 */
bool kingdom_guild_society_lines(int assoc_id, char *out, size_t out_len)
{
	if (!kingdom_enabled() || !out || out_len == 0)
		return false;

	const kingdom_realm *realm = kingdom_find_realm(assoc_id);

	if (!realm)
		return false;

	/* Territory is the single integer: claims 1..highest_claim, nothing
	 * else. Ring 0 is what kingdom_ring_for_index() answers for an index
	 * outside 1..KINGDOM_MAX_SQUARES, so a realm that has converted but not
	 * yet claimed gets its own line rather than "out to ring 0". */
	const int held = realm->highest_claim;

	if (held > 0)
		checked_appendf(out, out_len,
				"Realm territory:     &+W%d&n of &+W%d&n squares, out to ring"
				" &+W%d&n of &+W%d&n.\n",
				held, KINGDOM_MAX_SQUARES, kingdom_ring_for_index(held),
				KINGDOM_MAX_RING);
	else
		checked_appendf(out, out_len,
				"Realm territory:     &+W%d&n of &+W%d&n squares -- none claimed"
				" yet.\n",
				held, KINGDOM_MAX_SQUARES);

	/*
	 * WHAT THE LAND IS FOR (ruled 2026-09-05: show the benefits under `soc`).
	 *
	 * Every figure is asked of the module that owns it -- the guard allowance,
	 * the level cap, the upkeep -- rather than restated here, so this can never
	 * quietly disagree with what the game actually does. That is the same rule
	 * `kingdom help` follows.
	 *
	 * The ring marks read `held >= kingdom_ring_last_index(ring)`, which is
	 * precisely the predicate kingdom_guard_level_cap() uses: a part-built ring
	 * has not paid for its tier yet, and saying otherwise would over-promise.
	 */
	checked_appendf(out, out_len, "Realm rings:        ");
	for (int ring = 1; ring <= KINGDOM_MAX_RING; ring++)
	{
		const int last = kingdom_ring_last_index(ring);
		const int size = kingdom_ring_size(ring);
		int in_ring = held - kingdom_ring_first_index(ring) + 1;

		if (in_ring < 0)
			in_ring = 0;
		if (in_ring > size)
			in_ring = size;

		checked_appendf(out, out_len, " %s%d:%d/%d&n",
				held >= last ? "&+G" :
				in_ring > 0  ? "&+Y" :
					       "&+w",
				ring, in_ring, size);
	}
	checked_appendf(out, out_len, ".\n");

	checked_appendf(out, out_len,
			"Realm garrison:      &+W%d&n guard%s permitted &+w(one per %d squares"
			" held)&n, to level &+W%d&n\n"
			"                     &+w(level %d at the base, %d once ring 1 is whole,"
			" then %d more per ring to %d).&n\n",
			kingdom_guard_allowance(*realm),
			kingdom_guard_allowance(*realm) == 1 ? "" : "s",
			kingdom_cfg.guards_per_squares, kingdom_guard_level_cap(*realm),
			KINGDOM_GUARD_BASE_LEVEL, KINGDOM_GUARD_FIRST_TIER_LEVEL,
			KINGDOM_GUARD_TIER_STEP, KINGDOM_GUARD_TOP_LEVEL);

	checked_appendf(out, out_len,
			"Realm champion:      %s &+w(one only, and only while all %d squares are"
			" held).&n\n",
			realm->champion_class	    ? "&+Wraised&n" :
			held >= KINGDOM_MAX_SQUARES ? "&+Ynone yet -- the land allows one&n" :
						      "&+wnot while the realm is short of land&n",
			KINGDOM_MAX_SQUARES);

	checked_appendf(out, out_len,
			"Realm harvest:       &+wevery %d owned squares that suit a resource add"
			" a step to what its nodes give.&n\n",
			KINGDOM_HARVEST_SQUARES_PER_STEP);

	checked_appendf(out, out_len,
			"Realm upkeep:        &+W%ld&n coin per cycle &+w(%ld per square; a cycle"
			" is %d minutes).&n\n",
			kingdom_upkeep_due(*realm), kingdom_cfg.upkeep_per_square,
			kingdom_cfg.upkeep_period_seconds / 60);

	/* Dormancy. Same test and same "lost" wording as the status table's seat
	 * line, with what dormancy actually costs the guild spelled out, because
	 * `soc` is where a member who never types `kingdom` will meet it. */
	if (!valid_rnum(realm->hall_rnum))
		checked_appendf(out, out_len,
				"Realm seat:          &+Rlost -- the guildhall is gone or has"
				" moved&n.\n"
				"                     &+wThe realm is dormant: no upkeep is"
				" charged, its guards have gone, and\n"
				"                     it can neither claim nor abandon until a"
				" main hall stands on its seat.&n\n");

	/* Standing, only when it is worth saying. arrears_text() renders the rung
	 * the upkeep code decided; this does not work out which rung applies. */
	if (realm->arrears != KARR_CURRENT)
		checked_appendf(out, out_len, "Realm standing:      %s.\n",
				arrears_text(realm->arrears));

	/* The materials store. "Realm resources:" is 16 characters and the loop
	 * opens each entry with a space, so the first name lands in column 22
	 * with Guild::display()'s values. */
	checked_appendf(out, out_len, "Realm resources:    ");
	for (int res = 0; res < KRES_MAX; res++)
	{
		/* Same NULL-into-%s caution as kingdom_show_status(): the names
		 * come from another file in the module. */
		const char *res_name = kingdom_resource_name(res);

		checked_appendf(out, out_len, " %s &+W%ld&n", res_name ? res_name : "?",
				realm->resources[res]);
	}
	checked_appendf(out, out_len, ".\n");
	checked_appendf(out, out_len,
			"                     &+wHeld by the realm, not coin: spendable on its"
			" works, never withdrawable.&n\n");

	return true;
}

/* ------------------------------------------------------------------ *
 * The room banner
 * ------------------------------------------------------------------ */

/* One line for the map legend or the room description, or NULL when the square
 * belongs to nobody. Ends in \r\n, the convention every send_to_char() string
 * in this tree follows.
 *
 * The returned pointer is a single static buffer owned by the module: the
 * caller must use it before the next call and must never free or keep it.
 */
const char *kingdom_room_banner(int rnum)
{
	static char banner[ASC_MAX_STR + 128];

	if (!kingdom_enabled())
		return NULL;

	/* Ask the module's O(1) vnum index rather than walking realms: this is
	 * called from movement and from the map renderer. 0 means unowned, and
	 * 0 is also the "no guild" value of GET_ASSOC_ID (guild/assocs.h:19). */
	const int assoc_id = kingdom_owner_of_room(rnum);
	if (assoc_id <= 0)
		return NULL;

	const std::string name = realm_name(assoc_id);

	/* Which ring the square sits in, derived through the geometry helpers so
	 * the banner cannot disagree with the grid. Everything here is optional
	 * decoration: a realm whose anchor has not resolved still gets a line. */
	int ring = 0;
	const kingdom_realm *realm = kingdom_find_realm(assoc_id);

	if (realm && valid_rnum(realm->hall_rnum))
	{
		int hall_zone = -1, hall_x = -1, hall_y = -1;
		int here_zone = -1, here_x = -1, here_y = -1;

		if (kingdom_square_of_room(realm->hall_rnum, &hall_zone, &hall_x, &hall_y) &&
		    kingdom_square_of_room(rnum, &here_zone, &here_x, &here_y) &&
		    hall_zone == here_zone)
			ring = kingdom_ring_for_index(
				kingdom_index_for_offset(here_x - hall_x, here_y - hall_y));
	}

	banner[0] = '\0';
	if (ring > 0)
		APPENDF(banner, "&+GThis land lies within the realm of &n%s&+G, ring %d.&n\r\n",
			name.c_str(), ring);
	else
		APPENDF(banner, "&+GThis land lies within the realm of &n%s&+G.&n\r\n",
			name.c_str());

	return banner;
}
