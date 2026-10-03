#include "sql/zone_story_quest_state_repository.h"
#include "sql/sql.h"
#include "sql/sql_transaction.h"
#include "world/zone_story_quest_feature.h"
#include <array>
#include <charconv>
#include <cerrno>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>
#include <set>
#include <zlib.h>

namespace
{
#ifndef __NO_MYSQL__
using zone_story_quest_state::records;
std::array<records, 256> buckets;
std::array<size_t, 256> stored_bytes{};
bool loaded_buckets = false;

std::string packed_bucket(const records &values)
{
	const std::string payload = zone_story_quest_state::encode(values);
	if (payload.size() < 1024)
		return payload;
	if (payload.size() > 64U * 1024U * 1024U)
		return {};
	uLongf size = compressBound(payload.size());
	std::string compressed(size, '\0');
	if (compress2(reinterpret_cast<Bytef *>(compressed.data()), &size,
		      reinterpret_cast<const Bytef *>(payload.data()), payload.size(),
		      Z_BEST_SPEED) != Z_OK)
		return {};
	compressed.resize(size);
	return "ZSQZ|1|" + std::to_string(payload.size()) + "\n" +
	       zone_story_quest_state::hex(compressed) + "\n";
}

bool unpack_bucket(std::string_view payload, records *values)
{
	if (!payload.starts_with("ZSQZ|1|"))
		return zone_story_quest_state::decode_journal(payload, values);
	const auto end = payload.find('\n');
	if (end == std::string_view::npos || end <= 7 || !payload.ends_with('\n'))
		return false;
	size_t decoded_size = 0;
	const auto number = payload.substr(7, end - 7);
	const auto parsed =
		std::from_chars(number.data(), number.data() + number.size(), decoded_size);
	if (parsed.ec != std::errc{} || parsed.ptr != number.data() + number.size() ||
	    decoded_size < 7 || decoded_size > 64U * 1024U * 1024U)
		return false;
	std::string compressed;
	if (!zone_story_quest_state::unhex(payload.substr(end + 1, payload.size() - end - 2),
					   &compressed))
		return false;
	std::string decoded(decoded_size, '\0');
	uLongf actual = decoded_size;
	uLong compressed_size = compressed.size();
	if (uncompress2(reinterpret_cast<Bytef *>(decoded.data()), &actual,
			reinterpret_cast<const Bytef *>(compressed.data()),
			&compressed_size) != Z_OK ||
	    actual != decoded_size || compressed_size != compressed.size())
		return false;
	return zone_story_quest_state::decode(decoded, values);
}

bool execute(const std::string &query)
{
	// Dynamic SQL bypasses qry's small stack buffer. All writes below use this
	// same, guarded main-thread connection and one InnoDB transaction.
	return sql_trace_exec("zone-story/state", query.data(), query.size(), true, true);
}
bool number(const char *text, unsigned long *value)
{
	if (!text || !*text)
		return false;
	char *end = nullptr;
	errno = 0;
	*value = std::strtoul(text, &end, 10);
	return !errno && end != text && !*end;
}
#endif
sql_zone_story_quest_state_result invalid(std::string *error, const char *message)
{
	if (error)
		*error = message;
	return sql_zone_story_quest_state_result::invalid;
}
#ifndef __NO_MYSQL__
sql_zone_story_quest_state_result make_statement(uint32_t revision, unsigned id,
						 const records &values, std::string *statement,
						 size_t *stored_size, std::string *error)
{
	const std::string payload = packed_bucket(values);
	if (payload.empty() || payload.size() >= 16U * 1024U * 1024U)
	{
		return invalid(error, "zone-story SQL bucket exceeds encoding capacity");
	}
	std::unique_ptr<char, decltype(&std::free)> escaped(sql_escape_string(payload.c_str()),
							    &std::free);
	if (!escaped)
	{
		if (error)
			*error = "zone-story SQL bucket could not be escaped";
		return sql_zone_story_quest_state_result::io_error;
	}
	*statement =
		"INSERT INTO zone_story_quest_state "
		"(state_id,state_version,catalog_revision,state_blob,updated_at) VALUES (" +
		std::to_string(id) + ",2," + std::to_string(revision) + ",'" + escaped.get() +
		"',UTC_TIMESTAMP(6)) ON DUPLICATE KEY UPDATE state_version=VALUES(state_version),"
		"catalog_revision=VALUES(catalog_revision),state_blob=VALUES(state_blob),updated_at=VALUES(updated_at)";
	*stored_size = payload.size();
	return sql_zone_story_quest_state_result::ok;
}
#endif
}

