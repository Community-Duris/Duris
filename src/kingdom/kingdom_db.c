/*
 *  kingdom_db.c
 *  Duris
 *
 *  Persistence for the kingdom realms: the four entry points declared at the
 *  foot of kingdom_internal.h, implemented once against MariaDB and once
 *  against the flat-file store.
 *
 *  WHY A COMPILE-TIME SPLIT AND NOT A RUNTIME ONE
 *  ---------------------------------------------
 *  persistence_mode_configure() refuses to boot a MariaDB-client build in
 *  flatfile-primary mode, refuses to boot a client-free build in
 *  mariadb-primary mode, and rejects the hybrid mode outright
 *  (src/persistence/persistence_mode.c:113-142). Exactly one store is
 *  reachable in any given binary, and persistence_mode_flatfile_root() is
 *  NULL under the default MariaDB build, so a flat-file-only store would
 *  silently not persist in production. The split is therefore the same
 *  __NO_MYSQL__ flag the Makefile sets for PERSISTENCE_BACKEND=flatfile
 *  (src/Makefile:42-51) rather than a runtime branch carrying a dead half.
 *
 *  WHAT IS PERSISTED
 *  -----------------
 *  One record per realm, eleven integers wide. THE TERRITORY IS A SINGLE
 *  INTEGER: because the claim order is fixed and a ring completes before the
 *  next opens, a realm owns claims 1..highest_claim and nothing else. So
 *  claiming a square, reverting an outer ring for arrears, and losing the
 *  whole realm are all one small integer write -- there is no per-square
 *  table and no 80-row rewrite anywhere in this file.
 *
 *  The anchor is stored as hall_vnum ONLY. hall_rnum, zone_idx and
 *  hall_x/hall_y are derived state; kingdom_db_load_all() deliberately leaves
 *  them at their "unresolved" defaults (hall_rnum 0, matching real_room0()'s
 *  sentinel) because resolving them needs a loaded world, and this module must
 *  be safe to load from before the world is up. kingdom_initialize() resolves
 *  the anchors and builds the square index after loading, exactly as its
 *  contract in kingdom.h says.
 */

#include "kingdom/kingdom_internal.h"

#include "core/structs.h"

#include "core/utility.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <vector>

#ifdef __NO_MYSQL__
#include "flatfile/flatfile_store.h"
#include "persistence/persistence_mode.h"

#include <algorithm>
#include <array>
#include <limits>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>
#else
#include "sql/sql.h"
#endif

/* ------------------------------------------------------------------ *
 * Shared between the two backends
 * ------------------------------------------------------------------ */

namespace
{
/* Adding a resource type without adding its column here would silently drop
 * the new counter on every save and zero it on every load. Break the build
 * instead: both encoders below enumerate exactly these four. */
static_assert(KRES_MAX == 4, "kingdom_db persists exactly four resource counters");

/* The resource counters are declared `long` but travel as a fixed-width
 * int64_t on disk and as %ld in SQL. If long ever narrows, the flat-file
 * round-trip would truncate silently instead of failing. */
static_assert(sizeof(long) >= sizeof(int64_t),
	      "kingdom resource counters must round-trip through int64_t");

/* The flat-file encoder writes the realm's `int` fields as int32_t while the
 * decoder reads them back through sizeof(T) on the `int` field itself. That is
 * symmetric only while the two are the same width, so pin it here: a narrower
 * int would desynchronise the payload cursor and silently misread every record
 * after the first rather than fail the checksum. */
static_assert(sizeof(int) == sizeof(int32_t),
	      "kingdom realm integers are encoded as int32_t and decoded as int");

/* One realm per association at most, and the association flat store caps
 * itself at 65536 associations for the same reason -- a length field read off
 * disk, or a row count read out of a table, must never authorise an unbounded
 * allocation. */
constexpr size_t kingdom_realm_maximum = 65536;

/* REJECT a bad record rather than repair it. highest_claim indexes
 * KINGDOM_CLAIM_ORDER in every other file of this module, so a value past
 * KINGDOM_MAX_SQUARES read back from a corrupt store would walk off the end
 * of that table; arrears indexes the ladder.
 *
 * Both backends reject per RECORD and never per store, and both log the
 * rejection -- but THE TWO END STATES ARE NOT THE SAME, and an operator
 * reading the log has to know which backend produced it:
 *
 *   MariaDB: the loader logs the row and skips it. Nothing on the load path
 *     deletes or rewrites a row it refused, so the corrupt row is still in
 *     kingdom_realms after the boot that rejected it: readable for forensics
 *     and repairable by hand, after which the next boot loads it. (Not
 *     immortal -- a later upsert for that same assoc_id, or
 *     kingdom_on_guild_deleted()'s DELETE, still replaces or removes it. The
 *     point is only that the REJECTION itself destroys nothing.)
 *
 *   Flat file: the decoder logs the record and drops it from the decoded
 *     catalogue, and every write republishes the whole authority file from
 *     that catalogue. So the FIRST publish after the drop -- any single-realm
 *     save, any flush -- writes the file without the record and ERASES IT
 *     FROM DISK for good. THE FLAT-FILE DROP IS DESTRUCTIVE AND THE SQL DROP
 *     IS NOT: copy the authority file aside before running a server that has
 *     logged one, or the evidence goes with the next write.
 *
 * Only the flat file's checksum, layout and assoc_id ordering fail the whole
 * load. Both save paths (single-record and flush) refuse to write a record
 * that fails this, leaving it dirty in memory. Losing one guild's territory
 * is the outcome in every case, rather than the process. */
bool record_is_sane(const kingdom_realm &realm)
{
	if (realm.assoc_id <= 0 || realm.realm_id < 0)
		return false;
	if (realm.hall_vnum < 0)
		return false;
	if (realm.highest_claim < 0 || realm.highest_claim > KINGDOM_MAX_SQUARES)
		return false;
	if (realm.arrears < KARR_CURRENT || realm.arrears > KARR_RINGS_REVERTING)
		return false;
	if (realm.missed_cycles < 0)
		return false;
	if (realm.upkeep_paid_through < 0)
		return false;
	for (int res = 0; res < KRES_MAX; res++)
		if (realm.resources[res] < 0)
			return false;
	return true;
}
} /* namespace */

