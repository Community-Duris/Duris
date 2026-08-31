#include "persistence/critical_command_repository.h"
#include "epic_command.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mysql.h>
#include <string>
#include <vector>

extern "C" MYSQL *sql_pool_acquire(void)
{
	return nullptr;
}
extern "C" void sql_pool_release(MYSQL *) {}
extern "C" MYSQL *sql_pool_replace_connection(MYSQL *)
{
	return nullptr;
}

static MYSQL *connection = nullptr;

static void execute(const std::string &sql)
{
	assert(mysql_real_query(connection, sql.data(), sql.size()) == 0);
}

static unsigned long long scalar(const std::string &sql)
{
	execute(sql);
	MYSQL_RES *result = mysql_store_result(connection);
	assert(result);
	MYSQL_ROW row = mysql_fetch_row(result);
	assert(row && row[0]);
	const unsigned long long value = strtoull(row[0], nullptr, 10);
	mysql_free_result(result);
	return value;
}

static std::string operation_hex(const critical_operation_id &operation_id)
{
	char encoded[CRITICAL_COMMAND_ID_HEX_SIZE] = {};
	assert(critical_operation_id_to_hex(operation_id, encoded, sizeof(encoded)));
	return encoded;
}

static critical_command command_for(uint32_t pid, int64_t delta, epic_reason_type reason,
				    uint16_t flags, int64_t reason_id)
{
	critical_operation_id operation_id = {};
	assert(critical_operation_id_generate(&operation_id));
	critical_command command = {};
	assert(epic_command_build(&command, operation_id,
				  { .pid = pid,
				    .delta = delta,
				    .reason = reason,
				    .flags = flags,
				    .reason_id = reason_id },
				  std::numeric_limits<uint64_t>::max(),
				  critical_source_site::command,
				  critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	return command;
}

static epic_command_result result_of(const critical_apply_result &applied)
{
	epic_command_result result = {};
	assert(epic_command_decode_result(applied.result_payload.data(), applied.result_size,
					  &result));
	return result;
}

int main()
{
	connection = mysql_init(nullptr);
	assert(connection);
	const char *port_text = getenv("DB_PORT");
	assert(mysql_real_connect(
		connection, getenv("DB_HOST"), getenv("DB_USER"), getenv("DB_PASSWD"),
		getenv("EPIC_TEST_DB_NAME"),
		static_cast<unsigned int>(strtoul(port_text ? port_text : "3306", nullptr, 10)),
		nullptr, 0));
	execute("DELETE FROM player_data WHERE name='__epic_transaction_test__'");
	execute("INSERT INTO player_data(name,epics,epic_revision,active) "
		"VALUES('__epic_transaction_test__',100,0,0)");
	const uint32_t pid = static_cast<uint32_t>(mysql_insert_id(connection));
	std::vector<std::string> operations;

	critical_command award = command_for(pid, 25, epic_reason_type::quest_award, 0, 77);
	operations.push_back(operation_hex(award.operation_id));
	critical_apply_result applied = critical_command_repository_apply(connection, award);
	assert(applied.outcome == critical_apply_outcome::applied && !applied.error_code);
	epic_command_result result = result_of(applied);
	assert(result.balance == 125 && result.revision == 1 && result.delta == 25);
	critical_apply_result duplicate = critical_command_repository_apply(connection, award);
	assert(duplicate.outcome == critical_apply_outcome::already_applied);
	result = result_of(duplicate);
	assert(result.balance == 125 && result.revision == 1);
	critical_command mismatch = award;
	mismatch.payload[4] = 26;
	assert(critical_command_repository_apply(connection, mismatch).error_code == EEXIST);

	critical_command spend = command_for(pid, -40, epic_reason_type::store_purchase,
					     EPIC_COMMAND_REQUIRE_FUNDS, 901);
	operations.push_back(operation_hex(spend.operation_id));
	applied = critical_command_repository_apply(connection, spend);
	assert(applied.outcome == critical_apply_outcome::applied);
	result = result_of(applied);
	assert(result.balance == 85 && result.revision == 2 && result.delta == -40);

	critical_command rejected = command_for(pid, -100, epic_reason_type::store_purchase,
						EPIC_COMMAND_REQUIRE_FUNDS, 902);
	operations.push_back(operation_hex(rejected.operation_id));
	applied = critical_command_repository_apply(connection, rejected);
	assert(applied.outcome == critical_apply_outcome::terminal_failure &&
	       applied.error_code == ENOSPC);
	result = result_of(applied);
	assert(result.balance == 85 && result.revision == 2 && result.delta == -100);
	duplicate = critical_command_repository_apply(connection, rejected);
	assert(duplicate.outcome == critical_apply_outcome::terminal_failure &&
	       duplicate.error_code == ENOSPC);

	assert(scalar("SELECT epics FROM player_data WHERE pid=" + std::to_string(pid)) == 85);
	assert(scalar("SELECT epic_revision FROM player_data WHERE pid=" + std::to_string(pid)) ==
	       2);
	assert(scalar("SELECT COUNT(*) FROM epic_ledger WHERE pid=" + std::to_string(pid)) == 2);
	assert(scalar("SELECT opening_balance+COALESCE((SELECT SUM(delta) FROM epic_ledger l "
		      "WHERE l.pid=b.pid),0) FROM epic_balance_baseline b WHERE pid=" +
		      std::to_string(pid)) == 85);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox o JOIN epic_ledger l "
		      "ON l.operation_id=o.operation_id WHERE l.pid=" +
		      std::to_string(pid)) == 2);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE operation_id=UNHEX('" +
		      operation_hex(rejected.operation_id) + "')") == 0);

	for (const std::string &operation : operations)
	{
		execute("DELETE d FROM critical_outbox_delivery_dedupe d JOIN critical_outbox o "
			"ON o.outbox_id=d.outbox_id WHERE o.operation_id=UNHEX('" +
			operation + "')");
		execute("DELETE FROM critical_outbox WHERE operation_id=UNHEX('" + operation +
			"')");
		execute("DELETE FROM epic_ledger WHERE operation_id=UNHEX('" + operation + "')");
		execute("DELETE FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
			operation + "')");
	}
	execute("DELETE FROM epic_balance_baseline WHERE pid=" + std::to_string(pid));
	execute("DELETE FROM player_data WHERE pid=" + std::to_string(pid));
	mysql_close(connection);
	return 0;
}
