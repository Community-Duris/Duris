#include "item/artifact_mana_store.h"

#include "flatfile/flatfile_store.h"
#ifndef __NO_MYSQL__
#include "sql/sql_pool.h"
#include "sql/sql_thread_init.h"
#endif

#include <array>
#include <charconv>
#include <cstring>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <vector>

namespace
{
constexpr std::array<uint8_t, 8> magic = { 'D', 'U', 'R', 'M', 'A', 'N', 'A', 1 };
constexpr size_t payload_size = 8 + 8 * 8;
constexpr size_t file_size = payload_size + SHA256_DIGEST_LENGTH;

std::array<uint64_t, 8> fields(const artifact_mana_record &r)
{
	return { r.uid,	     r.profile_id,   r.profile_revision, r.version,
		 r.capacity, r.regeneration, r.reserve,		 r.settled_at };
}

artifact_mana_record from_fields(const std::array<uint64_t, 8> &f)
{
	return { f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7] };
}

std::string filename(uint64_t uid)
{
	return "artifact-mana-" + std::to_string(uid);
}

std::vector<uint8_t> encode(const artifact_mana_record &record)
{
	std::vector<uint8_t> bytes(magic.begin(), magic.end());
	for (uint64_t value : fields(record))
		for (unsigned index = 0; index < 8; ++index)
			bytes.push_back(static_cast<uint8_t>(value >> (index * 8)));
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data(), bytes.size(), digest.data());
	bytes.insert(bytes.end(), digest.begin(), digest.end());
	return bytes;
}

artifact_mana_read read_flat(const std::string &root, uint64_t uid, artifact_mana_record &record)
{
	std::vector<uint8_t> bytes;
	const auto result =
		flatfile_read(root + "/domains", filename(uid), file_size, &bytes, nullptr);
	if (result == flatfile_read_result::not_found)
		return artifact_mana_read::missing;
	if (result != flatfile_read_result::ok || bytes.size() != file_size ||
	    memcmp(bytes.data(), magic.data(), magic.size()))
		return artifact_mana_read::error;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data(), payload_size, digest.data());
	if (CRYPTO_memcmp(digest.data(), bytes.data() + payload_size, digest.size()))
		return artifact_mana_read::error;
	std::array<uint64_t, 8> values = {};
	for (size_t field = 0; field < values.size(); ++field)
		for (unsigned index = 0; index < 8; ++index)
			values[field] |= uint64_t(bytes[8 + field * 8 + index]) << (index * 8);
	record = from_fields(values);
	return artifact_mana_valid(record) && record.uid == uid ? artifact_mana_read::found :
								  artifact_mana_read::error;
}

#ifndef __NO_MYSQL__
struct mysql_thread_scope
{
	const bool ready = sql_worker_thread_init() == 0;
	~mysql_thread_scope()
	{
		if (ready)
			mysql_thread_end();
	}
};

constexpr const char *columns =
	"item_uid,profile_id,profile_revision,version,capacity,regeneration,reserve,settled_at";

artifact_mana_read read_sql(MYSQL *connection, uint64_t uid, artifact_mana_record &record)
{
	const auto query = std::string("SELECT ") + columns +
			   " FROM artifact_mana WHERE item_uid=" + std::to_string(uid);
	if (mysql_real_query(connection, query.data(), query.size()))
		return artifact_mana_read::error;
	MYSQL_RES *result = mysql_store_result(connection);
	if (!result)
		return artifact_mana_read::error;
	if (!mysql_num_rows(result))
	{
		mysql_free_result(result);
		return artifact_mana_read::missing;
	}
	MYSQL_ROW row = mysql_fetch_row(result);
	std::array<uint64_t, 8> values = {};
	bool valid = row && mysql_num_rows(result) == 1 &&
		     mysql_num_fields(result) == values.size();
	for (size_t i = 0; valid && i < values.size(); ++i)
	{
		if (!row[i])
		{
			valid = false;
			break;
		}
		const char *end = row[i] + strlen(row[i]);
		const auto parsed = std::from_chars(row[i], end, values[i]);
		valid = parsed.ec == std::errc() && parsed.ptr == end;
	}
	mysql_free_result(result);
	record = from_fields(values);
	return valid && artifact_mana_valid(record) && record.uid == uid ?
		       artifact_mana_read::found :
		       artifact_mana_read::error;
}

bool write_sql(MYSQL *connection, uint64_t expected, const artifact_mana_record &record)
{
	std::string query;
	if (!expected)
	{
		query = std::string("INSERT IGNORE INTO artifact_mana (") + columns + ") VALUES (";
		for (uint64_t value : fields(record))
			query += std::to_string(value) + ",";
		query.back() = ')';
	}
	else
	{
		query = "UPDATE artifact_mana SET profile_id=" + std::to_string(record.profile_id) +
			",profile_revision=" + std::to_string(record.profile_revision) +
			",version=" + std::to_string(record.version) +
			",capacity=" + std::to_string(record.capacity) +
			",regeneration=" + std::to_string(record.regeneration) +
			",reserve=" + std::to_string(record.reserve) +
			",settled_at=" + std::to_string(record.settled_at) +
			" WHERE item_uid=" + std::to_string(record.uid) +
			" AND version=" + std::to_string(expected);
	}
	if (mysql_real_query(connection, query.data(), query.size()))
		return false;
	artifact_mana_record stored;
	return read_sql(connection, record.uid, stored) == artifact_mana_read::found &&
	       stored == record;
}
#endif
} // namespace

artifact_mana_read artifact_mana_store_read(bool sql, const std::string &root, uint64_t uid,
					    artifact_mana_record &record)
{
	if (!uid)
		return artifact_mana_read::error;
	if (!sql)
		return read_flat(root, uid, record);
#ifndef __NO_MYSQL__
	mysql_thread_scope thread;
	if (!thread.ready)
		return artifact_mana_read::error;
	MYSQL *connection = sql_pool_acquire();
	if (!connection)
		return artifact_mana_read::error;
	const auto result = read_sql(connection, uid, record);
	sql_pool_release(connection);
	return result;
#else
	return artifact_mana_read::error;
#endif
}

bool artifact_mana_store_write(bool sql, const std::string &root, uint64_t expected,
			       const artifact_mana_record &record)
{
	if (!artifact_mana_valid(record) || record.version <= expected ||
	    (!expected && (record.version != 1 || record.reserve)))
		return false;
	if (!sql)
	{
		int lock = -1;
		const auto directory = root + "/domains";
		if (!flatfile_lock_acquire(directory, ".artifact-mana.lock", &lock, nullptr))
			return false;
		artifact_mana_record stored;
		const auto result = read_flat(root, record.uid, stored);
		const bool identical = result == artifact_mana_read::found && stored == record;
		const bool allowed = (expected && result == artifact_mana_read::found &&
				      stored.version == expected) ||
				     (!expected && result == artifact_mana_read::missing);
		const bool written =
			identical ||
			(allowed && flatfile_atomic_write(directory, filename(record.uid),
							  encode(record), nullptr));
		flatfile_lock_release(lock);
		return written;
	}
#ifndef __NO_MYSQL__
	mysql_thread_scope thread;
	if (!thread.ready)
		return false;
	MYSQL *connection = sql_pool_acquire();
	if (!connection)
		return false;
	const bool result = write_sql(connection, expected, record);
	sql_pool_release(connection);
	return result;
#else
	return false;
#endif
}
