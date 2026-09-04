/*
 *  kingdom_config.c
 *  Duris
 *
 *  lib/kingdom.cfg -> kingdom_cfg, plus the resource name table.
 *
 *  This is deliberately the same shape as the other tuning loaders already in
 *  the tree -- world/hardcore_config.c:158-186 and
 *  combat/frag_cap_config.c:113-140 -- rather than a new parser: one
 *  `key=value` line per setting, '#' comments, unknown keys logged and
 *  skipped, and every value validated against a range before it is allowed to
 *  displace the compiled default.
 *
 *  THE FILE IS OPTIONAL AND ITS ABSENCE IS NORMAL, NOT AN ERROR. The default
 *  member initialisers on struct kingdom_config (kingdom/kingdom_internal.h:116-131)
 *  start `enabled` at false, so a server with no lib/kingdom.cfg boots with
 *  the whole subsystem switched off: kingdom_enabled() is false, the commands
 *  refuse, and kingdom_footprint_check() permits everything, which is exactly
 *  the pre-kingdom behaviour (kingdom/kingdom.h:23-29). Turning kingdoms on is
 *  opt-in and takes an explicit `kingdom.enabled=true`.
 *
 *  The keys, all of which are optional:
 *
 *      kingdom.enabled                     true|false (default false)
 *      kingdom.claim.cost.base             coin       (default 1000000)
 *      kingdom.claim.cost.growth.permille  per mille  (default 1050)
 *      kingdom.claim.material.base         units      (default 25)
 *      kingdom.upkeep.per.square           coin       (default 250)
 *      kingdom.upkeep.period.seconds       seconds    (default 604800)
 *      kingdom.guards.per.squares          squares    (default 5)
 *      kingdom.guard.cost.base             coin       (default 5000000)
 *      kingdom.guard.cost.max              coin       (default 20000000)
 *      kingdom.min.hometown.distance       squares    (default 5)
 *      kingdom.min.entrance.distance       squares    (default 5)
 *
 *  Values are range-checked but NOT judged against the live world: whether a
 *  cost or a distance is sensible for this map is an operator decision. The
 *  ranges below exist to stop a typo doing arithmetic damage, not to balance
 *  the feature.
 */

#include "kingdom/kingdom_internal.h"

#include "core/structs.h"

#include "core/prototypes.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KINGDOM_CONFIG_FILE "lib/kingdom.cfg"

/* ------------------------------------------------------------------ *
 * Value ranges
 * ------------------------------------------------------------------ *
 * Coin narrows to int on the way into the guild treasury -- Guild::sub_money
 * takes four ints (guild/assocs.h:282) and the treasury fields themselves are
 * unsigned int (guild/assocs.h:321) -- so these ceilings are picked to keep
 * the largest aggregate the kingdom code can compute inside 32 bits rather
 * than to express taste. The worst case is buying a whole ring 4: 32 claims at
 * indices 49..80, i.e. base * 32 + per_square * 2064. At the ceilings that is
 * about 1.35e9, comfortably under 2^31; at the compiled defaults it is 1.1e7.
 */
#define KINGDOM_CLAIM_BASE_MAX 10000000L

/* Growth per mille: 1000 is flat and 2000 doubles every square. The floor is
 * structural -- below 1000 land would get cheaper the more of it a realm held,
 * inverting the curve -- and the ceiling keeps a typo from pricing ring 4 in
 * numbers no treasury can hold. kingdom_compound() clamps to the ceiling
 * anyway, so this bound is about telling the operator, not about safety. */
#define KINGDOM_CLAIM_GROWTH_MIN 1000
#define KINGDOM_CLAIM_GROWTH_MAX 2000

/* Units of each resource for the first square. The ceiling is far above what
 * the harvest economy can supply, so it bounds the arithmetic rather than the
 * design. */
#define KINGDOM_CLAIM_MATERIAL_MAX 100000L

/* What a guard may cost in copper, either to raise or fully upgraded. The
 * ceiling matches the claim base's, so no guard can cost more than land. */
#define KINGDOM_GUARD_COST_MAX 100000000L

/* A full 80-square realm at the ceiling owes 8e8 per cycle, still inside 32
 * bits, so kingdom_upkeep_due() cannot overflow a caller that narrows it. */
