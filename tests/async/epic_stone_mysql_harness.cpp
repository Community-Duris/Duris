#include "persistence/critical_command_repository.h"
#include "world/zone_touch_command.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
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

namespace
{
MYSQL *db = nullptr;
unsigned int operation_serial = 0;

void execute(const std::string &sql)
{
	if (mysql_real_query(db, sql.data(), sql.size()) != 0)
	{
		// SQL and connection details are deliberately excluded from diagnostics.
		std::cerr << "Synthetic fixture SQL failed with code " << mysql_errno(db) << '\n';
		std::abort();
	}
}

uint64_t scalar(const std::string &sql)
{
	execute(sql);
	MYSQL_RES *rows = mysql_store_result(db);
	assert(rows);
	MYSQL_ROW row = mysql_fetch_row(rows);
	assert(row && row[0]);
	const auto value = strtoull(row[0], nullptr, 10);
	mysql_free_result(rows);
	return value;
}

void migration(const char *path)
{
	std::ifstream file(path);
	assert(file.good());
	std::string sql((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	execute(sql);
	int next = 0;
	do
	{
		MYSQL_RES *rows = mysql_store_result(db);
		if (rows)
			mysql_free_result(rows);
		else
			assert(mysql_field_count(db) == 0);
		next = mysql_next_result(db);
		assert(next <= 0);
	} while (next == 0);
}

std::string hex(const critical_operation_id &id)
{
	char value[CRITICAL_COMMAND_ID_HEX_SIZE] = {};
	assert(critical_operation_id_to_hex(id, value, sizeof(value)));
	return value;
}

std::string operation_where(const critical_command &command)
{
	return "operation_id=UNHEX('" + hex(command.operation_id) + "')";
}

critical_command make_command(const zone_touch_payload &payload)
{
	critical_operation_id id = {};
	++operation_serial;
	id.bytes[0] = static_cast<uint8_t>(operation_serial);
	id.bytes[1] = static_cast<uint8_t>(operation_serial >> 8);
	id.bytes[15] = 166;
	critical_command command;
	assert(zone_touch_command_build(&command, id, payload));
	command.accepted_at_usec = 1;
	return command;
}

zone_touch_payload fixture(uint32_t base, unsigned int count, bool record_zone = true)
{
	zone_touch_payload payload = {};
	payload.zone_number = record_zone ? base : 0;
	payload.stone_uid = base;
	payload.stone_level = 56;
	payload.record_zone = record_zone;
	payload.toucher_pid = base;
	payload.boot_time = 1000;
	payload.touched_at = 1100;
	payload.epic_value = 9;
	payload.alignment_delta = 1;
	payload.reset_requested = 1;
	payload.group_size = count;
	for (unsigned int i = 0; i < count; ++i)
	{
		payload.participant_pids[i] = base + i;
		payload.awards[i] = { static_cast<int32_t>(10 + i), static_cast<int32_t>(i), 0 };
		execute("INSERT INTO player_data(pid,epics,epic_revision) VALUES(" +
			std::to_string(base + i) + ",100,0)");
	}
	if (record_zone)
		execute("INSERT INTO zones(number,alignment,reset_perc) VALUES(" +
			std::to_string(base) + ",0,0)");
	return payload;
}

zone_touch_result receipt(const critical_apply_result &applied)
{
	zone_touch_result result;
	assert(zone_touch_command_decode_result(applied.result_payload.data(), applied.result_size,
						&result));
	return result;
}

void verify_awards(const zone_touch_payload &payload, const critical_command &command,
		   const zone_touch_result &result)
{
	assert(result.stone_uid == payload.stone_uid && result.group_size == payload.group_size);
	for (size_t i = 0; i < payload.group_size; ++i)
	{
		const auto pid = std::to_string(payload.participant_pids[i]);
		assert(scalar("SELECT epics FROM player_data WHERE pid=" + pid) == 110 + i);
		assert(scalar("SELECT epic_revision FROM player_data WHERE pid=" + pid) == 1);
		assert(result.balances[i] == 110 + static_cast<int64_t>(i) &&
		       result.revisions[i] == 1);
		assert(scalar("SELECT opening_balance FROM epic_balance_baseline WHERE pid=" +
			      pid) == 100);
		critical_command child;
		assert(zone_touch_award_command(command, i, &child));
		assert(scalar("SELECT COUNT(*) FROM epic_ledger WHERE " + operation_where(child) +
			      " AND pid=" + pid + " AND delta=" + std::to_string(10 + i)) == 1);
		assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE " +
			      operation_where(child)) == 1);
	}
	assert(scalar("SELECT COUNT(*) FROM epic_stone_claim WHERE stone_uid=" +
		      std::to_string(payload.stone_uid)) == 1);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE " + operation_where(command)) ==
	       1);
	const auto zone = std::to_string(payload.zone_number);
	assert(scalar("SELECT COUNT(*) FROM zone_touches WHERE zone_number=" + zone) ==
	       payload.record_zone);
	assert(scalar("SELECT COUNT(*) FROM zone_touch_outcome_participant WHERE " +
		      operation_where(command)) == (payload.record_zone ? payload.group_size : 0));
	if (payload.record_zone)
	{
		assert(scalar("SELECT alignment FROM zones WHERE number=" + zone) == 1);
		assert(scalar("SELECT reset_perc FROM zones WHERE number=" + zone) == 1);
		assert(scalar("SELECT UNIX_TIMESTAMP(last_touch) FROM zones WHERE number=" +
			      zone) == 1100);
	}
}

