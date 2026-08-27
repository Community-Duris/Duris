#include "artifact_guild_repository.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <mysql.h>
#include <string>

namespace
{
struct artifact_state
{
	int64_t timer;
	int32_t bind_owner_pid;
	int64_t bind_timer;
	uint64_t revision;
};

bool execute(MYSQL *connection, const std::string &sql)
{
	return mysql_real_query(connection, sql.data(), sql.size()) == 0;
}

bool parse_i64(const char *text, int64_t *value)
{
	if (!text || !value)
		return false;
	char *end = nullptr;
	errno = 0;
	const long long parsed = strtoll(text, &end, 10);
	if (errno || !end || *end)
		return false;
	*value = parsed;
	return true;
}

bool parse_u64(const char *text, uint64_t *value)
{
	if (!text || !value)
		return false;
	char *end = nullptr;
	errno = 0;
	const unsigned long long parsed = strtoull(text, &end, 10);
	if (errno || !end || *end)
		return false;
	*value = parsed;
	return true;
}

std::string bytes_hex(const uint8_t *bytes, size_t size)
{
	static const char HEX[] = "0123456789abcdef";
	std::string result(size * 2, '0');
	for (size_t index = 0; index < size; ++index)
	{
		result[index * 2] = HEX[bytes[index] >> 4];
		result[index * 2 + 1] = HEX[bytes[index] & 15];
	}
	return result;
}

bool load_artifact(MYSQL *connection, int32_t vnum, artifact_state *state)
{
	if (!execute(connection,
		     "SELECT s.timer_epoch,s.bind_owner_pid,s.bind_timer_epoch,s.revision,"
		     "COALESCE(UNIX_TIMESTAMP(a.timer),0),COALESCE(b.owner_pid,0),"
		     "COALESCE(b.timer,0) FROM artifact_domain_state s JOIN artifacts a ON "
		     "a.vnum=s.vnum LEFT JOIN artifact_bind b ON b.vnum=s.vnum WHERE s.vnum=" +
			     std::to_string(vnum) + " FOR UPDATE"))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	MYSQL_ROW row = rows ? mysql_fetch_row(rows) : nullptr;
	int64_t bind_owner = 0, legacy_timer = 0, legacy_bind_owner = 0, legacy_bind_timer = 0;
	const bool ok = row && parse_i64(row[0], &state->timer) && parse_i64(row[1], &bind_owner) &&
			parse_i64(row[2], &state->bind_timer) &&
			parse_u64(row[3], &state->revision) && parse_i64(row[4], &legacy_timer) &&
			parse_i64(row[5], &legacy_bind_owner) &&
			parse_i64(row[6], &legacy_bind_timer) && bind_owner >= INT32_MIN &&
			bind_owner <= INT32_MAX && state->timer == legacy_timer &&
			bind_owner == legacy_bind_owner && state->bind_timer == legacy_bind_timer;
	if (ok)
		state->bind_owner_pid = static_cast<int32_t>(bind_owner);
	if (rows)
		mysql_free_result(rows);
	if (!ok && mysql_errno(connection) == 0)
		errno = row ? ESTALE : ENOENT;
	return ok;
}

bool load_guild(MYSQL *connection, uint32_t guild_id, uint64_t *prestige, uint64_t *construction,
		uint64_t *revision)
{
	if (!execute(connection,
		     "SELECT prestige,construction,outcome_revision FROM guilds WHERE id=" +
			     std::to_string(guild_id) + " FOR UPDATE"))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	MYSQL_ROW row = rows ? mysql_fetch_row(rows) : nullptr;
	const bool ok = row && parse_u64(row[0], prestige) && parse_u64(row[1], construction) &&
			parse_u64(row[2], revision);
	if (rows)
		mysql_free_result(rows);
	if (!ok && mysql_errno(connection) == 0)
		errno = ENOENT;
	return ok;
}

bool apply_delta(uint64_t value, int64_t delta, uint64_t *result)
{
	if (!result || (delta < 0 && static_cast<uint64_t>(-(delta + 1)) + 1 > value) ||
	    (delta > 0 && static_cast<uint64_t>(delta) > UINT64_MAX - value))
		return false;
	*result = delta < 0 ? value - (static_cast<uint64_t>(-(delta + 1)) + 1) :
			      value + static_cast<uint64_t>(delta);
	return true;
}
} // namespace