#define KINGDOM_UPKEEP_MAX 10000000L

/* One minute to one week. The floor is not taste either: the upkeep event
 * reschedules itself by this period, so a value near zero would spin the event
 * queue charging arrears continuously. */
#define KINGDOM_UPKEEP_PERIOD_MIN 60
#define KINGDOM_UPKEEP_PERIOD_MAX 604800

/* kingdom_guard_allowance() divides by this, so 1 is a hard floor rather than
 * a preference. Anything above KINGDOM_MAX_SQUARES means even a complete realm
 * is allowed no guards at all, which is how guards are switched off from this
 * file alone. */
#define KINGDOM_GUARD_DENOM_MIN 1
#define KINGDOM_GUARD_DENOM_MAX 1000

/* Map squares. 0 disables the matching proximity test; the ceiling is wider
 * than the widest map zone, so an operator can also refuse every site. */
#define KINGDOM_DISTANCE_MAX 1000

/* ------------------------------------------------------------------ *
 * The global
 * ------------------------------------------------------------------ */

/* Every field of struct kingdom_config carries a default member initialiser,
 * and the implicit default constructor is therefore constexpr, so this object
 * is constant-initialised at load time. That matters: kingdom_config_load()
 * runs during boot, but anything that reads kingdom_cfg before then -- most
 * importantly kingdom_enabled() -- still sees `enabled == false` rather than
 * an unsequenced dynamic initialiser. */
kingdom_config kingdom_cfg;

/* The reset target for a reload. Holding a second default-constructed copy
 * keeps the actual numbers in exactly one place, the header. */
static const kingdom_config kingdom_defaults{};

/* ------------------------------------------------------------------ *
 * Scalar parsing
 * ------------------------------------------------------------------ */

/* Whole decimal text -> long. Refuses a partial parse and an out-of-range
 * value; *result is written only on success. */
static bool kingdom_parse_long(const char *text, long *result)
{
	char *end;
	long parsed;

	errno = 0;
	parsed = strtol(text, &end, 10);
	/* end == text rejects "abc"; *end rejects the "12abc" tail that atoi()
	 * would have silently accepted as 12. */
	if (errno || end == text || *end != '\0')
		return false;
	*result = parsed;
	return true;
}

/* true/yes/1 and false/no/0, exact and case-sensitive. Anything else is
 * refused and leaves *result untouched. */
static bool kingdom_parse_bool(const char *text, bool *result)
{
	if (!strcmp(text, "true") || !strcmp(text, "yes") || !strcmp(text, "1"))
	{
		*result = true;
		return true;
	}
	if (!strcmp(text, "false") || !strcmp(text, "no") || !strcmp(text, "0"))
	{
		*result = false;
		return true;
	}
	return false;
}

/* Store `value` in *field if it parses as a long within [low, high]; otherwise
 * log the rejection and leave the field -- the compiled default -- alone. */
static void kingdom_set_long(const char *key, const char *value, long *field, long low, long high)
{
	long parsed;

	if (!kingdom_parse_long(value, &parsed) || parsed < low || parsed > high)
	{
		logit(LOG_KINGDOM, "Invalid kingdom config value %s=%s; retaining default.", key,
		      value);
		return;
	}
	*field = parsed;
}

/* As kingdom_set_long() for an int field. The bounds are ints, so a value that
 * passes the range check narrows without truncation. */
static void kingdom_set_int(const char *key, const char *value, int *field, int low, int high)
{
	long parsed;

	if (!kingdom_parse_long(value, &parsed) || parsed < low || parsed > high)
	{
		logit(LOG_KINGDOM, "Invalid kingdom config value %s=%s; retaining default.", key,
		      value);
		return;
	}
	/* Safe narrowing: low and high are ints, so parsed is already in range. */
	*field = (int)parsed;
}

/* Store `value` in *field if kingdom_parse_bool() accepts it; otherwise log
 * the rejection and leave the field -- the compiled default -- alone. */
static void kingdom_set_bool(const char *key, const char *value, bool *field)
{
	bool parsed;

	if (!kingdom_parse_bool(value, &parsed))
	{
		logit(LOG_KINGDOM, "Invalid kingdom config value %s=%s; retaining default.", key,
		      value);
		return;
	}
	*field = parsed;
}

