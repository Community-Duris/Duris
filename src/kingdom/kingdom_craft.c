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
 *
 *  THE STORE (ruled 2026-09-15). In the store room, `list` shows what the
 *  hall's workshops make and `buy <item> [form]` buys one. Every piece is MADE
 *  AT PURCHASE for the buyer's own level (capped at 56), so a level 10 cannot
 *  buy level-56 work and a level 56 still gets sound mid-level gear. It costs
 *  platinum, which is DESTROYED -- a money sink, credited to no treasury -- and
 *  realm material, drawn through kingdom_resource_spend(), the store's one way
 *  out. It is ORDINARY PROPERTY (ruled 2026-09-16) -- worn by anyone who can
 *  wear it, given, looted and sold, and worth a tenth of its price in a shop's
 *  ledger -- and it carries no effect flags and no procs: only armour, hit
 *  points, mana, attributes, hitroll and damroll.
 *
 *  The catalogue is ONE compiled table below so the numbers can be tuned in
 *  one place; the arithmetic that scales them is kingdom_craft_math.h.
 */

#include "kingdom/kingdom_internal.h"

#include "core/structs.h"

#include "core/config.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "economy/currency_transaction.h"
#include "guild/assocs.h"
#include "guild/guildhall.h"
#include "item/item_movement_transaction.h"
#include "item/objmisc.h"
#include "kingdom/kingdom_craft_bind.h"
#include "kingdom/kingdom_craft_math.h"
#include "net/comm.h"

#include <climits>
#include <cstring>
#include <string>

/* core/constant.c, declared by each file that asks CAN_CARRY_OBJ() (it reads
 * the strength table through CAN_CARRY_W) -- no header exports it. */
extern struct str_app_type str_app[];

/* The approved worked examples, checked at compile time as well as by
 * tests/async/test_kingdom_craft_math.py: a change to the curve that moves a
 * published price stops the build here. */
static_assert(kingdom_craft_price_platinum(10, 10, 1000) == 12, "L10 price at W1");
static_assert(kingdom_craft_material_units(10, 10, 1000) == 9, "L10 material at W1");
static_assert(kingdom_craft_price_platinum(10, 56, 1000) == 58, "L56 price at W1");
static_assert(kingdom_craft_material_units(10, 56, 1000) == 32, "L56 material at W1");
static_assert(kingdom_craft_item_level(62) == KINGDOM_CRAFT_TOP_LEVEL, "no gear above 56");
static_assert(KINGDOM_CRAFT_TOP_LEVEL == MAXLVLMORTAL, "the store's ceiling is a mortal's");

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

/* ------------------------------------------------------------------ *
 * The catalogue
 * ------------------------------------------------------------------ *
 * PROVISIONAL NUMBERS, ruled 2026-09-15 and meant to be tuned: every value
 * below is what a LEVEL-56 piece carries, and kingdom_craft_scaled_stat()
 * scales each line down to the buyer's level (max(1, round(top x L / 56))).
 * Weapon dice come from kingdom_craft_weapon_dice()'s level bands instead.
 *
 * No line here is an effect flag or a proc, and none can be: a row holds
 * only AC, APPLY_* stat lines, dice and a weapon type. There are no saving
 * throws and no racial-maximum applies.
 *
 * AC AND MATERIAL. apply_ac() (magic/affects.c) gives worn armour the GREATER
 * of a figure derived from its material and slot and the AC the object
 * carries -- value[0] on armour, value[3] on a shield. For ordinary armour
 * that figure is a floor; under store gear it would be the wrong one, handing
 * a level-10 buyer level-56 armour class (steel on the body alone is 24). So
 * apply_ac() drops the figure for store pieces alone -- known by their blank's
 * vnum, or by the maker's mark when that vnum cannot be resolved
 * (kingdom_store_piece.h) -- and the level-scaled AC below is exactly what the
 * buyer gets. Every piece is made of its REAL material -- steel from
 * the forge, cloth or leather from the loom, silver from the jeweller -- so
 * everything else that reads a material treats it as what it is.
 */

/* One stat line: an APPLY_* location and its value on a level-56 piece. */
struct kingdom_craft_line
{
	int location;
	int top;
};

/* A form the buyer chooses between: 'buy ring health'. */
struct kingdom_craft_variant
{
	const char *name;
	kingdom_craft_line line;
};

enum kingdom_craft_kind
{
	KCRAFT_WEAPON, /* ITEM_WEAPON: type in value[0], dice in value[1..2] */
	KCRAFT_ARMOR, /* ITEM_ARMOR: AC in value[0] */
	KCRAFT_SHIELD, /* ITEM_SHIELD: AC in value[3]; the engine's shield block asks for this type */
	KCRAFT_WORN, /* ITEM_WORN: no armour class at all */
};

struct kingdom_craft_item
{
	const char *keyword; /* what the buyer types */
	const char *noun; /* "steel longsword", after "a"/"an" or "a pair of" */
	bool pair; /* "a pair of steel greaves" */
	int station; /* GH_ROOM_TYPE_FORGE, _LOOM or _JEWELLER */
	int kind;
	int wear; /* the one wear bit: ITEM_WIELD or an ITEM_WEAR_* */
	int weight_tenths; /* the design's W, which prices it: 15, 20, 12, 10 or 8 */
	int weapon_type; /* WEAPON_*, weapons only */
	bool two_handed;
	int ac_top; /* armour class at level 56; 0 for none */
	kingdom_craft_line lines[2]; /* lines every copy carries; location 0 ends them */
	const kingdom_craft_variant *variants; /* the forms, or NULL for one way only */
	int variant_count;
	int material; /* MAT_* */
	int pounds; /* what it weighs in a pack */
};

