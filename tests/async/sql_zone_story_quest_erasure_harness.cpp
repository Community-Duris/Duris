#include "sql/sql.h"
#include "sql/sql_transaction.h"
#include "sql/zone_story_quest_state_repository.h"
#include "world/zone_story_quest_feature.h"

#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>

namespace
{
using result = sql_zone_story_quest_state_result;
size_t allocation_remaining = 0;
bool allocation_hit = false;
bool transaction = true, missing = false, query_failed = false, write_failed = false;
bool escape_failed = false, null_lengths = false;
int null_column = -1;
unsigned reads = 0, writes = 0, results_created = 0, results_freed = 0;
unsigned escapes_created = 0, escapes_freed = 0;
char *live_escape = nullptr;
std::string version = "1", revision = "7", payload, last_read, attempted, saved;
MYSQL_ROW row;
char *columns[3];
unsigned long lengths[3];
MYSQL connection;
const char *select_sql =
	"SELECT state_version,catalog_revision,state_blob FROM zone_story_quest_state WHERE state_id=1 LIMIT 1";

void reset(const std::string &state)
{
	assert(allocation_remaining == 0 && !live_escape && results_created == results_freed);
	assert(escapes_created == escapes_freed);
	payload = state;
	saved = state;
	std::string{}.swap(attempted);
	last_read.clear();
	version = "1";
	revision = "7";
	transaction = true;
	missing = query_failed = write_failed = escape_failed = null_lengths = false;
	null_column = -1;
	reads = writes = results_created = results_freed = escapes_created = escapes_freed = 0;
	DB = &connection;
}

void clean()
{
	assert(results_created == results_freed && escapes_created == escapes_freed &&
	       !live_escape);
	assert(transaction); // The helper never resolves its caller's transaction.
}

void refused(const zone_story_quest_catalog::catalog &catalog, const std::vector<uint32_t> &pids,
	     result expected)
{
	const std::string before = saved;
	std::string error;
	assert(sql_zone_story_quest_state_remove_player_aliases(7, 2, pids, catalog, &error) ==
	       expected);
	assert(writes == 0 && saved == before);
	clean();
}
} // namespace

// Walk actual production allocations. Fail once, so error reporting can allocate.
void *operator new(size_t size)
{
	if (allocation_remaining && --allocation_remaining == 0)
	{
		allocation_hit = true;
		throw std::bad_alloc();
	}
	if (void *pointer = std::malloc(size ? size : 1))
		return pointer;
	throw std::bad_alloc();
}
void *operator new[](size_t size)
{
	return ::operator new(size);
}
void operator delete(void *pointer) noexcept
{
	std::free(pointer);
}
void operator delete[](void *pointer) noexcept
{
	std::free(pointer);
}
void operator delete(void *pointer, size_t) noexcept
{
	std::free(pointer);
}
void operator delete[](void *pointer, size_t) noexcept
{
	std::free(pointer);
}

// Production owns sql_escape_string's malloc result via std::free. Observe real
// cleanup, including an exception inside qry, without substituting its owner.
extern "C" void __real_free(void *);
extern "C" void __wrap_free(void *pointer)
{
	if (pointer && pointer == live_escape)
	{
		live_escape = nullptr;
		++escapes_freed;
	}
	__real_free(pointer);
}

MYSQL *DB = &connection;
bool sql_in_transaction()
{
	return transaction;
}