#ifndef __NO_MYSQL__

/* ------------------------------------------------------------------ *
 * MariaDB backend
 * ------------------------------------------------------------------ *
 * Table kingdom_realms, assoc_id PRIMARY KEY. The schema is owned by
 * migrations/, not by this file: nothing in src/ issues CREATE TABLE.
 *
 * NOTHING HERE IS ESCAPED, ON PURPOSE. Every column is an integer and every
 * value is formatted with %d / %ld / %lld from a field of the realm struct,
 * so no player-supplied text can reach a statement and there is nothing for
 * escape_str() (src/sql/sql.c:1107) to do. The realm carries no name -- the
 * guild's name lives in the association record.
 */

/* Defined below kingdom_db_load_all(), which calls it: the roster loader reads
 * realms that pass has already filed, so it belongs after them in the file and
 * needs a forward declaration to be reached from before. */
static void kingdom_db_load_rosters(void);

namespace
{
/* ONE list, used by both the SELECT and the INSERT, so the two can never
 * drift apart and the row[] indices below always mean what they say. */
const char *const kingdom_realm_columns = "assoc_id,realm_id,hall_vnum,highest_claim,"
					  "res_mineral,res_wood,res_fibre,res_water,"
					  "upkeep_paid_through,arrears,missed_cycles";
constexpr unsigned int kingdom_realm_column_count = 11;
} /* namespace */

/* Replace kingdom_realms with every row of the kingdom_realms table, anchors
 * unresolved and flags clear. Rows with a NULL column, a record that fails
 * record_is_sane() or a duplicate assoc_id are skipped, each with its own log
 * line naming the row, and then counted in the closing "rejected" total. A
 * skipped row is left in the table -- see the record_is_sane() banner for why
 * that differs from the flat-file backend. False when the SELECT fails, the
 * column count is wrong, memory runs out, or the cap truncates the load --
 * never for an empty table. */
bool kingdom_db_load_all(void)
{
	kingdom_realms.clear();

	MYSQL_RES *result =
		db_query("SELECT %s FROM kingdom_realms ORDER BY assoc_id", kingdom_realm_columns);
	if (!result)
	{
		/* db_query_at() returns the stored result set for any statement
		 * that ran, so a SELECT matching nothing still yields a non-NULL
		 * empty set (src/sql/sql.c db_query_at -> mysql_store_result).
		 * NULL therefore means the statement FAILED, never "no kingdoms",
		 * and reporting success here would tell the caller that every
		 * realm had ceased to exist. */
		logit(LOG_KINGDOM, "kingdom_db_load_all: SELECT from kingdom_realms failed; "
				   "no realms loaded");
		return false;
	}

	if (mysql_num_fields(result) != kingdom_realm_column_count)
	{
		/* Schema drift. Without this the row[] reads below would run past
		 * the end of a short row. */
		logit(LOG_KINGDOM,
		      "kingdom_db_load_all: kingdom_realms returned %u columns, expected %u; "
		      "run the kingdom migration",
		      mysql_num_fields(result), kingdom_realm_column_count);
		mysql_free_result(result);
		return false;
	}

	size_t loaded = 0;
	size_t rejected = 0;
	bool complete = true;

	try
	{
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(result)) != NULL)
		{
			if (kingdom_realms.size() >= kingdom_realm_maximum)
			{
				logit(LOG_KINGDOM,
				      "kingdom_db_load_all: stopped at %zu realms, the cap; "
				      "the rest of kingdom_realms was not loaded",
				      kingdom_realm_maximum);
				complete = false;
				break;
			}

			unsigned int null_column = kingdom_realm_column_count;
			for (unsigned int column = 0; column < kingdom_realm_column_count; column++)
				if (!row[column])
				{
					null_column = column;
					break;
				}
			if (null_column != kingdom_realm_column_count)
			{
				/* One line per rejected row, the same shape as the
				 * corrupt-record drop below: the aggregate "rejected"
				 * total at the foot cannot tell an operator WHICH row
				 * to repair. The column is reported as its index into
				 * kingdom_realm_columns, which is printed with it,
				 * rather than as a second name list that could drift
				 * out of step with the SELECT. assoc_id is printed as
				 * the raw string because assoc_id is row[0] and may be
				 * the NULL column itself. */
				logit(LOG_KINGDOM,
				      "kingdom_db_load_all: rejecting a row of kingdom_realms "
				      "(assoc_id %s): column %u of \"%s\", counting from 0, is "
				      "NULL; the row is left in the table for repair",
				      row[0] ? row[0] : "NULL", null_column, kingdom_realm_columns);
				rejected++;
				continue;
			}

			kingdom_realm realm;
			realm.assoc_id = atoi(row[0]);
			realm.realm_id = atoi(row[1]);
			realm.hall_vnum = atoi(row[2]);
			realm.highest_claim = atoi(row[3]);
			for (int res = 0; res < KRES_MAX; res++)
				realm.resources[res] = strtol(row[4 + res], NULL, 10);
			realm.upkeep_paid_through = static_cast<time_t>(strtoll(row[8], NULL, 10));
			realm.arrears = atoi(row[9]);
			realm.missed_cycles = atoi(row[10]);
			/* Anchor left unresolved on purpose -- see the file banner.
			 * dirty and payment_pending are runtime-only and stay at
			 * their defaults: just read, by definition clean. */

			if (!record_is_sane(realm))
			{
				logit(LOG_KINGDOM,
				      "kingdom_db_load_all: rejecting corrupt row for "
				      "association %d (claim %d, arrears %d); the row is left "
				      "in the table for repair",
				      realm.assoc_id, realm.highest_claim, realm.arrears);
				rejected++;
				continue;
			}
			if (!kingdom_realms.emplace(realm.assoc_id, realm).second)
			{
				logit(LOG_KINGDOM,
				      "kingdom_db_load_all: duplicate realm for association %d; "
				      "keeping the first",
				      realm.assoc_id);
				rejected++;
				continue;
			}
			loaded++;
		}
	}
	catch (const std::bad_alloc &)
	{
		logit(LOG_KINGDOM, "kingdom_db_load_all: out of memory after %zu realms", loaded);
		kingdom_realms.clear();
		mysql_free_result(result);
		return false;
	}

	mysql_free_result(result);

	if (rejected)
		logit(LOG_KINGDOM, "kingdom_db_load_all: loaded %zu realms, rejected %zu", loaded,
		      rejected);
	else
		logit(LOG_KINGDOM, "kingdom_db_load_all: loaded %zu realms", loaded);

	/* Rosters AFTER the realms, because each row is filed against a realm
	 * this pass has already put in the map. */
	if (complete)
		kingdom_db_load_rosters();

	/* A truncated load is a failed load: the caller must not treat the
	 * squares of the realms it never saw as unowned. */
	return complete;
}