static const kingdom_craft_variant kingdom_craft_belt_forms[] = {
	{ "strength", { APPLY_STR, 3 } },
	{ "endurance", { APPLY_CON, 3 } },
};

static const kingdom_craft_variant kingdom_craft_hood_forms[] = {
	{ "focus", { APPLY_INT, 2 } },
	{ "faith", { APPLY_WIS, 2 } },
};

static const kingdom_craft_variant kingdom_craft_ring_forms[] = {
	{ "health", { APPLY_HIT, 12 } },
	{ "accuracy", { APPLY_HITROLL, 2 } },
	{ "focus", { APPLY_INT, 3 } },
	{ "faith", { APPLY_WIS, 3 } },
};

static const kingdom_craft_variant kingdom_craft_bracelet_forms[] = {
	{ "strength", { APPLY_STR, 3 } },  { "dexterity", { APPLY_DEX, 3 } },
	{ "endurance", { APPLY_CON, 3 } }, { "focus", { APPLY_INT, 3 } },
	{ "faith", { APPLY_WIS, 3 } },
};

static const kingdom_craft_variant kingdom_craft_necklace_forms[] = {
	{ "health", { APPLY_HIT, 10 } },
	{ "mana", { APPLY_MANA, 15 } },
};

#define KCRAFT_FORMS(table) table, (int)(sizeof(table) / sizeof(table[0]))
#define KCRAFT_ONE_WAY NULL, 0
#define KCRAFT_WEAPON_LINES(n)                              \
	{                                                   \
		{ APPLY_HITROLL, n }, { APPLY_DAMROLL, n }, \
	}
#define KCRAFT_LINE(location, top)           \
	{                                    \
		{ location, top }, { 0, 0 }, \
	}
#define KCRAFT_NO_LINES             \
	{                           \
		{ 0, 0 }, { 0, 0 }, \
	}

static const kingdom_craft_item kingdom_craft_catalogue[] = {
	/* The forge: weapons and metal armour. */
	{ "dagger", "steel dagger", false, GH_ROOM_TYPE_FORGE, KCRAFT_WEAPON, ITEM_WIELD, 15,
	  WEAPON_DAGGER, false, 0, KCRAFT_WEAPON_LINES(3), KCRAFT_ONE_WAY, MAT_STEEL, 2 },
	{ "longsword", "steel longsword", false, GH_ROOM_TYPE_FORGE, KCRAFT_WEAPON, ITEM_WIELD, 15,
	  WEAPON_LONGSWORD, false, 0, KCRAFT_WEAPON_LINES(3), KCRAFT_ONE_WAY, MAT_STEEL, 8 },
	{ "mace", "steel mace", false, GH_ROOM_TYPE_FORGE, KCRAFT_WEAPON, ITEM_WIELD, 15,
	  WEAPON_MACE, false, 0, KCRAFT_WEAPON_LINES(3), KCRAFT_ONE_WAY, MAT_STEEL, 10 },
	{ "axe", "steel hand axe", false, GH_ROOM_TYPE_FORGE, KCRAFT_WEAPON, ITEM_WIELD, 15,
	  WEAPON_AXE, false, 0, KCRAFT_WEAPON_LINES(3), KCRAFT_ONE_WAY, MAT_STEEL, 7 },
	{ "warhammer", "steel warhammer", false, GH_ROOM_TYPE_FORGE, KCRAFT_WEAPON, ITEM_WIELD, 15,
	  WEAPON_HAMMER, false, 0, KCRAFT_WEAPON_LINES(3), KCRAFT_ONE_WAY, MAT_STEEL, 12 },
	{ "greatsword", "steel greatsword", false, GH_ROOM_TYPE_FORGE, KCRAFT_WEAPON, ITEM_WIELD,
	  20, WEAPON_2HANDSWORD, true, 0, KCRAFT_WEAPON_LINES(4), KCRAFT_ONE_WAY, MAT_STEEL, 15 },
	{ "quarterstaff", "ash quarterstaff", false, GH_ROOM_TYPE_FORGE, KCRAFT_WEAPON, ITEM_WIELD,
	  20, WEAPON_STAFF, true, 0, KCRAFT_WEAPON_LINES(4), KCRAFT_ONE_WAY, MAT_HARDWOOD, 8 },
	{ "breastplate", "steel breastplate", false, GH_ROOM_TYPE_FORGE, KCRAFT_ARMOR,
	  ITEM_WEAR_BODY, 15, 0, false, 25, KCRAFT_LINE(APPLY_HIT, 15), KCRAFT_ONE_WAY, MAT_STEEL,
	  25 },
	{ "helm", "steel helm", false, GH_ROOM_TYPE_FORGE, KCRAFT_ARMOR, ITEM_WEAR_HEAD, 10, 0,
	  false, 10, KCRAFT_LINE(APPLY_HIT, 8), KCRAFT_ONE_WAY, MAT_STEEL, 6 },
	{ "vambraces", "steel vambraces", true, GH_ROOM_TYPE_FORGE, KCRAFT_ARMOR, ITEM_WEAR_ARMS,
	  10, 0, false, 10, KCRAFT_LINE(APPLY_STR, 2), KCRAFT_ONE_WAY, MAT_STEEL, 5 },
	{ "greaves", "steel greaves", true, GH_ROOM_TYPE_FORGE, KCRAFT_ARMOR, ITEM_WEAR_LEGS, 10, 0,
	  false, 10, KCRAFT_LINE(APPLY_HIT, 8), KCRAFT_ONE_WAY, MAT_STEEL, 8 },
	{ "boots", "steel-shod boots", true, GH_ROOM_TYPE_FORGE, KCRAFT_ARMOR, ITEM_WEAR_FEET, 10,
	  0, false, 8, KCRAFT_LINE(APPLY_AGI, 2), KCRAFT_ONE_WAY, MAT_STEEL, 5 },
	{ "gauntlets", "steel gauntlets", true, GH_ROOM_TYPE_FORGE, KCRAFT_ARMOR, ITEM_WEAR_HANDS,
	  10, 0, false, 8, KCRAFT_LINE(APPLY_HITROLL, 2), KCRAFT_ONE_WAY, MAT_STEEL, 4 },
	{ "shield", "steel shield", false, GH_ROOM_TYPE_FORGE, KCRAFT_SHIELD, ITEM_WEAR_SHIELD, 12,
	  0, false, 15, KCRAFT_LINE(APPLY_HIT, 8), KCRAFT_ONE_WAY, MAT_STEEL, 12 },

	/* The loom: cloth and leather. */
	{ "cloak", "woollen cloak", false, GH_ROOM_TYPE_LOOM, KCRAFT_ARMOR, ITEM_WEAR_ABOUT, 10, 0,
	  false, 8, KCRAFT_LINE(APPLY_HIT, 10), KCRAFT_ONE_WAY, MAT_CLOTH, 3 },
	{ "belt", "leather belt", false, GH_ROOM_TYPE_LOOM, KCRAFT_ARMOR, ITEM_WEAR_WAIST, 10, 0,
	  false, 5, KCRAFT_NO_LINES, KCRAFT_FORMS(kingdom_craft_belt_forms), MAT_LEATHER, 1 },
	{ "robe", "woven robe", false, GH_ROOM_TYPE_LOOM, KCRAFT_ARMOR, ITEM_WEAR_BODY, 15, 0,
	  false, 15, KCRAFT_LINE(APPLY_MANA, 20), KCRAFT_ONE_WAY, MAT_CLOTH, 4 },
	{ "hood", "woven hood", false, GH_ROOM_TYPE_LOOM, KCRAFT_ARMOR, ITEM_WEAR_HEAD, 10, 0,
	  false, 6, KCRAFT_NO_LINES, KCRAFT_FORMS(kingdom_craft_hood_forms), MAT_CLOTH, 1 },
	{ "gloves", "leather gloves", true, GH_ROOM_TYPE_LOOM, KCRAFT_ARMOR, ITEM_WEAR_HANDS, 10, 0,
	  false, 5, KCRAFT_LINE(APPLY_DEX, 2), KCRAFT_ONE_WAY, MAT_LEATHER, 1 },

	/* The jeweller. A ring carries no armour class, so it is worn, not armour. */
	{ "ring", "silver ring", false, GH_ROOM_TYPE_JEWELLER, KCRAFT_WORN, ITEM_WEAR_FINGER, 8, 0,
	  false, 0, KCRAFT_NO_LINES, KCRAFT_FORMS(kingdom_craft_ring_forms), MAT_SILVER, 1 },
	{ "bracelet", "silver bracelet", false, GH_ROOM_TYPE_JEWELLER, KCRAFT_ARMOR,
	  ITEM_WEAR_WRIST, 8, 0, false, 4, KCRAFT_NO_LINES,
	  KCRAFT_FORMS(kingdom_craft_bracelet_forms), MAT_SILVER, 1 },
	{ "necklace", "silver necklace", false, GH_ROOM_TYPE_JEWELLER, KCRAFT_ARMOR, ITEM_WEAR_NECK,
	  8, 0, false, 4, KCRAFT_NO_LINES, KCRAFT_FORMS(kingdom_craft_necklace_forms), MAT_SILVER,
	  1 },
};

