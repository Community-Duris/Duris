#include "player/player_death_recovery_query.h"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

namespace
{
struct fake_result
{
	unsigned int fields = 0;
	std::vector<std::vector<std::string>> rows;
	size_t next = 0;
	std::vector<char *> current;
	std::vector<unsigned long> lengths;
};
std::string last_sql;
unsigned int read_calls = 0, list_calls = 0;
player_revision_t after_revision_seen = 0;
bool fail_list = false, fail_read = false;
const int32_t test_pid = 71;
critical_operation_id case_id = {};
player_snapshot archive = {};
}

extern "C" int mysql_real_query(MYSQL *connection, const char *sql, unsigned long length)
{
	last_sql.assign(sql, length);
	if (last_sql.find("START TRANSACTION") != std::string::npos)
		connection->server_status = SERVER_STATUS_IN_TRANS;
	else if (last_sql == "COMMIT" || last_sql == "ROLLBACK")
		connection->server_status = SERVER_STATUS_AUTOCOMMIT;
	return 0;
}
extern "C" int mysql_query(MYSQL *connection, const char *sql)
{
	return mysql_real_query(connection, sql, std::strlen(sql));
}
extern "C" MYSQL_RES *mysql_store_result(MYSQL *)
{
	if (last_sql.find("SELECT ac.pid FROM account_characters") == std::string::npos)
		return nullptr;
	auto *result = new fake_result();
	result->fields = 1;
	if (last_sql.find("ac.pid=71") != std::string::npos &&
	    last_sql.find("LOWER(ac.account_name)=LOWER('owner')") != std::string::npos &&
	    last_sql.find("LOWER(ac.char_name)=LOWER('Hero')") != std::string::npos &&
	    last_sql.find("LOWER(pd.name)=LOWER('Hero')") != std::string::npos)
		result->rows.push_back({ "71" });
	return reinterpret_cast<MYSQL_RES *>(result);
}
extern "C" unsigned int mysql_field_count(MYSQL *)
{
	return 0;
}
extern "C" unsigned int mysql_num_fields(MYSQL_RES *rows)
{
	return reinterpret_cast<fake_result *>(rows)->fields;
}
extern "C" my_ulonglong mysql_num_rows(MYSQL_RES *rows)
{
	return reinterpret_cast<fake_result *>(rows)->rows.size();
}
extern "C" MYSQL_ROW mysql_fetch_row(MYSQL_RES *rows)
{
	auto *result = reinterpret_cast<fake_result *>(rows);
	if (result->next >= result->rows.size())
		return nullptr;
	const auto &row = result->rows[result->next++];
	result->current.clear();
	result->lengths.clear();
	for (const auto &value : row)
	{
		result->current.push_back(const_cast<char *>(value.c_str()));
		result->lengths.push_back(value.size());
	}
	return result->current.data();
}
extern "C" unsigned long *mysql_fetch_lengths(MYSQL_RES *rows)
{
	return reinterpret_cast<fake_result *>(rows)->lengths.data();
}
extern "C" void mysql_free_result(MYSQL_RES *rows)
{
	delete reinterpret_cast<fake_result *>(rows);
}
extern "C" unsigned long mysql_real_escape_string(MYSQL *, char *out, const char *in,
						  unsigned long length)
{
	std::memcpy(out, in, length);
	out[length] = '\0';
	return length;
}
extern "C" unsigned int mysql_errno(MYSQL *)
{
	return 0;
}
extern "C" const char *mysql_sqlstate(MYSQL *)
{
	return "00000";
}

extern "C" void sql_pool_discard_connection(MYSQL *) {}

bool critical_operation_id_is_zero(const critical_operation_id &id)
{
	for (uint8_t byte : id.bytes)
		if (byte)
			return false;
	return true;
}

player_death_conflict_result player_death_conflict_read(MYSQL *, int32_t pid,
							const critical_operation_id &id,
							player_snapshot *output) noexcept
{
	++read_calls;
	if (fail_read)
		return { player_death_conflict_outcome::failed, EIO, 0 };
	if (pid != test_pid || id.bytes != case_id.bytes)
		return { player_death_conflict_outcome::not_found, ENOENT, 0 };
	*output = archive;
	return { player_death_conflict_outcome::read, 0, 21 };
}

player_death_conflict_result
player_death_conflict_list(MYSQL *, int32_t pid, player_revision_t after,
			   std::vector<player_death_conflict_case> *output) noexcept
{
	++list_calls;
	after_revision_seen = after;
	if (fail_list)
		return { player_death_conflict_outcome::failed, 1146, 0 };
	if (pid != test_pid)
		return { player_death_conflict_outcome::failed, EINVAL, 0 };
	player_death_conflict_case entry = {};
	entry.operation_id = case_id;
	entry.save_revision = 22;
	entry.source_revision = 21;
	entry.corpse_item_uid = 901;
	if (entry.save_revision > after)
		output->push_back(entry);
	return { player_death_conflict_outcome::read, 0, 21 };
}