/* Upsert one realm's row, joining any open transaction (qry() runs on the
 * shared connection). False when the record fails record_is_sane() or the
 * statement fails; the caller's dirty flag is untouched either way.
 *
 * THIS PRIMITIVE DOES NOT TEST payment_pending, deliberately: it is what
 * kingdom_persist_payment() calls to publish a pending record together with
 * the guild debit that justifies it, so a guard here would deadlock that
 * pairing. The obligation is therefore the CALLER's -- every call to this
 * function from outside kingdom_db.c and kingdom_persist_payment() must be
 * guarded by !payment_pending and leave a pending record dirty for
 * kingdom_upkeep_retry_pending() to carry. */
bool kingdom_db_save_realm(const kingdom_realm &realm)
{
	if (!record_is_sane(realm))
	{
		logit(LOG_KINGDOM,
		      "kingdom_db_save_realm: refusing to persist an invalid record for "
		      "association %d (claim %d, arrears %d)",
		      realm.assoc_id, realm.highest_claim, realm.arrears);
		return false;
	}

	/* assoc_id is the primary key, so one upsert covers both create and
	 * update and no caller has to know which it is doing. Guild ids are
	 * reused by found_asc(), which is exactly why kingdom_on_guild_deleted()
	 * must DELETE the row rather than leave it for the next holder of the id
	 * to inherit through this statement. */
	if (!qry("INSERT INTO kingdom_realms (%s) VALUES "
		 "(%d,%d,%d,%d,%ld,%ld,%ld,%ld,%lld,%d,%d) "
		 "ON DUPLICATE KEY UPDATE realm_id=VALUES(realm_id),"
		 "hall_vnum=VALUES(hall_vnum),highest_claim=VALUES(highest_claim),"
		 "res_mineral=VALUES(res_mineral),res_wood=VALUES(res_wood),"
		 "res_fibre=VALUES(res_fibre),res_water=VALUES(res_water),"
		 "upkeep_paid_through=VALUES(upkeep_paid_through),"
		 "arrears=VALUES(arrears),missed_cycles=VALUES(missed_cycles)",
		 kingdom_realm_columns, realm.assoc_id, realm.realm_id, realm.hall_vnum,
		 realm.highest_claim, realm.resources[KRES_MINERAL], realm.resources[KRES_WOOD],
		 realm.resources[KRES_FIBRE], realm.resources[KRES_WATER],
		 static_cast<long long>(realm.upkeep_paid_through), realm.arrears,
		 realm.missed_cycles))
	{
		logit(LOG_KINGDOM, "kingdom_db_save_realm: upsert failed for association %d",
		      realm.assoc_id);
		return false;
	}

	return kingdom_db_save_roster(realm);
}

/*
 * Publish a realm's garrison roster: the bought guards and the champion.
 *
 * DELETE-THEN-INSERT rather than a row-by-row diff. The roster is at most
 * seventeen tiny rows, it changes only on a hire, a promotion or a death, and
 * the two statements run inside the same transaction the realm's own upsert
 * joins -- so the whole roster is replaced atomically and no bookkeeping is
 * needed to notice a slot that emptied. A diff would be more code for less
 * certainty.
 *
 * A MISSING TABLE IS NOT AN ERROR HERE, and that is deliberate:
 * kingdom_garrison is outside the runtime contract's table list (see the
 * migration and runtime_compatibility_contract.h), so a database at head 0008
 * still boots and still runs kingdoms -- it simply cannot keep a roster. The
 * failure is logged once per attempt and the realm's own row still lands,
 * which is the same graceful degradation kingdom_realms itself was given.
 */
bool kingdom_db_save_roster(const kingdom_realm &realm)
{
	if (realm.assoc_id <= 0)
		return false;

	if (!qry("DELETE FROM kingdom_garrison WHERE assoc_id=%d", realm.assoc_id))
	{
		logit(LOG_KINGDOM,
		      "kingdom_db_save_roster: could not clear the roster for association %d; "
		      "the garrison will not persist",
		      realm.assoc_id);
		return false;
	}

	for (int slot = 0; slot < KINGDOM_GUARD_SLOTS; slot++)
	{
		if (realm.guards[slot].level <= 0)
			continue;

		if (!qry("INSERT INTO kingdom_garrison (assoc_id,slot,guard_class,level) "
			 "VALUES (%d,%d,%d,%d)",
			 realm.assoc_id, slot, realm.guards[slot].guard_class,
			 realm.guards[slot].level))
		{
			logit(LOG_KINGDOM,
			      "kingdom_db_save_roster: INSERT failed for association %d slot %d",
			      realm.assoc_id, slot);
			return false;
		}
	}

	if (realm.champion_class)
	{
		if (!qry("INSERT INTO kingdom_garrison (assoc_id,slot,guard_class,level) "
			 "VALUES (%d,%d,%d,%d)",
			 realm.assoc_id, KINGDOM_CHAMPION_SLOT, realm.champion_class,
			 KINGDOM_CHAMPION_LEVEL))
		{
			logit(LOG_KINGDOM,
			      "kingdom_db_save_roster: INSERT failed for association %d champion",
			      realm.assoc_id);
			return false;
		}
	}

	return true;
}

/*
 * Fill in every loaded realm's roster in ONE query rather than one per realm:
 * the table is keyed by association and the whole of it is wanted, so a single
 * ordered scan costs one round trip whatever the number of realms.
 *
 * Rows for a realm that is not loaded are skipped in silence -- they are the
 * normal residue of a guild deleted while the server was down, and
 * kingdom_on_guild_deleted() clears them the next time that id is used. A row
 * with a slot or level outside its range is dropped with a line naming it,
 * because a guard at level 900 would otherwise walk out of the gate.
 */
