#include "persistence/critical_command_repository.h"

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <mysql.h>
#include <string>
#include <thread>

extern "C" MYSQL *sql_pool_acquire(void)
{
	return nullptr;
}
extern "C" MYSQL *sql_pool_replace_connection(MYSQL *)
{
	return nullptr;
}
extern "C" void sql_pool_release(MYSQL *) {}

static unsigned long long scalar(MYSQL *connection, const std::string &sql)
{
	assert(mysql_real_query(connection, sql.data(), sql.size()) == 0);
	MYSQL_RES *result = mysql_store_result(connection);
	assert(result);
	MYSQL_ROW row = mysql_fetch_row(result);
	assert(row && row[0]);
	char *end = nullptr;
	const unsigned long long value = strtoull(row[0], &end, 10);
	assert(end && !*end);
	mysql_free_result(result);
	return value;
}

static critical_command command(unsigned int identity, int64_t delta)
{
	critical_command value = {};
	value.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	for (size_t index = 0; index < value.operation_id.bytes.size(); ++index)
		value.operation_id.bytes[index] = static_cast<uint8_t>(identity + index);
	value.type = critical_command_type::test;
	value.payload_version = 1;
	value.source_site = critical_source_site::recovery;
	value.deadline_class = critical_deadline_class::recovery;
	value.accepted_at_usec = 1700000000000000ULL + identity;
	value.keys = { { critical_entity_type::item, 20 }, { critical_entity_type::player, 10 } };
	for (unsigned int byte = 0; byte < 8; ++byte)
		value.payload.push_back(
			static_cast<uint8_t>(static_cast<uint64_t>(delta) >> (byte * 8)));
	assert(critical_command_normalize(&value));
	return value;
}

static MYSQL *connect_database()
{
	MYSQL *connection = mysql_init(nullptr);
	assert(connection);
	const char *port_text = getenv("DB_PORT");
	assert(mysql_real_connect(
		connection, getenv("DB_HOST"), getenv("DB_USER"), getenv("DB_PASSWD"),
		getenv("CRITICAL_TEST_DB_NAME"),
		static_cast<unsigned int>(strtoul(port_text ? port_text : "3306", nullptr, 10)),
		nullptr, 0));
	return connection;
}

