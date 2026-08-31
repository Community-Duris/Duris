#include "item/item_uid_allocator.h"

#include "world/db.h"
#include "flatfile/flatfile_item_uid_allocator.h"
#include "persistence/persistence_mode.h"

#include <cstdlib>
#include <cstring>
#include <limits>
#include <mysql.h>
#include <mutex>

namespace
{
uint64_t range_next = 0;
uint64_t range_end = 0;
std::mutex range_mutex;

bool execute(MYSQL *connection, const char *sql)
{
	return mysql_real_query(connection, sql, strlen(sql)) == 0;
}
} // namespace

bool item_uid_allocator_reserve(MYSQL *connection, uint64_t count)
{
	std::lock_guard<std::mutex> guard(range_mutex);
	if (!count || range_next != range_end)
		return false;
	if (!persistence_mode_requires_mysql())
	{
		const char *root = persistence_mode_flatfile_root();
		uint64_t first = 0;
		std::string error;
		if (!root || flatfile_item_uid_reserve(root, count, &first, &error) !=
				     flatfile_item_uid_result::ok)
			return false;
		range_next = first;
		range_end = first + count;
		next_obj_uid = static_cast<unsigned long>(first);
		return true;
	}
	if (!connection || !execute(connection, "START TRANSACTION"))
		return false;
	if (!execute(connection,
		     "SELECT next_uid FROM item_uid_allocator WHERE allocator_id=1 FOR UPDATE"))
	{
		execute(connection, "ROLLBACK");
		return false;
	}
	MYSQL_RES *result = mysql_store_result(connection);
	MYSQL_ROW row = result ? mysql_fetch_row(result) : nullptr;
	uint64_t first = 0;
	if (row && row[0])
		first = strtoull(row[0], nullptr, 10);
	if (result)
		mysql_free_result(result);
	if (!first || first > std::numeric_limits<uint64_t>::max() - count)
	{
		execute(connection, "ROLLBACK");
		return false;
	}
	const uint64_t end = first + count;
	MYSQL_STMT *statement = mysql_stmt_init(connection);
	static const char SQL[] =
		"UPDATE item_uid_allocator SET next_uid=? WHERE allocator_id=1 AND next_uid=?";
	if (!statement || mysql_stmt_prepare(statement, SQL, sizeof(SQL) - 1) != 0)
	{
		if (statement)
			mysql_stmt_close(statement);
		execute(connection, "ROLLBACK");
		return false;
	}
	MYSQL_BIND bindings[2] = {};
	bindings[0].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[0].buffer = const_cast<uint64_t *>(&end);
	bindings[0].is_unsigned = true;
	bindings[1].buffer_type = MYSQL_TYPE_LONGLONG;
	bindings[1].buffer = &first;
	bindings[1].is_unsigned = true;
	const bool updated = mysql_stmt_bind_param(statement, bindings) == 0 &&
			     mysql_stmt_execute(statement) == 0 &&
			     mysql_stmt_affected_rows(statement) == 1;
	mysql_stmt_close(statement);
	if (!updated || !execute(connection, "COMMIT"))
	{
		execute(connection, "ROLLBACK");
		return false;
	}
	range_next = first;
	range_end = end;
	next_obj_uid = static_cast<unsigned long>(first);
	return true;
}

uint64_t item_uid_allocator_next(void)
{
	std::lock_guard<std::mutex> guard(range_mutex);
	if (!range_next || range_next >= range_end)
		return 0;
	const uint64_t allocated = range_next++;
	next_obj_uid = static_cast<unsigned long>(range_next);
	return allocated;
}

uint64_t item_uid_allocator_remaining(void)
{
	std::lock_guard<std::mutex> guard(range_mutex);
	return range_end >= range_next ? range_end - range_next : 0;
}

void item_uid_allocator_reset_for_tests(void)
{
	std::lock_guard<std::mutex> guard(range_mutex);
	range_next = 0;
	range_end = 0;
	next_obj_uid = 1;
}