static void kingdom_db_load_rosters(void)
{
	MYSQL_RES *result = db_query("SELECT assoc_id,slot,guard_class,level FROM "
				     "kingdom_garrison ORDER BY assoc_id,slot");

	if (!result)
	{
		/* Head 0006 has no such table. Kingdoms still run; no realm can
		 * keep a garrison until the database is migrated. */
		logit(LOG_KINGDOM, "kingdom_db_load_rosters: kingdom_garrison could not be read; "
				   "no realm will field guards this boot");
		return;
	}

	if (mysql_num_fields(result) != 4)
	{
		logit(LOG_KINGDOM, "kingdom_db_load_rosters: kingdom_garrison has the wrong shape; "
				   "no rosters loaded");
		mysql_free_result(result);
		return;
	}

	size_t loaded = 0, rejected = 0;
	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)) != NULL)
	{
		if (!row[0] || !row[1] || !row[2] || !row[3])
		{
			rejected++;
			continue;
		}

		const int assoc_id = atoi(row[0]);
		const int slot = atoi(row[1]);
		const int guard_class = atoi(row[2]);
		const int level = atoi(row[3]);

		kingdom_realm *realm = kingdom_find_realm(assoc_id);

		if (!realm)
			continue;

		if (slot == KINGDOM_CHAMPION_SLOT)
		{
			if (level == KINGDOM_CHAMPION_LEVEL && guard_class)
				realm->champion_class = guard_class;
			else
				rejected++;
			continue;
		}

		if (slot < 0 || slot >= KINGDOM_GUARD_SLOTS || level < KINGDOM_GUARD_BASE_LEVEL ||
		    level > KINGDOM_GUARD_TOP_LEVEL)
		{
			logit(LOG_KINGDOM,
			      "kingdom_db_load_rosters: dropping association %d slot %d level %d "
			      "as out of range",
			      assoc_id, slot, level);
			rejected++;
			continue;
		}

		realm->guards[slot].guard_class = guard_class;
		realm->guards[slot].level = level;
		loaded++;
	}

	mysql_free_result(result);

	if (rejected)
		logit(LOG_KINGDOM, "kingdom_db_load_rosters: loaded %zu guard(s), rejected %zu",
		      loaded, rejected);
	else
		logit(LOG_KINGDOM, "kingdom_db_load_rosters: loaded %zu guard(s)", loaded);
}

/* Delete the row for assoc_id. True when the statement ran, whether or not a
 * row matched; false for a non-positive id or a failed statement. */
bool kingdom_db_delete_realm(int assoc_id)
{
	if (assoc_id <= 0)
	{
		logit(LOG_KINGDOM, "kingdom_db_delete_realm: refusing association id %d", assoc_id);
		return false;
	}

	/* Idempotent: a DELETE that matched nothing still succeeded, which is what
	 * kingdom_on_guild_deleted() needs when the guild never had a realm. */
	if (!qry("DELETE FROM kingdom_realms WHERE assoc_id=%d", assoc_id))
	{
		logit(LOG_KINGDOM, "kingdom_db_delete_realm: DELETE failed for association %d",
		      assoc_id);
		return false;
	}

	/* The roster goes with the realm, and for the same reason the realm's own
	 * row does: association ids are reused, so a garrison left behind would
	 * be inherited by whoever founds the next guild on that id. Its failure
	 * does NOT fail the delete -- the realm is gone either way, and a
	 * database at head 0008 has no such table to clear. */
	if (!qry("DELETE FROM kingdom_garrison WHERE assoc_id=%d", assoc_id))
		logit(LOG_KINGDOM,
		      "kingdom_db_delete_realm: could not clear the roster for association %d",
		      assoc_id);

	return true;
}

/* Upsert every dirty realm, one statement each, clearing dirty on success. A
 * realm with payment_pending set is HELD BACK -- not written, counted and
 * logged -- because its record carries a paid mark whose guild debit is not
 * yet durable; kingdom_persist_payment() writes that pair. A failed upsert
 * leaves the record dirty for the next flush. Never erases from the map. */
void kingdom_db_flush_dirty(void)
{
	size_t saved = 0;
	size_t failed = 0;
	size_t held = 0;

	for (auto &entry : kingdom_realms)
	{
		if (!entry.second.dirty)
			continue;
		if (entry.second.payment_pending)
		{
			/* Publishing this alone would record a payment the guild has
			 * not durably made. It stays dirty AND pending until the pair
			 * lands through kingdom_upkeep_retry_pending(). */
			held++;
			continue;
		}
		if (kingdom_db_save_realm(entry.second))
		{
			entry.second.dirty = false;
			saved++;
		}
		else
		{
			/* Leave the flag set: the next cycle retries. Clearing it on
			 * failure would drop the write for good. */
			failed++;
		}
	}

	if (failed || held)
		logit(LOG_KINGDOM,
		      "kingdom_db_flush_dirty: saved %zu realms, %zu failed and still dirty, "
		      "%zu held back (payment pending)",
		      saved, failed, held);
}

#else /* __NO_MYSQL__ */

/* ------------------------------------------------------------------ *
 * Flat-file backend
 * ------------------------------------------------------------------ *
 * Same on-disk shape as the other flat-file authorities
 * (src/flatfile/flatfile_nexus_repository.c): magic, version, payload length,
 * monotonic revision, SHA-256 of the payload, then the payload. Written
 * through flatfile_atomic_write() under an advisory lock so a crash mid-write
 * cannot leave a half-file behind.
 *
 * All records live in ONE file because the whole catalogue is a few tens of
 * bytes per realm; there is no per-realm file to fan out and no per-square
 * anything to store.
 */