MYSQL_RES *db_query_at(persistence_query_site, const char *format, ...)
{
	++reads;
	last_read = format;
	assert(last_read == select_sql || last_read == std::string(select_sql) + " FOR UPDATE");
	if (query_failed)
		return nullptr;
	auto *value = static_cast<MYSQL_RES *>(std::malloc(sizeof(MYSQL_RES)));
	assert(value);
	++results_created;
	return value;
}
extern "C" MYSQL_ROW mysql_fetch_row(MYSQL_RES *)
{
	if (missing)
		return nullptr;
	columns[0] = version.data();
	columns[1] = revision.data();
	columns[2] = payload.data();
	if (null_column >= 0)
		columns[null_column] = nullptr;
	row = columns;
	return row;
}
extern "C" unsigned long *mysql_fetch_lengths(MYSQL_RES *)
{
	lengths[0] = version.size();
	lengths[1] = revision.size();
	lengths[2] = payload.size();
	return null_lengths ? nullptr : lengths;
}
extern "C" void mysql_free_result(MYSQL_RES *value)
{
	++results_freed;
	std::free(value);
}
char *sql_escape_string(const char *input)
{
	if (escape_failed)
		return nullptr;
	assert(!live_escape);
	const size_t size = std::strlen(input) + 1;
	live_escape = static_cast<char *>(std::malloc(size));
	assert(live_escape);
	std::memcpy(live_escape, input, size);
	++escapes_created;
	return live_escape;
}
bool qry_at(persistence_query_site, const char *format, ...)
{
	assert(std::strstr(format, "INSERT INTO zone_story_quest_state") == format);
	assert(std::strstr(format, "ON DUPLICATE KEY UPDATE"));
	va_list arguments;
	va_start(arguments, format);
	const auto catalog_revision = va_arg(arguments, unsigned int);
	const char *state = va_arg(arguments, const char *);
	va_end(arguments);
	assert(catalog_revision == 7 && state == live_escape);
	++writes;
	attempted = state;
	if (write_failed)
		return false;
	saved = attempted;
	return true;
}