#undef KCRAFT_FORMS
#undef KCRAFT_ONE_WAY
#undef KCRAFT_WEAPON_LINES
#undef KCRAFT_LINE
#undef KCRAFT_NO_LINES

/* Two fixed lines and a chosen form must fit the object's affect slots. */
static_assert(MAX_OBJ_AFFECT >= 3, "a store piece needs up to three stat lines");

/* Which resources a workshop draws, primary first (the 70% share). */
static void kingdom_craft_station_resources(int station, int *primary, int *secondary)
{
	switch (station)
	{
	case GH_ROOM_TYPE_LOOM:
		*primary = KRES_FIBRE;
		*secondary = KRES_WATER;
		break;
	case GH_ROOM_TYPE_JEWELLER:
		*primary = KRES_MINERAL;
		*secondary = KRES_WATER;
		break;
	default: /* the forge */
		*primary = KRES_MINERAL;
		*secondary = KRES_WOOD;
		break;
	}
}

/* How a piece's long description names where it was made. */
static const char *kingdom_craft_makers(int station)
{
	switch (station)
	{
	case GH_ROOM_TYPE_LOOM:
		return "the looms";
	case GH_ROOM_TYPE_JEWELLER:
		return "the jewellers";
	default:
		return "the forges";
	}
}

/* ------------------------------------------------------------------ *
 * The bill
 * ------------------------------------------------------------------ */

struct kingdom_craft_bill
{
	long platinum; /* destroyed on purchase */
	long costs[KRES_MAX]; /* drawn from the realm's stores */
};

/* What one piece costs at `level`, from the curve in kingdom_craft_math.h. */
static kingdom_craft_bill kingdom_craft_bill_for(const kingdom_craft_item &item, int level)
{
	kingdom_craft_bill bill = {};
	int primary = KRES_MINERAL;
	int secondary = KRES_WOOD;

	bill.platinum = kingdom_craft_price_platinum(item.weight_tenths, level,
						     kingdom_cfg.craft_price_permille);

	const long units = kingdom_craft_material_units(item.weight_tenths, level,
							kingdom_cfg.craft_resource_permille);

	kingdom_craft_station_resources(item.station, &primary, &secondary);
	bill.costs[primary] += kingdom_craft_primary_share(units);
	bill.costs[secondary] += kingdom_craft_secondary_share(units);
	return bill;
}

