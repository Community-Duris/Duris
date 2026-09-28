#include "persistence/economic_accounting_repository.h"

#include <mysql/errmsg.h>

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

namespace
{
const char *required(const char *name)
{
	const char *value = getenv(name);
	assert(value && *value);
	return value;
}

MYSQL *connect_fixture()
{
	// Guard before constructing any client. No option files or inherited socket.
	assert(!strcmp(required("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA"), "1"));
	assert(!strcmp(required("DB_HOST"), "127.0.0.1"));
	assert(!getenv("DB_SOCKET") || !*getenv("DB_SOCKET"));
	const std::string schema = required("DB_NAME");
	assert(schema.starts_with("economic_schema_test_") &&
	       schema.find_first_not_of(
		       "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") ==
		       std::string::npos);
	char *end = nullptr;
	const auto port = strtoul(required("DB_PORT"), &end, 10);
	assert(end && !*end && port > 0 && port <= 65535);
	auto *connection = mysql_init(nullptr);
	assert(connection);
	unsigned int timeout = 5, protocol = MYSQL_PROTOCOL_TCP;
	assert(!mysql_options(connection, MYSQL_OPT_CONNECT_TIMEOUT, &timeout));
	assert(!mysql_options(connection, MYSQL_OPT_READ_TIMEOUT, &timeout));
	assert(!mysql_options(connection, MYSQL_OPT_WRITE_TIMEOUT, &timeout));
	assert(!mysql_options(connection, MYSQL_OPT_PROTOCOL, &protocol));
	assert(mysql_real_connect(connection, "127.0.0.1", required("DB_USER"),
				  required("DB_PASSWD"), schema.c_str(),
				  static_cast<unsigned int>(port), nullptr, 0));
	return connection;
}

void execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
	{
		fprintf(stderr, "synthetic authority fixture failed, SQL error %u\n",
			mysql_errno(connection));
		abort();
	}
}

void assert_snapshot_equal(const economic_sql_authority_snapshot &actual,
			   const economic_sql_authority_snapshot &expected)
{
	assert(actual.lineage_revision == expected.lineage_revision);
	assert(actual.lineage.bytes == expected.lineage.bytes);
	assert(actual.epoch.bytes == expected.epoch.bytes);
	assert(actual.mappings.size() == expected.mappings.size());
	for (size_t index = 0; index < expected.mappings.size(); ++index)
	{
		const auto &a = actual.mappings[index], &b = expected.mappings[index];
		assert(economic_account_key_equal(a.request.account, b.request.account));
		assert(a.request.native_id == b.request.native_id &&
		       a.request.locator_kind == b.request.locator_kind &&
		       a.revision == b.revision);
	}
}

critical_operation_id new_id()
{
	critical_operation_id id = {};
	assert(critical_operation_id_generate(&id));
	return id;
}

std::string literal(const critical_operation_id &id)
{
	char hex[CRITICAL_COMMAND_ID_HEX_SIZE] = {};
	assert(critical_operation_id_to_hex(id, hex, sizeof(hex)));
	return std::string("UNHEX('") + hex + "')";
}
}

