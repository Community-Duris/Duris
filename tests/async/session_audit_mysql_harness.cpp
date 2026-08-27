#include "critical_command_repository.h"
#include "session_audit_command.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <mysql.h>
#include <string>

extern "C" MYSQL *sql_pool_acquire(void)
{
	return nullptr;
}
extern "C" void sql_pool_release(MYSQL *) {}
extern "C" MYSQL *sql_pool_replace_connection(MYSQL *)
{
	return nullptr;
}

namespace
{
constexpr uint32_t PID = 2147000411U;

void execute(MYSQL *db, const std::string &sql)
{
	if (mysql_real_query(db, sql.data(), sql.size()) != 0)
	{
		std::cerr << mysql_error(db) << "\nSQL: " << sql << '\n';
		std::abort();
	}
}

uint64_t scalar(MYSQL *db, const std::string &sql)
{
	execute(db, sql);
	MYSQL_RES *result = mysql_store_result(db);
	assert(result);
	MYSQL_ROW row = mysql_fetch_row(result);
	assert(row && row[0]);
	const uint64_t value = strtoull(row[0], nullptr, 10);
	mysql_free_result(result);
	return value;
}

void cleanup(MYSQL *db)
{
	execute(db, "DELETE FROM session_audit_outcome WHERE pid=" + std::to_string(PID));
	execute(db, "DELETE FROM critical_operation_inbox WHERE created_at>=@audit_started "
		    "AND command_type=13");
}
} // namespace

int main()
{
	MYSQL *db = mysql_init(nullptr);
	assert(db);
	const char *port_text = getenv("DB_PORT");
	const unsigned int port = port_text ? static_cast<unsigned int>(atoi(port_text)) : 3306;
	assert(mysql_real_connect(db, getenv("DB_HOST"), getenv("DB_USER"), getenv("DB_PASSWD"),
				  getenv("DB_NAME"), port, nullptr, 0));
	execute(db, "SET @audit_started=CURRENT_TIMESTAMP(6)");
	cleanup(db);
	critical_operation_id operation = {};
	assert(critical_operation_id_generate(&operation));
	critical_command command = {};
	assert(session_audit_command_build(&command, operation,
					   { PID, session_audit_event::login, 1700000000 }));
	command.accepted_at_usec = 1;
	const critical_apply_result applied = critical_command_repository_apply(db, command);
	assert(applied.outcome == critical_apply_outcome::applied);
	assert(scalar(db, "SELECT COUNT(*) FROM session_audit_outcome WHERE pid=" +
				  std::to_string(PID) + " AND event_type=1") == 1);
	assert(critical_command_repository_apply(db, command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(scalar(db, "SELECT COUNT(*) FROM session_audit_outcome WHERE pid=" +
				  std::to_string(PID)) == 1);
	cleanup(db);
	mysql_close(db);
	std::cout << "typed session audit apply and replay passed\n";
}