bool artifact_guild_repository_execute(MYSQL *connection, const critical_command &command,
				       artifact_guild_result *result, unsigned int *result_code,
				       bool *mutation_applied)
{
	if (!connection || !result || !result_code || !mutation_applied)
		return false;
	artifact_guild_payload payload = {};
	if (!artifact_guild_command_decode_payload(command, &payload))
	{
		errno = EINVAL;
		return false;
	}
	*result = {};
	*result_code = 0;
	*mutation_applied = false;

	std::array<artifact_state, ARTIFACT_GUILD_MAX_ARTIFACTS> artifact_states = {};
	for (size_t index = 0; index < payload.artifact_count; ++index)
	{
		const auto &entry = payload.artifacts[index];
		if (!load_artifact(connection, entry.vnum, &artifact_states[index]))
			return false;
		const auto &state = artifact_states[index];
		if (state.revision != entry.expected_revision ||
		    state.timer != entry.expected_timer ||
		    state.bind_owner_pid != entry.expected_bind_owner_pid ||
		    state.bind_timer != entry.expected_bind_timer)
		{
			errno = ESTALE;
			return false;
		}
	}

	uint64_t guild_prestige = 0, guild_construction = 0, guild_revision = 0;
	if (payload.guild_id && !load_guild(connection, payload.guild_id, &guild_prestige,
					    &guild_construction, &guild_revision))
		return false;
	if (payload.guild_id && guild_revision != payload.expected_guild_revision)
	{
		errno = ESTALE;
		return false;
	}
	uint64_t final_prestige = guild_prestige, final_construction = guild_construction;
	if (!apply_delta(guild_prestige, payload.prestige_delta, &final_prestige) ||
	    !apply_delta(guild_construction, payload.construction_delta, &final_construction))
	{
		errno = ERANGE;
		return false;
	}
	const uint64_t final_guild_revision = payload.guild_id ? guild_revision + 1 : 0;
	const std::string operation =
		bytes_hex(command.operation_id.bytes.data(), command.operation_id.bytes.size());
	const std::string parent = bytes_hex(payload.parent_operation_id.bytes.data(),
					     payload.parent_operation_id.bytes.size());
	if (!execute(
		    connection,
		    "INSERT INTO artifact_guild_outcome(operation_id,parent_operation_id,actor_pid,"
		    "guild_id,prestige_delta,construction_delta,artifact_count,guild_prestige_after,"
		    "guild_construction_after,guild_revision) VALUES(UNHEX('" +
			    operation + "'),UNHEX('" + parent + "')," +
			    std::to_string(payload.actor_pid) + "," +
			    std::to_string(payload.guild_id) + "," +
			    std::to_string(payload.prestige_delta) + "," +
			    std::to_string(payload.construction_delta) + "," +
			    std::to_string(payload.artifact_count) + "," +
			    std::to_string(final_prestige) + "," +
			    std::to_string(final_construction) + "," +
			    std::to_string(final_guild_revision) + ")"))
		return false;

	result->guild_id = payload.guild_id;
	result->prestige = final_prestige;
	result->construction = final_construction;
	result->guild_revision = final_guild_revision;
	result->artifact_count = payload.artifact_count;
	for (size_t index = 0; index < payload.artifact_count; ++index)
	{
		const auto &entry = payload.artifacts[index];
		const uint64_t revision = artifact_states[index].revision + 1;
		if (!execute(connection,
			     "UPDATE artifact_domain_state SET timer_epoch=" +
				     std::to_string(entry.timer) +
				     ",bind_owner_pid=" + std::to_string(entry.bind_owner_pid) +
				     ",bind_timer_epoch=" + std::to_string(entry.bind_timer) +
				     ",revision=" + std::to_string(revision) +
				     " WHERE vnum=" + std::to_string(entry.vnum) +
				     " AND revision=" + std::to_string(entry.expected_revision)) ||
		    mysql_affected_rows(connection) != 1 ||
		    !execute(connection, "UPDATE artifacts SET timer=FROM_UNIXTIME(" +
						 std::to_string(entry.timer) +
						 "),lastUpdate=SYSDATE() WHERE vnum=" +
						 std::to_string(entry.vnum)) ||
		    !execute(
			    connection,
			    "INSERT INTO artifact_bind(vnum,owner_pid,timer) VALUES(" +
				    std::to_string(entry.vnum) + "," +
				    std::to_string(entry.bind_owner_pid) + "," +
				    std::to_string(entry.bind_timer) +
				    ") ON DUPLICATE KEY UPDATE owner_pid=VALUES(owner_pid),timer=VALUES(timer)") ||
		    !execute(
			    connection,
			    "INSERT INTO artifact_guild_outcome_delta(operation_id,artifact_index,vnum,"
			    "flags,timer_before,timer_after,bind_owner_before,bind_owner_after,"
			    "bind_timer_before,bind_timer_after,revision) VALUES(UNHEX('" +
				    operation + "')," + std::to_string(index) + "," +
				    std::to_string(entry.vnum) + "," + std::to_string(entry.flags) +
				    "," + std::to_string(entry.expected_timer) + "," +
				    std::to_string(entry.timer) + "," +
				    std::to_string(entry.expected_bind_owner_pid) + "," +
				    std::to_string(entry.bind_owner_pid) + "," +
				    std::to_string(entry.expected_bind_timer) + "," +
				    std::to_string(entry.bind_timer) + "," +
				    std::to_string(revision) + ")") ||
		    !execute(connection,
			     "INSERT INTO artifact_delta_ledger(operation_id,artifact_index,vnum,"
			     "timer_delta,bind_owner_pid,bind_timer_epoch,revision) VALUES(UNHEX('" +
				     operation + "')," + std::to_string(index) + "," +
				     std::to_string(entry.vnum) + "," +
				     std::to_string(entry.timer - entry.expected_timer) + "," +
				     std::to_string(entry.bind_owner_pid) + "," +
				     std::to_string(entry.bind_timer) + "," +
				     std::to_string(revision) + ")"))
			return false;
		result->artifacts[index] = { entry.vnum, entry.timer, entry.bind_owner_pid,
					     entry.bind_timer, revision };
	}
	if (payload.guild_id)
	{
		if (!execute(connection,
			     "UPDATE guilds SET prestige=" + std::to_string(final_prestige) +
				     ",construction=" + std::to_string(final_construction) +
				     ",outcome_revision=" + std::to_string(final_guild_revision) +
				     " WHERE id=" + std::to_string(payload.guild_id) +
				     " AND outcome_revision=" + std::to_string(guild_revision)) ||
		    mysql_affected_rows(connection) != 1 ||
		    !execute(connection,
			     "INSERT INTO guild_outcome_ledger(operation_id,guild_id,prestige_delta,"
			     "construction_delta,prestige_after,construction_after,guild_revision) "
			     "VALUES(UNHEX('" +
				     operation + "')," + std::to_string(payload.guild_id) + "," +
				     std::to_string(payload.prestige_delta) + "," +
				     std::to_string(payload.construction_delta) + "," +
				     std::to_string(final_prestige) + "," +
				     std::to_string(final_construction) + "," +
				     std::to_string(final_guild_revision) + ")"))
			return false;
	}
	*mutation_applied = true;
	return true;
}
