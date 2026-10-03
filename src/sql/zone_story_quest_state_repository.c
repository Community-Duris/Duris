#include "sql/zone_story_quest_state_repository.h"

#include "sql/sql.h"
#include "sql/sql_transaction.h"
#include "world/zone_story_quest_feature.h"

#include <cerrno>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>

#ifndef __NO_MYSQL__
#include <cstdlib>
#include <mysql.h>
#endif

namespace
{
sql_zone_story_quest_state_result load_state(uint32_t expected_catalog_revision, std::string *state,
					     bool locked, std::string *error)
{
	if (!expected_catalog_revision || !state)
	{
		if (error)
			*error = "zone-story SQL state output is null";
		return sql_zone_story_quest_state_result::invalid;
	}
#ifdef __NO_MYSQL__
	(void)locked;
	if (error)
		*error = "SQL state repository is unavailable in a flat-file build";
	return sql_zone_story_quest_state_result::io_error;
#else
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
		db_query(
			locked ?
				"SELECT state_version,catalog_revision,state_blob FROM zone_story_quest_state WHERE state_id=1 LIMIT 1 FOR UPDATE" :
				"SELECT state_version,catalog_revision,state_blob FROM zone_story_quest_state WHERE state_id=1 LIMIT 1"),
		&mysql_free_result);
	if (!result)
	{
		if (error)
			*error = "zone-story SQL state query failed";
		return sql_zone_story_quest_state_result::io_error;
	}
	MYSQL_ROW row = mysql_fetch_row(result.get());
	unsigned long *lengths = mysql_fetch_lengths(result.get());
	if (!row)
	{
		return sql_zone_story_quest_state_result::not_found;
	}
	if (!row[0] || !row[1] || !row[2] || !lengths)
	{
		if (error)
			*error = "zone-story SQL state row is null";
		return sql_zone_story_quest_state_result::invalid;
	}
	char *version_end = nullptr;
	char *revision_end = nullptr;
	errno = 0;
	const unsigned long version = std::strtoul(row[0], &version_end, 10);
	const int version_errno = errno;
	errno = 0;
	const unsigned long revision = std::strtoul(row[1], &revision_end, 10);
	const int revision_errno = errno;
	if (version_errno || revision_errno || version_end == row[0] || revision_end == row[1] ||
	    *version_end || *revision_end || version != 1 ||
	    revision != expected_catalog_revision ||
	    revision > std::numeric_limits<uint32_t>::max())
	{
		if (error)
			*error = "zone-story SQL state schema or catalog revision is invalid";
		return sql_zone_story_quest_state_result::invalid;
	}
	state->assign(row[2], lengths[2]);
	return sql_zone_story_quest_state_result::ok;
#endif
}

} // namespace

sql_zone_story_quest_state_result
sql_zone_story_quest_state_load(uint32_t expected_catalog_revision, std::string *state,
				std::string *error)
try
{
	return load_state(expected_catalog_revision, state, false, error);
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
	/* MEDIUMTEXT is capped at 16 MiB; reject larger blobs before issuing SQL. */
	if (!catalog_revision || state.size() > 16U * 1024U * 1024U)
	{
		if (error)
			*error = "zone-story SQL state is oversized";
		return sql_zone_story_quest_state_result::invalid;
	}
#ifdef __NO_MYSQL__
	(void)catalog_revision;
	(void)state;
	if (error)
		*error = "SQL state repository is unavailable in a flat-file build";
	return sql_zone_story_quest_state_result::io_error;
#else
	std::unique_ptr<char, decltype(&std::free)> escaped(sql_escape_string(state.c_str()),
							    &std::free);
	if (!escaped)
	{
		if (error)
			*error = "zone-story SQL state could not be escaped";
		return sql_zone_story_quest_state_result::io_error;
	}
	const bool saved = qry(
		"INSERT INTO zone_story_quest_state (state_id,state_version,catalog_revision,state_blob,updated_at) "
		"VALUES (1,1,%u,'%s',UTC_TIMESTAMP(6)) "
		"ON DUPLICATE KEY UPDATE state_version=VALUES(state_version), "
		"catalog_revision=VALUES(catalog_revision),state_blob=VALUES(state_blob),updated_at=VALUES(updated_at)",
		catalog_revision, escaped.get());
	if (!saved)
	{
		if (error)
			*error = "zone-story SQL state write failed";
		return sql_zone_story_quest_state_result::io_error;
	}
	return sql_zone_story_quest_state_result::ok;
#endif
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
	// Borrow the account erasure transaction; never open or resolve one here.
	if (!DB || !sql_in_transaction())
		return sql_zone_story_quest_state_result::invalid;
	std::string before;
	const auto loaded = load_state(expected_catalog_revision, &before, true, error);
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
	if (after == before)
		return sql_zone_story_quest_state_result::ok;
	return sql_zone_story_quest_state_save(expected_catalog_revision, after, error);
#endif
}
catch (const std::bad_alloc &)
{
	errno = ENOMEM;
	return sql_zone_story_quest_state_result::io_error;
}
