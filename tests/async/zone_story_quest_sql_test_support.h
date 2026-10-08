#ifndef ZONE_STORY_QUEST_SQL_TEST_SUPPORT_H
#define ZONE_STORY_QUEST_SQL_TEST_SUPPORT_H
#include "sql/sql.h"
#include "sql/sql_transaction.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
MYSQL *DB = nullptr;
namespace
{
int fail_insert = 0;
bool transaction_active = false;
}
bool sql_in_transaction()
{
	return transaction_active;
}
bool sql_begin_transaction()
{
	assert(!transaction_active);
	if (mysql_query(DB, "START TRANSACTION"))
		return false;
	return transaction_active = true;
}
bool sql_commit()
{
	assert(transaction_active);
	if (mysql_query(DB, "COMMIT"))
		return false;
	transaction_active = false;
	return true;
}
bool sql_rollback()
{
	assert(transaction_active);
	const bool restored = !mysql_query(DB, "ROLLBACK");
	transaction_active = false;
	return restored;
}
MYSQL_RES *db_query_at(persistence_query_site, const char *format, ...)
{
	char query[4096];
	va_list args;
	va_start(args, format);
	vsnprintf(query, sizeof(query), format, args);
	va_end(args);
	assert(!mysql_query(DB, query));
	return mysql_store_result(DB);
}
char *sql_escape_string(const char *value)
{
	const auto size = strlen(value);
	char *escaped = static_cast<char *>(malloc(size * 2 + 1));
	mysql_real_escape_string(DB, escaped, value, size);
	return escaped;
}
bool sql_trace_exec_at(persistence_query_site, const char *, const char *query, size_t size, bool,
		       bool)
{
	if (!strncmp(query, "INSERT", 6) && fail_insert > 0 && --fail_insert == 0)
		return false;
	return !mysql_real_query(DB, query, size);
}

#endif