void verify_rollback(const zone_touch_payload &payload, const critical_command &command)
{
	for (size_t i = 0; i < payload.group_size; ++i)
	{
		const auto pid = std::to_string(payload.participant_pids[i]);
		assert(scalar("SELECT COUNT(*) FROM player_data WHERE pid=" + pid +
			      " AND epic_revision<>0") == 0);
		assert(scalar("SELECT COUNT(*) FROM epic_balance_baseline WHERE pid=" + pid) == 0);
		assert(scalar("SELECT COUNT(*) FROM epic_ledger WHERE pid=" + pid) == 0);
		critical_command child;
		assert(zone_touch_award_command(command, i, &child));
		assert(scalar("SELECT COUNT(*) FROM critical_operation_inbox WHERE " +
			      operation_where(child)) == 0);
		assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE " +
			      operation_where(child)) == 0);
	}
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE " + operation_where(command)) ==
	       0);
	assert(scalar("SELECT COUNT(*) FROM epic_stone_claim WHERE stone_uid=" +
		      std::to_string(payload.stone_uid)) == 0);
	const auto zone = std::to_string(payload.zone_number);
	assert(scalar("SELECT COUNT(*) FROM zone_touches WHERE zone_number=" + zone) == 0);
	assert(scalar("SELECT COUNT(*) FROM zone_touch_outcome WHERE " +
		      operation_where(command)) == 0);
	assert(scalar("SELECT COUNT(*) FROM zone_touch_outcome_participant WHERE " +
		      operation_where(command)) == 0);
	if (payload.record_zone)
		assert(scalar("SELECT COUNT(*) FROM zones WHERE number=" + zone +
			      " AND alignment=0 AND reset_perc=0 AND last_touch IS NULL") == 1);
}

void success_and_replay(uint32_t base, unsigned int count, bool record_zone = true)
{
	auto payload = fixture(base, count, record_zone);
	if (count > 1)
	{
		std::swap(payload.participant_pids[0], payload.participant_pids[count - 1]);
		payload.toucher_pid = payload.participant_pids[0];
	}
	auto command = make_command(payload);
	auto applied = critical_command_repository_apply(db, command);
	assert(applied.outcome == critical_apply_outcome::applied && applied.error_code == 0);
	auto result = receipt(applied);
	assert(!result.recovered_claim);
	verify_awards(payload, command, result);
	const auto outbox_count = scalar("SELECT COUNT(*) FROM critical_outbox");
	auto duplicate = critical_command_repository_apply(db, command);
	assert(duplicate.outcome == critical_apply_outcome::already_applied);
	assert(duplicate.result_size == applied.result_size &&
	       duplicate.result_payload == applied.result_payload);
	verify_awards(payload, command, receipt(duplicate));
	// A fresh command for a surviving object must recover the ORIGINAL awards.
	auto altered = payload;
	altered.awards[0].amount += 99;
	const auto ledger_count = scalar("SELECT COUNT(*) FROM epic_ledger");
	auto retouch = make_command(altered);
	duplicate = critical_command_repository_apply(db, retouch);
	assert(duplicate.outcome == critical_apply_outcome::applied && !duplicate.error_code);
	result = receipt(duplicate);
	assert(result.recovered_claim && result.awards[0].amount == payload.awards[0].amount);
	verify_awards(payload, command, result);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox") == outbox_count + 1);
	assert(scalar("SELECT COUNT(*) FROM epic_stone_claim WHERE " + operation_where(retouch)) ==
	       0);
	duplicate = critical_command_repository_apply(db, retouch);
	assert(duplicate.outcome == critical_apply_outcome::already_applied);
	assert(receipt(duplicate).recovered_claim);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE " + operation_where(retouch)) ==
	       1);
	assert(scalar("SELECT COUNT(*) FROM epic_ledger") == ledger_count);
	assert(scalar("SELECT COUNT(*) FROM critical_operation_inbox i WHERE status=1 AND result_code=0 "
		      "AND NOT EXISTS (SELECT 1 FROM critical_outbox o WHERE o.operation_id=i.operation_id)") ==
	       0);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox") == outbox_count + 1);
}