/* True when the key was recognised, whether or not its value survived
 * validation. The caller only counts recognised keys; a bad value has already
 * been logged by the setter above. */
static bool kingdom_apply_value(const char *key, const char *value)
{
#define KCFG_BOOL(name, field)                                    \
	if (!strcmp(key, name))                                   \
	{                                                         \
		kingdom_set_bool(key, value, &kingdom_cfg.field); \
		return true;                                      \
	}
#define KCFG_INT(name, field, low, high)                                    \
	if (!strcmp(key, name))                                             \
	{                                                                   \
		kingdom_set_int(key, value, &kingdom_cfg.field, low, high); \
		return true;                                                \
	}
#define KCFG_LONG(name, field, low, high)                                    \
	if (!strcmp(key, name))                                              \
	{                                                                    \
		kingdom_set_long(key, value, &kingdom_cfg.field, low, high); \
		return true;                                                 \
	}

	KCFG_BOOL("kingdom.enabled", enabled)
	KCFG_LONG("kingdom.claim.cost.base", claim_cost_base, 0L, KINGDOM_CLAIM_BASE_MAX)
	KCFG_INT("kingdom.claim.cost.growth.permille", claim_cost_growth_permille,
		 KINGDOM_CLAIM_GROWTH_MIN, KINGDOM_CLAIM_GROWTH_MAX)
	KCFG_LONG("kingdom.claim.material.base", claim_material_base, 0L,
		  KINGDOM_CLAIM_MATERIAL_MAX)
	KCFG_LONG("kingdom.guard.cost.base", guard_cost_base, 0L, KINGDOM_GUARD_COST_MAX)
	KCFG_LONG("kingdom.guard.cost.max", guard_cost_max, 0L, KINGDOM_GUARD_COST_MAX)
	KCFG_LONG("kingdom.upkeep.per.square", upkeep_per_square, 0L, KINGDOM_UPKEEP_MAX)
	KCFG_INT("kingdom.upkeep.period.seconds", upkeep_period_seconds, KINGDOM_UPKEEP_PERIOD_MIN,
		 KINGDOM_UPKEEP_PERIOD_MAX)
	KCFG_INT("kingdom.guards.per.squares", guards_per_squares, KINGDOM_GUARD_DENOM_MIN,
		 KINGDOM_GUARD_DENOM_MAX)
	KCFG_INT("kingdom.min.hometown.distance", min_hometown_distance, 0, KINGDOM_DISTANCE_MAX)
	KCFG_INT("kingdom.min.entrance.distance", min_entrance_distance, 0, KINGDOM_DISTANCE_MAX)

#undef KCFG_BOOL
#undef KCFG_INT
#undef KCFG_LONG

	logit(LOG_KINGDOM, "Unknown kingdom config key: %s", key);
	return false;
}

/* ------------------------------------------------------------------ *
 * The loader
 * ------------------------------------------------------------------ */

/* Reset kingdom_cfg to the compiled defaults, then overlay lib/kingdom.cfg if
 * it exists: one key=value per line, '#'/';' comments and [section] headers
 * skipped, every recognised value range-checked before it displaces a default.
 * A missing file is normal and leaves kingdoms disabled. */
