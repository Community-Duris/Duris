#include "sql/sql.h"
#include "sql/sql_transaction.h"
#include "sql/zone_story_quest_state_repository.h"
#include "world/zone_story_quest_feature.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
#include <zlib.h>

namespace
{
using result = sql_zone_story_quest_state_result;
using zone_story_quest_state::records;
struct row_state
{
	std::string version, revision, payload;
};
using row_store = std::map<unsigned, row_state>;
row_store committed, staged;
row_store::const_iterator cursor;
size_t allocation_remaining = 0;
bool allocation_hit = false;
bool transaction = true, missing = false, query_failed = false;
bool escape_failed = false, null_lengths = false;
int null_column = -1;
unsigned reads = 0, writes = 0, fail_write = 0;
unsigned results_created = 0, results_freed = 0, escapes_created = 0, escapes_freed = 0;
unsigned begins = 0, commits = 0, rollbacks = 0;
char *live_escape = nullptr;
std::string last_read, current_id, bad_version, bad_revision;
char *columns[4];
unsigned long lengths[4];
MYSQL connection;
const char *select_sql =
	"SELECT state_id,state_version,catalog_revision,state_blob FROM zone_story_quest_state ORDER BY state_id";

records decoded(const row_store &rows)
{
	records all;
	for (const auto &[id, row] : rows)
	{
		(void)id;
		records values;
		if (row.payload.starts_with("ZSQF|"))
			values = zone_story_quest_state::split_document(row.payload);
		else if (row.payload.starts_with("ZSQZ|1|"))
		{
			const auto end = row.payload.find('\n');
			std::string compressed;
			assert(zone_story_quest_state::unhex(
				std::string_view(row.payload)
					.substr(end + 1, row.payload.size() - end - 2),
				&compressed));
			std::string unpacked(std::stoul(row.payload.substr(7, end - 7)), '\0');
			uLongf size = unpacked.size();
			assert(uncompress(reinterpret_cast<Bytef *>(unpacked.data()), &size,
					  reinterpret_cast<const Bytef *>(compressed.data()),
					  compressed.size()) == Z_OK);
			assert(size == unpacked.size() &&
			       zone_story_quest_state::decode(unpacked, &values));
		}
		else
			assert(zone_story_quest_state::decode_journal(row.payload, &values));
		for (const auto &[key, value] : values)
			assert(all.emplace(key, value).second);
	}
	return all;
}

void reset(const std::string &state, bool bucketed = false)
{
	assert(allocation_remaining == 0 && !live_escape && results_created == results_freed);
	assert(escapes_created == escapes_freed);
	committed.clear();
	if (bucketed)
	{
		std::map<unsigned, records> groups;
		for (const auto &[key, value] : zone_story_quest_state::split_document(state))
			groups[zone_story_quest_state::bucket(key)][key] = value;
		for (const auto &[id, values] : groups)
			committed[id] = { "2", "7", zone_story_quest_state::encode(values) };
	}
	else
		committed[1] = { "1", "7", state };
	staged = committed;
	last_read.clear();
	bad_version.clear();
	bad_revision.clear();
	transaction = true;
	missing = query_failed = escape_failed = null_lengths = false;
	null_column = -1;
	reads = writes = fail_write = results_created = results_freed = escapes_created =
		escapes_freed = 0;
	begins = commits = rollbacks = 0;
	DB = &connection;
}

void clean(bool borrowed = true)
{
	assert(results_created == results_freed && escapes_created == escapes_freed &&
	       !live_escape);
	assert(transaction == borrowed);
	if (borrowed)
		assert(begins == 0 && commits == 0 && rollbacks == 0);
}

void refused(const zone_story_quest_catalog::catalog &catalog, const std::vector<uint32_t> &pids,
	     result expected)
{
	std::string error;
	assert(sql_zone_story_quest_state_remove_player_aliases(7, 2, pids, catalog, &error) ==
	       expected);
	assert(writes == 0);
	clean();
}

records erased(const std::string &before, const zone_story_quest_catalog::catalog &catalog)
{
	zone_story_quest_feature::service state(catalog);
	assert(state.deserialize_state(before));
	assert(state.erase_character_all_seasons(1, 2) && state.erase_character_all_seasons(2, 2));
	return zone_story_quest_state::split_document(state.serialize_state());
}
}