int main()
{
	zone_story_quest_catalog::catalog catalog;
	catalog.content_revision = 7;
	const std::vector<uint32_t> pids{ 1, 2 };
	const std::string original = "ZSQF|1\nN|1|1|54617267657441|1\n"
				     "N|1|9|556e72656c61746564|1\nN|3|2|54617267657442|1\n"
				     "N|7|1|486973746f726963616c41|1\nX|5|88\nH|9|1\n";
	const std::string expected = "ZSQF|1\nN|1|9|556e72656c61746564|1\n"
				     "X|1|1\nX|2|1\nX|2|2\nX|3|2\nX|5|88\nX|7|1\nX|9|1\n";
	reset(original);
	transaction = false;
	assert(sql_zone_story_quest_state_remove_player_aliases(7, 2, pids, catalog) ==
	       result::invalid);
	assert(reads == 0 && writes == 0 && !transaction);
	transaction = true;
	DB = nullptr;
	refused(catalog, pids, result::invalid);
	assert(reads == 0);

	for (const auto &invalid_pids : { std::vector<uint32_t>{}, std::vector<uint32_t>{ 0 },
					  std::vector<uint32_t>(1025, 1) })
	{
		reset(original);
		refused(catalog, invalid_pids, result::invalid);
		assert(reads == 0);
	}
	for (const auto &bad_version : { "", "0", "2", "1junk", "184467440737095516160" })
	{
		reset(original);
		version = bad_version;
		refused(catalog, pids, result::invalid);
	}
	for (const auto &bad_revision : { "", "8", "7junk", "4294967296", "-1" })
	{
		reset(original);
		revision = bad_revision;
		refused(catalog, pids, result::invalid);
	}
	for (int column = 0; column < 3; ++column)
	{
		reset(original);
		null_column = column;
		refused(catalog, pids, result::invalid);
	}
	reset(original);
	null_lengths = true;
	refused(catalog, pids, result::invalid);
	for (const auto &bad_payload :
	     { "", "ZSQF|2\n", "N|1|1|54657374|1\n", "ZSQF|1\nN|broken\n", "ZSQF|1\nN|1|1|5" })
	{
		reset(bad_payload);
		refused(catalog, pids, result::invalid);
	}
	reset(original);
	auto bad_catalog = catalog;
	bad_catalog.content_revision = 8;
	refused(bad_catalog, pids, result::invalid);
	reset(original);
	bad_catalog = catalog;
	bad_catalog.schema_version = 2;
	refused(bad_catalog, pids, result::invalid);

	reset(original);
	missing = true;
	refused(catalog, pids, result::not_found);
	assert(reads == 1 && results_freed == 1);
	reset(original);
	query_failed = true;
	refused(catalog, pids, result::io_error);
	assert(reads == 1 && results_created == 0);

	reset(original);
	assert(sql_zone_story_quest_state_remove_player_aliases(7, 2, pids, catalog) == result::ok);
	assert(reads == 1 && last_read == std::string(select_sql) + " FOR UPDATE");
	assert(writes == 1 && saved == expected);
	clean();
	reset(expected);
	assert(sql_zone_story_quest_state_remove_player_aliases(7, 2, pids, catalog) == result::ok);
	assert(reads == 1 && writes == 0 && saved == expected);
	clean();

	reset(original);
	escape_failed = true;
	refused(catalog, pids, result::io_error);
	reset(original);
	write_failed = true;
	assert(sql_zone_story_quest_state_remove_player_aliases(7, 2, pids, catalog) ==
	       result::io_error);
	assert(writes == 1 && attempted == expected && saved == original);
	clean();

	// Include enough unrelated state to force ostringstream and SQL-argument
	// allocations. Every injected allocation must yield no write or full state.
	zone_story_quest_feature::service large(catalog);
	for (uint32_t pid = 1; pid < 30; ++pid)
		large.remember_character(1, pid, std::string(128, 'x'));
	large.remember_character(7, 1, "historical");
	const auto large_before = large.serialize_state();
	reset(large_before);
	assert(sql_zone_story_quest_state_remove_player_aliases(7, 2, pids, catalog) == result::ok);
	const auto large_after = saved;
	assert(large_after.find("N|1|1|") == std::string::npos);
	assert(large_after.find("N|7|1|") == std::string::npos);
	assert(large_after.find("N|1|9|") != std::string::npos);
	clean();
	size_t failures = 0, result_failures = 0, escape_failures = 0;
	bool completed = false;
	for (size_t index = 1; index < 10000; ++index)
	{
		reset(large_before);
		std::string error;
		allocation_hit = false;
		allocation_remaining = index;
		const auto outcome = sql_zone_story_quest_state_remove_player_aliases(
			7, 2, pids, catalog, &error);
		allocation_remaining = 0;
		clean();
		assert(attempted.empty() || attempted == large_after);
		if (outcome != result::ok)
			assert(saved == large_before);
		else
			assert(saved == large_after);
		if (allocation_hit)
		{
			++failures;
			if (results_created)
				++result_failures;
			if (escapes_created)
				++escape_failures;
		}
		else
		{
			assert(outcome == result::ok);
			completed = true;
			break;
		}
	}
	assert(completed && failures && result_failures && escape_failures);

	// Public reads stay unlocked and release MYSQL_RES if output assignment fails.
	reset(large_before);
	std::string output;
	assert(sql_zone_story_quest_state_load(7, &output) == result::ok);
	assert(output == large_before && last_read == select_sql);
	clean();
	completed = false;
	size_t load_failures = 0;
	for (size_t index = 1; index < 100; ++index)
	{
		reset(large_before);
		std::string destination, error;
		allocation_hit = false;
		allocation_remaining = index;
		const auto outcome = sql_zone_story_quest_state_load(7, &destination, &error);
		allocation_remaining = 0;
		clean();
		assert(writes == 0);
		if (allocation_hit)
		{
			assert(outcome == result::io_error);
			if (results_created)
				++load_failures;
		}
		else
		{
			assert(outcome == result::ok && destination == large_before);
			completed = true;
			break;
		}
	}
	assert(completed && load_failures);
	std::cout << "PASS: SQL quest alias erasure, refusal, locks, retry and allocation cleanup ("
		  << failures << " injected failures; " << escape_failures
		  << " after escape allocation)\n";
}