int main()
{
	assert(mysql_library_init(0, nullptr, nullptr) == 0);
	auto *owner = connect_fixture();
	auto *reader = connect_fixture();
	auto *writer = connect_fixture();
	execute(owner, "SET SESSION innodb_lock_wait_timeout=1");
	execute(writer, "SET SESSION innodb_lock_wait_timeout=1");
	execute(reader, "SET SESSION innodb_lock_wait_timeout=1");
	const auto lineage = new_id(), epoch = new_id(), operation = new_id();
	const auto l = literal(lineage), e = literal(epoch), op = literal(operation);
	execute(owner, "START TRANSACTION");
	execute(owner,
		"INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,"
		"schema_version,payload_version,status,result_code,durable_revision,result_payload) VALUES(" +
			op + ",REPEAT(CHAR(1),32),REPEAT(CHAR(2),32),1,2,1,1,0,0,'')");
	execute(owner,
		"INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,transition_digest,creating_operation_id) VALUES(" +
			l + "," + e + ",1,1,REPEAT(CHAR(1),32)," + op + ")");
	execute(owner, "INSERT INTO economic_lineage_state(lineage,active_epoch,revision) VALUES(" +
			       l + "," + e + ",7)");
	std::vector<economic_sql_mapping_request> requests;
	for (uint64_t index = 1; index <= 2; ++index)
	{
		execute(owner,
			"INSERT INTO economic_account_mapping(lineage,account_kind,context_id,backend_kind,locator_kind,native_id,active_native_id,creating_operation_id,revision) VALUES(" +
				l + "," + std::to_string(index) + ",0,1,1," +
				std::to_string(index) + "," + std::to_string(index) + "," + op +
				",9)");
		requests.push_back({ { lineage, static_cast<economic_account_kind>(index),
				       mysql_insert_id(owner), 0 },
				     1,
				     index });
	}
	execute(owner, "COMMIT");
	economic_sql_authority_snapshot snapshot;
	snapshot.lineage_revision = 999;
	assert(economic_sql_lock_authority(owner, lineage, epoch, requests, &snapshot) == EPERM);
	assert(snapshot.lineage_revision == 999);
	assert(economic_sql_lock_authority(nullptr, lineage, epoch, requests, &snapshot) == EINVAL);

	using client_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	client_flag reconnect = true;
	execute(owner, "START TRANSACTION");
	assert(!mysql_options(owner, MYSQL_OPT_RECONNECT, &reconnect));
	assert(economic_sql_lock_authority(owner, lineage, epoch, requests, &snapshot) == EPERM);
	reconnect = false;
	assert(!mysql_options(owner, MYSQL_OPT_RECONNECT, &reconnect));
	execute(owner, "ROLLBACK");

	// All failures preserve the caller's output; caller retains rollback ownership.
	auto fails = [&](const critical_operation_id &expected_epoch,
			 const std::vector<economic_sql_mapping_request> &input, unsigned int error)
	{
		execute(owner, "START TRANSACTION");
		snapshot.lineage_revision = 999;
		assert(economic_sql_lock_authority(owner, lineage, expected_epoch, input,
						   &snapshot) == error);
		assert(snapshot.lineage_revision == 999 && snapshot.mappings.empty());
		assert(owner->server_status & SERVER_STATUS_IN_TRANS);
		execute(owner, "ROLLBACK");
	};
	fails(new_id(), requests, ESTALE);
	auto invalid = requests;
	invalid.push_back(invalid.front());
	fails(epoch, invalid, EINVAL);
	invalid.assign(ECONOMIC_ACCOUNTING_MAX_ACCOUNTS + 1, requests.front());
	fails(epoch, invalid, E2BIG);
	invalid = requests;
	invalid[0].account.authority_id = UINT64_MAX;
	fails(epoch, invalid, ENOENT);
	invalid = requests;
	invalid[0].account.lineage = new_id();
	fails(epoch, invalid, EINVAL);
	invalid = requests;
	invalid[0].account.kind = economic_account_kind::sink;
	fails(epoch, invalid, EINVAL);
	invalid = requests;
	invalid[0].native_id = 0;
	fails(epoch, invalid, EINVAL);
	invalid = requests;
	invalid[0].native_id = 9876;
	fails(epoch, invalid, ESTALE);
	invalid = requests;
	invalid[0].locator_kind = 2;
	fails(epoch, invalid, ESTALE);
	invalid = requests;
	invalid[0].account.kind = economic_account_kind::bank;
	fails(epoch, invalid, ESTALE);
	invalid = requests;
	invalid[0].account.context_id = 2;
	fails(epoch, invalid, ESTALE);

	execute(owner, "START TRANSACTION");
	execute(owner, "UPDATE economic_lineage_state SET active_epoch=NULL WHERE lineage=" + l);
	assert(economic_sql_lock_authority(owner, lineage, epoch, requests, &snapshot) == ENODATA);
	execute(owner, "ROLLBACK");
	const auto mapping_where =
		" WHERE mapping_id=" + std::to_string(requests[0].account.authority_id);
	execute(owner, "START TRANSACTION");
	execute(owner, "UPDATE economic_account_mapping SET backend_kind=2" + mapping_where);
	assert(economic_sql_lock_authority(owner, lineage, epoch, requests, &snapshot) == ESTALE);
	execute(owner, "ROLLBACK");

	// An exclusive metadata writer also blocks preparation. Propagate the SQL
	// retry error without replacing the caller's output or ending its transaction.
	execute(writer, "START TRANSACTION");
	execute(writer, "UPDATE economic_account_mapping SET revision=revision+1" + mapping_where);
	execute(owner, "START TRANSACTION");
	assert(economic_sql_lock_authority(owner, lineage, epoch, requests, &snapshot) == 1205);
	assert(snapshot.lineage_revision == 999 && snapshot.mappings.empty());
	execute(owner, "ROLLBACK");
	execute(writer, "ROLLBACK");

	// Opposite caller orders use the same sorted lock order. Concurrent readers
	// share metadata locks; administrative transitions/retirement must wait.
	std::reverse(requests.begin(), requests.end());
	execute(owner, "START TRANSACTION");
	assert(economic_sql_lock_authority(owner, lineage, epoch, requests, &snapshot) == 0);
	assert(snapshot.lineage_revision == 7 && snapshot.mappings.size() == 2);
	assert(snapshot.mappings[0].request.account.authority_id <
	       snapshot.mappings[1].request.account.authority_id);
	assert(snapshot.mappings[0].revision == 9 && snapshot.epoch.bytes == epoch.bytes);
	// A later failed read must not replace an already populated prior snapshot.
	auto later_invalid = requests;
	auto &second = *std::max_element(
		later_invalid.begin(), later_invalid.end(), [](const auto &a, const auto &b)
		{ return a.account.authority_id < b.account.authority_id; });
	second.native_id = 9988;
	const auto prior = snapshot;
	assert(economic_sql_lock_authority(owner, lineage, epoch, later_invalid, &snapshot) ==
	       ESTALE);
	assert_snapshot_equal(snapshot, prior);
	execute(owner, "ROLLBACK");
	execute(owner, "START TRANSACTION");
	assert(economic_sql_lock_authority(owner, lineage, epoch, requests, &snapshot) == 0);
	economic_sql_authority_snapshot other;
	execute(reader, "START TRANSACTION");
	std::reverse(requests.begin(), requests.end());
	assert(economic_sql_lock_authority(reader, lineage, epoch, requests, &other) == 0);
	execute(reader, "ROLLBACK");
	auto blocked = [&](const std::string &sql)
	{
		execute(writer, "START TRANSACTION");
		assert(mysql_query(writer, sql.c_str()) != 0);
		assert(mysql_errno(writer) == 1205);
		execute(writer, "ROLLBACK");
	};
	blocked("UPDATE economic_lineage_state SET active_epoch=NULL,revision=revision+1 WHERE lineage=" +
		l);
	blocked("UPDATE economic_account_mapping SET active_native_id=NULL,retiring_operation_id=" +
		op + ",revision=revision+1" + mapping_where);
	execute(owner, "ROLLBACK");
	assert(economic_sql_lock_authority(owner, lineage, epoch, requests, &snapshot) == EPERM);

	// Read the full unsigned SQL domain without narrowing through int64_t.
	execute(owner, "START TRANSACTION");
	execute(owner,
		"UPDATE economic_lineage_state SET revision=18446744073709551615 WHERE lineage=" +
			l);
	execute(owner,
		"UPDATE economic_account_mapping SET native_id=18446744073709551615,active_native_id=18446744073709551615,context_id=18446744073709551615,revision=18446744073709551615" +
			mapping_where);
	auto wide = requests;
	wide[0].native_id = UINT64_MAX;
	wide[0].account.context_id = UINT64_MAX;
	assert(economic_sql_lock_authority(owner, lineage, epoch, wide, &snapshot) == 0);
	assert(snapshot.lineage_revision == UINT64_MAX &&
	       snapshot.mappings[0].revision == UINT64_MAX);
	execute(owner, "ROLLBACK");

	// Retirement and native-ID reuse cannot satisfy a frozen lifetime request.
	execute(writer, "START TRANSACTION");
	execute(writer,
		"UPDATE economic_account_mapping SET active_native_id=NULL,retiring_operation_id=" +
			op + ",revision=revision+1" + mapping_where);
	execute(writer,
		"INSERT INTO economic_account_mapping(lineage,account_kind,context_id,backend_kind,locator_kind,native_id,active_native_id,creating_operation_id) VALUES(" +
			l + ",1,0,1,1,1,1," + op + ")");
	const auto replacement = mysql_insert_id(writer);
	execute(writer, "COMMIT");
	execute(owner, "START TRANSACTION");
	assert(economic_sql_lock_authority(owner, lineage, epoch, requests, &snapshot) == ESTALE);
	execute(owner, "ROLLBACK");
	requests[0].account.authority_id = replacement;
	execute(owner, "START TRANSACTION");
	assert(economic_sql_lock_authority(owner, lineage, epoch, requests, &snapshot) == 0);
	execute(owner, "COMMIT");
	assert(economic_sql_lock_authority(owner, lineage, epoch, requests, &snapshot) == EPERM);

	// A valid row from another lineage must fail the persisted-row comparison,
	// even when every request field except the row's stored lineage matches.
	const auto foreign_lineage = new_id(), foreign_epoch = new_id();
	const auto fl = literal(foreign_lineage), fe = literal(foreign_epoch);
	execute(owner, "START TRANSACTION");
	execute(owner,
		"INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,transition_digest,creating_operation_id) VALUES(" +
			fl + "," + fe + ",1,1,REPEAT(CHAR(1),32)," + op + ")");
	execute(owner, "INSERT INTO economic_lineage_state(lineage,active_epoch,revision) VALUES(" +
			       fl + "," + fe + ",1)");
	execute(owner,
		"INSERT INTO economic_account_mapping(lineage,account_kind,context_id,backend_kind,locator_kind,native_id,active_native_id,creating_operation_id) VALUES(" +
			fl + ",1,0,1,1,1,1," + op + ")");
	auto foreign_request = requests;
	foreign_request[0].account.authority_id = mysql_insert_id(owner);
	const auto before_foreign = snapshot;
	assert(economic_sql_lock_authority(owner, lineage, epoch, foreign_request, &snapshot) ==
	       ESTALE);
	assert_snapshot_equal(snapshot, before_foreign);
	execute(owner, "ROLLBACK"); // Includes all synthetic foreign-lineage rows.

	// Kill only this fixture's own local session after acquiring real locks.
	// Automatic reconnect is still disabled; a stale status bit cannot turn the
	// lost transaction into a successful identity snapshot.
	execute(owner, "START TRANSACTION");
	assert(economic_sql_lock_authority(owner, lineage, epoch, requests, &snapshot) == 0);
	const auto before_disconnect = snapshot;
	const auto owner_session = mysql_thread_id(owner);
	assert(owner_session && owner_session != mysql_thread_id(writer));
	execute(writer, "KILL CONNECTION " + std::to_string(owner_session));
	const auto disconnected =
		economic_sql_lock_authority(owner, lineage, epoch, requests, &snapshot);
	assert(disconnected == CR_SERVER_GONE_ERROR || disconnected == CR_SERVER_LOST);
	assert_snapshot_equal(snapshot, before_disconnect);
	mysql_close(owner);
	owner = nullptr;

	// Clean only this run's synthetic identity rows, in foreign-key order.
	execute(writer, "START TRANSACTION");
	execute(writer, "DELETE FROM economic_account_mapping WHERE lineage=" + l);
	execute(writer, "DELETE FROM economic_lineage_state WHERE lineage=" + l);
	execute(writer, "DELETE FROM economic_epoch WHERE lineage=" + l);
	execute(writer, "DELETE FROM critical_operation_inbox WHERE operation_id=" + op);
	execute(writer, "COMMIT");
	mysql_close(writer);
	mysql_close(reader);
	mysql_library_end();
	puts("SQL accounting authority: identity, transaction, shared-lock, retirement and disconnect checks passed");
}