int main()
{
	case_id.bytes[0] = 0x2a;
	archive.schema_version = PLAYER_SNAPSHOT_DEATH_EVIDENCE_SCHEMA_VERSION;
	archive.pid = test_pid;
	archive.revision = 22;
	archive.death.emplace();
	archive.death->operation_id = case_id;
	player_item_snapshot item = {};
	item.object_uid = 901;
	item.vnum = 4102;
	item.name = "  &R sword\x1b[31m $gold %s \xc3\xa9 ";
	archive.death->corpse.push_back(item);
	archive.death->conflict_evidence.emplace();
	archive.death->conflict_evidence->player_items.rows.push_back({ std::nullopt });
	archive.death->conflict_evidence->player_items.rows.push_back({ std::string() });

	player_death_recovery_query_request request = {};
	request.kind = player_death_recovery_query_kind::list;
	assert(player_death_recovery_query_request_valid(request, test_pid, "owner", "Hero"));
	assert(!player_death_recovery_query_request_valid(request, 0, "owner", "Hero"));
	assert(!player_death_recovery_query_request_valid(request, test_pid, "", "Hero"));
	assert(!player_death_recovery_query_request_valid(request, test_pid, "owner", "Hero\n"));

	MYSQL connection = {};
	connection.server_status = SERVER_STATUS_AUTOCOMMIT;
	request.after_revision = 11;
	auto listed = player_death_recovery_query_execute(&connection, test_pid, "owner", "Hero",
							  request);
	assert(listed.outcome == player_death_recovery_query_outcome::read);
	assert(listed.cases.size() == 1 && after_revision_seen == 11);
	assert(list_calls == 1 && read_calls == 0);
	assert(connection.server_status == SERVER_STATUS_AUTOCOMMIT);

	request.after_revision = 22;
	auto next_page = player_death_recovery_query_execute(&connection, test_pid, "owner", "Hero",
							     request);
	assert(next_page.outcome == player_death_recovery_query_outcome::read);
	assert(next_page.cases.empty() && after_revision_seen == 22);
	assert(list_calls == 2);

	auto unauthorized = player_death_recovery_query_execute(&connection, test_pid, "other",
								"Hero", request);
	assert(unauthorized.outcome == player_death_recovery_query_outcome::unauthorized);
	assert(list_calls == 2);

	fail_list = true;
	auto missing_schema = player_death_recovery_query_execute(&connection, test_pid, "owner",
								  "Hero", request);
	assert(missing_schema.outcome == player_death_recovery_query_outcome::failed);
	assert(missing_schema.error_code == 1146 && missing_schema.cases.empty());
	assert(connection.server_status == SERVER_STATUS_AUTOCOMMIT);
	fail_list = false;

	request = {};
	request.kind = player_death_recovery_query_kind::detail;
	request.operation_id = case_id;
	assert(player_death_recovery_query_request_valid(request, test_pid, "owner", "Hero"));
	auto detail = player_death_recovery_query_execute(&connection, test_pid, "owner", "Hero",
							  request);
	assert(detail.outcome == player_death_recovery_query_outcome::read);
	assert(detail.detail_identity.operation_id.bytes == case_id.bytes);
	assert(player_death_recovery_detail_identity_valid(test_pid, case_id,
							   detail.detail_identity, archive));
	assert(!player_death_recovery_detail_identity_valid(test_pid + 1, case_id,
							    detail.detail_identity, archive));
	critical_operation_id other_identity = case_id;
	other_identity.bytes[1] = 1;
	assert(!player_death_recovery_detail_identity_valid(test_pid, other_identity,
							    detail.detail_identity, archive));
	assert(detail.detail_summary.size() <= PLAYER_DEATH_RECOVERY_SUMMARY_MAX);
	assert(detail.detail_summary.find("sword") != std::string::npos);
	assert(detail.detail_summary.find("correlation=") != std::string::npos);
	assert(detail.detail_summary.find("recovery_owner=retained_death_conflict") !=
	       std::string::npos);
	assert(detail.detail_summary.find("Unresolved archive evidence only") != std::string::npos);
	assert(detail.detail_summary.find("not a completed terminal recovery") !=
	       std::string::npos);
	assert(detail.detail_summary.find("&R") == std::string::npos);
	assert(detail.detail_summary.find("$gold") == std::string::npos);
	assert(detail.detail_summary.find("%s") == std::string::npos);
	assert(detail.detail_summary.find('\x1b') == std::string::npos);
	assert(detail.detail_summary.find("items=2") != std::string::npos);
	assert(detail.detail_summary.find("Raw payload withheld") != std::string::npos);

	critical_operation_id wrong_case = case_id;
	wrong_case.bytes[1] = 1;
	request.operation_id = wrong_case;
	auto missing_case = player_death_recovery_query_execute(&connection, test_pid, "owner",
								"Hero", request);
	assert(missing_case.outcome == player_death_recovery_query_outcome::not_found);
	request.operation_id = case_id;
	fail_read = true;
	auto read_failure = player_death_recovery_query_execute(&connection, test_pid, "owner",
								"Hero", request);
	assert(read_failure.outcome == player_death_recovery_query_outcome::failed);
	assert(read_failure.detail_summary.empty());
	assert(connection.server_status == SERVER_STATUS_AUTOCOMMIT);
	return 0;
}
