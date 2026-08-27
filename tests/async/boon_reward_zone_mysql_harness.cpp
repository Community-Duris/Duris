#include "boon_reward_command.h"
#include "critical_command_repository.h"
#include "zone_touch_command.h"

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
constexpr uint32_t PID = 2147000311U;
constexpr uint32_t PARTICIPANT_PID = 2147000312U;
constexpr int ZONE = 2147000313;

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
	execute(db, "DELETE FROM critical_outbox WHERE operation_id IN (SELECT operation_id FROM "
		    "boon_reward_outcome WHERE pid=" +
			    std::to_string(PID) +
			    ") OR operation_id IN "
			    "(SELECT operation_id FROM zone_touch_outcome WHERE zone_number=" +
			    std::to_string(ZONE) + ")");
	execute(db, "DELETE FROM zone_touch_outcome_participant WHERE pid IN (" +
			    std::to_string(PID) + "," + std::to_string(PARTICIPANT_PID) + ")");
	execute(db, "DELETE FROM zone_touch_outcome WHERE zone_number=" + std::to_string(ZONE));
	execute(db,
		"DELETE FROM boon_reward_outcome_entry WHERE operation_id IN (SELECT operation_id "
		"FROM boon_reward_outcome WHERE pid=" +
			std::to_string(PID) + ")");
	execute(db, "DELETE FROM boon_reward_outcome WHERE pid=" + std::to_string(PID));
	execute(db, "DELETE FROM critical_operation_inbox WHERE created_at>=@boon_zone_started "
		    "AND command_type IN (11,12)");
	execute(db, "DELETE FROM zone_touches WHERE zone_number=" + std::to_string(ZONE));
	execute(db, "DELETE FROM zones WHERE number=" + std::to_string(ZONE));
	execute(db, "DELETE FROM boons_progress WHERE pid=" + std::to_string(PID));
	execute(db, "DELETE FROM boons_shop WHERE pid=" + std::to_string(PID));
	execute(db, "DELETE FROM boons WHERE author='transaction-harness'");
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
	execute(db, "SET @boon_zone_started=CURRENT_TIMESTAMP(6)");
	cleanup(db);
	execute(db,
		"INSERT INTO boons(time,duration,racewar,type,opt,criteria,bonus,author,active) "
		"VALUES(0,0,0,10,2,55,7,'transaction-harness',1)");
	const uint64_t boon_id = mysql_insert_id(db);
	boon_reward_payload boon = { PID, 1, 55, ZONE, 2, 55, 0, 0, 0 };
	critical_operation_id boon_operation = {};
	assert(critical_operation_id_generate(&boon_operation));
	critical_command boon_command = {};
	assert(boon_reward_command_build(&boon_command, boon_operation, boon));
	boon_command.accepted_at_usec = 1;
	const critical_apply_result boon_applied =
		critical_command_repository_apply(db, boon_command);
	assert(boon_applied.outcome == critical_apply_outcome::applied);
	assert(scalar(db, "SELECT counter=-1 FROM boons_progress WHERE pid=" + std::to_string(PID) +
				  " AND boonid=" + std::to_string(boon_id)) == 1);
	assert(scalar(db, "SELECT points FROM boons_shop WHERE pid=" + std::to_string(PID)) == 7);
	assert(critical_command_repository_apply(db, boon_command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(scalar(db, "SELECT points FROM boons_shop WHERE pid=" + std::to_string(PID)) == 7);

	execute(db, "INSERT INTO zones(number,name,epic_type,alignment) VALUES(" +
			    std::to_string(ZONE) + ",'transaction harness',1,0)");
	zone_touch_payload zone = {};
	zone.zone_number = ZONE;
	zone.toucher_pid = PID;
	zone.boot_time = 1000;
	zone.touched_at = 1100;
	zone.group_size = 2;
	zone.participant_pids[0] = PID;
	zone.participant_pids[1] = PARTICIPANT_PID;
	zone.epic_value = 9;
	zone.alignment_delta = 1;
	zone.reset_requested = 1;
	critical_operation_id zone_operation = {};
	assert(critical_operation_id_generate(&zone_operation));
	critical_command zone_command = {};
	assert(zone_touch_command_build(&zone_command, zone_operation, zone));
	zone_command.accepted_at_usec = 2;
	assert(critical_command_repository_apply(db, zone_command).outcome ==
	       critical_apply_outcome::applied);
	assert(scalar(db, "SELECT alignment FROM zones WHERE number=" + std::to_string(ZONE)) == 1);
	assert(scalar(db, "SELECT COUNT(*) FROM zone_touches WHERE zone_number=" +
				  std::to_string(ZONE)) == 1);
	assert(scalar(db, "SELECT COUNT(*) FROM zone_touch_outcome_participant WHERE "
			  "operation_id IN (SELECT operation_id FROM zone_touch_outcome WHERE "
			  "zone_number=" +
				  std::to_string(ZONE) + ")") == 2);
	assert(critical_command_repository_apply(db, zone_command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(scalar(db, "SELECT COUNT(*) FROM zone_touches WHERE zone_number=" +
				  std::to_string(ZONE)) == 1);
	cleanup(db);
	mysql_close(db);
	std::cout << "boon reward and immutable group zone-touch apply/replay passed\n";
}