void legacy_metadata_only()
{
	auto payload = fixture(1000, 2);
	payload.stone_uid = 0;
	auto command = make_command(payload);
	assert(command.payload_version == 1);
	const auto ledger_count = scalar("SELECT COUNT(*) FROM epic_ledger");
	const auto claim_count = scalar("SELECT COUNT(*) FROM epic_stone_claim");
	const auto applied = critical_command_repository_apply(db, command);
	assert(applied.outcome == critical_apply_outcome::applied && !applied.error_code);
	assert(receipt(applied).stone_uid == 0);
	assert(scalar("SELECT COUNT(*) FROM epic_ledger") == ledger_count);
	assert(scalar("SELECT COUNT(*) FROM epic_stone_claim") == claim_count);
	assert(scalar("SELECT COUNT(*) FROM player_data WHERE pid IN (1000,1001) AND epics=100 AND epic_revision=0") ==
	       2);
	assert(scalar("SELECT COUNT(*) FROM zone_touches WHERE zone_number=1000") == 1);
	assert(scalar("SELECT COUNT(*) FROM zone_touch_outcome_participant WHERE " +
		      operation_where(command)) == 2);
	assert(critical_command_repository_apply(db, command).outcome ==
	       critical_apply_outcome::already_applied);
	assert(scalar("SELECT COUNT(*) FROM zone_touches WHERE zone_number=1000") == 1);
}
void rejection(uint32_t base, bool missing, bool revision_overflow = false)
{
	auto payload = fixture(base, 15);
	const auto last = std::to_string(base + 14);
	if (missing)
		execute("DELETE FROM player_data WHERE pid=" + last);
	else if (revision_overflow)
		execute("UPDATE player_data SET epic_revision=18446744073709551615 WHERE pid=" +
			last);
	else
		execute("UPDATE player_data SET epics=9223372036854775807 WHERE pid=" + last);
	auto command = make_command(payload);
	auto applied = critical_command_repository_apply(db, command);
	assert(applied.outcome == critical_apply_outcome::terminal_failure);
	assert(applied.error_code == static_cast<unsigned int>(missing ? ENOENT : ERANGE));
	// Normalize only the deliberately invalid fixture revision before checking rollback.
	if (revision_overflow)
	{
		assert(scalar("SELECT epic_revision FROM player_data WHERE pid=" + last) ==
		       UINT64_MAX);
		execute("UPDATE player_data SET epic_revision=0 WHERE pid=" + last);
	}
	verify_rollback(payload, command);
	assert(scalar("SELECT epics FROM player_data WHERE pid=" + std::to_string(base)) == 100);
	if (!missing && !revision_overflow)
		assert(scalar("SELECT epics FROM player_data WHERE pid=" + last) == INT64_MAX);
	if (!missing)
	{
		auto repeated = critical_command_repository_apply(db, command);
		assert(repeated.outcome == critical_apply_outcome::terminal_failure &&
		       repeated.error_code == ERANGE);
		verify_rollback(payload, command);
	}
	// Repair fixture, then prove the failed attempt did not consume the stone UID.
	if (missing)
		execute("INSERT INTO player_data(pid,epics,epic_revision) VALUES(" + last +
			",100,0)");
	else
		execute("UPDATE player_data SET epics=100 WHERE pid=" + last);
	auto retry = make_command(payload);
	auto recovered = critical_command_repository_apply(db, retry);
	assert(recovered.outcome == critical_apply_outcome::applied && !recovered.error_code);
	verify_awards(payload, retry, receipt(recovered));
}