namespace
{
constexpr std::array<uint8_t, 8> kingdom_magic = { 'D', 'U', 'R', 'K', 'I', 'N', 'G', 0 };
/* 2 since 2026-09-04: the record gained the garrison roster. decode_catalog()
 * still reads version 1 files and gives their realms an empty roster, which is
 * what a version 1 file means -- guards were derived from land then and none
 * had been bought. Every write is version 2. */
constexpr uint32_t kingdom_file_version = 2;
/* RAISED WITH THE RECORD, 2026-09-04. A version 1 record was 64 payload bytes,
 * so the 65536-realm cap came to 4 MiB and 8 MiB was ample headroom. Version 2
 * carries the roster and is 196, which puts the same cap at 12.25 MiB -- past
 * the old ceiling, so a server at the cap would have failed to publish at all.
 * 32 MiB restores the same generous margin over the cap and still bounds what
 * a corrupt length field can ask this code to read. */
constexpr size_t kingdom_file_maximum_bytes = 32 * 1024 * 1024;
constexpr const char *kingdom_filename = "kingdom_realms";
constexpr const char *kingdom_lock_filename = "kingdom_realms.lock";

enum class flat_result
{
	ok,
	not_found,
	invalid,
	io_error
};

struct kingdom_catalog
{
	uint64_t revision = 0;
	std::vector<kingdom_realm> records;
};

struct encoder
{
	std::vector<uint8_t> bytes;
	bool valid = true;

	/* Append `value` little-endian in sizeof(T) bytes; an allocation
	 * failure clears `valid` and every later call is ignored. */
	template <typename T> void number(T value)
	{
		/* Once a push has failed there is no partial file worth building:
		 * stop appending, exactly as flatfile_association_repository.c's
		 * encoder does, so `valid` is the only thing callers must test. */
		if (!valid)
			return;
		using U = std::make_unsigned_t<T>;
		U bits = static_cast<U>(value);
		try
		{
			for (size_t index = 0; index < sizeof(T); ++index)
			{
				bytes.push_back(static_cast<uint8_t>(bits & 0xff));
				bits >>= 8;
			}
		}
		catch (const std::bad_alloc &)
		{
			valid = false;
		}
	}

	/* Append `size` bytes verbatim; a NULL pointer with a non-zero size, or
	 * an allocation failure, clears `valid`. */
	void raw(const uint8_t *data, size_t size)
	{
		if (!valid || (!data && size))
		{
			valid = false;
			return;
		}
		try
		{
			bytes.insert(bytes.end(), data, data + size);
		}
		catch (const std::bad_alloc &)
		{
			valid = false;
		}
	}
};

struct decoder
{
	const uint8_t *data;
	size_t size;
	size_t offset = 0;

