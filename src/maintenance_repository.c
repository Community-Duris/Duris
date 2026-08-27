#include "maintenance_repository.h"

#include "frag_cap_config.h"
#include "persistence_observability.h"
#include "sql_pool.h"

#include <mysql/mysql.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <fcntl.h>
#include <map>
#include <set>
#include <string>
#include <unistd.h>
#include <vector>

namespace
{
struct connection_guard
{
	MYSQL *connection;
	~connection_guard()
	{
		if (connection)
			sql_pool_release(connection);
	}
};

maintenance_result failure(const maintenance_request &request, maintenance_outcome outcome,
			   uint32_t code)
{
	return { request.work_id, request.job_id, outcome, request.cursor, 0, 0, code, 0, {} };
}

bool before_deadline(const maintenance_request &request)
{
	return persistence_observability_now_usec() <= request.deadline_usec;
}

bool parse_positive(const char *text, int64_t *value)
{
	if (!text || !*text || !value)
		return false;
	errno = 0;
	char *end = nullptr;
	const unsigned long long parsed = strtoull(text, &end, 10);
	if (errno || !end || *end || !parsed || parsed > INT64_MAX)
		return false;
	*value = static_cast<int64_t>(parsed);
	return true;
}

bool query_values(MYSQL *connection, const maintenance_request &request, const std::string &sql,
		  maintenance_result *result)
{
	if (!connection || !result || !before_deadline(request) ||
	    mysql_real_query(connection, sql.data(), sql.size()) != 0)
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return false;
	MYSQL_ROW row = nullptr;
	while ((row = mysql_fetch_row(rows)))
	{
		int64_t value = 0;
		if (result->value_count >= request.row_budget ||
		    result->value_count >= result->values.size() || !parse_positive(row[0], &value))
		{
			mysql_free_result(rows);
			return false;
		}
		result->values[result->value_count++] = value;
	}
	mysql_free_result(rows);
	result->rows = static_cast<uint32_t>(result->value_count);
	if (result->value_count)
		result->next_cursor =
			static_cast<uint64_t>(result->values[result->value_count - 1]);
	result->outcome = result->value_count == request.row_budget ? maintenance_outcome::more :
								      maintenance_outcome::complete;
	return before_deadline(request);
}

std::string limit_sql(const maintenance_request &request)
{
	return std::to_string(request.row_budget);
}

bool execute_sql(MYSQL *connection, const std::string &sql)
{
	return connection && mysql_real_query(connection, sql.data(), sql.size()) == 0;
}

bool begin_transaction(MYSQL *connection)
{
	return execute_sql(connection, "START TRANSACTION");
}

maintenance_result sql_failure(MYSQL *connection, const maintenance_request &request)
{
	execute_sql(connection, "ROLLBACK");
	return failure(request, maintenance_outcome::retryable_failure,
		       mysql_errno(connection) ? mysql_errno(connection) : EIO);
}

bool timer_value(MYSQL *connection, const std::string &name, int64_t *value)
{
	if (!value || !execute_sql(connection, "SELECT date FROM timers WHERE name='" + name + "'"))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return false;
	MYSQL_ROW row = mysql_fetch_row(rows);
	*value = row && row[0] ? strtoll(row[0], nullptr, 10) : 0;
	mysql_free_result(rows);
	return true;
}

bool marker_matches_and_lock(MYSQL *connection, const char *prefix, uint64_t work_id,
			     uint64_t cursor, bool *matches)
{
	if (!matches)
		return false;
	const std::string base(prefix);
	const std::string names[4] = { base + "_wh", base + "_wl", base + "_ch", base + "_cl" };
	std::string insert = "INSERT IGNORE INTO timers(name,date) VALUES";
	for (size_t index = 0; index < 4; ++index)
	{
		if (index)
			insert += ',';
		insert += "('" + names[index] + "',0)";
	}
	if (!execute_sql(connection, insert))
		return false;
	std::string select = "SELECT name,date FROM timers WHERE name IN ('" + names[0] + "','" +
			     names[1] + "','" + names[2] + "','" + names[3] + "') FOR UPDATE";
	if (!execute_sql(connection, select))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return false;
	int32_t observed[4] = {};
	bool found[4] = {};
	MYSQL_ROW row = nullptr;
	while ((row = mysql_fetch_row(rows)))
		for (size_t index = 0; index < 4; ++index)
			if (row[0] && row[1] && names[index] == row[0])
			{
				observed[index] = static_cast<int32_t>(strtol(row[1], nullptr, 10));
				found[index] = true;
			}
	mysql_free_result(rows);
	const int32_t expected[4] = { static_cast<int32_t>(work_id >> 32),
				      static_cast<int32_t>(work_id),
				      static_cast<int32_t>(cursor >> 32),
				      static_cast<int32_t>(cursor) };
	*matches = true;
	for (size_t index = 0; index < 4; ++index)
		*matches = *matches && found[index] && observed[index] == expected[index];
	if (*matches)
		return true;
	std::string update = "UPDATE timers SET date=CASE name";
	for (size_t index = 0; index < 4; ++index)
		update += " WHEN '" + names[index] + "' THEN " + std::to_string(expected[index]);
	update += " END WHERE name IN ('" + names[0] + "','" + names[1] + "','" + names[2] + "','" +
		  names[3] + "')";
	return execute_sql(connection, update);
}

maintenance_result execute_epic_balance(MYSQL *connection, const maintenance_request &request)
{
	if (request.value_count != 2 || request.values[0] <= 0 || request.values[1] < 0)
		return failure(request, maintenance_outcome::permanent_failure, EINVAL);
	const std::string query =
		"SELECT id,alignment,COALESCE(UNIX_TIMESTAMP(last_touch),0) FROM zones "
		"WHERE epic_type>0 AND id>" +
		std::to_string(request.cursor) + " ORDER BY id LIMIT " + limit_sql(request);
	if (!begin_transaction(connection) || !execute_sql(connection, query))
		return sql_failure(connection, request);
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return sql_failure(connection, request);
	maintenance_result result = failure(request, maintenance_outcome::complete, 0);
	MYSQL_ROW row = nullptr;
	while ((row = mysql_fetch_row(rows)))
	{
		if (!row[0] || !row[1] || !row[2] || result.rows >= request.row_budget)
		{
			mysql_free_result(rows);
			return sql_failure(connection, request);
		}
		const uint64_t id = strtoull(row[0], nullptr, 10);
		const int alignment = atoi(row[1]);
		const int64_t touched = strtoll(row[2], nullptr, 10);
		if (!id)
		{
			mysql_free_result(rows);
			return sql_failure(connection, request);
		}
		result.next_cursor = id;
		++result.rows;
		std::string update;
		if (!touched)
			update = "UPDATE zones SET last_touch=FROM_UNIXTIME(" +
				 std::to_string(request.values[0]) +
				 ") WHERE id=" + std::to_string(id) + " AND last_touch IS NULL";
		else if (alignment && request.values[0] - touched > request.values[1])
			update = "UPDATE zones SET alignment=alignment" +
				 std::string(alignment > 0 ? "-1" : "+1") +
				 ",last_touch=FROM_UNIXTIME(" + std::to_string(request.values[0]) +
				 ") WHERE id=" + std::to_string(id) +
				 " AND alignment=" + std::to_string(alignment) +
				 " AND UNIX_TIMESTAMP(last_touch)=" + std::to_string(touched);
		if (!before_deadline(request) ||
		    (!update.empty() && !execute_sql(connection, update)))
		{
			mysql_free_result(rows);
			return sql_failure(connection, request);
		}
	}
	mysql_free_result(rows);
	if (!before_deadline(request) || !execute_sql(connection, "COMMIT"))
		return sql_failure(connection, request);
	result.outcome = result.rows == request.row_budget ? maintenance_outcome::more :
							     maintenance_outcome::complete;
	return result;
}

maintenance_result execute_epic_modifiers(MYSQL *connection, const maintenance_request &request)
{
	if (request.value_count != 5 || request.values[0] <= 0 || request.values[1] < 0 ||
	    request.values[3] < request.values[4])
		return failure(request, maintenance_outcome::permanent_failure, EINVAL);
	if (!begin_transaction(connection))
		return sql_failure(connection, request);
	if (!request.cursor)
	{
		int64_t last_run = 0;
		if (!timer_value(connection, "epic_zone_mod", &last_run))
			return sql_failure(connection, request);
		if (request.values[0] <= last_run + request.values[1])
		{
			if (!execute_sql(connection, "COMMIT"))
				return sql_failure(connection, request);
			return failure(request, maintenance_outcome::complete, 0);
		}
	}
	const std::string selection = "SELECT id FROM zones WHERE epic_type>0 AND id>" +
				      std::to_string(request.cursor) + " ORDER BY id LIMIT " +
				      limit_sql(request);
	if (!execute_sql(connection, selection))
		return sql_failure(connection, request);
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return sql_failure(connection, request);
	std::string ids;
	uint32_t count = 0;
	uint64_t next_cursor = request.cursor;
	MYSQL_ROW row = nullptr;
	while ((row = mysql_fetch_row(rows)))
	{
		int64_t id = 0;
		if (!parse_positive(row[0], &id) || count >= request.row_budget)
		{
			mysql_free_result(rows);
			return sql_failure(connection, request);
		}
		if (count)
			ids += ',';
		ids += std::to_string(id);
		next_cursor = static_cast<uint64_t>(id);
		++count;
	}
	mysql_free_result(rows);
	bool applied = false;
	if (!marker_matches_and_lock(connection, "maintenance_epic_mod", request.work_id,
				     request.cursor, &applied))
		return sql_failure(connection, request);
	if (!before_deadline(request))
		return sql_failure(connection, request);
	if (count && !applied)
	{
		const double add = request.values[2] / 1000000.0;
		const double maximum = request.values[3] / 1000000.0;
		const double minimum = request.values[4] / 1000000.0;
		const std::string update = "UPDATE zones SET frequency_mod=LEAST(" +
					   std::to_string(maximum) + ",GREATEST(" +
					   std::to_string(minimum) + ",frequency_mod+" +
					   std::to_string(add) + ")) WHERE id IN (" + ids + ')';
		if (!execute_sql(connection, update))
			return sql_failure(connection, request);
	}
	if (count < request.row_budget &&
	    !execute_sql(connection, "REPLACE INTO timers(name,date) VALUES('epic_zone_mod'," +
					     std::to_string(request.values[0]) + ')'))
		return sql_failure(connection, request);
	if (!before_deadline(request) || !execute_sql(connection, "COMMIT"))
		return sql_failure(connection, request);
	maintenance_result result = failure(request,
					    count == request.row_budget ?
						    maintenance_outcome::more :
						    maintenance_outcome::complete,
					    0);
	result.next_cursor = next_cursor;
	result.rows = count;
	return result;
}

maintenance_result execute_zone_trophy(MYSQL *connection, const maintenance_request &request)
{
	if (request.value_count != 4 || request.values[0] <= 0 || request.values[2] < 0 ||
	    request.values[3] < 0)
		return failure(request, maintenance_outcome::permanent_failure, EINVAL);
	if (!request.values[1])
		return failure(request, maintenance_outcome::complete, 0);
	if (!begin_transaction(connection))
		return sql_failure(connection, request);
	if (!request.cursor)
	{
		int64_t last_run = 0;
		if (!timer_value(connection, "zone_trophy_reduction", &last_run))
			return sql_failure(connection, request);
		if (request.values[0] <= last_run + request.values[2])
		{
			if (!execute_sql(connection, "COMMIT"))
				return sql_failure(connection, request);
			return failure(request, maintenance_outcome::complete, 0);
		}
	}
	const uint64_t cursor_pid = request.cursor >> 32;
	const uint32_t cursor_zone = static_cast<uint32_t>(request.cursor);
	const std::string selection =
		"SELECT pid,zone_number FROM zone_trophy WHERE (pid,zone_number)>(" +
		std::to_string(cursor_pid) + ',' + std::to_string(cursor_zone) +
		") ORDER BY pid,zone_number LIMIT " + limit_sql(request);
	if (!execute_sql(connection, selection))
		return sql_failure(connection, request);
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return sql_failure(connection, request);
	std::string predicates;
	uint32_t count = 0;
	uint64_t next_cursor = request.cursor;
	MYSQL_ROW row = nullptr;
	while ((row = mysql_fetch_row(rows)))
	{
		int64_t pid = 0;
		int64_t zone = 0;
		if (!parse_positive(row[0], &pid) || !parse_positive(row[1], &zone) ||
		    pid > UINT32_MAX || zone > UINT32_MAX || count >= request.row_budget)
		{
			mysql_free_result(rows);
			return sql_failure(connection, request);
		}
		if (count)
			predicates += " OR ";
		predicates += "(pid=" + std::to_string(pid) +
			      " AND zone_number=" + std::to_string(zone) + ')';
		next_cursor = (static_cast<uint64_t>(pid) << 32) | static_cast<uint32_t>(zone);
		++count;
	}
	mysql_free_result(rows);
	bool applied = false;
	if (!marker_matches_and_lock(connection, "maintenance_trophy", request.work_id,
				     request.cursor, &applied))
		return sql_failure(connection, request);
	if (!before_deadline(request))
		return sql_failure(connection, request);
	if (count && !applied)
	{
		const double multiplier = request.values[3] / 1000000.0;
		if (!execute_sql(connection, "UPDATE zone_trophy SET exp=FLOOR(exp*" +
						     std::to_string(multiplier) + ") WHERE " +
						     predicates) ||
		    !execute_sql(connection,
				 "DELETE FROM zone_trophy WHERE exp<=0 AND (" + predicates + ')'))
			return sql_failure(connection, request);
	}
	if (count < request.row_budget &&
	    !execute_sql(connection,
			 "REPLACE INTO timers(name,date) VALUES('zone_trophy_reduction'," +
				 std::to_string(request.values[0]) + ')'))
		return sql_failure(connection, request);
	if (!before_deadline(request) || !execute_sql(connection, "COMMIT"))
		return sql_failure(connection, request);
	maintenance_result result = failure(request,
					    count == request.row_budget ?
						    maintenance_outcome::more :
						    maintenance_outcome::complete,
					    0);
	result.next_cursor = next_cursor;
	result.rows = count;
	return result;
}

maintenance_result execute_level_cap(MYSQL *connection, const maintenance_request &request)
{
	if (request.value_count != 1 || request.values[0] <= 0)
		return failure(request, maintenance_outcome::permanent_failure, EINVAL);
	if (!begin_transaction(connection))
		return sql_failure(connection, request);
	bool already_applied = false;
	if (!marker_matches_and_lock(connection, "maintenance_level_cap", request.work_id, 0,
				     &already_applied))
		return sql_failure(connection, request);
	if (already_applied)
	{
		if (!execute_sql(connection, "COMMIT"))
			return sql_failure(connection, request);
		return failure(request, maintenance_outcome::complete, 0);
	}
	if (!before_deadline(request) ||
	    !execute_sql(connection,
			 "SELECT id,most_frags,racewar_leader,level,UNIX_TIMESTAMP(next_update) "
			 "FROM level_cap ORDER BY id LIMIT 1 FOR UPDATE"))
		return sql_failure(connection, request);
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return sql_failure(connection, request);
	MYSQL_ROW row = mysql_fetch_row(rows);
	if (!row || !row[0] || !row[1] || !row[2] || !row[3] || !row[4])
	{
		mysql_free_result(rows);
		if (!execute_sql(connection, "COMMIT"))
			return sql_failure(connection, request);
		return failure(request, maintenance_outcome::complete, 0);
	}
	const uint64_t id = strtoull(row[0], nullptr, 10);
	const double old_max = strtod(row[1], nullptr);
	const int racewar = atoi(row[2]);
	const int old_level = atoi(row[3]);
	const int64_t next_update = strtoll(row[4], nullptr, 10);
	mysql_free_result(rows);
	const auto *config = frag_cap_config_get();
	if (!id || !config || racewar == 0 || old_level < 0 ||
	    old_level >= config->cap_maximum_level)
	{
		if (!execute_sql(connection, "COMMIT"))
			return sql_failure(connection, request);
		return failure(request, maintenance_outcome::complete, 0);
	}
	if (!execute_sql(connection,
			 "SELECT COALESCE(SUM(total_frags),0) FROM frag_leaderboard WHERE racewar=" +
				 std::to_string(racewar)))
		return sql_failure(connection, request);
	rows = mysql_store_result(connection);
	row = rows ? mysql_fetch_row(rows) : nullptr;
	if (!rows || !row || !row[0])
	{
		if (rows)
			mysql_free_result(rows);
		return sql_failure(connection, request);
	}
	const int64_t raw_frags = strtoll(row[0], nullptr, 10);
	mysql_free_result(rows);
	const double frags = raw_frags / 100.0;
	uint64_t created_boon = 0;
	if (request.values[0] >= next_update &&
	    old_level < frag_cap_config_cap_level_from_frags(frags))
	{
		const std::string boon =
			"INSERT INTO boons(time,duration,racewar,type,opt,criteria,criteria2,bonus,"
			"bonus2,random,active,pid,rpt) SELECT " +
			std::to_string(request.values[0]) + ',' +
			std::to_string(frag_cap_config_boon_duration_minutes()) + ',' +
			std::to_string(racewar) + ",1,3,1,-1," +
			std::to_string(frag_cap_config_boon_bonus()) +
			",0,0,1,0,1 WHERE (SELECT COUNT(*) FROM boons WHERE active=1)<99";
		const int next_level =
			std::min(old_level + config->cap_level_step, config->cap_maximum_level);
		const int64_t next_time =
			request.values[0] + 86400LL * frag_cap_config_timer_days(old_level);
		if (!execute_sql(connection, boon))
			return sql_failure(connection, request);
		if (mysql_affected_rows(connection) == 1)
			created_boon = mysql_insert_id(connection);
		if (!execute_sql(connection,
				 "UPDATE level_cap SET most_frags=" + std::to_string(frags) +
					 ",racewar_leader=" + std::to_string(racewar) +
					 ",level=" + std::to_string(next_level) +
					 ",next_update=FROM_UNIXTIME(" + std::to_string(next_time) +
					 ") WHERE id=" + std::to_string(id)))
			return sql_failure(connection, request);
	}
	else if (frags > old_max &&
		 !execute_sql(connection,
			      "UPDATE level_cap SET most_frags=" + std::to_string(frags) +
				      ",racewar_leader=" + std::to_string(racewar) +
				      " WHERE id=" + std::to_string(id)))
		return sql_failure(connection, request);
	if (!before_deadline(request) || !execute_sql(connection, "COMMIT"))
		return sql_failure(connection, request);
	maintenance_result result = failure(request, maintenance_outcome::complete, 0);
	result.rows = 1;
	if (created_boon)
	{
		result.value_count = 3;
		result.values[0] = static_cast<int64_t>(created_boon);
		result.values[1] = racewar;
		result.values[2] = 0;
	}
	return result;
}

struct boon_scan_row
{
	int64_t id;
	int64_t created_at;
	int64_t duration;
	int racewar;
	int option;
	int criteria;
	int pid;
};

maintenance_result execute_boon_scan(MYSQL *connection, const maintenance_request &request)
{
	if (request.value_count < 3 || request.values[0] <= 0 || request.values[1] < 0)
		return failure(request, maintenance_outcome::permanent_failure, EINVAL);
	const size_t epic_records = static_cast<size_t>(request.values[1]);
	const size_t ctf_count_index = 2 + epic_records * 2;
	if (ctf_count_index >= request.value_count || request.values[ctf_count_index] < 0)
		return failure(request, maintenance_outcome::permanent_failure, EINVAL);
	const size_t ctf_records = static_cast<size_t>(request.values[ctf_count_index]);
	if (ctf_count_index + 1 + ctf_records * 3 != request.value_count)
		return failure(request, maintenance_outcome::permanent_failure, EINVAL);
	std::map<int, int> epic_completions;
	for (size_t index = 0; index < epic_records; ++index)
	{
		const int zone = static_cast<int>(request.values[2 + index * 2]);
		if (zone > 0)
			++epic_completions[zone];
	}
	struct ctf_state
	{
		int type;
		int room;
	};
	std::map<int, ctf_state> ctf_states;
	for (size_t index = 0; index < ctf_records; ++index)
	{
		const size_t offset = ctf_count_index + 1 + index * 3;
		ctf_states[static_cast<int>(request.values[offset])] = {
			static_cast<int>(request.values[offset + 1]),
			static_cast<int>(request.values[offset + 2])
		};
	}
	if (!begin_transaction(connection))
		return sql_failure(connection, request);
	const std::string selection =
		"SELECT id,time,duration,racewar,opt,criteria,pid FROM boons WHERE active=1 AND id>" +
		std::to_string(request.cursor) + " ORDER BY id LIMIT " + limit_sql(request);
	if (!execute_sql(connection, selection))
		return sql_failure(connection, request);
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return sql_failure(connection, request);
	std::vector<boon_scan_row> boons;
	std::set<int> zone_ids;
	std::set<int> nexus_ids;
	MYSQL_ROW row = nullptr;
	while ((row = mysql_fetch_row(rows)))
	{
		if (boons.size() >= request.row_budget)
		{
			mysql_free_result(rows);
			return sql_failure(connection, request);
		}
		bool valid = true;
		for (size_t column = 0; column < 7; ++column)
			valid = valid && row[column];
		if (!valid)
		{
			mysql_free_result(rows);
			return sql_failure(connection, request);
		}
		boon_scan_row boon = { strtoll(row[0], nullptr, 10),
				       strtoll(row[1], nullptr, 10),
				       strtoll(row[2], nullptr, 10),
				       atoi(row[3]),
				       atoi(row[4]),
				       static_cast<int>(strtod(row[5], nullptr)),
				       atoi(row[6]) };
		if (boon.id <= 0)
		{
			mysql_free_result(rows);
			return sql_failure(connection, request);
		}
		if (boon.option == 1)
			zone_ids.insert(boon.criteria);
		else if (boon.option == 9)
			nexus_ids.insert(boon.criteria);
		boons.push_back(boon);
	}
	mysql_free_result(rows);
	std::map<int, int> zone_stones;
	if (!zone_ids.empty())
	{
		std::string query = "SELECT number,stonecount FROM zones WHERE number IN (";
		for (auto iterator = zone_ids.begin(); iterator != zone_ids.end(); ++iterator)
		{
			if (iterator != zone_ids.begin())
				query += ',';
			query += std::to_string(*iterator);
		}
		query += ')';
		if (!execute_sql(connection, query))
			return sql_failure(connection, request);
		rows = mysql_store_result(connection);
		if (!rows)
			return sql_failure(connection, request);
		while ((row = mysql_fetch_row(rows)))
			if (row[0] && row[1])
				zone_stones[atoi(row[0])] = atoi(row[1]);
		mysql_free_result(rows);
	}
	std::map<int, int> nexus_alignments;
	if (!nexus_ids.empty())
	{
		std::string query = "SELECT id,align FROM nexus_stones WHERE id IN (";
		for (auto iterator = nexus_ids.begin(); iterator != nexus_ids.end(); ++iterator)
		{
			if (iterator != nexus_ids.begin())
				query += ',';
			query += std::to_string(*iterator);
		}
		query += ')';
		if (!execute_sql(connection, query))
			return sql_failure(connection, request);
		rows = mysql_store_result(connection);
		if (!rows)
			return sql_failure(connection, request);
		while ((row = mysql_fetch_row(rows)))
			if (row[0] && row[1])
				nexus_alignments[atoi(row[0])] = atoi(row[1]);
		mysql_free_result(rows);
	}
	struct expired_boon
	{
		boon_scan_row boon;
		int reason;
	};
	std::vector<expired_boon> expired;
	for (const auto &boon : boons)
	{
		int reason = 0;
		if (boon.duration != -1 && boon.created_at + boon.duration * 60 < request.values[0])
			reason = 7;
		else if (boon.option == 1 && zone_stones.count(boon.criteria) &&
			 epic_completions[boon.criteria] >= zone_stones[boon.criteria])
			reason = 6;
		else if (boon.option == 9 && nexus_alignments.count(boon.criteria) &&
			 ((nexus_alignments[boon.criteria] == 3 && boon.racewar == 1) ||
			  (nexus_alignments[boon.criteria] == -3 && boon.racewar == 2)))
			reason = 6;
		else if (boon.option == 13 && ctf_states.count(boon.criteria) &&
			 ctf_states[boon.criteria].type == 4 && !ctf_states[boon.criteria].room)
			reason = 6;
		if (reason)
			expired.push_back({ boon, reason });
	}
	bool already_applied = false;
	if (!marker_matches_and_lock(connection, "maintenance_boon", request.work_id,
				     request.cursor, &already_applied))
		return sql_failure(connection, request);
	if (!before_deadline(request))
		return sql_failure(connection, request);
	if (!expired.empty() && !already_applied)
	{
		std::string ids;
		for (size_t index = 0; index < expired.size(); ++index)
		{
			if (index)
				ids += ',';
			ids += std::to_string(expired[index].boon.id);
		}
		if (!execute_sql(connection,
				 "UPDATE boons SET active=0,duration=0 WHERE active=1 AND id IN (" +
					 ids + ')'))
			return sql_failure(connection, request);
	}
	if (!before_deadline(request) || !execute_sql(connection, "COMMIT"))
		return sql_failure(connection, request);
	maintenance_result result = failure(request,
					    boons.size() == request.row_budget ?
						    maintenance_outcome::more :
						    maintenance_outcome::complete,
					    0);
	result.rows = static_cast<uint32_t>(boons.size());
	if (!boons.empty())
		result.next_cursor = static_cast<uint64_t>(boons.back().id);
	if (!before_deadline(request))
		return sql_failure(connection, request);
	if (!already_applied)
		for (const auto &entry : expired)
		{
			if (result.value_count + 6 > result.values.size())
				break;
			result.values[result.value_count++] = entry.boon.id;
			result.values[result.value_count++] = entry.boon.racewar;
			result.values[result.value_count++] = entry.boon.pid;
			result.values[result.value_count++] = entry.reason;
			result.values[result.value_count++] = entry.boon.option;
			result.values[result.value_count++] = entry.boon.criteria;
		}
	return result;
}

maintenance_result execute_cargo_market(MYSQL *connection, const maintenance_request &request)
{
	constexpr size_t ports = 10;
	constexpr size_t expected_values = 3 + ports * ports * 4;
	if (request.value_count != expected_values || request.values[0] <= 0 ||
	    (request.values[1] != 0 && request.values[1] != 1) ||
	    (request.values[2] != 0 && request.values[2] != 1) ||
	    (!request.values[1] && !request.values[2]) || request.row_budget < ports * ports)
		return failure(request, maintenance_outcome::permanent_failure, EINVAL);
	if (!begin_transaction(connection))
		return sql_failure(connection, request);
	bool already_applied = false;
	if (!marker_matches_and_lock(connection, "maintenance_cargo", request.work_id, 0,
				     &already_applied))
		return sql_failure(connection, request);
	if (!already_applied)
	{
		if (request.values[1])
		{
			if (!execute_sql(connection, "DELETE FROM ship_cargo_market_mods") ||
			    !execute_sql(connection, "DELETE FROM ship_cargo_prices"))
				return sql_failure(connection, request);
			std::string prices =
				"INSERT INTO ship_cargo_prices(type,port_id,cargo_type,price) VALUES";
			std::string modifiers =
				"INSERT INTO ship_cargo_market_mods(type,port_id,cargo_type,modifier) VALUES";
			bool first = true;
			for (size_t port = 0; port < ports; ++port)
				for (size_t type = 0; type < ports; ++type)
				{
					const size_t offset = 3 + (port * ports + type) * 4;
					if (!first)
					{
						prices += ',';
						modifiers += ',';
					}
					prices += "('CARGO'," + std::to_string(port) + ',' +
						  std::to_string(type) + ',' +
						  std::to_string(request.values[offset]) +
						  "),('CONTRABAND'," + std::to_string(port) + ',' +
						  std::to_string(type) + ',' +
						  std::to_string(request.values[offset + 1]) + ')';
					modifiers += "('CARGO'," + std::to_string(port) + ',' +
						     std::to_string(type) + ',' +
						     std::to_string(request.values[offset + 2] /
								    1000000.0) +
						     "),('CONTRABAND'," + std::to_string(port) +
						     ',' + std::to_string(type) + ',' +
						     std::to_string(request.values[offset + 3] /
								    1000000.0) +
						     ')';
					first = false;
				}
			if (!execute_sql(connection, prices) || !execute_sql(connection, modifiers))
				return sql_failure(connection, request);
		}
		std::string timers = "REPLACE INTO timers(name,date) VALUES";
		if (request.values[1])
			timers += "('update_cargo'," + std::to_string(request.values[0]) + ')';
		if (request.values[2])
		{
			if (request.values[1])
				timers += ',';
			timers += "('update_delayed_cargo_prices'," +
				  std::to_string(request.values[0]) + ')';
		}
		if (!execute_sql(connection, timers))
			return sql_failure(connection, request);
	}
	if (!before_deadline(request) || !execute_sql(connection, "COMMIT"))
		return sql_failure(connection, request);
	maintenance_result result = failure(request, maintenance_outcome::complete, 0);
	result.rows = ports * ports;
	return result;
}

bool write_all(int descriptor, const char *data, size_t size)
{
	while (size)
	{
		const ssize_t written = write(descriptor, data, size);
		if (written < 0 && errno == EINTR)
			continue;
		if (written <= 0)
			return false;
		data += written;
		size -= static_cast<size_t>(written);
	}
	return true;
}

bool atomic_replace(const char *path, const char *data, size_t size)
{
	const std::string temporary = std::string(path) + ".tmp";
	const int descriptor = open(temporary.c_str(),
				    O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0640);
	if (descriptor < 0)
		return false;
	const bool written = write_all(descriptor, data, size);
	const bool synced = written && fsync(descriptor) == 0;
	const bool closed = close(descriptor) == 0;
	if (!synced || !closed || rename(temporary.c_str(), path) != 0)
	{
		unlink(temporary.c_str());
		return false;
	}
	return true;
}

maintenance_result execute_web_status(const maintenance_request &request)
{
	if (!before_deadline(request) || !request.content_size ||
	    request.content_size >= request.content.size() ||
	    request.content[request.content_size] != '\0')
		return failure(request, maintenance_outcome::permanent_failure, EINVAL);
	if (!atomic_replace("lib/reports/status", request.content.data(), request.content_size))
		return failure(request, maintenance_outcome::retryable_failure,
			       errno ? errno : EIO);
	maintenance_result result = failure(request, maintenance_outcome::complete, 0);
	result.rows = 1;
	return result;
}

bool statistics_marker_matches(MYSQL *connection, int32_t high, int32_t low, bool *matches)
{
	if (!matches)
		return false;
	if (!execute_sql(connection,
			 "SELECT name,date FROM timers WHERE name IN "
			 "('maintenance_statistics_high','maintenance_statistics_low') FOR UPDATE"))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return false;
	bool high_match = false;
	bool low_match = false;
	MYSQL_ROW row = nullptr;
	while ((row = mysql_fetch_row(rows)))
	{
		if (!row[0] || !row[1])
			continue;
		if (!strcmp(row[0], "maintenance_statistics_high"))
			high_match = strtol(row[1], nullptr, 10) == high;
		else if (!strcmp(row[0], "maintenance_statistics_low"))
			low_match = strtol(row[1], nullptr, 10) == low;
	}
	mysql_free_result(rows);
	*matches = high_match && low_match;
	return true;
}

bool statistics_file_has_work(const char *path, uint64_t work_id)
{
	const int descriptor = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
	if (descriptor < 0)
		return false;
	const off_t end = lseek(descriptor, 0, SEEK_END);
	if (end < 0)
	{
		close(descriptor);
		return false;
	}
	const off_t start = end > 8192 ? end - 8192 : 0;
	if (lseek(descriptor, start, SEEK_SET) < 0)
	{
		close(descriptor);
		return false;
	}
	char tail[8193] = {};
	const ssize_t count = read(descriptor, tail, sizeof(tail) - 1);
	close(descriptor);
	if (count < 0)
		return false;
	char marker[64] = {};
	snprintf(marker, sizeof(marker), " %llu\r\n", (unsigned long long)work_id);
	return strstr(tail, marker) != nullptr;
}

maintenance_result execute_statistics(MYSQL *connection, const maintenance_request &request)
{
	if (request.value_count != 12 || request.values[0] <= 0)
		return failure(request, maintenance_outcome::permanent_failure, EINVAL);
	const int32_t high = static_cast<int32_t>(request.work_id >> 32);
	const int32_t low = static_cast<int32_t>(request.work_id);
	if (!execute_sql(connection, "START TRANSACTION") ||
	    !execute_sql(connection,
			 "INSERT IGNORE INTO timers(name,date) VALUES"
			 "('maintenance_statistics_high',0),('maintenance_statistics_low',0)"))
		return failure(request, maintenance_outcome::retryable_failure,
			       mysql_errno(connection));
	bool already_applied = false;
	if (!statistics_marker_matches(connection, high, low, &already_applied))
	{
		execute_sql(connection, "ROLLBACK");
		return failure(request, maintenance_outcome::retryable_failure,
			       mysql_errno(connection) ? mysql_errno(connection) : EIO);
	}
	if (!before_deadline(request))
	{
		execute_sql(connection, "ROLLBACK");
		return failure(request, maintenance_outcome::retryable_failure, ETIMEDOUT);
	}
	if (!already_applied)
	{
		std::string insert = "INSERT INTO statistics(date,goods_count,evils_count,"
				     "illithids_count,undeads_count,gods_count,in_guildhall_count,"
				     "sum_goods_levels,sum_evils_levels,sum_undeads_levels,"
				     "sum_illithids_levels,unique_ips_count) VALUES (";
		for (size_t index = 0; index < request.value_count; ++index)
		{
			if (index)
				insert += ',';
			insert += std::to_string(request.values[index]);
		}
		insert += ')';
		const std::string markers =
			"UPDATE timers SET date=CASE name WHEN 'maintenance_statistics_high' THEN " +
			std::to_string(high) + " ELSE " + std::to_string(low) +
			" END WHERE name IN ('maintenance_statistics_high','maintenance_statistics_low')";
		if (!execute_sql(connection, insert) || !execute_sql(connection, markers))
		{
			execute_sql(connection, "ROLLBACK");
			return failure(request, maintenance_outcome::retryable_failure,
				       mysql_errno(connection));
		}
	}
	if (!execute_sql(connection, "COMMIT"))
	{
		execute_sql(connection, "ROLLBACK");
		return failure(request, maintenance_outcome::retryable_failure,
			       mysql_errno(connection));
	}
	time_t captured = static_cast<time_t>(request.values[0]);
	struct tm local = {};
	if (!localtime_r(&captured, &local))
		return failure(request, maintenance_outcome::permanent_failure, EINVAL);
	char date[16] = {};
	char path[128] = {};
	char line[512] = {};
	strftime(date, sizeof(date), "%Y%m%d", &local);
	snprintf(path, sizeof(path), "lib/statistics/statistics_general%s", date);
	const int line_size =
		snprintf(line, sizeof(line),
			 "%02d %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %llu\r\n",
			 local.tm_hour, (long long)request.values[1], (long long)request.values[2],
			 (long long)request.values[3], (long long)request.values[4],
			 (long long)request.values[5], (long long)request.values[6],
			 (long long)request.values[7], (long long)request.values[8],
			 (long long)request.values[9], (long long)request.values[10],
			 (long long)request.values[11], (unsigned long long)request.work_id);
	if (line_size <= 0 || static_cast<size_t>(line_size) >= sizeof(line))
		return failure(request, maintenance_outcome::permanent_failure, EOVERFLOW);
	if (!before_deadline(request))
		return failure(request, maintenance_outcome::retryable_failure, ETIMEDOUT);
	if (!statistics_file_has_work(path, request.work_id))
	{
		const int descriptor =
			open(path, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC | O_NOFOLLOW, 0640);
		if (descriptor < 0)
			return failure(request, maintenance_outcome::retryable_failure,
				       errno ? errno : EIO);
		const bool written = write_all(descriptor, line, static_cast<size_t>(line_size));
		const bool synced = written && fsync(descriptor) == 0;
		const bool closed = close(descriptor) == 0;
		if (!synced || !closed)
		{
			return failure(request, maintenance_outcome::retryable_failure,
				       errno ? errno : EIO);
		}
	}
	maintenance_result result = failure(request, maintenance_outcome::complete, 0);
	result.rows = already_applied ? 0 : 1;
	return result;
}

maintenance_result execute_auction(MYSQL *connection, const maintenance_request &request)
{
	maintenance_result result = failure(request, maintenance_outcome::permanent_failure, 0);
	const std::string sql =
		"SELECT id FROM auctions WHERE id>" + std::to_string(request.cursor) +
		" AND end_time<NOW() AND status=1 AND custody_state=1 ORDER BY id LIMIT " +
		limit_sql(request);
	if (!query_values(connection, request, sql, &result))
		return failure(request, maintenance_outcome::retryable_failure,
			       mysql_errno(connection));
	return result;
}

maintenance_result execute_catalog(MYSQL *connection, const maintenance_request &request)
{
	maintenance_result result = failure(request, maintenance_outcome::permanent_failure, 0);
	const std::string sql = "SELECT number FROM zones WHERE task_zone=1 AND number>" +
				std::to_string(request.cursor) + " ORDER BY number LIMIT " +
				limit_sql(request);
	if (!query_values(connection, request, sql, &result))
		return failure(request, maintenance_outcome::retryable_failure,
			       mysql_errno(connection));
	return result;
}

maintenance_result execute_poll(MYSQL *connection, const maintenance_request &request)
{
	maintenance_result result = failure(request, maintenance_outcome::permanent_failure, 0);
	const std::string selection =
		"SELECT id FROM polls WHERE id>" + std::to_string(request.cursor) +
		" AND is_active=1 AND expires_at<UNIX_TIMESTAMP() ORDER BY id LIMIT " +
		limit_sql(request);
	if (!query_values(connection, request, selection, &result))
		return failure(request, maintenance_outcome::retryable_failure,
			       mysql_errno(connection));
	if (!result.value_count)
		return result;
	std::string update = "UPDATE polls SET is_active=0 WHERE is_active=1 AND id IN (";
	for (size_t index = 0; index < result.value_count; ++index)
	{
		if (index)
			update += ',';
		update += std::to_string(result.values[index]);
	}
	update += ')';
	if (!before_deadline(request) ||
	    mysql_real_query(connection, update.data(), update.size()) != 0)
		return failure(request, maintenance_outcome::retryable_failure,
			       mysql_errno(connection));
	return result;
}
} // namespace