/* True when the bill asks for any material at all. */
static bool kingdom_craft_bill_wants_material(const kingdom_craft_bill &bill)
{
	for (int res = 0; res < KRES_MAX; res++)
	{
		if (bill.costs[res] > 0)
			return true;
	}
	return false;
}

/* True when the realm's stores cover every line of the bill. */
static bool kingdom_craft_stores_cover(const kingdom_realm &realm, const kingdom_craft_bill &bill)
{
	for (int res = 0; res < KRES_MAX; res++)
	{
		if (realm.resources[res] < bill.costs[res])
			return false;
	}
	return true;
}

/* "34 mineral, 14 wood" -- the non-zero lines of a bill -- into `out`. */
static void kingdom_craft_bill_text(const kingdom_craft_bill &bill, char *out, size_t out_len)
{
	out[0] = '\0';
	for (int res = 0; res < KRES_MAX; res++)
	{
		if (bill.costs[res] <= 0)
			continue;
		checked_appendf(out, out_len, "%s%ld %s", out[0] ? ", " : "", bill.costs[res],
				kingdom_resource_name(res));
	}
	if (!out[0])
		checked_appendf(out, out_len, "no material");
}

/* ------------------------------------------------------------------ *
 * Finding what the buyer asked for
 * ------------------------------------------------------------------ */

/* The row a buyer's word names among what `built` makes: an exact keyword
 * first, then a prefix that fits exactly one row. *ambiguous is set when a
 * prefix fits several. An empty word names nothing: is_abbrev("", x) is true
 * for every x, so it must never reach the prefix pass. */
static const kingdom_craft_item *kingdom_craft_find(const char *word, unsigned built,
						    bool *ambiguous)
{
	*ambiguous = false;
	if (!word || !*word)
		return NULL;

	const kingdom_craft_item *found = NULL;
	int matches = 0;

	for (const auto &item : kingdom_craft_catalogue)
	{
		if (!(built & (1u << item.station)))
			continue;
		if (!str_cmp(word, item.keyword))
			return &item;
		if (is_abbrev(word, item.keyword))
		{
			found = &item;
			matches++;
		}
	}
	if (matches > 1)
	{
		*ambiguous = true;
		return NULL;
	}
	return found;
}

/* The form of `item` a buyer's word names, by exact name or a prefix that
 * fits exactly one. NULL for none, for several, and for an empty word. */
static const kingdom_craft_variant *kingdom_craft_find_form(const kingdom_craft_item &item,
							    const char *word)
{
	if (!word || !*word)
		return NULL;

	const kingdom_craft_variant *found = NULL;
	int matches = 0;

	for (int i = 0; i < item.variant_count; i++)
	{
		if (!str_cmp(word, item.variants[i].name))
			return &item.variants[i];
		if (is_abbrev(word, item.variants[i].name))
		{
			found = &item.variants[i];
			matches++;
		}
	}
	return matches == 1 ? found : NULL;
}

/* A piece's forms into `out`, `joiner` between them and `last_joiner` before
 * the last: (", ", " or ") gives "health, accuracy, focus or faith" and
 * ("/", "/") gives "health/accuracy/focus/faith". */
static void kingdom_craft_forms_text(const kingdom_craft_item &item, const char *joiner,
				     const char *last_joiner, char *out, size_t out_len)
{
	out[0] = '\0';
	for (int i = 0; i < item.variant_count; i++)
	{
		checked_appendf(out, out_len, "%s%s",
				i == 0			    ? "" :
				i == item.variant_count - 1 ? last_joiner :
							      joiner,
				item.variants[i].name);
	}
}

/* ------------------------------------------------------------------ *
 * Making the piece
 * ------------------------------------------------------------------ */

/* Build one piece of `item` at `level` for `buyer`, in NOWHERE, stamped and
 * ready to grant; NULL when the blank cannot be loaded.
 *
 * WHAT IS WRITTEN: the type, the one wear bit, material, weight, AC or dice,
 * the stat lines, the strings and the store flags. WHAT IS NOT: any affect
 * mask and any proc value -- the blank carries none and nothing here adds one
 * (ruled 2026-09-15: store gear has no effect flags) -- and any class or race
 * restriction, which the design leaves off.
 *
 * ORDINARY PROPERTY (ruled 2026-09-16). A piece can be worn by anyone who can
 * wear it, given, dropped, looted from a corpse and sold: it is not soulbound
 * and carries no NOSELL. It is worth a tenth of what it cost in a shop's
 * ledger (kingdom_craft_resale_copper(), kingdom.craft.resale.permille), and a
 * shopkeeper pays its own fraction of that again, so a looter nets a modest
 * sum rather than nothing.
 *
 * STILL STAMPED: CRAFTED (it cannot be given to a mob) and STOREITEM (it
 * cannot be salvaged back into materials -- gear must not become a way to mint
 * the realm's resources). The maker's mark names the purchase it came from
 * (see THE MARK below); it binds nothing. NULL, with nothing left behind, if
 * the blank will not load or the buyer has no player id to stamp it with. */
