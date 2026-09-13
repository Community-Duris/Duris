#include "player/player_load_repository.h"
#include "player/player_snapshot_repository.h"
#include "persistence/persistence_observability.h"
#include "net/output_preference_codec.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
MYSQL *connect()
{
	assert(std::string(std::getenv("DB_NAME")) == "colorization_test");
	MYSQL *connection = mysql_init(nullptr);
	assert(connection &&
	       mysql_real_connect(connection, std::getenv("DB_HOST"), "root",
				  std::getenv("DB_PASSWD"), "colorization_test",
				  static_cast<unsigned>(std::atoi(std::getenv("DB_PORT"))), nullptr,
				  0));
	return connection;
}
void execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
	{
		std::cerr << mysql_error(connection) << '\n';
		std::abort();
	}
}
player_snapshot load(MYSQL *connection, int pid)
{
	player_load_request request{};
	request.request_id = 1;
	request.pid = pid;
	request.account_name = "color_test_account";
	request.deadline_usec = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	auto result = player_load_repository_execute(connection, request);
	if (result.outcome != player_load_outcome::applied)
		std::cerr << "load failed: component="
			  << (result.failed_component ? result.failed_component : "none")
			  << " error=" << result.error_code << '\n';
	assert(result.outcome == player_load_outcome::applied);
	assert(result.metrics.query_count == PLAYER_LOAD_QUERY_MAX);
	return result.snapshot;
}
}
int main()
{
	MYSQL *connection = connect();
	execute(connection,
		"INSERT INTO accounts(account_name,password) VALUES('color_test_account','')");
	execute(connection,
		"INSERT INTO player_data(name,account_name,racewar,level,race,last_room) "
		"VALUES('ColorOne','color_test_account',1,25,1,100)");
	const int first_pid = static_cast<int>(mysql_insert_id(connection));
	execute(connection,
		"INSERT INTO player_data(name,account_name,racewar,level,race,last_room) "
		"VALUES('ColorTwo','color_test_account',1,25,1,100)");
	const int second_pid = static_cast<int>(mysql_insert_id(connection));
	auto first = load(connection, first_pid), second = load(connection, second_pid);
	assert(first.output_preferences.empty() && second.output_preferences.empty());
	first.components = second.components = PLAYER_COMPONENT_STATUS;
	++first.revision;
	++second.revision;
	first.output_preferences = "v1;m=1;1=3;12=27";
	second.output_preferences = "v1;12=20";
	for (const auto *snapshot : { &first, &second })
		assert(player_snapshot_repository_apply(connection, *snapshot).outcome ==
		       player_save_apply_outcome::applied);
	mysql_close(connection);
	connection = connect();
	assert(load(connection, first_pid).output_preferences == first.output_preferences);
	assert(load(connection, second_pid).output_preferences == second.output_preferences);
	assert(player_snapshot_repository_apply(connection, first).outcome ==
	       player_save_apply_outcome::already_applied);

	// A later component failing must roll back settings and the checkpoint revision together.
	auto failed = first;
	++failed.revision;
	failed.components |= PLAYER_COMPONENT_TROPHIES;
	failed.output_preferences.clear();
	failed.trophies = { { 12, 50 }, { 12, 60 } };
	assert(player_snapshot_repository_apply(connection, failed).outcome ==
	       player_save_apply_outcome::terminal_failure);
	assert(load(connection, first_pid).output_preferences == first.output_preferences);
	assert(load(connection, first_pid).revision == first.revision);

	// Reset one retains other choices and motion; reset all restores empty defaults.
	first.output_preferences = "v1;m=1;1=3";
	++first.revision;
	assert(player_snapshot_repository_apply(connection, first).outcome ==
	       player_save_apply_outcome::applied);
	assert(load(connection, first_pid).output_preferences == first.output_preferences);
	--first.revision;
	assert(player_snapshot_repository_apply(connection, first).outcome ==
	       player_save_apply_outcome::stale_revision);
	first.revision += 2;
	first.output_preferences.clear();
	assert(player_snapshot_repository_apply(connection, first).outcome ==
	       player_save_apply_outcome::applied);
	assert(load(connection, first_pid).output_preferences.empty());
	assert(load(connection, second_pid).output_preferences == second.output_preferences);
	execute(connection,
		"UPDATE player_data SET output_preferences='v1;m=1;1=3;12=99;999=27' WHERE pid=" +
			std::to_string(first_pid));
	auto invalid = decode_output_preferences(load(connection, first_pid).output_preferences);
	assert(invalid.motion_off && invalid.choices[1] == 3 && invalid.choices[12] == 0);
	execute(connection, "UPDATE player_data SET output_preferences='v99;m=1;12=27' WHERE pid=" +
				    std::to_string(first_pid));
	assert(decode_output_preferences(load(connection, first_pid).output_preferences) ==
	       OutputPreferenceState{});
	mysql_close(connection);
	std::cout
		<< "Output preferences SQL: two characters, defaults, reconnect, retry, rollback, resets and invalid values passed\n";
}