static sql_zone_story_quest_state_result load_state(uint32_t expected_catalog_revision,
						    std::string *state, std::string *error,
						    bool *legacy, bool locked)
{
	if (!expected_catalog_revision || !state)
		return invalid(error, "invalid zone-story SQL state request");
#ifdef __NO_MYSQL__
	(void)legacy;
	(void)locked;
	if (error)
		*error = "SQL state repository is unavailable in a flat-file build";
	return sql_zone_story_quest_state_result::io_error;
#else
	if (!locked)
		loaded_buckets = false;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
		db_query(
			locked ?
				"SELECT state_id,state_version,catalog_revision,state_blob FROM zone_story_quest_state ORDER BY state_id FOR UPDATE" :
				"SELECT state_id,state_version,catalog_revision,state_blob FROM zone_story_quest_state ORDER BY state_id"),
		&mysql_free_result);
	if (!result)
	{
		if (error)
			*error = "zone-story SQL state query failed";
		return sql_zone_story_quest_state_result::io_error;
	}
	std::array<records, 256> candidate;
	std::array<size_t, 256> candidate_bytes{};
	bool aggregate = false;
	size_t count = 0;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result.get())))
	{
		const unsigned long *lengths = mysql_fetch_lengths(result.get());
		unsigned long id = 0, version = 0, revision = 0;
		if (!lengths || !row[3] || !number(row[0], &id) || !number(row[1], &version) ||
		    !number(row[2], &revision) || id < 1 || id > 255 ||
		    (version != 1 && version != 2) ||
		    (revision != expected_catalog_revision &&
		     !(revision == 1 && expected_catalog_revision == 2)))
		{
			return invalid(
				error,
				"zone-story SQL state schema or catalog revision is invalid");
		}
		++count;
		const std::string payload(row[3], lengths[3]);
		if (id == 1 && payload.starts_with("ZSQF|"))
		{
			aggregate = true;
			for (auto &[key, value] : zone_story_quest_state::split_document(payload))
				candidate[zone_story_quest_state::bucket(key)].emplace(
					std::move(key), std::move(value));
		}
		else
		{
			records values;
			if (version != 2 || !unpack_bucket(payload, &values))
			{
				return invalid(error, "zone-story SQL bucket is corrupt");
			}
			for (const auto &[key, value] : values)
				if (zone_story_quest_state::bucket(key) != id || value.empty())
				{
					return invalid(
						error,
						"zone-story SQL record is in the wrong bucket");
				}
			candidate[id] = std::move(values);
			candidate_bytes[id] = payload.size();
		}
	}
	if (aggregate && count != 1)
		return invalid(error, "mixed legacy and record zone-story SQL state");
	if (count && !candidate[1].count("meta"))
		return invalid(error, "zone-story SQL metadata is missing");
	if (legacy)
		*legacy = aggregate;
	if (!count)
	{
		if (!locked)
		{
			buckets = {};
			stored_bytes = {};
			loaded_buckets = true;
		}
		return sql_zone_story_quest_state_result::not_found;
	}
	records values;
	for (const auto &bucket : candidate)
		values.insert(bucket.begin(), bucket.end());
	*state = zone_story_quest_state::document(values);
	if (!locked)
	{
		buckets = std::move(candidate);
		stored_bytes = candidate_bytes;
		loaded_buckets = true;
	}
	return sql_zone_story_quest_state_result::ok;
#endif
}

sql_zone_story_quest_state_result
sql_zone_story_quest_state_load(uint32_t expected_catalog_revision, std::string *state,
				std::string *error, bool *legacy)
try
{
	return load_state(expected_catalog_revision, state, error, legacy, false);
}
catch (const std::bad_alloc &)
{
	errno = ENOMEM;
	return sql_zone_story_quest_state_result::io_error;
}

sql_zone_story_quest_state_result
sql_zone_story_quest_records_save(uint32_t catalog_revision,
				  const zone_story_quest_state::changes &updates,
				  std::string *error)