void injected_failure(uint32_t base, const char *table, const char *timing)
{
	auto payload = fixture(base, 15);
	auto command = make_command(payload);
	// Fail only the parent, after every child ledger/outbox has been written.
	execute("CREATE TRIGGER fail_stone_write BEFORE " + std::string(timing) + " ON " + table +
		" FOR EACH ROW BEGIN IF NEW.operation_id=UNHEX('" + hex(command.operation_id) +
		"') THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='synthetic failure'; END IF; END");
	auto applied = critical_command_repository_apply(db, command);
	assert(applied.outcome != critical_apply_outcome::applied &&
	       applied.outcome != critical_apply_outcome::already_applied &&
	       applied.error_code != 0);
	verify_rollback(payload, command);
	assert(scalar("SELECT COUNT(*) FROM critical_operation_inbox WHERE " +
		      operation_where(command)) == 0);
	execute("DROP TRIGGER fail_stone_write");
	applied = critical_command_repository_apply(db, command);
	assert(applied.outcome == critical_apply_outcome::applied && !applied.error_code);
	verify_awards(payload, command, receipt(applied));
}
} // namespace

int main()
{
	const char *name = getenv("EPIC_TEST_DB_NAME");
	const std::string database = name ? name : "";
	if (database.rfind("epic_stone_test_", 0) != 0 || database.size() > 64 ||
	    database.size() <= 16 ||
	    !std::all_of(database.begin(), database.end(),
			 [](unsigned char c) { return std::isalnum(c) || c == '_'; }))
	{
		std::cerr << "Refusing test: a new epic_stone_test_<unique> database is required\n";
		return 2;
	}
	assert(getenv("DB_HOST") && getenv("DB_PORT") && getenv("DB_USER") && getenv("DB_PASSWD"));
	db = mysql_init(nullptr);
	assert(db);
	// Do not select or inspect any existing application database.
	assert(mysql_real_connect(
		db, getenv("DB_HOST"), getenv("DB_USER"), getenv("DB_PASSWD"), nullptr,
		static_cast<unsigned int>(strtoul(getenv("DB_PORT"), nullptr, 10)), nullptr,
		CLIENT_MULTI_STATEMENTS));
	execute("CREATE DATABASE `" + database + "`"); // intentionally fails if it exists
	assert(mysql_select_db(db, database.c_str()) == 0);
	execute("CREATE TABLE player_data(pid INT UNSIGNED PRIMARY KEY,epics BIGINT NOT NULL,"
		"epic_revision BIGINT UNSIGNED NOT NULL DEFAULT 0) ENGINE=InnoDB");
	execute("CREATE TABLE zones(number INT PRIMARY KEY,alignment INT NOT NULL DEFAULT 0,"
		"reset_perc INT NOT NULL DEFAULT 0,last_touch DATETIME NULL) ENGINE=InnoDB");
	execute("CREATE TABLE zone_touches(id BIGINT PRIMARY KEY AUTO_INCREMENT,boot_time DATETIME,"
		"touched_at DATETIME,zone_number INT,toucher_pid INT UNSIGNED,group_size INT,"
		"epic_value INT,alignment_delta INT) ENGINE=InnoDB");
	migration("migrations/critical_command_inbox_outbox.sql");
	migration("migrations/epic_ledger_balance.sql");
	migration("migrations/boon_reward_zone_outcome.sql");
	migration("migrations/immutable/0012_epic_stone_claim.sql");
	migration("migrations/immutable/0012_epic_stone_claim.sql");
	success_and_replay(100, 1);
	success_and_replay(200, 15);
	success_and_replay(300, 3, false);
	legacy_metadata_only();
	rejection(400, true);
	rejection(500, false);
	rejection(600, false, true);
	injected_failure(700, "zone_touch_outcome", "INSERT");
	injected_failure(800, "critical_outbox", "INSERT");
	injected_failure(900, "critical_operation_inbox", "UPDATE");
	mysql_close(db);
	std::cout << "epic stone atomic payout, claim replay, rejection and SQL rollback passed\n";
}
