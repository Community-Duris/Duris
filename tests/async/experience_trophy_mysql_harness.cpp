#include "player/player_load_repository.h"
#include "player/player_snapshot_repository.h"
#include "persistence/persistence_observability.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
MYSQL *connect()
{
	assert(std::string(std::getenv("DB_NAME")) == "experience_trophy_test");
	MYSQL *connection = mysql_init(nullptr);
	assert(connection &&
	       mysql_real_connect(connection, "127.0.0.1", "root", std::getenv("DB_PASSWD"),
				  "experience_trophy_test",
				  static_cast<unsigned int>(std::atoi(std::getenv("DB_PORT"))),
				  nullptr, 0));
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

player_load_result load(MYSQL *connection, int pid)
{
	player_load_request request{};
	request.request_id = 1;
	request.pid = pid;
	request.account_name = "trophy_test_account";
	request.deadline_usec = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	auto result = player_load_repository_execute(connection, request);
	if (result.outcome != player_load_outcome::applied)
		std::cerr << "load failed: component="
			  << (result.failed_component ? result.failed_component : "none")
			  << " queries=" << result.metrics.query_count
			  << " error=" << result.error_code << '\n';
	assert(result.outcome == player_load_outcome::applied);
	assert(result.metrics.query_count == PLAYER_LOAD_QUERY_MAX);
	return result;
}

int64_t xp(const player_snapshot &snapshot)
{
	for (const auto &field : snapshot.status_integers)
		if (field.field == player_status_field::experience)
			return field.signed_value;
	std::abort();
}

void set_xp(player_snapshot &snapshot, int64_t value)
{
	for (auto &field : snapshot.status_integers)
		if (field.field == player_status_field::experience)
		{
			field.signed_value = value;
			return;
		}
	std::abort();
}
} // namespace

int main()
{
	MYSQL *connection = connect();
	execute(connection,
		"INSERT INTO accounts(account_name,password) VALUES('trophy_test_account','')");
	execute(connection,
		"INSERT INTO player_data(name,account_name,racewar,level,race,last_room) "
		"VALUES('TrophyTest','trophy_test_account',1,25,1,100)");
	const int pid = static_cast<int>(mysql_insert_id(connection));
	auto initial = load(connection, pid);
	assert(initial.snapshot.trophies.empty());
	player_snapshot checkpoint = initial.snapshot;
	checkpoint.components = PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_TROPHIES;
	checkpoint.revision = initial.snapshot.revision + 1;
	checkpoint.trophies = { { 12, 345 }, { 34, 678 } };
	set_xp(checkpoint, 1023);
	assert(player_snapshot_repository_apply(connection, checkpoint).outcome ==
	       player_save_apply_outcome::applied);
	const auto first_revision = checkpoint.revision;
	mysql_close(connection);
	connection = connect(); // No in-process snapshot cache can satisfy this reload.
	auto restored = load(connection, pid);
	assert(restored.snapshot.revision == first_revision && xp(restored.snapshot) == 1023);
	assert(restored.snapshot.trophies.size() == 2 &&
	       restored.snapshot.trophies[0].experience == 345 &&
	       restored.snapshot.trophies[1].experience == 678);
	assert(player_snapshot_repository_apply(connection, checkpoint).outcome ==
	       player_save_apply_outcome::already_applied);

	// Failure after status update and trophy deletion must roll both back.
	checkpoint.revision++;
	set_xp(checkpoint, 9999);
	checkpoint.trophies = { { 12, 9999 }, { 12, 9999 } };
	assert(player_snapshot_repository_apply(connection, checkpoint).outcome ==
	       player_save_apply_outcome::terminal_failure);
	restored = load(connection, pid);
	assert(restored.snapshot.revision == first_revision && xp(restored.snapshot) == 1023 &&
	       restored.snapshot.trophies.size() == 2 &&
	       restored.snapshot.trophies[0].experience == 345);

	checkpoint.trophies = { { 12, 9999 } };
	assert(player_snapshot_repository_apply(connection, checkpoint).outcome ==
	       player_save_apply_outcome::applied);
	checkpoint.revision = first_revision;
	assert(player_snapshot_repository_apply(connection, checkpoint).outcome ==
	       player_save_apply_outcome::stale_revision);
	restored = load(connection, pid);
	assert(xp(restored.snapshot) == 9999 && restored.snapshot.trophies.size() == 1 &&
	       restored.snapshot.trophies[0].experience == 9999);
	checkpoint.revision = first_revision + 2;
	checkpoint.trophies.clear();
	assert(player_snapshot_repository_apply(connection, checkpoint).outcome ==
	       player_save_apply_outcome::applied);
	assert(load(connection, pid).snapshot.trophies.empty());
	mysql_close(connection);
	std::cout << "experience trophy SQL checkpoint/reload/retry/rollback tests passed\n";
}