// Fail one production allocation at a time; error reporting may still allocate.
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
bool sql_begin_transaction()
{
	assert(!transaction);
	++begins;
	staged = committed;
	return transaction = true;
}
bool sql_commit()
{
	assert(transaction);
	++commits;
	committed = staged;
	transaction = false;
	return true;
}
bool sql_rollback()
{
	assert(transaction);
	++rollbacks;
	staged = committed;
	transaction = false;
	return true;
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
	cursor = staged.begin();
	return value;
}
extern "C" MYSQL_ROW mysql_fetch_row(MYSQL_RES *)
{
	if (missing || cursor == staged.end())
		return nullptr;
	current_id = std::to_string(cursor->first);
	const auto &value = cursor->second;
	columns[0] = current_id.data();
	columns[1] = const_cast<char *>((bad_version.empty() ? value.version : bad_version).data());
	columns[2] =
		const_cast<char *>((bad_revision.empty() ? value.revision : bad_revision).data());
	columns[3] = const_cast<char *>(value.payload.data());
	for (int index = 0; index < 4; ++index)
		lengths[index] = std::strlen(columns[index]);
	if (null_column >= 0)
		columns[null_column] = nullptr;
	++cursor;
	return columns;
}
extern "C" unsigned long *mysql_fetch_lengths(MYSQL_RES *)
{
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
bool sql_trace_exec_at(persistence_query_site, const char *, const char *query, size_t size, bool,
		       bool)
{
	assert(transaction);
	assert(std::string_view(query, size).starts_with("INSERT INTO zone_story_quest_state "));
	++writes;
	if (writes == fail_write)
		return false;
	const std::string statement(query, size);
	const auto at = statement.find("VALUES (");
	unsigned id = 0, version = 0, revision = 0;
	assert(at != std::string::npos && std::sscanf(statement.c_str() + at, "VALUES (%u,%u,%u,'",
						      &id, &version, &revision) == 3);
	assert(id > 0 && id <= 255 && version == 2 && revision == 7);
	const auto begin = statement.find(",'", at);
	const auto end = statement.find("',UTC_TIMESTAMP(6))", begin);
	assert(begin != std::string::npos && end != std::string::npos);
	staged[id] = { "2", "7", statement.substr(begin + 2, end - begin - 2) };
	return true;
}

int main()
{
	zone_story_quest_catalog::catalog catalog;
	catalog.content_revision = 7;
	const std::vector<uint32_t> pids{ 1, 2 };
	const std::string original =
		"ZSQF|3\nK|0\nN|1|1|54617267657441|1\n"
		"V|1|1|5000|544842|100|arrival\nM|1|1|500023|544842|100\n"
		"N|1|9|556e72656c61746564|1\n"
		"V|1|9|5000|544842|100|arrival\nM|1|9|500023|544842|100\n"
		"N|3|2|54617267657442|1\nN|7|1|486973746f726963616c41|1\nX|5|88\nH|9|1\n";
	const auto original_facts = zone_story_quest_state::split_document(original);
	const auto expected = erased(original, catalog);
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
	for (const auto &version : { "", "0", "3", "1junk", "184467440737095516160" })
	{
		reset(original);
		staged[1].version = version;
		refused(catalog, pids, result::invalid);
	}
	for (const auto &revision : { "", "8", "7junk", "4294967296", "-1" })
	{
		reset(original);
		staged[1].revision = revision;
		refused(catalog, pids, result::invalid);
	}
	for (int column = 0; column < 4; ++column)
	{
		reset(original);
		null_column = column;
		refused(catalog, pids, result::invalid);
	}
	reset(original);
	null_lengths = true;
	refused(catalog, pids, result::invalid);
	for (const auto &payload :
	     { "", "ZSQF|2\n", "N|1|1|54657374|1\n", "ZSQF|1\nN|broken\n", "ZSQF|1\nN|1|1|5" })
	{
		reset(payload);
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

	for (bool bucketed : { false, true })
	{
		reset(original, bucketed);
		assert(sql_zone_story_quest_state_remove_player_aliases(7, 2, pids, catalog) ==
		       result::ok);
		assert(reads == 1 && last_read == std::string(select_sql) + " FOR UPDATE");
		assert(writes > 0 && decoded(staged) == expected &&
		       decoded(committed) == original_facts);
		clean();
		// The real deletion owner commits after all of its domains are prepared.
		committed = staged;
		writes = 0;
		assert(sql_zone_story_quest_state_remove_player_aliases(7, 2, pids, catalog) ==
		       result::ok);
		assert(writes == 0 && decoded(staged) == expected);
		clean();
	}
	const std::string legacy = "ZSQF|1\nN|1|1|546172676574|1\nN|1|9|556e72656c61746564|1\n";
	reset(legacy);
	assert(sql_zone_story_quest_state_remove_player_aliases(7, 2, pids, catalog) == result::ok);
	const auto legacy_after = zone_story_quest_state::document(decoded(staged));
	assert(legacy_after.starts_with("ZSQF|3\n") &&
	       legacy_after.find("N|1|1|") == std::string::npos &&
	       legacy_after.find("N|1|9|") != std::string::npos);
	clean();
	reset(original);
	escape_failed = true;
	refused(catalog, pids, result::io_error);
	reset(original, true);
	fail_write = 2;
	assert(sql_zone_story_quest_state_remove_player_aliases(7, 2, pids, catalog) ==
	       result::io_error);
	assert(writes == 2 && decoded(committed) == original_facts);
	clean();
	staged = committed; // Caller rollback discards partial preparation.
	assert(decoded(staged) == original_facts);

	zone_story_quest_feature::service large(catalog);
	for (uint32_t pid = 1; pid < 30; ++pid)
		large.remember_character(1, pid, std::string(128, 'x'));
	large.remember_character(7, 1, "historical");
	const auto large_before = large.serialize_state();
	const auto large_facts = zone_story_quest_state::split_document(large_before);
	const auto large_after = erased(large_before, catalog);
	size_t failures = 0, result_failures = 0, escape_failures = 0;
	bool completed = false;
	for (size_t index = 1; index < 10000; ++index)
	{
		reset(large_before, true);
		std::string error;
		allocation_hit = false;
		allocation_remaining = index;
		const auto outcome = sql_zone_story_quest_state_remove_player_aliases(
			7, 2, pids, catalog, &error);
		allocation_remaining = 0;
		clean();
		assert(decoded(committed) == large_facts);
		if (outcome == result::ok)
			assert(decoded(staged) == large_after);
		if (allocation_hit)
		{
			assert(outcome == result::io_error);
			++failures;
			result_failures += results_created > 0;
			escape_failures += escapes_created > 0;
		}
		else
		{
			assert(outcome == result::ok);
			completed = true;
			break;
		}
	}
	assert(completed && failures && result_failures && escape_failures);
	completed = false;
	size_t load_failures = 0;
	for (size_t index = 1; index < 10000; ++index)
	{
		reset(large_before, true);
		std::string destination, error;
		allocation_hit = false;
		allocation_remaining = index;
		const auto outcome = sql_zone_story_quest_state_load(7, &destination, &error);
		allocation_remaining = 0;
		clean();
		assert(writes == 0 && last_read == select_sql);
		if (allocation_hit)
		{
			assert(outcome == result::io_error);
			load_failures += results_created > 0;
		}
		else
		{
			assert(outcome == result::ok &&
			       zone_story_quest_state::split_document(destination) == large_facts);
			completed = true;
			break;
		}
	}
	assert(completed && load_failures);

	// Standalone writers must never commit an existing deletion transaction.
	reset(original, true);
	std::string loaded;
	assert(sql_zone_story_quest_state_load(7, &loaded) == result::ok);
	zone_story_quest_state::changes delta;
	delta.values["character:1:9"] = "N|1|9|52656e616d6564|1\n";
	assert(sql_zone_story_quest_records_save(7, delta) == result::invalid);
	assert(writes == 0);
	clean();
	transaction = false;
	fail_write = 1;
	assert(sql_zone_story_quest_records_save(7, delta) == result::io_error);
	assert(begins == 1 && commits == 0 && rollbacks == 1 &&
	       decoded(committed) == original_facts);
	clean(false);
	fail_write = 0;
	assert(sql_zone_story_quest_records_save(7, delta) == result::ok);
	assert(begins == 2 && commits == 1 && rollbacks == 1);
	auto renamed = original_facts;
	renamed["character:1:9"] = delta.values.at("character:1:9");
	assert(decoded(committed) == renamed);
	clean(false);
	std::cout
		<< "PASS: SQL legacy/bucket erasure, saved encounters, locks, caller rollback, retry and allocation cleanup ("
		<< failures << " injected failures; " << escape_failures
		<< " after escape allocation)\n";
}
