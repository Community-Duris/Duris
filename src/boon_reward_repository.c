#include "boon_reward_repository.h"

#include "boon.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <mysql.h>
#include <string>
#include <vector>

namespace
{
struct boon_row
{
	uint32_t id;
	uint8_t type;
	uint8_t option;
	double criteria;
	double criteria2;
	double bonus;
	double bonus2;
	uint32_t target_pid;
	bool repeat;
};

bool execute(MYSQL *connection, const std::string &sql)
{
	return mysql_real_query(connection, sql.data(), sql.size()) == 0;
}

bool parse_u32(const char *text, uint32_t *value)
{
	if (!text || !value)
		return false;
	char *end = nullptr;
	errno = 0;
	const unsigned long parsed = strtoul(text, &end, 10);
	if (errno || !end || *end || parsed > UINT32_MAX)
		return false;
	*value = static_cast<uint32_t>(parsed);
	return true;
}

bool parse_double(const char *text, double *value)
{
	if (!text || !value)
		return false;
	char *end = nullptr;
	errno = 0;
	const double parsed = strtod(text, &end);
	if (errno || !end || *end || !std::isfinite(parsed))
		return false;
	*value = parsed;
	return true;
}

bool progress_option(uint8_t option)
{
	return option == BOPT_MOB || option == BOPT_RACE || option == BOPT_FRAGS;
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

std::string eligibility(const boon_reward_payload &payload)
{
	std::string clause;
	switch (payload.option)
	{
	case BOPT_NONE:
		clause = " AND criteria=" + std::to_string(payload.zone_number);
		break;
	case BOPT_ZONE:
	case BOPT_OP:
	case BOPT_NEXUS:
	case BOPT_CTF:
	case BOPT_CTFB:
		clause = " AND criteria=" + std::to_string(static_cast<int>(payload.data));
		break;
	case BOPT_LEVEL:
		clause = " AND (criteria=" + std::to_string(payload.level) + " OR criteria=0)";
		break;
	case BOPT_MOB:
		if (!(payload.victim_flags & 1))
			return " AND 0";
		clause = " AND (criteria2=" + std::to_string(payload.victim_vnum) +
			 " OR criteria2=-1)";
		break;
	case BOPT_RACE:
		if (!(payload.victim_flags & 2))
			return " AND 0";
		clause = " AND criteria2=" + std::to_string(payload.victim_race);
		break;
	case BOPT_FRAG:
		clause = " AND criteria<=" + std::to_string(payload.data);
		break;
	default:
		break;
	}
	return clause;
}

bool load_boons(MYSQL *connection, const boon_reward_payload &payload, std::vector<boon_row> *boons)
{
	const std::string sql =
		"SELECT id,type,opt,criteria,criteria2,bonus,bonus2,pid,rpt FROM boons WHERE "
		"opt=" +
		std::to_string(payload.option) +
		" AND active=1 AND (racewar=0 OR racewar=" + std::to_string(payload.racewar) +
		") AND (pid=0 OR pid=" + std::to_string(payload.pid) + ")" + eligibility(payload) +
		" ORDER BY id LIMIT " + std::to_string(BOON_REWARD_MAX_RESULTS + 1) + " FOR UPDATE";
	if (!execute(connection, sql))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		return false;
	MYSQL_ROW row = nullptr;
	while ((row = mysql_fetch_row(rows)))
	{
		boon_row boon = {};
		uint32_t type = 0, option = 0, repeat = 0;
		if (!parse_u32(row[0], &boon.id) || !parse_u32(row[1], &type) ||
		    !parse_u32(row[2], &option) || !parse_double(row[3], &boon.criteria) ||
		    !parse_double(row[4], &boon.criteria2) || !parse_double(row[5], &boon.bonus) ||
		    !parse_double(row[6], &boon.bonus2) || !parse_u32(row[7], &boon.target_pid) ||
		    !parse_u32(row[8], &repeat) || type >= MAX_BTYPE || option >= MAX_BOPT)
		{
			mysql_free_result(rows);
			errno = EINVAL;
			return false;
		}
		boon.type = static_cast<uint8_t>(type);
		boon.option = static_cast<uint8_t>(option);
		boon.repeat = repeat != 0;
		boons->push_back(boon);
	}
	mysql_free_result(rows);
	if (boons->size() > BOON_REWARD_MAX_RESULTS)
	{
		errno = E2BIG;
		return false;
	}
	return true;
}

bool load_progress(MYSQL *connection, uint32_t boon_id, uint32_t pid, uint64_t *id, double *counter)
{
	if (!execute(connection, "SELECT id,counter FROM boons_progress WHERE boonid=" +
					 std::to_string(boon_id) + " AND pid=" +
					 std::to_string(pid) + " ORDER BY id LIMIT 1 FOR UPDATE"))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	MYSQL_ROW row = rows ? mysql_fetch_row(rows) : nullptr;
	bool ok = true;
	if (row)
	{
		uint32_t parsed_id = 0;
		ok = parse_u32(row[0], &parsed_id) && parse_double(row[1], counter);
		*id = parsed_id;
	}
	else
	{
		*id = 0;
		*counter = 0;
	}
	if (rows)
		mysql_free_result(rows);
	return ok;
}
} // namespace

bool boon_reward_repository_execute(MYSQL *connection, const critical_command &command,
				    boon_reward_result *result, unsigned int *result_code,
				    bool *mutation_applied)
{
	if (!connection || !result || !result_code || !mutation_applied)
		return false;
	boon_reward_payload payload = {};
	if (!boon_reward_command_decode_payload(command, &payload))
	{
		errno = EINVAL;
		return false;
	}
	*result = {};
	result->pid = payload.pid;
	*result_code = 0;
	*mutation_applied = false;
	std::vector<boon_row> boons;
	if (!load_boons(connection, payload, &boons))
		return false;
	const std::string operation =
		bytes_hex(command.operation_id.bytes.data(), command.operation_id.bytes.size());
	if (!execute(connection,
		     "INSERT INTO boon_reward_outcome(operation_id,pid,`option`,event_value,entry_count) "
		     "VALUES(UNHEX('" +
			     operation + "')," + std::to_string(payload.pid) + "," +
			     std::to_string(payload.option) + "," + std::to_string(payload.data) +
			     "," + std::to_string(boons.size()) + ")"))
		return false;
	for (size_t index = 0; index < boons.size(); ++index)
	{
		const boon_row &boon = boons[index];
		uint64_t progress_id = 0;
		double counter = 0;
		if (!load_progress(connection, boon.id, payload.pid, &progress_id, &counter))
			return false;
		bool completed = false;
		if (!progress_id)
		{
			counter = boon.option == BOPT_FRAGS ?
					  payload.data :
					  (boon.option == BOPT_RACE || boon.option == BOPT_MOB ?
						   1.0 :
						   0.0);
			if (!execute(connection,
				     "INSERT INTO boons_progress(boonid,pid,counter) VALUES(" +
					     std::to_string(boon.id) + "," +
					     std::to_string(payload.pid) + "," +
					     std::to_string(counter) + ")"))
				return false;
			progress_id = mysql_insert_id(connection);
		}
		else if (progress_option(boon.option) && counter != -1)
		{
			counter += boon.option == BOPT_FRAGS ? payload.data : 1.0;
		}
		if (counter != -1)
			completed = !progress_option(boon.option) || counter >= boon.criteria;
		const double stored_counter = completed ? (boon.repeat ? 0.0 : -1.0) : counter;
		if (!execute(connection,
			     "UPDATE boons_progress SET counter=" + std::to_string(stored_counter) +
				     " WHERE id=" + std::to_string(progress_id)))
			return false;
		if (completed && boon.type == BTYPE_STATS &&
		    !execute(connection,
			     "INSERT INTO boons_shop(pid,points,stats) VALUES(" +
				     std::to_string(payload.pid) + ",0," +
				     std::to_string(static_cast<int>(boon.bonus)) +
				     ") ON DUPLICATE KEY UPDATE stats=stats+VALUES(stats)"))
			return false;
		if (completed && boon.type == BTYPE_POINT &&
		    !execute(connection,
			     "INSERT INTO boons_shop(pid,points,stats) VALUES(" +
				     std::to_string(payload.pid) + "," +
				     std::to_string(static_cast<int>(boon.bonus)) +
				     ",0) ON DUPLICATE KEY UPDATE points=points+VALUES(points)"))
			return false;
		if (completed && boon.target_pid == payload.pid && !boon.repeat &&
		    !execute(connection, "UPDATE boons SET active=0,duration=0 WHERE id=" +
						 std::to_string(boon.id)))
			return false;
		const uint8_t flags = (progress_option(boon.option) ? BOON_RESULT_PROGRESS : 0) |
				      (completed ? BOON_RESULT_COMPLETED : 0);
		if (!execute(connection,
			     "INSERT INTO boon_reward_outcome_entry(operation_id,entry_index,boon_id,"
			     "counter_after,completed,reward_type,reward_value) VALUES(UNHEX('" +
				     operation + "')," + std::to_string(index) + "," +
				     std::to_string(boon.id) + "," +
				     std::to_string(stored_counter) + "," +
				     std::to_string(completed) + "," + std::to_string(boon.type) +
				     "," + std::to_string(boon.bonus) + ")"))
			return false;
		result->entries[index] = { boon.id,
					   boon.type,
					   boon.option,
					   flags,
					   static_cast<uint8_t>(boon.repeat),
					   boon.criteria,
					   boon.criteria2,
					   boon.bonus,
					   boon.bonus2,
					   stored_counter };
	}
	result->entry_count = static_cast<uint16_t>(boons.size());
	*mutation_applied = true;
	return true;
}