static P_obj kingdom_craft_make(P_char buyer, const kingdom_craft_item &item,
				const kingdom_craft_variant *form, int level,
				const char *realm_name)
{
	P_obj obj = read_object(VOBJ_KINGDOM_CRAFT_BLANK, VIRTUAL);

	if (!obj)
		return NULL;

	switch (item.kind)
	{
	case KCRAFT_WEAPON:
		obj->type = ITEM_WEAPON;
		break;
	case KCRAFT_SHIELD:
		obj->type = ITEM_SHIELD;
		break;
	case KCRAFT_WORN:
		obj->type = ITEM_WORN;
		break;
	default:
		obj->type = ITEM_ARMOR;
		break;
	}

	obj->wear_flags = ITEM_TAKE | item.wear;
	obj->material = item.material;
	obj->weight = item.pounds;
	/* What a shop's ledger says it is worth: a tenth of its purchase price at
	 * the shipped scale. The keeper pays its own fraction of this again. */
	obj->cost = static_cast<int>(kingdom_craft_resale_copper(
		item.weight_tenths, level, kingdom_cfg.craft_price_permille,
		kingdom_cfg.craft_resale_permille));
	obj->anti_flags = 0;
	obj->anti2_flags = 0;

	for (size_t i = 0; i < sizeof(obj->value) / sizeof(obj->value[0]); i++)
		obj->value[i] = 0;
	for (int i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		obj->affected[i].location = APPLY_NONE;
		obj->affected[i].modifier = 0;
	}

	/* No effect flags, whatever the blank carries (ruled 2026-09-15). Zeroed
	 * outright rather than trusted to the prototype, so an edit to #48018 can
	 * never carry an affect onto store gear. */
	obj->bitvector = 0;
	obj->bitvector2 = 0;
	obj->bitvector3 = 0;
	obj->bitvector4 = 0;
	obj->bitvector5 = 0;

	const int ac = kingdom_craft_scaled_stat(item.ac_top, level);

	switch (item.kind)
	{
	case KCRAFT_WEAPON:
	{
		const kingdom_craft_dice dice = kingdom_craft_weapon_dice(level, item.two_handed);

		obj->value[0] = item.weapon_type;
		obj->value[1] = dice.count;
		obj->value[2] = dice.size;
		if (item.two_handed)
			SET_BIT(obj->extra_flags, ITEM_TWOHANDS);
		break;
	}
	case KCRAFT_SHIELD:
		/* Shield layout as randomeq.c writes it: kind, shape and size in
		 * value[0..2], armour class in value[3]. */
		obj->value[0] = 1;
		obj->value[1] = 1;
		obj->value[2] = 1;
		obj->value[3] = ac;
		break;
	case KCRAFT_ARMOR:
		obj->value[0] = ac;
		break;
	default:
		break;
	}

	int slot = 0;

	for (const kingdom_craft_line &line : item.lines)
	{
		if (line.location && slot < MAX_OBJ_AFFECT)
		{
			obj->affected[slot].location = line.location;
			obj->affected[slot].modifier = kingdom_craft_scaled_stat(line.top, level);
			slot++;
		}
	}
	if (form && slot < MAX_OBJ_AFFECT)
	{
		obj->affected[slot].location = form->line.location;
		obj->affected[slot].modifier = kingdom_craft_scaled_stat(form->line.top, level);
	}

	SET_BIT(obj->extra2_flags, ITEM2_CRAFTED);
	SET_BIT(obj->extra2_flags, ITEM2_STOREITEM);

	const char *noun_start = item.noun;
	const bool vowel = strchr("aeiouAEIOU", noun_start[0]) != NULL;
	std::string shown = item.pair ? "a pair of " : (vowel ? "an " : "a ");

	shown += item.noun;
	if (form)
	{
		shown += " of ";
		shown += form->name;
	}

	std::string keywords = item.noun;

	keywords += " ";
	keywords += item.keyword;
	if (form)
	{
		keywords += " ";
		keywords += form->name;
	}
	/* THE MAKER'S MARK names the purchase a piece came from, as the buyer's
	 * PLAYER ID in a token no character name can equal (kingdom_craft_bind.h).
	 * It BINDS NOTHING -- store gear is ordinary property -- and it is how the
	 * engine still knows a piece is store gear when its object index is
	 * unresolved, which decides one thing: that apply_ac() must not put a
	 * material armour-class floor under armour scaled to the level it was made
	 * at (kingdom_store_piece.h).
	 *
	 * The mark is written as the piece's ACTION DESCRIPTION, never as one of
	 * its keywords. Keywords are what commands target, so a token among them
	 * would let anyone type `get kingdom-bound-1042 bag` and read a player id
	 * off other people's gear. Nothing shows an action description for armour,
	 * a weapon or jewellery -- only notes and corpses use it -- and it is
	 * saved with the object on every path a piece can travel.
	 *
	 * The buyer's NAME is NOT a keyword. It was one while a piece stayed with
	 * the character who bought it, so the piece read as theirs. Now that gear
	 * circulates -- given, looted, sold -- a keyword like that would answer to
	 * `get tyrus`, `sell tyrus` or `junk tyrus` wherever the piece lay, ahead
	 * of any character or mob of that name. The realm that made it is named in
	 * its long description instead, where nothing targets it. */
	char bind_token[KINGDOM_CRAFT_BIND_TOKEN_LEN];

	if (!kingdom_craft_bind_token(GET_PID(buyer), bind_token, sizeof(bind_token)))
	{
		extract_obj(obj, FALSE);
		return NULL;
	}
	keywords += " kingdom";

	std::string long_description = shown;

	long_description[0] = static_cast<char>(toupper(static_cast<unsigned char>(shown[0])));
	long_description += " from ";
	long_description += kingdom_craft_makers(item.station);
	long_description += " of ";
	long_description += realm_name;
	long_description += "&n lies here.";

	/* The engine's restring helpers: each frees a string this object already
	 * owns, dups the new one and marks str_mask, so free_obj() frees exactly
	 * what is ours and never the prototype's. */
	set_keywords(obj, keywords.c_str());
	set_short_description(obj, shown.c_str());
	set_long_description(obj, long_description.c_str());
	/* There is no set_action_description(); this is what the engine's own
	 * writers do (cmd/mail.c, classes/necromancy.c). Marking str_mask makes
	 * the string ours, so free_obj() frees it and never the prototype's, and
	 * every save path carries it (STRUNG_DESC3). */
	obj->action_description = str_dup(bind_token);
	SET_BIT(obj->str_mask, STRUNG_DESC3);

	return obj;
}