try
{
	if (!catalog_revision)
		return invalid(error, "zone-story SQL catalog revision is invalid");
#ifdef __NO_MYSQL__
	(void)updates;
	if (error)
		*error = "SQL state repository is unavailable in a flat-file build";
	return sql_zone_story_quest_state_result::io_error;
#else
	if (!DB || sql_in_transaction())
		return invalid(error, "zone-story SQL writes require their own transaction");
	if (!loaded_buckets && !updates.replace)
		return invalid(error, "zone-story SQL state has not been loaded");
	std::map<unsigned, records> changed;
	if (updates.replace)
	{
		for (unsigned id = 1; id <= 255; ++id)
			if (!buckets[id].empty() || stored_bytes[id] > 7)
				changed[id] = {};
		changed[1] = {};
	}
	for (const auto &[key, value] : updates.values)
	{
		const unsigned id = zone_story_quest_state::bucket(key);
		const auto previous = buckets[id].find(key);
		if (!updates.replace &&
		    ((value.empty() && previous == buckets[id].end()) ||
		     (previous != buckets[id].end() && previous->second == value)))
			continue;
		if (!changed.count(id))
			changed[id] = updates.replace ? records() : buckets[id];
		if (value.empty())
			changed[id].erase(key);
		else
			changed[id][key] = value;
	}
	const auto metadata = changed.find(1);
	if ((metadata != changed.end() && !metadata->second.count("meta")) ||
	    (updates.replace && (metadata == changed.end() || !metadata->second.count("meta"))) ||
	    (!updates.replace && !buckets[1].count("meta") &&
	     (metadata == changed.end() || !metadata->second.count("meta"))))
		return invalid(error, "zone-story SQL metadata is missing");
	if (changed.empty())
		return sql_zone_story_quest_state_result::ok;
	std::vector<std::string> statements;
	std::map<unsigned, size_t> next_stored;
	for (const auto &[id, values] : changed)
	{
		// Fast compressed snapshots keep changed bucket writes small as receipts
		// accumulate. Replacing a snapshot also physically removes erased data.
		std::string statement;
		size_t bytes = 0;
		const auto prepared =
			make_statement(catalog_revision, id, values, &statement, &bytes, error);
		if (prepared != sql_zone_story_quest_state_result::ok)
			return prepared;
		statements.push_back(std::move(statement));
		next_stored[id] = bytes;
	}
	struct transaction_scope
	{
		bool active = false;
		~transaction_scope()
		{
			if (active)
				sql_rollback();
		}
	} transaction;
	bool saved = transaction.active = sql_begin_transaction();
	for (const auto &statement : statements)
	{
		if (!saved)
			break;
		saved = execute(statement);
	}
	if (saved)
		saved = sql_commit();
	if (saved)
		transaction.active = false;
	if (!saved)
	{
		if (error)
			*error = "zone-story SQL state transaction failed";
		return sql_zone_story_quest_state_result::io_error;
	}
	for (auto &[id, values] : changed)
	{
		buckets[id] = std::move(values);
		stored_bytes[id] = next_stored.at(id);
	}
	loaded_buckets = true;
	return sql_zone_story_quest_state_result::ok;
#endif
}
catch (const std::bad_alloc &)
{
	errno = ENOMEM;
	return sql_zone_story_quest_state_result::io_error;
}

sql_zone_story_quest_state_result sql_zone_story_quest_state_save(uint32_t catalog_revision,
								  const std::string &state,
								  std::string *error)
try
{
	return sql_zone_story_quest_records_save(
		catalog_revision, { zone_story_quest_state::split_document(state), true }, error);
}
catch (const std::bad_alloc &)
{
	errno = ENOMEM;
	return sql_zone_story_quest_state_result::io_error;
}

sql_zone_story_quest_state_result sql_zone_story_quest_state_remove_player_aliases(
	uint32_t expected_catalog_revision, uint32_t current_season_id,
	const std::vector<uint32_t> &pids, const zone_story_quest_catalog::catalog &catalog,
	std::string *error)
try
{
	if (!expected_catalog_revision || !current_season_id || pids.empty() || pids.size() > 1024)
		return sql_zone_story_quest_state_result::invalid;
	for (uint32_t pid : pids)
		if (!pid)
			return sql_zone_story_quest_state_result::invalid;
#ifdef __NO_MYSQL__
	(void)catalog;
	(void)error;
	return sql_zone_story_quest_state_result::io_error;
#else
	if (!DB || !sql_in_transaction())
		return sql_zone_story_quest_state_result::invalid;
	std::string before;
	bool legacy = false;
	const auto loaded = load_state(expected_catalog_revision, &before, error, &legacy, true);
	if (loaded != sql_zone_story_quest_state_result::ok)
		return loaded;
	zone_story_quest_feature::service state;
	if (catalog.content_revision != expected_catalog_revision ||
	    !state.set_catalog(catalog, error) || !state.deserialize_state(before, error))
		return sql_zone_story_quest_state_result::invalid;
	for (uint32_t pid : pids)
		if (!state.erase_character_all_seasons(pid, current_season_id))
			return sql_zone_story_quest_state_result::invalid;
	const std::string after = state.serialize_state(error);
	if (after.empty())
		return sql_zone_story_quest_state_result::io_error;
	const auto previous = zone_story_quest_state::split_document(before);
	const auto next = zone_story_quest_state::split_document(after);
	if (previous == next)
		return sql_zone_story_quest_state_result::ok;
	std::array<records, 256> old_buckets, new_buckets;
	for (const auto &[key, value] : previous)
		old_buckets[zone_story_quest_state::bucket(key)].emplace(key, value);
	for (const auto &[key, value] : next)
		new_buckets[zone_story_quest_state::bucket(key)].emplace(key, value);
	std::vector<std::string> statements;
	for (unsigned id = 1; id <= 255; ++id)
	{
		if (old_buckets[id] == new_buckets[id] && (!legacy || new_buckets[id].empty()))
			continue;
		std::string statement;
		size_t bytes = 0;
		const auto prepared = make_statement(expected_catalog_revision, id, new_buckets[id],
						     &statement, &bytes, error);
		if (prepared != sql_zone_story_quest_state_result::ok)
			return prepared;
		statements.push_back(std::move(statement));
	}
	// The caller owns commit/rollback. Invalidate caches until authority is reloaded.
	loaded_buckets = false;
	for (const auto &statement : statements)
		if (!execute(statement))
			return sql_zone_story_quest_state_result::io_error;
	return sql_zone_story_quest_state_result::ok;
#endif
}
catch (const std::bad_alloc &)
{
	errno = ENOMEM;
	return sql_zone_story_quest_state_result::io_error;
}
