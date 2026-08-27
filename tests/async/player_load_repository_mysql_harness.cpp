#include "player_load_repository.h"
#include "persistence_observability.h"

#include <mysql/mysql.h>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <strings.h>

namespace
{
const char *required_env(const char *name)
{
	const char *value = std::getenv(name);
	assert(value && *value);
	return value;
}
}

int main()
{
	MYSQL *connection = mysql_init(nullptr);
	assert(connection);
	assert(mysql_real_connect(
		connection, required_env("DB_HOST"), required_env("DB_USER"),
		required_env("DB_PASSWD"), required_env("DB_NAME"),
		static_cast<unsigned int>(std::strtoul(required_env("DB_PORT"), nullptr, 10)),
		nullptr, CLIENT_MULTI_STATEMENTS));
	std::string character = required_env("GAME_ACCOUNT_CHARACTER_NAME");
	std::string escaped(character.size() * 2 + 1, '\0');
	escaped.resize(mysql_real_escape_string(connection, escaped.data(), character.data(),
						character.size()));
	const std::string pid_sql = "SELECT pid FROM player_data WHERE name='" + escaped + "'";
	assert(mysql_real_query(connection, pid_sql.data(), pid_sql.size()) == 0);
	MYSQL_RES *rows = mysql_store_result(connection);
	assert(rows);
	MYSQL_ROW row = mysql_fetch_row(rows);
	assert(row && row[0]);
	const int pid = std::atoi(row[0]);
	mysql_free_result(rows);

	const uint64_t now = persistence_observability_now_usec();
	player_load_request request = {
		PLAYER_LOAD_SCHEMA_VERSION,	77, pid, required_env("GAME_ACCOUNT_NAME"),
		now + PLAYER_LOAD_TIMEOUT_USEC, {}
	};
	assert(player_load_request_valid(request, now));
	player_load_result result = player_load_repository_execute(connection, request);
	if (result.outcome != player_load_outcome::applied)
		std::cerr << "outcome=" << static_cast<int>(result.outcome)
			  << " error=" << result.error_code << " mysql=" << mysql_errno(connection)
			  << " queries=" << result.metrics.query_count << '\n';
	assert(result.outcome == player_load_outcome::applied);
	assert(result.request_id == request.request_id && result.pid == pid);
	assert(result.snapshot.pid == pid);
	assert(result.snapshot.components == PLAYER_LOAD_SESSION01_COMPONENTS);
	assert(result.metrics.query_count == 14);
	assert(result.metrics.row_count > 0 &&
	       result.metrics.row_count <= PLAYER_SNAPSHOT_MAX_ROWS);
	assert(result.metrics.byte_count > 0 &&
	       result.metrics.byte_count <= PLAYER_SNAPSHOT_MAX_BYTES);
	assert(result.metrics.transaction_usec <= PLAYER_LOAD_TIMEOUT_USEC);
	assert(result.snapshot.status_strings.size() == 7);
	assert(result.snapshot.status_integers.size() == 63);
	assert(strcasecmp(result.account_name.c_str(), required_env("GAME_ACCOUNT_NAME")) == 0);

	const std::string values_sql =
		"SELECT copper,silver,gold,platinum,epics,frags,oldfrags,wallet_revision,"
		"epic_revision,frag_revision FROM player_data WHERE pid=" +
		std::to_string(pid);
	assert(mysql_real_query(connection, values_sql.data(), values_sql.size()) == 0);
	rows = mysql_store_result(connection);
	assert(rows);
	row = mysql_fetch_row(rows);
	assert(row);
	assert(result.domains.wallet[0] == std::strtoull(row[0], nullptr, 10));
	assert(result.domains.wallet[1] == std::strtoull(row[1], nullptr, 10));
	assert(result.domains.wallet[2] == std::strtoull(row[2], nullptr, 10));
	assert(result.domains.wallet[3] == std::strtoull(row[3], nullptr, 10));
	assert(result.domains.epics == std::strtoll(row[4], nullptr, 10));
	assert(result.domains.frags == std::strtoll(row[5], nullptr, 10));
	assert(result.domains.old_frags == std::strtoll(row[6], nullptr, 10));
	assert(result.domains.wallet_revision == std::strtoull(row[7], nullptr, 10));
	assert(result.domains.epic_revision == std::strtoull(row[8], nullptr, 10));
	assert(result.domains.frag_revision == std::strtoull(row[9], nullptr, 10));
	mysql_free_result(rows);

	player_load_request by_name = {};
	by_name.request_id = 78;
	by_name.player_name = character;
	by_name.deadline_usec = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	player_load_result name_result = player_load_repository_execute(connection, by_name);
	assert(name_result.outcome == player_load_outcome::applied);
	assert(name_result.pid == pid && name_result.request_id == by_name.request_id);

	player_load_request wrong_account = request;
	wrong_account.request_id = 79;
	wrong_account.account_name = "definitely-not-the-configured-account";
	wrong_account.deadline_usec =
		persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	player_load_result rejected = player_load_repository_execute(connection, wrong_account);
	assert(rejected.outcome == player_load_outcome::component_failure);

	player_load_request missing = request;
	missing.request_id = 80;
	missing.pid = 2147483647;
	missing.deadline_usec = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	player_load_result absent = player_load_repository_execute(connection, missing);
	assert(absent.outcome == player_load_outcome::not_found);

	request.request_id = 0;
	assert(!player_load_request_valid(request, persistence_observability_now_usec()));
	mysql_close(connection);
	std::cout << "consistent player-load repository snapshot passed\n";
}