/* ------------------------------------------------------------------ *
 * list
 * ------------------------------------------------------------------ */

#define KINGDOM_STORE_BUF 8192

/* Every piece the hall's workshops make, priced for this buyer. */
static void kingdom_store_list(P_char ch, const kingdom_realm &realm, P_Guild guild, unsigned built,
			       int level)
{
	char out[KINGDOM_STORE_BUF] = "";
	const std::string name = guild->get_name();

	APPENDF(out, "&+YThe Guild Store of&n %s&n\r\n", name.c_str());
	APPENDF(out, "Every piece is made for you, at your level: &+W%d&n.\r\n", level);
	APPENDF(out, "The realm's stores:");
	for (int res = 0; res < KRES_MAX; res++)
		APPENDF(out, " %s &+Y%ld&n", kingdom_resource_name(res), realm.resources[res]);
	APPENDF(out, ".\r\n");

	int current_station = -1;

	for (const auto &item : kingdom_craft_catalogue)
	{
		if (!(built & (1u << item.station)))
			continue;

		if (item.station != current_station)
		{
			current_station = item.station;
			APPENDF(out, "\r\n&+WFrom the %s:&n\r\n", kingdom_works_name(item.station));
		}

		const kingdom_craft_bill bill = kingdom_craft_bill_for(item, level);
		char material[96];
		char forms[96];

		kingdom_craft_bill_text(bill, material, sizeof(material));
		kingdom_craft_forms_text(item, "/", "/", forms, sizeof(forms));

		APPENDF(out, "  &+W%-13s&n %5ld platinum  %-24s %s%s\r\n", item.keyword,
			bill.platinum, material, forms,
			kingdom_craft_stores_cover(realm, bill) ? "" : " &+R(out of materials)&n");
	}

	APPENDF(out,
		"\r\n'&+Wbuy <item> [form]&n' buys one, as in '&+Wbuy ring health&n'. The "
		"platinum is\r\n"
		"destroyed; the material comes from the realm's stores. Store gear is yours to\r\n"
		"keep, give or sell -- a shop pays a little for it -- and it carries no magical\r\n"
		"effects.\r\n");

	send_to_char(out, ch);
}

/* ------------------------------------------------------------------ *
 * buy
 * ------------------------------------------------------------------ */

/* `buy <item> [form]`. THE ORDER IS THE SAFETY -- check first, so nothing
 * needs refunding:
 *
 *   1. what, and which form                      -- refuse, nothing made
 *   2. the piece is made, and delivery is asked:  -- refuse, piece discarded
 *      a grant already pending, or a load the
 *      buyer cannot carry
 *   3. MATERIAL IS CHECKED BEFORE COIN (ruled), the shortfall named
 *   4. the coin is checked
 *   5. the coin is taken -- DESTROYED, credited to no treasury
 *   6. the material is spent through kingdom_resource_spend(), the store's
 *      one way out, which cannot refuse now: it was checked in 3 and nothing
 *      has run since. If it ever did, the purchase stops and the coin goes
 *      back, rather than a piece going out unpaid for
 *   7. the piece is granted; a refusal here (transient, and impossible to
 *      check first) undoes the purchase: coin and material both go back
 *   8. the realm's record is written, not left to the next flush
 *
 * The coin goes before the material, as in a claim, because SUB_MONEY() can
 * refuse (a currency transaction still in flight for the buyer) and a realm
 * must not lose material to a purchase that then fails to be paid for. */