	/* Read sizeof(T) little-endian bytes at the cursor into *value and
	 * advance; false, with nothing consumed, when fewer remain. */
	template <typename T> bool number(T *value)
	{
		if (!value || size - offset < sizeof(T))
			return false;
		using U = std::make_unsigned_t<T>;
		U bits = 0;
		for (size_t index = 0; index < sizeof(T); ++index)
			bits |= static_cast<U>(data[offset++]) << (index * 8);
		*value = static_cast<T>(bits);
		return true;
	}
};

/* The directory under the flat-file state root that holds the realm
 * authority and its lock file. */
std::string metadata_directory(const std::string &root)
{
	return root + "/metadata";
}

/* persistence_mode_flatfile_root() just returns active_flatfile_root
 * (src/persistence/persistence_mode.c:170-173), which is cleared to NULL on
 * every configure (:100) and assigned only on the flatfile-primary path
 * (:133-134). A NULL here therefore means persistence was never configured --
 * fail loudly rather than write into a relative path next to the working
 * directory. */
bool flat_root(std::string *root)
{
	const char *configured = persistence_mode_flatfile_root();
	if (!configured || !*configured)
		return false;
	try
	{
		root->assign(configured);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

/* The LAYOUT half of catalogue sanity: at most kingdom_realm_maximum records
 * in strictly increasing assoc_id order. This is what the decoder demands of
 * a file as a whole; it says nothing about the fields of any one record. */
bool records_are_ordered(const std::vector<kingdom_realm> &records)
{
	if (records.size() > kingdom_realm_maximum)
		return false;
	for (size_t index = 1; index < records.size(); ++index)
	{
		/* Strictly increasing assoc_id: this both keeps the file canonical
		 * and makes a duplicated realm unrepresentable. */
		if (records[index - 1].assoc_id >= records[index].assoc_id)
			return false;
	}
	return true;
}

/* Full catalogue sanity, for the ENCODER: ordered, and every record passes
 * record_is_sane(). The decoder does not use this -- it drops an insane record
 * individually -- so a catalogue that came through decode_catalog() always
 * satisfies it, and a caller upserting a record it has already checked
 * cannot break it. */
bool records_are_sane(const std::vector<kingdom_realm> &records)
{
	if (!records_are_ordered(records))
		return false;
	for (const auto &record : records)
		if (!record_is_sane(record))
			return false;
	return true;
}

/* Serialise a catalogue into the on-disk image: magic, version, payload
 * length, revision, SHA-256 of the payload, then the payload of count +
 * records. False for a NULL output, a zero revision, an unsane catalogue, an
 * allocation failure, or an image over kingdom_file_maximum_bytes. */
bool encode_catalog(const kingdom_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || !catalog.revision || !records_are_sane(catalog.records))
		return false;

	encoder payload;
	payload.number<uint32_t>(static_cast<uint32_t>(catalog.records.size()));
	for (const auto &record : catalog.records)
	{
		payload.number<int32_t>(record.assoc_id);
		payload.number<int32_t>(record.realm_id);
		payload.number<int32_t>(record.hall_vnum);
		payload.number<int32_t>(record.highest_claim);
		for (int res = 0; res < KRES_MAX; res++)
			payload.number<int64_t>(record.resources[res]);
		payload.number<int64_t>(static_cast<int64_t>(record.upkeep_paid_through));
		payload.number<int32_t>(record.arrears);
		payload.number<int32_t>(record.missed_cycles);

		/* VERSION 2 ADDS THE ROSTER, appended after the fields version 1
		 * wrote so the record's existing layout is untouched. Written as a
		 * fixed KINGDOM_GUARD_SLOTS pairs plus the champion's class rather
		 * than as a count and a list: the array is seventeen small
		 * integers, a fixed span needs no length field to be trusted, and
		 * a decoder reading a fixed span cannot be walked off the end by a
		 * corrupt count. */
		for (int slot = 0; slot < KINGDOM_GUARD_SLOTS; slot++)
		{
			payload.number<int32_t>(record.guards[slot].guard_class);
			payload.number<int32_t>(record.guards[slot].level);
		}
		payload.number<int32_t>(record.champion_class);
	}
	if (!payload.valid || payload.bytes.size() > kingdom_file_maximum_bytes)
		return false;

	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.bytes.data(), payload.bytes.size(), digest.data());

	encoder file;
	file.raw(kingdom_magic.data(), kingdom_magic.size());
	file.number(kingdom_file_version);
	file.number<uint32_t>(static_cast<uint32_t>(payload.bytes.size()));
	file.number(catalog.revision);
	file.raw(digest.data(), digest.size());
	file.raw(payload.bytes.data(), payload.bytes.size());
	if (!file.valid || file.bytes.size() > kingdom_file_maximum_bytes)
		return false;

	*bytes = std::move(file.bytes);
	return true;
}

/* Parse an on-disk image into a catalogue. False -- the whole file rejected --
 * for a bad magic, version, length, revision, checksum or record count, a
 * short or over-long payload, or records out of assoc_id order. A record whose
 * FIELDS fail record_is_sane() is logged and dropped on its own and decoding
 * continues, matching the MariaDB loader's per-row rejection in shape but NOT
 * in what it leaves behind: the next publish rewrites this whole file from the
 * decoded catalogue, so the dropped record is erased from disk permanently,
 * whereas the MariaDB loader leaves its rejected row in the table for
 * forensics and hand repair. See the record_is_sane() banner. */
bool decode_catalog(const std::vector<uint8_t> &bytes, kingdom_catalog *catalog)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!catalog || bytes.size() < header_size ||
	    memcmp(bytes.data(), kingdom_magic.data(), kingdom_magic.size()))
		return false;

	decoder header{ bytes.data() + kingdom_magic.size(), bytes.size() - kingdom_magic.size() };
	uint32_t version = 0;
	uint32_t payload_size = 0;
	uint64_t revision = 0;
	/* VERSION 1 IS STILL READ. A server upgraded across the 2026-09-04
	 * roster change has a version 1 file on disk, and refusing it would
	 * present every realm as never having existed -- the worst possible
	 * reading of "the format changed". A version 1 record decodes with an
	 * empty roster, which is exactly what it means: guards were derived from
	 * land then, and nothing had been bought. The next write is version 2. */
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) || version < 1 || version > kingdom_file_version ||
	    !revision || payload_size != bytes.size() - header_size)
		return false;

	const bool has_roster = version >= 2;

	const uint8_t *expected_digest = bytes.data() + 24;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload_bytes, payload_size, digest.data());
	/* Constant-time compare, as everywhere else in the flat store. */
	if (CRYPTO_memcmp(expected_digest, digest.data(), digest.size()))
		return false;

	decoder payload{ payload_bytes, payload_size };
	uint32_t count = 0;
	if (!payload.number(&count) || count > kingdom_realm_maximum)
		return false;

	kingdom_catalog decoded;
	decoded.revision = revision;
	try
	{
		decoded.records.resize(count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}

	for (auto &record : decoded.records)
	{
		int64_t upkeep = 0;
		if (!payload.number(&record.assoc_id) || !payload.number(&record.realm_id) ||
		    !payload.number(&record.hall_vnum) || !payload.number(&record.highest_claim))
			return false;
		for (int res = 0; res < KRES_MAX; res++)
		{
			int64_t amount = 0;
			if (!payload.number(&amount))
				return false;
			record.resources[res] = static_cast<long>(amount);
		}
		if (!payload.number(&upkeep) || !payload.number(&record.arrears) ||
		    !payload.number(&record.missed_cycles))
			return false;
		record.upkeep_paid_through = static_cast<time_t>(upkeep);

		if (has_roster)
		{
			for (int slot = 0; slot < KINGDOM_GUARD_SLOTS; slot++)
			{
				if (!payload.number(&record.guards[slot].guard_class) ||
				    !payload.number(&record.guards[slot].level))
					return false;
			}
			if (!payload.number(&record.champion_class))
				return false;
		}
		/* Anchor stays unresolved and the record is clean: see the banner. */
	}
	if (payload.offset != payload.size || !records_are_ordered(decoded.records))
		return false;

	/* Per-record sanity is judged AFTER the layout is proven, and record by
	 * record: one corrupt field costs that realm, not the catalogue. The
	 * order check above ran over every record, dropped ones included, so a
	 * kept subsequence is still strictly ordered. */
	{
		size_t kept = 0;
		for (size_t index = 0; index < decoded.records.size(); ++index)
		{
			const kingdom_realm &record = decoded.records[index];

			if (!record_is_sane(record))
			{
				logit(LOG_KINGDOM,
				      "kingdom_db: dropping a corrupt realm record for "
				      "association %d (claim %d, arrears %d); the next write "
				      "of this file ERASES it from disk -- copy the realm "
				      "authority aside now if it is wanted for repair",
				      record.assoc_id, record.highest_claim, record.arrears);
				continue;
			}
			if (kept != index)
				decoded.records[kept] = record;
			kept++;
		}
		decoded.records.resize(kept);
	}

	*catalog = std::move(decoded);
	return true;
}

/* Read and decode the realm authority under `root`. not_found for an absent
 * file, io_error for an unreadable one, invalid for one that fails
 * decode_catalog(); each failure is logged. */
flat_result load_catalog(const std::string &root, kingdom_catalog *catalog)
{
	std::vector<uint8_t> bytes;
	std::string error;
	const auto read = flatfile_read(metadata_directory(root), kingdom_filename,
					kingdom_file_maximum_bytes, &bytes, &error);
	if (read == flatfile_read_result::not_found)
		return flat_result::not_found;
	if (read == flatfile_read_result::io_error)
	{
		logit(LOG_KINGDOM, "kingdom_db: cannot read the realm authority: %s",
		      error.empty() ? "io error" : error.c_str());
		return flat_result::io_error;
	}
	if (read != flatfile_read_result::ok || !decode_catalog(bytes, catalog))
	{
		logit(LOG_KINGDOM, "kingdom_db: the realm authority is corrupt: %s",
		      error.empty() ? "checksum or layout mismatch" : error.c_str());
		return flat_result::invalid;
	}
	return flat_result::ok;
}