void kingdom_config_load(void)
{
	FILE *fp;
	char line[256], key[128], value[128];
	int recognised = 0;

	/* Reset first, so this is idempotent. kingdom_initialize() may run again
	 * after a shutdown, and a reload must not inherit the previous file's
	 * values for keys the new file leaves out. */
	kingdom_cfg = kingdom_defaults;

	fp = fopen(KINGDOM_CONFIG_FILE, "r");
	if (!fp)
	{
		/* Normal, not an error: this is what a server that has never
		 * opted in looks like, and kingdom_cfg.enabled is already
		 * false. logit() creates its own parent directories
		 * (core/utility.c:771-773), so LOG_KINGDOM is safe on a tree
		 * that has never run the kingdom code before. */
		logit(LOG_STATUS, "Kingdom config %s unavailable; kingdoms disabled.",
		      KINGDOM_CONFIG_FILE);
		logit(LOG_KINGDOM, "No %s; using compiled defaults, kingdoms disabled.",
		      KINGDOM_CONFIG_FILE);
		return;
	}

	while (fgets(line, sizeof(line), fp))
	{
		const char *scan = line;
		size_t len = strlen(line);
		size_t key_len;

		/* Strip the line ending up front so it cannot reach a log
		 * message or, on a CRLF file, ride along inside a value. */
		while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
			line[--len] = '\0';

		while (*scan == ' ' || *scan == '\t')
			scan++;

		/* Blank, comment, or a [section] header. lib/crafting.cfg ships
		 * section headers and its loader drops them by way of having no
		 * '=' (economy/crafting.c:108-110), so an operator copying that
		 * file's style here must not get a wall of "unknown key". */
		if (*scan == '\0' || *scan == '#' || *scan == ';' || *scan == '[')
			continue;

		if (sscanf(scan, " %127[^=]= %127s", key, value) != 2)
		{
			logit(LOG_KINGDOM, "Ignoring malformed kingdom config line: %s", line);
			continue;
		}

		/* %[^=] stops AT the '=' but keeps everything before it, so
		 * `kingdom.enabled = true` yields the key "kingdom.enabled "
		 * with a trailing space and would miss every strcmp below.
		 * Trim it rather than making whitespace around '=' an invisible
		 * configuration error. */
		key_len = strlen(key);
		while (key_len > 0 && (key[key_len - 1] == ' ' || key[key_len - 1] == '\t'))
			key[--key_len] = '\0';

		if (key_len == 0)
		{
			logit(LOG_KINGDOM, "Ignoring kingdom config line with an empty key: %s",
			      line);
			continue;
		}

		if (kingdom_apply_value(key, value))
			recognised++;
	}
	fclose(fp);

	/* Belt and braces on the one value another translation unit divides by.
	 * The range check above already forbids 0, but kingdom_guard_allowance()
	 * has no way to defend itself if that range is ever widened. */
	if (kingdom_cfg.guards_per_squares < 1)
		kingdom_cfg.guards_per_squares = kingdom_defaults.guards_per_squares;

	logit(LOG_KINGDOM,
	      "Loaded %s: %d setting(s). claim %ld copper x%d/1000 compounding plus %ld of each "
	      "material, upkeep %ld/square every %d s, 1 guard per %d squares costing %ld to %ld, "
	      "hometown >= %d, entrance >= %d.",
	      KINGDOM_CONFIG_FILE, recognised, kingdom_cfg.claim_cost_base,
	      kingdom_cfg.claim_cost_growth_permille, kingdom_cfg.claim_material_base,
	      kingdom_cfg.upkeep_per_square, kingdom_cfg.upkeep_period_seconds,
	      kingdom_cfg.guards_per_squares, kingdom_cfg.guard_cost_base,
	      kingdom_cfg.guard_cost_max, kingdom_cfg.min_hometown_distance,
	      kingdom_cfg.min_entrance_distance);
	logit(LOG_STATUS, "Kingdom config loaded from %s; kingdoms %s.", KINGDOM_CONFIG_FILE,
	      kingdom_cfg.enabled ? "ENABLED" : "disabled");
}

/* ------------------------------------------------------------------ *
 * Resource names
 * ------------------------------------------------------------------ */

/* Indexed by enum kingdom_resource, so the ORDER here is load-bearing: it must
 * track kingdom/kingdom_internal.h:32-38. The static_assert is the guard --
 * add a resource without adding its name and the build stops, rather than the
 * display quietly printing "unknown" for it. */
static const char *const kingdom_resource_names[] = { "mineral", "wood", "fibre", "water" };

static_assert((int)(sizeof(kingdom_resource_names) / sizeof(kingdom_resource_names[0])) == KRES_MAX,
	      "every kingdom_resource needs a name in kingdom_resource_names");

/* Display name of a kingdom_resource, or "unknown" for any index outside
 * 0..KRES_MAX-1. Never NULL. */
const char *kingdom_resource_name(int res)
{
	/* Callers pass indices that came from persisted rows and from player
	 * input, so an out-of-range value is expected rather than a bug to
	 * assert on: answer with something printable. */
	if (res < 0 || res >= KRES_MAX)
		return "unknown";
	return kingdom_resource_names[res];
}