static void kingdom_store_buy(P_char ch, kingdom_realm &realm, P_Guild guild, unsigned built,
			      int level, char *argument)
{
	char want[MAX_INPUT_LENGTH];
	char form_word[MAX_INPUT_LENGTH];

	argument = one_argument(argument, want);
	one_argument(argument, form_word);

	if (!*want)
	{
		send_to_char("Buy what? '&+Wlist&n' shows what the workshops make.\r\n", ch);
		return;
	}

	bool ambiguous = false;
	const kingdom_craft_item *item = kingdom_craft_find(want, built, &ambiguous);

	if (!item)
	{
		if (ambiguous)
			send_to_char_f(
				ch,
				"'&+W%s&n' could be more than one piece. Type more of its name; "
				"'&+Wlist&n' shows them.\r\n",
				want);
		else
			send_to_char_f(
				ch,
				"The workshops here make no '&+W%s&n'. '&+Wlist&n' shows what "
				"they do make.\r\n",
				want);
		return;
	}

	const kingdom_craft_variant *form = NULL;

	if (item->variant_count > 0)
	{
		form = kingdom_craft_find_form(*item, form_word);
		if (!form)
		{
			char forms[96];

			kingdom_craft_forms_text(*item, ", ", " or ", forms, sizeof(forms));
			send_to_char_f(ch, "A %s is made %s. As in '&+Wbuy %s %s&n'.\r\n",
				       item->keyword, forms, item->keyword, item->variants[0].name);
			return;
		}
	}
	else if (*form_word)
	{
		send_to_char_f(ch, "A %s is made one way only; just '&+Wbuy %s&n'.\r\n",
			       item->keyword, item->keyword);
		return;
	}

	const kingdom_craft_bill bill = kingdom_craft_bill_for(*item, level);
	const long price = bill.platinum * 1000;

	if (price > INT_MAX)
	{
		send_to_char("That price is past what a purse can settle.\r\n", ch);
		return;
	}

	/* 2. Delivery. A grant already in flight for this buyer would make the
	 * new one wait on it or be refused, and a currency transaction in flight
	 * would make SUB_MONEY() refuse, so ask about both before anything is
	 * made. */
	if (item_movement_transaction_player_busy(ch) || currency_transaction_player_busy(ch))
	{
		send_to_char("You are still settling or receiving something; try again in a "
			     "moment.\r\n",
			     ch);
		return;
	}

	const std::string realm_name = guild->get_name();
	P_obj obj = kingdom_craft_make(ch, *item, form, level, realm_name.c_str());

	if (!obj)
	{
		send_to_char("The workshops cannot make that just now. Please petition.\r\n", ch);
		logit(LOG_KINGDOM,
		      "STORE: no %s could be made for %s (blank %d would not load, or no player "
		      "id to bind it to).",
		      item->keyword, GET_NAME(ch), VOBJ_KINGDOM_CRAFT_BLANK);
		return;
	}

	if (!CAN_CARRY_OBJ(ch, obj))
	{
		extract_obj(obj, FALSE);
		send_to_char("You could not carry it. Lighten your load first.\r\n", ch);
		return;
	}

	/* 3. Material, before coin, naming what is short. */
	if (!kingdom_craft_stores_cover(realm, bill))
	{
		char lack[192] = "";

		for (int res = 0; res < KRES_MAX; res++)
		{
			if (realm.resources[res] >= bill.costs[res])
				continue;
			checked_appendf(lack, sizeof(lack), "%s%ld more %s", lack[0] ? ", " : "",
					bill.costs[res] - realm.resources[res],
					kingdom_resource_name(res));
		}
		extract_obj(obj, FALSE);
		send_to_char_f(ch,
			       "The realm's stores cannot supply it: they need %s. More must be "
			       "harvested first.\r\n",
			       lack);
		return;
	}

	/* 4. Coin. */
	if (price > 0 && GET_MONEY(ch) < price)
	{
		extract_obj(obj, FALSE);
		send_to_char_f(ch, "That costs %ld platinum, and you do not carry so much.\r\n",
			       bill.platinum);
		return;
	}

	/* 5. The coin is taken from the buyer and credited to nobody: the
	 * platinum a member pays for store gear is DESTROYED (ruled 2026-09-15). */
	if (price > 0 && SUB_MONEY(ch, static_cast<int>(price), 0) != 0)
	{
		extract_obj(obj, FALSE);
		send_to_char("Your purse could not be settled just now; nothing has been taken. "
			     "Try again in a moment.\r\n",
			     ch);
		return;
	}

	/* 6. The material, through the store's only way out. */
	const bool wants_material = kingdom_craft_bill_wants_material(bill);

	if (wants_material && !kingdom_resource_spend(realm, bill.costs))
	{
		/* Unreachable in today's single-threaded loop -- the bill was checked
		 * in 3 and nothing has run since -- but a store must never hand over a
		 * piece the realm did not pay for, so the purchase stops here all the
		 * same: the piece is discarded and the buyer's platinum, taken in 5,
		 * goes back. No material moved, so there is none to return. */
		extract_obj(obj, FALSE);
		if (price > 0)
			ADD_MONEY(ch, static_cast<int>(price));
		send_to_char(
			"The realm's stores could not supply it after all, so nothing has been "
			"taken. Try again in a moment.\r\n",
			ch);
		logit(LOG_KINGDOM,
		      "STORE ABORTED: realm %d (assoc %d) refused a bill it had just been seen to "
		      "cover (%s, level %d); %s's %ld platinum went back and nothing was made.",
		      realm.realm_id, realm.assoc_id, item->keyword, level, GET_NAME(ch),
		      bill.platinum);
		return;
	}

	/* 7. The grant. Copy what the messages need first: once the grant is
	 * accepted the object belongs to the ownership coordinator and must not
	 * be touched again. */
	const std::string shown = obj->short_description;

	if (!item_creation_grant_submit_to_player(ch, obj, ch))
	{
		/* THE ONE REVERSAL. The coordinator refuses a grant only for reasons
		 * the buyer can neither cause nor check first -- a saturated queue, a
		 * failed allocation, an unavailable or overloaded coordinator
		 * (item_movement_reject_is_transient(), item/item_movement_transaction.c)
		 * -- and by now the coin is taken and the material drawn. The module's
		 * "nothing refunds" means check first so nothing needs refunding; this
		 * cannot be checked first, so the purchase is undone instead.
		 *
		 * The piece is still ours to discard, since the grant refused it. The
		 * buyer's own platinum goes back to the buyer. And the material goes
		 * back to the store it came from: that is the reversal of this
		 * purchase's own draw, not a withdrawal -- nothing enters the store that
		 * did not leave it a moment ago -- so the non-withdrawable ruling
		 * stands. */
		extract_obj(obj, FALSE);
		if (price > 0)
			ADD_MONEY(ch, static_cast<int>(price));
		if (wants_material)
		{
			for (int res = 0; res < KRES_MAX; res++)
			{
				if (bill.costs[res] > 0)
					kingdom_resource_deposit(realm, res, bill.costs[res]);
			}
			/* Written now rather than left to the flush, like the draw it
			 * undoes (below): the realm's record should hold its material
			 * back as promptly as the buyer's purse holds its coin. A write
			 * that does not land leaves the realm dirty for the flush, and
			 * says so in the log, as the sale's own write does. */
			if (!kingdom_persist_realm(realm))
				logit(LOG_KINGDOM,
				      "STORE RECORD PENDING: realm %d (assoc %d) holds %s's returned "
				      "material in memory only; the flush retries it.",
				      realm.realm_id, realm.assoc_id, GET_NAME(ch));
		}
		send_to_char(
			"The workshops could not hand the piece over just now, so nothing has "
			"been taken: your platinum and the realm's material are back where they "
			"were. Try again in a moment.\r\n",
			ch);
		logit(LOG_KINGDOM,
		      "STORE REVERSED: %s (assoc %d) was refused the grant of %s at level %d; %ld "
		      "platinum and %ld mineral, %ld wood, %ld fibre, %ld water put back.",
		      GET_NAME(ch), realm.assoc_id, shown.c_str(), level, bill.platinum,
		      bill.costs[KRES_MINERAL], bill.costs[KRES_WOOD], bill.costs[KRES_FIBRE],
		      bill.costs[KRES_WATER]);
		return;
	}

	/* 8. The draw is written at once instead of waiting for the next flush.
	 * The buyer's platinum is durable through its own currency transaction;
	 * without this a crash before the flush would bring the realm back with
	 * material it had already spent on a piece the buyer keeps. The write is
	 * the realm's alone -- no treasury moved -- and kingdom_persist_realm()
	 * keeps the pending rule: a realm with a paired payment still pending is
	 * left dirty for that retry, not published alone. kingdom_store_command()
	 * refuses a sale while one is pending, so this write is never held; a
	 * write that fails leaves the realm dirty for the flush, and is logged. */
	if (wants_material && !kingdom_persist_realm(realm))
		logit(LOG_KINGDOM,
		      "STORE RECORD PENDING: realm %d (assoc %d) holds %s's draw in memory only; "
		      "the flush retries it.",
		      realm.realm_id, realm.assoc_id, GET_NAME(ch));

	send_to_char_f(ch,
		       "You pay &+W%ld&n platinum, and your realm's workshops make you %s&n.\r\n",
		       bill.platinum, shown.c_str());
	act("$n settles a purchase at the counter.", FALSE, ch, 0, 0, TO_ROOM);

	logit(LOG_KINGDOM,
	      "STORE: %s (assoc %d) bought %s at level %d for %ld platinum (destroyed) and "
	      "%ld mineral, %ld wood, %ld fibre, %ld water.",
	      GET_NAME(ch), realm.assoc_id, shown.c_str(), level, bill.platinum,
	      bill.costs[KRES_MINERAL], bill.costs[KRES_WOOD], bill.costs[KRES_FIBRE],
	      bill.costs[KRES_WATER]);
}

