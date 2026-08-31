#include "world/zone_touch_repository.h"

#include <cerrno>
#include <mysql.h>
#include <string>

namespace
{
bool execute(MYSQL *connection, const std::string &sql)
{
	return mysql_real_query(connection, sql.data(), sql.size()) == 0;
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
} // namespace

bool zone_touch_repository_execute(MYSQL *connection, const critical_command &command,
				   zone_touch_result *result, unsigned int *result_code,
				   bool *mutation_applied)
{
	if (!connection || !result || !result_code || !mutation_applied ||
	    !zone_touch_command_decode_payload(command, result))
	{
		errno = EINVAL;
		return false;
	}
	*result_code = 0;
	*mutation_applied = false;
	if (!execute(connection, "SELECT number FROM zones WHERE number=" +
					 std::to_string(result->zone_number) + " FOR UPDATE"))
		return false;
	MYSQL_RES *rows = mysql_store_result(connection);
	const bool found = rows && mysql_fetch_row(rows);
	if (rows)
		mysql_free_result(rows);
	if (!found)
	{
		errno = ENOENT;
		return false;
	}
	const std::string operation =
		bytes_hex(command.operation_id.bytes.data(), command.operation_id.bytes.size());
	std::string update_zone = "UPDATE zones SET last_touch=FROM_UNIXTIME(" +
				  std::to_string(result->touched_at) + ")";
	if (result->alignment_delta)
		update_zone += ",alignment=LEAST(5,GREATEST(-5,alignment+(" +
			       std::to_string(result->alignment_delta) + ")))";
	if (result->reset_requested)
		update_zone += ",reset_perc=1";
	update_zone += " WHERE number=" + std::to_string(result->zone_number);
	if (!execute(connection, update_zone) ||
	    !execute(connection,
		     "INSERT INTO zone_touches(boot_time,touched_at,zone_number,toucher_pid,"
		     "group_size,epic_value,alignment_delta) VALUES(FROM_UNIXTIME(" +
			     std::to_string(result->boot_time) + "),FROM_UNIXTIME(" +
			     std::to_string(result->touched_at) + ")," +
			     std::to_string(result->zone_number) + "," +
			     std::to_string(result->toucher_pid) + "," +
			     std::to_string(result->group_size) + "," +
			     std::to_string(result->epic_value) + "," +
			     std::to_string(result->alignment_delta) + ")") ||
	    !execute(
		    connection,
		    "INSERT INTO zone_touch_outcome(operation_id,zone_number,toucher_pid,boot_time,"
		    "touched_at,group_size,epic_value,alignment_delta,reset_requested) VALUES(UNHEX('" +
			    operation + "')," + std::to_string(result->zone_number) + "," +
			    std::to_string(result->toucher_pid) + "," +
			    std::to_string(result->boot_time) + "," +
			    std::to_string(result->touched_at) + "," +
			    std::to_string(result->group_size) + "," +
			    std::to_string(result->epic_value) + "," +
			    std::to_string(result->alignment_delta) + "," +
			    std::to_string(result->reset_requested) + ")"))
		return false;
	for (size_t index = 0; index < result->group_size; ++index)
		if (!execute(connection, "INSERT INTO zone_touch_outcome_participant(operation_id,"
					 "participant_index,pid,epic_value) VALUES(UNHEX('" +
						 operation + "')," + std::to_string(index) + "," +
						 std::to_string(result->participant_pids[index]) +
						 "," + std::to_string(result->epic_value) + ")"))
			return false;
	*mutation_applied = true;
	return true;
}