maintenance_result maintenance_repository_execute(const maintenance_request &request, void *)
{
	if (!request.work_id || static_cast<size_t>(request.job_id) >= MAINTENANCE_JOB_COUNT ||
	    !request.row_budget || request.row_budget > MAINTENANCE_ROW_BUDGET_MAX ||
	    !request.time_budget_usec ||
	    request.time_budget_usec > MAINTENANCE_TIME_BUDGET_USEC_MAX ||
	    !before_deadline(request))
		return failure(request, maintenance_outcome::permanent_failure, EINVAL);
	if (request.job_id == maintenance_job_id::web_status)
		return execute_web_status(request);
	MYSQL *connection = sql_pool_acquire();
	if (!connection)
		return failure(request, maintenance_outcome::retryable_failure, EAGAIN);
	connection_guard guard = { connection };
	switch (request.job_id)
	{
	case maintenance_job_id::auction_due_scan:
		return execute_auction(connection, request);
	case maintenance_job_id::poll_expiration:
		return execute_poll(connection, request);
	case maintenance_job_id::epic_task_catalog:
		return execute_catalog(connection, request);
	case maintenance_job_id::epic_zone_balance:
		return execute_epic_balance(connection, request);
	case maintenance_job_id::level_cap:
		return execute_level_cap(connection, request);
	case maintenance_job_id::zone_trophy:
		return execute_zone_trophy(connection, request);
	case maintenance_job_id::epic_zone_modifiers:
		return execute_epic_modifiers(connection, request);
	case maintenance_job_id::boon_scan:
		return execute_boon_scan(connection, request);
	case maintenance_job_id::cargo_market:
		return execute_cargo_market(connection, request);
	case maintenance_job_id::operational_statistics:
		return execute_statistics(connection, request);
	default:
		return failure(request, maintenance_outcome::permanent_failure, ENOTSUP);
	}
}
