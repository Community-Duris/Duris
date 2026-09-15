/*
 *  kingdom_craft.c
 *  Duris
 *
 *  THE WORKS: a kingdom's forge, loom and jeweller, and the guild store that
 *  sells what they make.
 *
 *  Each is a guildhall room of its own type (GH_ROOM_TYPE_FORGE ..
 *  GH_ROOM_TYPE_GUILDSTORE, guild/guildhall.h), so owning one IS having the
 *  room. There is no table of stations anywhere and nothing to persist beyond
 *  the room row: a hall that loses the room loses what it made. This file asks
 *  the realm's MAIN hall what it holds each time and never keeps the answer.
 *
 *  Building them is `kingdom build` (kingdom_claim.c), beside the other verbs
 *  that spend the treasury and persist the realm and the guild as one pair.
 */

#include "kingdom/kingdom_internal.h"

#include "core/structs.h"

#include "core/prototypes.h"
#include "core/utils.h"
#include "guild/guildhall.h"

#include <cstring>

/* ------------------------------------------------------------------ *
 * The works
 * ------------------------------------------------------------------ */

/* In the order they are listed: the three workshops, then the store that
 * sells their work. The first letters differ, so any prefix names one. */
static const struct
{
	int type;
	const char *name;
} kingdom_works[] = {
	{ GH_ROOM_TYPE_FORGE, "forge" },
	{ GH_ROOM_TYPE_LOOM, "loom" },
	{ GH_ROOM_TYPE_JEWELLER, "jeweller" },
	{ GH_ROOM_TYPE_GUILDSTORE, "store" },
};

static_assert(sizeof(kingdom_works) / sizeof(kingdom_works[0]) == KINGDOM_WORKS_COUNT,
	      "every kingdom work needs a row in kingdom_works");

/* The player-facing word for a work's room type, or "workshop" for anything
 * else. Never NULL. */
const char *kingdom_works_name(int type)
{
	for (const auto &work : kingdom_works)
	{
		if (work.type == type)
			return work.name;
	}
	return "workshop";
}

/* The room type a player's word names, or 0. "jeweler" is taken as well as
 * "jeweller": it is not a prefix of it, and refusing a speller's honest word
 * would be pedantry. An empty word names nothing -- is_abbrev() would match
 * it against the first row. */
int kingdom_works_type_by_name(const char *word)
{
	if (!word || !*word)
		return 0;

	if (!str_cmp(word, "jeweler"))
		return GH_ROOM_TYPE_JEWELLER;

	for (const auto &work : kingdom_works)
	{
		if (is_abbrev(word, work.name))
			return work.type;
	}
	return 0;
}

/* True for the forge, the loom and the jeweller; false for the store. */
bool kingdom_works_is_workshop(int type)
{
	return type == GH_ROOM_TYPE_FORGE || type == GH_ROOM_TYPE_LOOM ||
	       type == GH_ROOM_TYPE_JEWELLER;
}

/* True when this hall holds a room of `type`. */
bool kingdom_works_hall_has(const Guildhall *hall, int type)
{
	if (!hall)
		return false;

	for (const GuildhallRoom *room : hall->rooms)
	{
		if (room && room->type == type)
			return true;
	}
	return false;
}

/* The works standing in this guild's MAIN hall, as a mask of (1u << type).
 * Only the main hall counts: an outpost anchors no realm and may build none. */
unsigned kingdom_works_built(int assoc_id)
{
	const Guildhall *hall = kingdom_main_hall(assoc_id);
	unsigned built = 0;

	for (const auto &work : kingdom_works)
	{
		if (kingdom_works_hall_has(hall, work.type))
			built |= 1u << work.type;
	}
	return built;
}

/* "forge, loom and store", in listing order, into `out`; an empty string when
 * nothing stands. Appends nothing past out_len. */
void kingdom_works_describe(unsigned built, char *out, size_t out_len)
{
	if (!out || out_len == 0)
		return;
	out[0] = '\0';

	int total = 0;

	for (const auto &work : kingdom_works)
	{
		if (built & (1u << work.type))
			total++;
	}

	int written = 0;

	for (const auto &work : kingdom_works)
	{
		if (!(built & (1u << work.type)))
			continue;
		written++;
		checked_appendf(out, out_len, "%s%s",
				written == 1	 ? "" :
				written == total ? " and " :
						   ", ",
				work.name);
	}
}