/* Bump the catalogue's revision, encode it and write it atomically as the
 * realm authority. False, logged, when the revision would wrap, the catalogue
 * will not encode, or the write fails; the caller must hold the lock. */
bool publish_catalog(const std::string &root, kingdom_catalog *catalog)
{
	if (!catalog || catalog->revision == std::numeric_limits<uint64_t>::max())
		return false;
	++catalog->revision;

	std::vector<uint8_t> bytes;
	if (!encode_catalog(*catalog, &bytes))
	{
		logit(LOG_KINGDOM, "kingdom_db: refusing to write an unencodable realm catalogue");
		return false;
	}

	std::string error;
	if (!flatfile_atomic_write(metadata_directory(root), kingdom_filename, bytes, &error))
	{
		logit(LOG_KINGDOM, "kingdom_db: cannot write the realm authority: %s",
		      error.empty() ? "io error" : error.c_str());
		return false;
	}
	return true;
}

/* Take the advisory lock on the realm authority, storing its descriptor in
 * *lock_fd for flatfile_lock_release(). False, logged, when the root is empty,
 * the pointer is NULL or the lock cannot be taken. */
bool acquire_lock(const std::string &root, int *lock_fd)
{
	std::string error;
	if (root.empty() || !lock_fd ||
	    !flatfile_lock_acquire(metadata_directory(root), kingdom_lock_filename, lock_fd,
				   &error))
	{
		logit(LOG_KINGDOM, "kingdom_db: cannot lock the realm authority: %s",
		      error.empty() ? "io error" : error.c_str());
		return false;
	}
	return true;
}

/* Insert or replace one realm, keeping the vector sorted by assoc_id so
 * records_are_sane() holds and the file stays canonical. `dirty` and
 * `payment_pending` are runtime flags that are never encoded, so the stored
 * copy carries both cleared. False only on allocation failure. */
bool upsert_record(std::vector<kingdom_realm> *records, const kingdom_realm &realm)
{
	if (!records)
		return false;

	kingdom_realm stored = realm;
	stored.dirty = false;
	stored.payment_pending = false;

	const auto position = std::lower_bound(records->begin(), records->end(), stored.assoc_id,
					       [](const kingdom_realm &candidate, int value)
					       { return candidate.assoc_id < value; });
	try
	{
		if (position != records->end() && position->assoc_id == stored.assoc_id)
			*position = stored;
		else
			records->insert(position, stored); /* invalidates `position` */
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}
} /* namespace */

/* Replace kingdom_realms with the records of the realm authority file,
 * anchors unresolved and flags clear. True for an absent file (an empty
 * world); false when persistence has no flat-file root, the file is
 * unreadable or corrupt as a whole, or memory runs out. Individual corrupt
 * records were already dropped and logged by decode_catalog(). */
bool kingdom_db_load_all(void)
{
	kingdom_realms.clear();

	std::string root;
	if (!flat_root(&root))
	{
		logit(LOG_KINGDOM,
		      "kingdom_db_load_all: no flat-file state root; persistence is not "
		      "configured, no realms loaded");
		return false;
	}

	kingdom_catalog catalog;
	const flat_result loaded = load_catalog(root, &catalog);
	if (loaded == flat_result::not_found)
	{
		/* First boot with kingdoms on: an absent authority is not an error,
		 * it is an empty world. Distinguished from a read failure, which
		 * must NOT be reported as "no kingdoms". */
		logit(LOG_KINGDOM, "kingdom_db_load_all: no realm authority yet; starting empty");
		return true;
	}
	if (loaded != flat_result::ok)
		return false;

	try
	{
		for (const auto &record : catalog.records)
			kingdom_realms[record.assoc_id] = record;
	}
	catch (const std::bad_alloc &)
	{
		logit(LOG_KINGDOM, "kingdom_db_load_all: out of memory loading realms");
		kingdom_realms.clear();
		return false;
	}

	logit(LOG_KINGDOM, "kingdom_db_load_all: loaded %zu realms (revision %llu)",
	      catalog.records.size(), static_cast<unsigned long long>(catalog.revision));
	return true;
}

/* Write ONE realm: under the lock, load the catalogue from disk, upsert this
 * record into it and publish the whole file again. A missing file is created;
 * a corrupt or unreadable one is never overwritten. False when the record
 * fails record_is_sane(), there is no state root, the lock, the load, the
 * merge or the publish fails. One full catalogue rewrite per call, which is
 * why a paid realm waits for the sweep's batched kingdom_db_flush_dirty()
 * instead of coming through here.
 *
 * THIS PRIMITIVE DOES NOT TEST payment_pending, deliberately: it is what
 * kingdom_persist_payment() calls to publish a pending record once the guild
 * debit that justifies it has been written, so a guard here would deadlock
 * that pairing. The obligation is therefore the CALLER's -- every call to this
 * function from outside kingdom_db.c and kingdom_persist_payment() must be
 * guarded by !payment_pending and leave a pending record dirty for
 * kingdom_upkeep_retry_pending() to carry. */
bool kingdom_db_save_realm(const kingdom_realm &realm)
{
	if (!record_is_sane(realm))
	{
		logit(LOG_KINGDOM,
		      "kingdom_db_save_realm: refusing to persist an invalid record for "
		      "association %d (claim %d, arrears %d)",
		      realm.assoc_id, realm.highest_claim, realm.arrears);
		return false;
	}

	std::string root;
	if (!flat_root(&root))
	{
		logit(LOG_KINGDOM, "kingdom_db_save_realm: no flat-file state root for %d",
		      realm.assoc_id);
		return false;
	}

	int lock_fd = -1;
	if (!acquire_lock(root, &lock_fd))
		return false;

	kingdom_catalog catalog;
	const flat_result loaded = load_catalog(root, &catalog);
	/* A missing file is fine -- this write creates it. A corrupt or
	 * unreadable one is not: rewriting it from a single realm would erase
	 * every other realm on disk. */
	if (loaded != flat_result::ok && loaded != flat_result::not_found)
	{
		flatfile_lock_release(lock_fd);
		return false;
	}

	if (!upsert_record(&catalog.records, realm))
	{
		flatfile_lock_release(lock_fd);
		logit(LOG_KINGDOM, "kingdom_db_save_realm: out of memory for association %d",
		      realm.assoc_id);
		return false;
	}

	const bool published = publish_catalog(root, &catalog);
	flatfile_lock_release(lock_fd);
	if (!published)
		logit(LOG_KINGDOM, "kingdom_db_save_realm: publish failed for association %d",
		      realm.assoc_id);
	return published;
}