int main()
{
	MYSQL *connection = connect_database();
	const char *delete_dedupe =
		"DELETE d FROM critical_outbox_delivery_dedupe d JOIN critical_outbox o ON "
		"o.outbox_id=d.outbox_id JOIN critical_operation_inbox i ON "
		"i.operation_id=o.operation_id WHERE i.command_type=1";
	const char *delete_outbox =
		"DELETE o FROM critical_outbox o JOIN critical_operation_inbox i ON "
		"i.operation_id=o.operation_id WHERE i.command_type=1";
	const char *delete_inbox = "DELETE FROM critical_operation_inbox WHERE command_type=1";
	assert(mysql_real_query(connection, delete_dedupe, strlen(delete_dedupe)) == 0);
	assert(mysql_real_query(connection, delete_outbox, strlen(delete_outbox)) == 0);
	assert(mysql_real_query(connection, delete_inbox, strlen(delete_inbox)) == 0);
	assert(mysql_real_query(connection, "DELETE FROM critical_test_state",
				strlen("DELETE FROM critical_test_state")) == 0);

	critical_command first = command(1, 5);
	critical_apply_result applied = critical_command_repository_apply(connection, first);
	assert(applied.outcome == critical_apply_outcome::applied && applied.durable_revision == 1);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM critical_operation_inbox WHERE command_type=1") == 1);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM critical_outbox o JOIN critical_operation_inbox i "
		      "ON i.operation_id=o.operation_id WHERE i.command_type=1") == 1);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM critical_test_state WHERE value=5 AND revision=1") ==
	       2);

	applied = critical_command_repository_apply(connection, first);
	assert(applied.outcome == critical_apply_outcome::already_applied &&
	       applied.durable_revision == 1);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM critical_operation_inbox WHERE command_type=1") == 1);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM critical_outbox o JOIN critical_operation_inbox i "
		      "ON i.operation_id=o.operation_id WHERE i.command_type=1") == 1);
	assert(scalar(connection, "SELECT SUM(value) FROM critical_test_state") == 10);
	applied = critical_command_repository_reconcile(connection, first);
	assert(applied.outcome == critical_apply_outcome::already_applied &&
	       applied.durable_revision == 1);

	critical_command mismatch = first;
	mismatch.payload[0] = 6;
	applied = critical_command_repository_apply(connection, mismatch);
	assert(applied.outcome == critical_apply_outcome::terminal_failure &&
	       applied.error_code == EEXIST);
	applied = critical_command_repository_reconcile(connection, mismatch);
	assert(applied.outcome == critical_apply_outcome::terminal_failure &&
	       applied.error_code == EEXIST);
	assert(scalar(connection, "SELECT SUM(value) FROM critical_test_state") == 10);

	critical_command second = command(33, 2);
	applied = critical_command_repository_apply(connection, second);
	assert(applied.outcome == critical_apply_outcome::applied && applied.durable_revision == 2);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM critical_operation_inbox WHERE command_type=1") == 2);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM critical_outbox o JOIN critical_operation_inbox i "
		      "ON i.operation_id=o.operation_id WHERE i.command_type=1") == 2);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM critical_test_state WHERE value=7 AND revision=2") ==
	       2);
	MYSQL *parallel_one = connect_database();
	MYSQL *parallel_two = connect_database();
	critical_apply_result parallel_results[2] = {};
	critical_command third = command(90, 1);
	critical_command fourth = command(120, 1);
	std::thread first_worker(
		[&]
		{ parallel_results[0] = critical_command_repository_apply(parallel_one, third); });
	std::thread second_worker(
		[&]
		{ parallel_results[1] = critical_command_repository_apply(parallel_two, fourth); });
	first_worker.join();
	second_worker.join();
	if (parallel_results[0].outcome == critical_apply_outcome::retryable_failure)
	{
		assert(parallel_results[0].error_code == 1205 ||
		       parallel_results[0].error_code == 1213);
		parallel_results[0] = critical_command_repository_apply(parallel_one, third);
	}
	if (parallel_results[1].outcome == critical_apply_outcome::retryable_failure)
	{
		assert(parallel_results[1].error_code == 1205 ||
		       parallel_results[1].error_code == 1213);
		parallel_results[1] = critical_command_repository_apply(parallel_two, fourth);
	}
	mysql_close(parallel_one);
	mysql_close(parallel_two);
	assert(parallel_results[0].outcome == critical_apply_outcome::applied);
	assert(parallel_results[1].outcome == critical_apply_outcome::applied);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM critical_test_state WHERE value=9 AND revision=4") ==
	       2);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM critical_operation_inbox WHERE command_type=1") == 4);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM critical_outbox o JOIN critical_operation_inbox i "
		      "ON i.operation_id=o.operation_id WHERE i.command_type=1") == 4);

	assert(mysql_real_query(
		       connection,
		       "UPDATE critical_test_state SET value=9223372036854775807 WHERE "
		       "entity_type=1 AND entity_id=10",
		       strlen("UPDATE critical_test_state SET value=9223372036854775807 WHERE "
			      "entity_type=1 AND entity_id=10")) == 0);
	critical_command overflow = command(65, 1);
	applied = critical_command_repository_apply(connection, overflow);
	assert(applied.outcome == critical_apply_outcome::terminal_failure &&
	       applied.error_code == ERANGE);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM critical_operation_inbox WHERE command_type=1") == 4);
	assert(scalar(connection,
		      "SELECT COUNT(*) FROM critical_outbox o JOIN critical_operation_inbox i "
		      "ON i.operation_id=o.operation_id WHERE i.command_type=1") == 4);
	assert(mysql_real_query(connection, delete_dedupe, strlen(delete_dedupe)) == 0);
	assert(mysql_real_query(connection, delete_outbox, strlen(delete_outbox)) == 0);
	assert(mysql_real_query(connection, delete_inbox, strlen(delete_inbox)) == 0);
	assert(mysql_real_query(connection, "DELETE FROM critical_test_state",
				strlen("DELETE FROM critical_test_state")) == 0);

	mysql_close(connection);
	return 0;
}
