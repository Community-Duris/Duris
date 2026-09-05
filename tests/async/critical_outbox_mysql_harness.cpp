#include "persistence/critical_outbox.h"

#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mysql.h>
#include <string>
#include <thread>

static MYSQL *database_connection = nullptr;

extern "C" MYSQL *sql_pool_acquire(void)
{
	return database_connection;
}
extern "C" void sql_pool_release(MYSQL *) {}
extern "C" MYSQL *sql_pool_replace_connection(MYSQL *)
{
	return database_connection;
}

static void execute(const char *sql)
{
	const int result = mysql_real_query(database_connection, sql, strlen(sql));
	if (result != 0)
		fprintf(stderr, "critical outbox query failed: %s\n",
			mysql_error(database_connection));
	assert(result == 0);
}

static unsigned long long scalar(const char *sql)
{
	execute(sql);
	MYSQL_RES *result = mysql_store_result(database_connection);
	assert(result);
	MYSQL_ROW row = mysql_fetch_row(result);
	assert(row && row[0]);
	const unsigned long long value = strtoull(row[0], nullptr, 10);
	mysql_free_result(result);
	return value;
}

static void clear_rows()
{
	execute("DELETE d FROM critical_outbox_delivery_dedupe d JOIN critical_outbox o ON "
		"o.outbox_id=d.outbox_id WHERE o.operation_id IN "
		"(UNHEX('00000000000000000000000000000001'),"
		"UNHEX('00000000000000000000000000000002'),"
		"UNHEX('00000000000000000000000000000003'))");
	execute("DELETE FROM critical_outbox WHERE operation_id IN "
		"(UNHEX('00000000000000000000000000000001'),"
		"UNHEX('00000000000000000000000000000002'),"
		"UNHEX('00000000000000000000000000000003'))");
	execute("DELETE FROM critical_operation_inbox WHERE operation_id IN "
		"(UNHEX('00000000000000000000000000000001'),"
		"UNHEX('00000000000000000000000000000002'),"
		"UNHEX('00000000000000000000000000000003'))");
	execute("DELETE FROM critical_test_state");
}

static void insert_record(unsigned int suffix)
{
	const char digit = static_cast<char>('0' + suffix);
	char sql[2048] = {};
	snprintf(
		sql, sizeof(sql),
		"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
		"command_type,schema_version,payload_version,status,result_payload,committed_at) "
		"VALUES(UNHEX('0000000000000000000000000000000%c'),UNHEX(REPEAT('11',32)),"
		"UNHEX(REPEAT('22',32)),1,1,1,1,'',CURRENT_TIMESTAMP(6));"
		"INSERT INTO critical_outbox(operation_id,event_index,destination,event_type,"
		"payload_version,payload) VALUES(UNHEX('0000000000000000000000000000000%c'),0,1,1,1,"
		"UNHEX(REPEAT('33',16)))",
		digit, digit);
	const char *separator = strchr(sql, ';');
	assert(separator);
	std::string first(sql, static_cast<size_t>(separator - sql));
	execute(first.c_str());
	execute(separator + 1);
}

struct delivery_state
{
	unsigned int calls;
	bool retry_first;
	bool terminal;
};

static critical_outbox_delivery_result deliver(const critical_outbox_record &record, void *raw)
{
	auto &state = *static_cast<delivery_state *>(raw);
	assert(record.outbox_id && record.destination == 1 && record.event_type == 1 &&
	       record.payload_version == 1 && record.payload.size() == 16);
	++state.calls;
	if (state.terminal)
		return critical_outbox_delivery_result::terminal_failure;
	if (state.retry_first && state.calls == 1)
		return critical_outbox_delivery_result::retryable_failure;
	return critical_outbox_delivery_result::delivered;
}

template <typename Predicate> static void wait_until(Predicate predicate)
{
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	while (!predicate())
	{
		assert(std::chrono::steady_clock::now() < deadline);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

int main()
{
	database_connection = mysql_init(nullptr);
	assert(database_connection);
	const char *port_text = getenv("DB_PORT");
	const char *database = getenv("CRITICAL_TEST_DB_NAME");
	assert(mysql_real_connect(
		database_connection, getenv("DB_HOST"), getenv("DB_USER"), getenv("DB_PASSWD"),
		database,
		static_cast<unsigned int>(strtoul(port_text ? port_text : "3306", nullptr, 10)),
		nullptr, 0));
	// The wrapper owns an isolated database with ordinary production-shaped tables.
	clear_rows();

	insert_record(1);
	delivery_state state = {};
	assert(critical_outbox_init(deliver, &state));
	wait_until([&] { return critical_outbox_health_copy().delivered == 1; });
	critical_outbox_shutdown();
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE status=1 AND operation_id="
		      "UNHEX('00000000000000000000000000000001')") == 1);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox_delivery_dedupe d JOIN "
		      "critical_outbox o ON o.outbox_id=d.outbox_id WHERE o.operation_id="
		      "UNHEX('00000000000000000000000000000001')") == 1);

	clear_rows();
	insert_record(2);
	state = { .calls = 0, .retry_first = true, .terminal = false };
	assert(critical_outbox_init(deliver, &state));
	wait_until([&] { return critical_outbox_health_copy().retries == 1; });
	critical_outbox_shutdown();
	execute("UPDATE critical_outbox SET next_attempt_at=CURRENT_TIMESTAMP(6) WHERE "
		"operation_id=UNHEX('00000000000000000000000000000002')");
	state.retry_first = false;
	assert(critical_outbox_init(deliver, &state));
	wait_until([&] { return critical_outbox_health_copy().delivered == 1; });
	critical_outbox_shutdown();
	assert(state.calls == 2);
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE status=1 AND attempt_count=1 "
		      "AND operation_id=UNHEX('00000000000000000000000000000002')") == 1);

	clear_rows();
	critical_reconciliation_report baseline = {};
	assert(critical_outbox_reconcile(&baseline));
	insert_record(3);
	state = { .calls = 0, .retry_first = false, .terminal = true };
	assert(critical_outbox_init(deliver, &state));
	wait_until([&] { return critical_outbox_health_copy().terminal_failures == 1; });
	critical_outbox_shutdown();
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE status=2 AND operation_id="
		      "UNHEX('00000000000000000000000000000003')") == 1);
	const uint64_t dead_id =
		scalar("SELECT outbox_id FROM critical_outbox WHERE status=2 AND operation_id="
		       "UNHEX('00000000000000000000000000000003')");
	assert(critical_outbox_retry_dead_letter(dead_id));
	assert(scalar("SELECT COUNT(*) FROM critical_outbox WHERE status=0 AND attempt_count=0 "
		      "AND operation_id=UNHEX('00000000000000000000000000000003')") == 1);
	critical_reconciliation_report report = {};
	assert(critical_outbox_reconcile(&report));
	assert(report.incomplete_inbox == baseline.incomplete_inbox &&
	       report.committed_without_outbox == baseline.committed_without_outbox &&
	       report.pending_outbox == baseline.pending_outbox + 1 &&
	       report.dead_letter_outbox == baseline.dead_letter_outbox);

	clear_rows();
	mysql_close(database_connection);
	database_connection = nullptr;
	return 0;
}