/* NOTHING TO DO, and that is the honest answer rather than a stub.
 *
 * The flat-file record IS the whole realm: encode_catalog() writes the roster
 * inline with the rest of the fields, so kingdom_db_save_realm() above has
 * already published it by the time anything could call this. It exists only
 * because the SQL half genuinely needs a separate statement against a separate
 * table, and one declaration must serve both builds. Answering true is
 * therefore correct, not optimistic: the roster is on disk. */
bool kingdom_db_save_roster(const kingdom_realm & /*realm*/)
{
	return true;
}

/* Remove one realm's record from the authority file under the lock and
 * publish it again. True when the file or the record is absent (idempotent);
 * false for a non-positive id, no state root, or a failed lock, load or
 * publish. */
bool kingdom_db_delete_realm(int assoc_id)
{
	if (assoc_id <= 0)
	{
		logit(LOG_KINGDOM, "kingdom_db_delete_realm: refusing association id %d", assoc_id);
		return false;
	}

	std::string root;
	if (!flat_root(&root))
	{
		logit(LOG_KINGDOM, "kingdom_db_delete_realm: no flat-file state root for %d",
		      assoc_id);
		return false;
	}

	int lock_fd = -1;
	if (!acquire_lock(root, &lock_fd))
		return false;

	kingdom_catalog catalog;
	const flat_result loaded = load_catalog(root, &catalog);
	if (loaded == flat_result::not_found)
	{
		/* Nothing stored, so nothing to remove. Idempotent, matching the
		 * SQL half's DELETE-matched-nothing. */
		flatfile_lock_release(lock_fd);
		return true;
	}
	if (loaded != flat_result::ok)
	{
		flatfile_lock_release(lock_fd);
		return false;
	}

	const auto position = std::lower_bound(catalog.records.begin(), catalog.records.end(),
					       assoc_id,
					       [](const kingdom_realm &candidate, int value)
					       { return candidate.assoc_id < value; });
	if (position == catalog.records.end() || position->assoc_id != assoc_id)
	{
		flatfile_lock_release(lock_fd);
		return true;
	}
	catalog.records.erase(position);

	const bool published = publish_catalog(root, &catalog);
	flatfile_lock_release(lock_fd);
	if (!published)
		logit(LOG_KINGDOM, "kingdom_db_delete_realm: publish failed for association %d",
		      assoc_id);
	return published;
}

/* Merge every dirty realm into the on-disk catalogue and publish it ONCE,
 * clearing dirty on the records that were written. A realm with
 * payment_pending set is HELD BACK -- not merged, counted and logged -- because
 * its paid mark's guild debit is not yet durable; kingdom_persist_payment()
 * owns that pair. A record failing record_is_sane() is skipped and stays
 * dirty. Nothing is written when no record qualifies; a failed publish leaves
 * every flag set for the next flush. Never erases from the map. */
void kingdom_db_flush_dirty(void)
{
	size_t pending = 0;
	size_t held = 0;
	for (const auto &entry : kingdom_realms)
	{
		if (!entry.second.dirty)
			continue;
		if (entry.second.payment_pending)
			held++;
		else
			pending++;
	}
	if (held)
		logit(LOG_KINGDOM,
		      "kingdom_db_flush_dirty: %zu realm(s) held back (payment pending), "
		      "%zu to write",
		      held, pending);
	if (!pending)
		return;

	std::string root;
	if (!flat_root(&root))
	{
		logit(LOG_KINGDOM,
		      "kingdom_db_flush_dirty: no flat-file state root; %zu realms still pending",
		      pending);
		return;
	}

	int lock_fd = -1;
	if (!acquire_lock(root, &lock_fd))
		return;

	kingdom_catalog catalog;
	const flat_result loaded = load_catalog(root, &catalog);
	if (loaded != flat_result::ok && loaded != flat_result::not_found)
	{
		flatfile_lock_release(lock_fd);
		return;
	}

	/* MERGE into what is on disk, never republish the in-memory map wholesale.
	 * If kingdom_db_load_all() failed earlier this boot the map is empty or
	 * short, and a wholesale rewrite would delete every realm it never saw.
	 * A record leaves this file through kingdom_db_delete_realm() and nowhere
	 * else. */
	std::vector<int> merged;
	bool encodable = true;
	for (auto &entry : kingdom_realms)
	{
		if (!entry.second.dirty)
			continue;
		if (entry.second.payment_pending)
			continue; /* held back: counted and logged above */
		if (!record_is_sane(entry.second))
		{
			logit(LOG_KINGDOM,
			      "kingdom_db_flush_dirty: skipping an invalid record for "
			      "association %d",
			      entry.second.assoc_id);
			continue; /* stays dirty; it must be repaired, not written */
		}
		if (!upsert_record(&catalog.records, entry.second))
		{
			encodable = false;
			break;
		}
		try
		{
			merged.push_back(entry.second.assoc_id);
		}
		catch (const std::bad_alloc &)
		{
			encodable = false;
			break;
		}
	}

	if (!encodable || merged.empty())
	{
		flatfile_lock_release(lock_fd);
		if (!encodable)
			logit(LOG_KINGDOM,
			      "kingdom_db_flush_dirty: out of memory; %zu realms still pending",
			      pending);
		return;
	}

	const bool published = publish_catalog(root, &catalog);
	flatfile_lock_release(lock_fd);

	if (!published)
	{
		/* Every flag stays set so the next cycle retries the whole batch. */
		logit(LOG_KINGDOM, "kingdom_db_flush_dirty: publish failed; %zu realms pending",
		      pending);
		return;
	}

	for (const int assoc_id : merged)
	{
		auto entry = kingdom_realms.find(assoc_id);
		if (entry != kingdom_realms.end())
			entry->second.dirty = false;
	}
	if (merged.size() != pending)
		logit(LOG_KINGDOM, "kingdom_db_flush_dirty: saved %zu realms, %zu still pending",
		      merged.size(), pending - merged.size());
}

#endif /* __NO_MYSQL__ */
