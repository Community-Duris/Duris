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
 * of that table; arrears indexes the ladder. A realm that fails this is
 * dropped with a log, which loses one guild's territory rather than the
 * process. */
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

namespace
{
/* ONE list, used by both the SELECT and the INSERT, so the two can never
 * drift apart and the row[] indices below always mean what they say. */
const char *const kingdom_realm_columns = "assoc_id,realm_id,hall_vnum,highest_claim,"
					  "res_mineral,res_wood,res_fibre,res_water,"
					  "upkeep_paid_through,arrears,missed_cycles";
constexpr unsigned int kingdom_realm_column_count = 11;
} /* namespace */

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

			bool row_is_whole = true;
			for (unsigned int column = 0; column < kingdom_realm_column_count; column++)
				if (!row[column])
					row_is_whole = false;
			if (!row_is_whole)
			{
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
			/* Anchor left unresolved on purpose -- see the file banner. */
			realm.dirty = false; /* just read: by definition clean */

			if (!record_is_sane(realm))
			{
				logit(LOG_KINGDOM,
				      "kingdom_db_load_all: rejecting corrupt row for "
				      "association %d (claim %d, arrears %d)",
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

	/* A truncated load is a failed load: the caller must not treat the
	 * squares of the realms it never saw as unowned. */
	return complete;
}

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
	return true;
}

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
	return true;
}

void kingdom_db_flush_dirty(void)
{
	size_t saved = 0;
	size_t failed = 0;

	for (auto &entry : kingdom_realms)
	{
		if (!entry.second.dirty)
			continue;
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

	if (failed)
		logit(LOG_KINGDOM, "kingdom_db_flush_dirty: saved %zu realms, %zu still pending",
		      saved, failed);
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
constexpr uint32_t kingdom_file_version = 1;
/* 64 payload bytes per realm at the cap is 4 MiB; 8 MiB leaves headroom and
 * bounds what a corrupt length field can ask us to read. */
constexpr size_t kingdom_file_maximum_bytes = 8 * 1024 * 1024;
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

bool records_are_sane(const std::vector<kingdom_realm> &records)
{
	if (records.size() > kingdom_realm_maximum)
		return false;
	for (size_t index = 0; index < records.size(); ++index)
	{
		if (!record_is_sane(records[index]))
			return false;
		/* Strictly increasing assoc_id: this both keeps the file canonical
		 * and makes a duplicated realm unrepresentable. */
		if (index && records[index - 1].assoc_id >= records[index].assoc_id)
			return false;
	}
	return true;
}

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
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) || version != kingdom_file_version || !revision ||
	    payload_size != bytes.size() - header_size)
		return false;

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
		/* Anchor stays unresolved and the record is clean: see the banner. */
	}
	if (payload.offset != payload.size || !records_are_sane(decoded.records))
		return false;

	*catalog = std::move(decoded);
	return true;
}

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
 * records_are_sane() holds and the file stays canonical. `dirty` is a runtime
 * flag that is never encoded, so the stored copy carries it cleared. */
bool upsert_record(std::vector<kingdom_realm> *records, const kingdom_realm &realm)
{
	if (!records)
		return false;

	kingdom_realm stored = realm;
	stored.dirty = false;

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

void kingdom_db_flush_dirty(void)
{
	size_t pending = 0;
	for (const auto &entry : kingdom_realms)
		if (entry.second.dirty)
			pending++;
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
