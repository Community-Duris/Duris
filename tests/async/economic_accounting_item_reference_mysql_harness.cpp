// Real SQL storage/error contract; fixture roots do not prove gameplay admission.
#include "item/economic_accounting_item_reference.h"
#include <mysql.h>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace
{
void execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()) != 0)
	{
		std::fprintf(stderr, "fixture SQL failed: %u\n", mysql_errno(connection));
		std::abort();
	}
}

uint64_t count(MYSQL *connection, const char *table)
{
	execute(connection, std::string("SELECT COUNT(*) FROM ") + table);
	MYSQL_RES *rows = mysql_store_result(connection);
	assert(rows);
	MYSQL_ROW row = mysql_fetch_row(rows);
	assert(row && row[0]);
	const uint64_t value = std::strtoull(row[0], nullptr, 10);
	assert(!mysql_fetch_row(rows));
	mysql_free_result(rows);
	return value;
}

void rejected(MYSQL *connection, const economic_accounting_item_reference &ref,
	      unsigned int expected)
{
	errno = EBADMSG;
	const bool inserted = economic_accounting_item_reference_insert(connection, ref);
	const unsigned int actual = errno;
	std::printf("reference rejection: inserted=%d expected=%u actual=%u\n", inserted, expected,
		    actual);
	std::fflush(stdout);
	assert(!inserted && actual == expected);
}
} // namespace

int main()
{
	const char *disposable = std::getenv("TEST_DB_DISPOSABLE");
	assert(disposable && std::strcmp(disposable, "1") == 0);
	const char *host = std::getenv("DB_HOST");
	assert(host && std::strcmp(host, "127.0.0.1") == 0);
	const char *database = std::getenv("DB_NAME");
	constexpr char prefix[] = "economic_schema_test_";
	assert(database && std::strncmp(database, prefix, sizeof(prefix) - 1) == 0);
	const char *port = std::getenv("DB_PORT");
	assert(port);
	assert(mysql_library_init(0, nullptr, nullptr) == 0);
	MYSQL *connection = mysql_init(nullptr);
	assert(connection);
	assert(mysql_real_connect(
		connection, host, std::getenv("DB_USER"), std::getenv("DB_PASSWD"), database,
		static_cast<unsigned int>(std::strtoul(port, nullptr, 10)), nullptr, 0));
	assert(count(connection, "economic_accounting_operation") == 0);
	assert(count(connection, "economic_accounting_item_reference") == 0);

	economic_accounting_item_reference ref = {};
	ref.operation_id.bytes.back() = 2;
	ref.legacy_operation_id.bytes.back() = 3;
	ref.item_uid = 777;
	ref.after_revision = 1;
	assert(economic_accounting_item_reference_validate(ref));
	rejected(nullptr, ref, EINVAL);
	// A syntactically valid reference is not an admitted accounting operation.
	rejected(connection, ref, 1452);
	assert(count(connection, "economic_accounting_operation") == 0);
	assert(count(connection, "economic_accounting_item_reference") == 0);

	// The Python harness supplies the reviewed schema test's synthetic storage
	// fixture in one transaction. This does not manufacture runtime authority.
	std::string statement;
	while (std::getline(std::cin, statement, ';'))
		if (!statement.empty())
			execute(connection, statement);
	assert(count(connection, "economic_accounting_operation") == 1);
	ref.item_uid = 778;
	rejected(connection, ref, 1452);
	assert(count(connection, "economic_accounting_item_reference") == 0);
	ref.item_uid = 777;
	assert(economic_accounting_item_reference_insert(connection, ref));
	rejected(connection, ref, 1062);
	assert(count(connection, "economic_accounting_item_reference") == 1);
	economic_accounting_item_reference loaded = {};
	assert(economic_accounting_item_reference_find_by_legacy(
		connection, ref.legacy_operation_id, 0, &loaded));
	assert(loaded.operation_id.bytes == ref.operation_id.bytes && loaded.item_uid == 777 &&
	       loaded.before_revision == 0 && loaded.after_revision == 1);
	errno = EBADMSG;
	assert(!economic_accounting_item_reference_find_by_legacy(
		connection, ref.legacy_operation_id, 1, &loaded));
	assert(errno == ENOENT);
	execute(connection, "ROLLBACK");
	assert(count(connection, "economic_accounting_operation") == 0);
	assert(count(connection, "economic_accounting_item_reference") == 0);

	// Prepared-statement errors must also survive close on both read and write.
	execute(connection, "RENAME TABLE economic_accounting_item_reference TO "
			    "economic_accounting_item_reference_error_fixture");
	rejected(connection, ref, 1146);
	errno = EBADMSG;
	assert(!economic_accounting_item_reference_find_by_legacy(
		connection, ref.legacy_operation_id, 0, &loaded));
	assert(errno == 1146);
	execute(connection, "RENAME TABLE economic_accounting_item_reference_error_fixture TO "
			    "economic_accounting_item_reference");
	assert(count(connection, "economic_accounting_item_reference") == 0);
	mysql_close(connection);
	mysql_library_end();
	std::puts(
		"PASS: SQL statement errors preserved; strict references, round trip and rollback");
}
