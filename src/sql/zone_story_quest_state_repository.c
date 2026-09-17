#include "sql/zone_story_quest_state_repository.h"

#include "sql/sql.h"

#include <cerrno>
#include <cstdlib>
#include <limits>

#ifndef __NO_MYSQL__
#include "sql/sql_player.h"

#include <cstdlib>
#include <mysql.h>
#endif

sql_zone_story_quest_state_result
sql_zone_story_quest_state_load(uint32_t expected_catalog_revision, std::string *state,
				std::string *error)
{
	if (!expected_catalog_revision || !state)
	{
		if (error)
			*error = "zone-story SQL state output is null";
		return sql_zone_story_quest_state_result::invalid;
	}
#ifdef __NO_MYSQL__
	if (error)
		*error = "SQL state repository is unavailable in a flat-file build";
	return sql_zone_story_quest_state_result::io_error;
#else
	MYSQL_RES *result = db_query("SELECT state_version,catalog_revision,state_blob "
				     "FROM zone_story_quest_state WHERE state_id=1 LIMIT 1");
	if (!result)
	{
		if (error)
			*error = "zone-story SQL state query failed";
		return sql_zone_story_quest_state_result::io_error;
	}
	MYSQL_ROW row = mysql_fetch_row(result);
	unsigned long *lengths = mysql_fetch_lengths(result);
	if (!row)
	{
		mysql_free_result(result);
		return sql_zone_story_quest_state_result::not_found;
	}
	if (!row[0] || !row[1] || !row[2] || !lengths)
	{
		mysql_free_result(result);
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
		mysql_free_result(result);
		if (error)
			*error = "zone-story SQL state schema or catalog revision is invalid";
		return sql_zone_story_quest_state_result::invalid;
	}
	state->assign(row[2], lengths[2]);
	mysql_free_result(result);
	return sql_zone_story_quest_state_result::ok;
#endif
}

sql_zone_story_quest_state_result sql_zone_story_quest_state_save(uint32_t catalog_revision,
								  const std::string &state,
								  std::string *error)
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
	char *escaped = sql_escape_string(state.c_str());
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
		catalog_revision, escaped);
	free(escaped);
	if (!saved)
	{
		if (error)
			*error = "zone-story SQL state write failed";
		return sql_zone_story_quest_state_result::io_error;
	}
	return sql_zone_story_quest_state_result::ok;
#endif
}