/* ------------------------------------------------------------------ *
 * The seam (kingdom.h)
 * ------------------------------------------------------------------ */

/* `list` or `buy` in a guild-store room. The gates, in order: the room is a
 * guild store; kingdoms are on; the buyer is a member in good standing of
 * THE HALL'S OWN guild (not an ally, not an applicant, not on parole); that
 * guild is a kingdom; this is its main hall and the realm is not dormant;
 * the realm is not in arrears; no sale while its paired treasury write is
 * pending; and at least one workshop stands. */
bool kingdom_store_command(struct char_data *ch, int room_vnum, bool buying, char *argument)
{
	if (!ch || IS_NPC(ch))
		return false;

	Guildhall *hall = Guildhall::find_by_vnum(room_vnum);
	GuildhallRoom *room = Guildhall::find_room_by_vnum(room_vnum);

	if (!hall || !room || room->type != GH_ROOM_TYPE_GUILDSTORE)
		return false;

	if (!kingdom_enabled())
	{
		send_to_char("The store is shut: kingdoms are not enabled on this world.\r\n", ch);
		return true;
	}

	P_Guild guild = GET_ASSOC(ch);

	/* GET_ASSOC() alone is not membership (an applicant and a banned
	 * character keep the pointer), so this is the module's own test, the one
	 * kingdom_char_owns_room() and do_kingdom apply. */
	if (!guild || guild->get_id() != static_cast<unsigned int>(hall->assoc_id) ||
	    !IS_MEMBER(GET_A_BITS(ch)) || !GT_PAROLE(GET_A_BITS(ch)))
	{
		send_to_char("The storekeeper serves this guild's own members in good standing, "
			     "and no one else.\r\n",
			     ch);
		return true;
	}

	kingdom_realm *realm = kingdom_find_realm(hall->assoc_id);

	if (!realm)
	{
		send_to_char("This guild is no kingdom, so its store has nothing to sell.\r\n", ch);
		return true;
	}

	if (hall != kingdom_main_hall(hall->assoc_id) || !kingdom_resolve_anchor(*realm))
	{
		send_to_char("The store is shut while the realm is dormant: no main hall of the "
			     "guild stands on its seat.\r\n",
			     ch);
		return true;
	}

	if (realm->arrears != KARR_CURRENT)
	{
		send_to_char("The store is shut while the realm is in arrears. Pay the upkeep "
			     "first.\r\n",
			     ch);
		return true;
	}

	/* A realm whose paired treasury write is still pending cannot publish its
	 * record alone (kingdom_persist_realm() holds it for the retry), so a sale
	 * now would leave its material draw in memory only. Selling waits until
	 * the pair lands; looking at the shelves does not. */
	if (buying && realm->payment_pending)
	{
		send_to_char("The realm's ledgers are still being settled. Try the counter again "
			     "in a moment.\r\n",
			     ch);
		return true;
	}

	const unsigned built = kingdom_works_built(hall->assoc_id) &
			       ((1u << GH_ROOM_TYPE_FORGE) | (1u << GH_ROOM_TYPE_LOOM) |
				(1u << GH_ROOM_TYPE_JEWELLER));

	if (!built)
	{
		send_to_char("The shelves are bare: the hall has no workshop to make anything.\r\n",
			     ch);
		return true;
	}

	/* THE LEVEL RULE. The buyer's own level, capped at 56, and nothing a
	 * buyer can type reaches it. */
	const int level = kingdom_craft_item_level(GET_LEVEL(ch));

	if (buying)
		kingdom_store_buy(ch, *realm, guild, built, level, argument);
	else
		kingdom_store_list(ch, *realm, guild, built, level);

	return true;
}
