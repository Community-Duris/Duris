#include "persistence/player_death_restitution_repository.h"

#include "item/item_transfer_command.h"
#include "persistence/critical_command_repository.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <ctime>
#include <cstring>
#include <limits>
#include <mysql.h>
#include <openssl/sha.h>
#include <string>
#include <type_traits>
#include <vector>

namespace
{
constexpr uint8_t ITEM_OWNER_PLAYER = 1;
thread_local unsigned int last_statement_error = 0;
using mysql_null_indicator = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;

struct owner_row
{
	uint64_t root_uid;
	uint64_t parent_uid;
	uint64_t revision;
	uint8_t state;
	uint32_t vnum;
};

struct projected_item
{
	uint64_t uid;
	uint32_t id;
	std::vector<uint8_t> state_payload;
	std::array<uint8_t, 32> state_digest;
};

bool exec_sql(MYSQL *connection, const char *sql)
{
	return connection && mysql_real_query(connection, sql, strlen(sql)) == 0;
}

bool connection_error(unsigned int error)
{
	return error == 2006 || error == 2013 || error == 2055;
}

bool retryable_error(unsigned int error)
{
	return connection_error(error) || error == EAGAIN || error == 1205 || error == 1213;
}

critical_apply_result failure(unsigned int error)
{
	return { retryable_error(error) ? critical_apply_outcome::retryable_failure :
					  critical_apply_outcome::terminal_failure,
		 0, error };
}

void rollback(MYSQL *connection)
{
	if (connection && !connection_error(mysql_errno(connection)))
		(void)exec_sql(connection, "ROLLBACK");
}

bool prepare(MYSQL_STMT **statement, MYSQL *connection, const char *sql)
{
	if (!statement || !connection)
		return false;
	last_statement_error = 0;
	*statement = mysql_stmt_init(connection);
	if (!*statement)
		return false;
	if (mysql_stmt_prepare(*statement, sql, strlen(sql)) != 0)
	{
		last_statement_error = mysql_stmt_errno(*statement);
		mysql_stmt_close(*statement);
		*statement = nullptr;
		return false;
	}
	return true;
}

bool statement_failure(MYSQL_STMT *statement)
{
	if (statement)
	{
		last_statement_error = mysql_stmt_errno(statement);
		mysql_stmt_close(statement);
	}
	return false;
}

unsigned int database_error(MYSQL *connection)
{
	return last_statement_error ? last_statement_error :
				      (connection ? mysql_errno(connection) : EINVAL);
}

template <typename T>
void bind_int(MYSQL_BIND *binding, enum_field_types type, T *value, bool uns = false)
{
	*binding = {};
	binding->buffer_type = type;
	binding->buffer = value;
	binding->is_unsigned = uns;
}

void bind_blob(MYSQL_BIND *binding, void *value, unsigned long *length)
{
	*binding = {};
	binding->buffer_type = MYSQL_TYPE_BLOB;
	binding->buffer = value;
	binding->buffer_length = length ? *length : 0;
	binding->length = length;
}

void bind_string(MYSQL_BIND *binding, void *value, unsigned long *length,
		 mysql_null_indicator *is_null = nullptr)
{
	*binding = {};
	binding->buffer_type = MYSQL_TYPE_STRING;
	binding->buffer = value;
	binding->buffer_length = length ? *length : 0;
	binding->length = length;
	binding->is_null = is_null;
}

bool read_save_revision(MYSQL *connection, uint32_t pid, uint64_t *revision,
			unsigned int *error_code)
{
	static const char SQL[] = "SELECT save_revision FROM player_data WHERE pid=? FOR UPDATE";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	MYSQL_BIND parameter = {};
	bind_int(&parameter, MYSQL_TYPE_LONG, &pid, true);
	MYSQL_BIND result = {};
	uint64_t actual = 0;
	bind_int(&result, MYSQL_TYPE_LONGLONG, &actual, true);
	if (mysql_stmt_bind_param(statement, &parameter) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0 ||
	    mysql_stmt_bind_result(statement, &result) != 0)
		return statement_failure(statement);
	const int fetched = mysql_stmt_fetch(statement);
	if (fetched == MYSQL_NO_DATA)
	{
		mysql_stmt_close(statement);
		*error_code = ENOENT;
		return true;
	}
	if (fetched != 0)
		return statement_failure(statement);
	mysql_stmt_close(statement);
	*revision = actual;
	return true;
}

bool verify_death_evidence(MYSQL *connection, const player_death_restitution_plan &plan,
			   unsigned int *error_code)
{
	static const char SQL[] =
		"SELECT 1 FROM player_death_disposition WHERE pid=? AND save_revision=? "
		"AND operation_id=? AND UNHEX(SHA2(payload,256))=? "
		"AND FLOOR(UNIX_TIMESTAMP(recorded_at))=? FOR UPDATE";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint32_t pid = plan.source_pid;
	uint64_t save_revision = plan.death_revision;
	int64_t loss_epoch = static_cast<int64_t>(plan.loss_epoch);
	unsigned long operation_length = plan.death_operation_id.bytes.size();
	unsigned long digest_length = plan.payload_digest.size();
	MYSQL_BIND parameters[5] = {};
	bind_int(&parameters[0], MYSQL_TYPE_LONG, &pid, true);
	bind_int(&parameters[1], MYSQL_TYPE_LONGLONG, &save_revision, true);
	bind_blob(&parameters[2], const_cast<uint8_t *>(plan.death_operation_id.bytes.data()),
		  &operation_length);
	bind_blob(&parameters[3], const_cast<uint8_t *>(plan.payload_digest.data()),
		  &digest_length);
	bind_int(&parameters[4], MYSQL_TYPE_LONGLONG, &loss_epoch, false);
	uint8_t found_value = 0;
	MYSQL_BIND result = {};
	bind_int(&result, MYSQL_TYPE_TINY, &found_value, true);
	if (mysql_stmt_bind_param(statement, parameters) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0 ||
	    mysql_stmt_bind_result(statement, &result) != 0)
		return statement_failure(statement);
	const int fetched = mysql_stmt_fetch(statement);
	if (fetched == MYSQL_NO_DATA)
	{
		mysql_stmt_close(statement);
		*error_code = ESTALE;
		return true;
	}
	if (fetched != 0)
		return statement_failure(statement);
	mysql_stmt_close(statement);
	return true;
}

bool verify_custody_evidence(MYSQL *connection, const player_death_restitution_plan &plan,
			     const player_death_restitution_item &item, unsigned int *error_code)
{
	static const char SQL[] =
		"SELECT 1 FROM player_death_custody WHERE pid=? AND save_revision=? AND item_uid=? "
		"AND root_item_uid=? AND parent_item_uid=? AND item_revision=? AND vnum=? "
		"AND state=? AND owner_type=? AND owner_id=? AND owner_context_id=? "
		"AND owner_revision=? FOR UPDATE";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint32_t pid = plan.source_pid;
	uint64_t save_revision = plan.death_revision;
	uint64_t item_uid = item.item_uid, root_uid = item.source_root_item_uid,
		 parent_uid = item.source_parent_item_uid,
		 source_revision = item.custody_item_revision,
		 custody_owner_id = item.custody_owner_id,
		 custody_owner_context_id = item.custody_owner_context_id,
		 custody_owner_revision = item.custody_owner_revision;
	int32_t vnum = static_cast<int32_t>(item.vnum);
	uint8_t custody_state = item.custody_state;
	uint8_t custody_owner_type = item.custody_owner_type;
	MYSQL_BIND parameters[12] = {};
	bind_int(&parameters[0], MYSQL_TYPE_LONG, &pid, true);
	bind_int(&parameters[1], MYSQL_TYPE_LONGLONG, &save_revision, true);
	bind_int(&parameters[2], MYSQL_TYPE_LONGLONG, &item_uid, true);
	bind_int(&parameters[3], MYSQL_TYPE_LONGLONG, &root_uid, true);
	bind_int(&parameters[4], MYSQL_TYPE_LONGLONG, &parent_uid, true);
	bind_int(&parameters[5], MYSQL_TYPE_LONGLONG, &source_revision, true);
	bind_int(&parameters[6], MYSQL_TYPE_LONG, &vnum, false);
	bind_int(&parameters[7], MYSQL_TYPE_TINY, &custody_state, true);
	bind_int(&parameters[8], MYSQL_TYPE_TINY, &custody_owner_type, true);
	bind_int(&parameters[9], MYSQL_TYPE_LONGLONG, &custody_owner_id, true);
	bind_int(&parameters[10], MYSQL_TYPE_LONGLONG, &custody_owner_context_id, true);
	bind_int(&parameters[11], MYSQL_TYPE_LONGLONG, &custody_owner_revision, true);
	uint8_t found_value = 0;
	MYSQL_BIND result = {};
	bind_int(&result, MYSQL_TYPE_TINY, &found_value, true);
	if (mysql_stmt_bind_param(statement, parameters) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0 ||
	    mysql_stmt_bind_result(statement, &result) != 0)
		return statement_failure(statement);
	const int fetched = mysql_stmt_fetch(statement);
	if (fetched == MYSQL_NO_DATA)
	{
		mysql_stmt_close(statement);
		*error_code = ESTALE;
		return true;
	}
	if (fetched != 0)
		return statement_failure(statement);
	mysql_stmt_close(statement);
	return true;
}

bool ensure_owner_revision(MYSQL *connection, uint32_t owner_id)
{
	static const char SQL[] =
		"INSERT IGNORE INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
		"VALUES(1,?,0,0)";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	MYSQL_BIND parameter = {};
	bind_int(&parameter, MYSQL_TYPE_LONG, &owner_id, true);
	if (mysql_stmt_bind_param(statement, &parameter) != 0 || mysql_stmt_execute(statement) != 0)
		return statement_failure(statement);
	mysql_stmt_close(statement);
	return true;
}

bool read_owner_revision(MYSQL *connection, uint32_t owner_id, uint64_t expected_revision,
			 uint64_t *revision, unsigned int *error_code)
{
	static const char SQL[] =
		"SELECT revision FROM item_owner_revision WHERE owner_type=? AND owner_id=? "
		"AND owner_context_id=0 FOR UPDATE";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint8_t owner_type = ITEM_OWNER_PLAYER;
	MYSQL_BIND parameters[2] = {};
	bind_int(&parameters[0], MYSQL_TYPE_TINY, &owner_type, true);
	bind_int(&parameters[1], MYSQL_TYPE_LONG, &owner_id, true);
	MYSQL_BIND result = {};
	uint64_t actual = 0;
	bind_int(&result, MYSQL_TYPE_LONGLONG, &actual, true);
	if (mysql_stmt_bind_param(statement, parameters) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0 ||
	    mysql_stmt_bind_result(statement, &result) != 0)
		return statement_failure(statement);
	const int fetched = mysql_stmt_fetch(statement);
	if (fetched == MYSQL_NO_DATA)
	{
		mysql_stmt_close(statement);
		*error_code = ENOENT;
		return true;
	}
	if (fetched != 0)
		return statement_failure(statement);
	mysql_stmt_close(statement);
	*revision = actual;
	if (actual != expected_revision)
		*error_code = ESTALE;
	return true;
}

bool lock_owner(MYSQL *connection, const player_death_restitution_plan &plan,
		const player_death_restitution_item &item, owner_row *owner,
		unsigned int *error_code)
{
	static const char SQL[] =
		"SELECT root_item_uid,COALESCE(parent_item_uid,0),item_revision,state,vnum "
		"FROM item_current_owner WHERE item_uid=? AND owner_type=1 AND owner_id=? "
		"AND owner_context_id=0 FOR UPDATE";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint64_t uid = item.item_uid;
	uint32_t source_pid = plan.source_pid;
	MYSQL_BIND parameters[2] = {};
	bind_int(&parameters[0], MYSQL_TYPE_LONGLONG, &uid, true);
	bind_int(&parameters[1], MYSQL_TYPE_LONG, &source_pid, true);
	MYSQL_BIND results[5] = {};
	bind_int(&results[0], MYSQL_TYPE_LONGLONG, &owner->root_uid, true);
	bind_int(&results[1], MYSQL_TYPE_LONGLONG, &owner->parent_uid, true);
	bind_int(&results[2], MYSQL_TYPE_LONGLONG, &owner->revision, true);
	bind_int(&results[3], MYSQL_TYPE_TINY, &owner->state, true);
	bind_int(&results[4], MYSQL_TYPE_LONG, &owner->vnum, false);
	if (mysql_stmt_bind_param(statement, parameters) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0 ||
	    mysql_stmt_bind_result(statement, results) != 0)
		return statement_failure(statement);
	const int fetched = mysql_stmt_fetch(statement);
	if (fetched == MYSQL_NO_DATA)
	{
		mysql_stmt_close(statement);
		*error_code = ESTALE;
		return true;
	}
	if (fetched != 0)
		return statement_failure(statement);
	mysql_stmt_close(statement);
	if (owner->root_uid != item.source_root_item_uid ||
	    owner->parent_uid != item.source_parent_item_uid ||
	    owner->revision != item.expected_item_revision ||
	    owner->state != item.expected_owner_state || owner->vnum != item.vnum)
		*error_code = ESTALE;
	return true;
}

bool uid_exists(MYSQL *connection, const char *sql, uint64_t uid, bool *exists)
{
	if (!exists)
		return false;
	*exists = false;
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, sql))
		return false;
	MYSQL_BIND parameter = {};
	bind_int(&parameter, MYSQL_TYPE_LONGLONG, &uid, true);
	uint8_t value = 0;
	MYSQL_BIND result = {};
	bind_int(&result, MYSQL_TYPE_TINY, &value, true);
	if (mysql_stmt_bind_param(statement, &parameter) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0 ||
	    mysql_stmt_bind_result(statement, &result) != 0)
		return statement_failure(statement);
	const int fetched = mysql_stmt_fetch(statement);
	if (fetched == MYSQL_NO_DATA)
	{
		mysql_stmt_close(statement);
		return true;
	}
	if (fetched != 0)
		return statement_failure(statement);
	*exists = true;
	mysql_stmt_close(statement);
	return true;
}

struct artifact_authority_lock
{
	bool domain_present;
	bool domain_item_uid_present;
	uint64_t domain_item_revision;
	bool baseline_present;
	bool bind_present;
	bool god_projection_present;
	bool mortal_projection_present;
};

bool mark_stale(unsigned int *error_code)
{
	if (error_code)
		*error_code = ESTALE;
	return true;
}

bool lock_artifact_domain(MYSQL *connection, const player_death_restitution_item &item,
			  artifact_authority_lock *locked, unsigned int *error_code)
{
	static const char SQL[] =
		"SELECT owned,loc_type,location,timer_epoch,artifact_type,bind_owner_pid,"
		"bind_timer_epoch,item_uid,COALESCE(item_revision,0),revision "
		"FROM artifact_domain_state WHERE vnum=? FOR UPDATE";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	int32_t vnum = static_cast<int32_t>(item.artifact_vnum);
	MYSQL_BIND parameter = {};
	bind_int(&parameter, MYSQL_TYPE_LONG, &vnum, false);
	uint8_t owned = 0, loc_type = 0, artifact_type = 0;
	int32_t location = 0, bind_owner = 0;
	int64_t timer_epoch = 0, bind_timer_epoch = 0;
	uint64_t item_uid = 0, item_revision = 0, revision = 0;
	mysql_null_indicator item_uid_null = 0;
	MYSQL_BIND results[10] = {};
	bind_int(&results[0], MYSQL_TYPE_TINY, &owned, true);
	bind_int(&results[1], MYSQL_TYPE_TINY, &loc_type, true);
	bind_int(&results[2], MYSQL_TYPE_LONG, &location, false);
	bind_int(&results[3], MYSQL_TYPE_LONGLONG, &timer_epoch, false);
	bind_int(&results[4], MYSQL_TYPE_TINY, &artifact_type, true);
	bind_int(&results[5], MYSQL_TYPE_LONG, &bind_owner, false);
	bind_int(&results[6], MYSQL_TYPE_LONGLONG, &bind_timer_epoch, false);
	bind_int(&results[7], MYSQL_TYPE_LONGLONG, &item_uid, true);
	results[7].is_null = &item_uid_null;
	bind_int(&results[8], MYSQL_TYPE_LONGLONG, &item_revision, true);
	bind_int(&results[9], MYSQL_TYPE_LONGLONG, &revision, true);
	if (mysql_stmt_bind_param(statement, &parameter) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0 ||
	    mysql_stmt_bind_result(statement, results) != 0)
		return statement_failure(statement);
	const int fetched = mysql_stmt_fetch(statement);
	if (fetched == MYSQL_NO_DATA)
	{
		mysql_stmt_close(statement);
		locked->domain_present = false;
		return true;
	}
	if (fetched != 0)
		return statement_failure(statement);
	mysql_stmt_close(statement);
	locked->domain_present = true;
	locked->domain_item_uid_present = !item_uid_null;
	locked->domain_item_revision = item_revision;
	const bool uid_matches =
		item.artifact_domain_item_uid_present ?
			(!item_uid_null && item_uid == item.artifact_domain_item_uid) :
			item_uid_null;
	if (!item.artifact_domain_present || owned != 1 ||
	    loc_type != item.artifact_source_location_type ||
	    location != item.artifact_source_location ||
	    timer_epoch != static_cast<int64_t>(item.artifact_source_timer_epoch) ||
	    artifact_type != item.artifact_type || bind_owner != item.artifact_bind_owner_pid ||
	    bind_timer_epoch != item.artifact_bind_timer_epoch || !uid_matches ||
	    item_revision != item.artifact_domain_item_revision ||
	    revision != item.artifact_domain_revision)
		return mark_stale(error_code);
	return true;
}

bool lock_artifact_baseline(MYSQL *connection, const player_death_restitution_item &item,
			    artifact_authority_lock *locked, unsigned int *error_code)
{
	static const char SQL[] =
		"SELECT opening_timer_epoch,opening_bind_owner_pid,opening_bind_timer_epoch,"
		"opening_revision FROM artifact_domain_baseline WHERE vnum=? FOR UPDATE";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	int32_t vnum = static_cast<int32_t>(item.artifact_vnum);
	MYSQL_BIND parameter = {};
	bind_int(&parameter, MYSQL_TYPE_LONG, &vnum, false);
	int64_t opening_timer = 0, opening_bind_timer = 0;
	int32_t opening_bind_owner = 0;
	uint64_t opening_revision = 0;
	MYSQL_BIND results[4] = {};
	bind_int(&results[0], MYSQL_TYPE_LONGLONG, &opening_timer, false);
	bind_int(&results[1], MYSQL_TYPE_LONG, &opening_bind_owner, false);
	bind_int(&results[2], MYSQL_TYPE_LONGLONG, &opening_bind_timer, false);
	bind_int(&results[3], MYSQL_TYPE_LONGLONG, &opening_revision, true);
	if (mysql_stmt_bind_param(statement, &parameter) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0 ||
	    mysql_stmt_bind_result(statement, results) != 0)
		return statement_failure(statement);
	const int fetched = mysql_stmt_fetch(statement);
	if (fetched == MYSQL_NO_DATA)
	{
		mysql_stmt_close(statement);
		locked->baseline_present = false;
		if (item.artifact_baseline_present)
			return mark_stale(error_code);
		return true;
	}
	if (fetched != 0)
		return statement_failure(statement);
	mysql_stmt_close(statement);
	locked->baseline_present = true;
	if (!item.artifact_baseline_present ||
	    opening_timer != static_cast<int64_t>(item.artifact_baseline_opening_timer_epoch) ||
	    opening_bind_owner != item.artifact_baseline_opening_bind_owner_pid ||
	    opening_bind_timer != item.artifact_baseline_opening_bind_timer_epoch ||
	    opening_revision != item.artifact_baseline_opening_revision)
		return mark_stale(error_code);
	return true;
}

bool lock_artifact_bind(MYSQL *connection, const player_death_restitution_item &item,
			artifact_authority_lock *locked, unsigned int *error_code)
{
	static const char SQL[] =
		"SELECT COALESCE(owner_pid,-1),COALESCE(timer,0) FROM artifact_bind "
		"WHERE vnum=? FOR UPDATE";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	int32_t vnum = static_cast<int32_t>(item.artifact_vnum);
	MYSQL_BIND parameter = {};
	bind_int(&parameter, MYSQL_TYPE_LONG, &vnum, false);
	int32_t owner_pid = 0, timer = 0;
	MYSQL_BIND results[2] = {};
	bind_int(&results[0], MYSQL_TYPE_LONG, &owner_pid, false);
	bind_int(&results[1], MYSQL_TYPE_LONG, &timer, false);
	if (mysql_stmt_bind_param(statement, &parameter) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0 ||
	    mysql_stmt_bind_result(statement, results) != 0)
		return statement_failure(statement);
	const int fetched = mysql_stmt_fetch(statement);
	if (fetched == MYSQL_NO_DATA)
	{
		mysql_stmt_close(statement);
		locked->bind_present = false;
		if (item.artifact_bind_present)
			return mark_stale(error_code);
		return true;
	}
	if (fetched != 0)
		return statement_failure(statement);
	mysql_stmt_close(statement);
	locked->bind_present = true;
	if (!item.artifact_bind_present || owner_pid != item.artifact_bind_owner_pid ||
	    timer != item.artifact_bind_timer_epoch)
		return mark_stale(error_code);
	return true;
}

bool lock_artifact_legacy(MYSQL *connection, const player_death_restitution_item &item,
			  const char *table, uint8_t expected_bit, bool *present,
			  unsigned int *error_code)
{
	const std::string SQL =
		std::string("SELECT (owned='Y'),COALESCE(locType,0),COALESCE(location,0),") +
		"COALESCE(UNIX_TIMESTAMP(timer),0),COALESCE(type,0) FROM " + table +
		" WHERE vnum=? FOR UPDATE";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL.c_str()))
		return false;
	int32_t vnum = static_cast<int32_t>(item.artifact_vnum);
	MYSQL_BIND parameter = {};
	bind_int(&parameter, MYSQL_TYPE_LONG, &vnum, false);
	uint8_t owned = 0, loc_type = 0;
	int32_t location = 0, artifact_type = 0;
	int64_t timer_epoch = 0;
	MYSQL_BIND results[5] = {};
	bind_int(&results[0], MYSQL_TYPE_TINY, &owned, true);
	bind_int(&results[1], MYSQL_TYPE_TINY, &loc_type, true);
	bind_int(&results[2], MYSQL_TYPE_LONG, &location, false);
	bind_int(&results[3], MYSQL_TYPE_LONGLONG, &timer_epoch, false);
	bind_int(&results[4], MYSQL_TYPE_LONG, &artifact_type, false);
	if (mysql_stmt_bind_param(statement, &parameter) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0 ||
	    mysql_stmt_bind_result(statement, results) != 0)
		return statement_failure(statement);
	const int fetched = mysql_stmt_fetch(statement);
	if (fetched == MYSQL_NO_DATA)
	{
		mysql_stmt_close(statement);
		*present = false;
		if (item.artifact_legacy_projection_mask & expected_bit)
			return mark_stale(error_code);
		return true;
	}
	if (fetched != 0)
		return statement_failure(statement);
	mysql_stmt_close(statement);
	*present = true;
	if (!(item.artifact_legacy_projection_mask & expected_bit) || owned != 1 ||
	    loc_type != item.artifact_source_location_type ||
	    location != item.artifact_source_location ||
	    timer_epoch != static_cast<int64_t>(item.artifact_source_timer_epoch) ||
	    artifact_type != item.artifact_type)
		return mark_stale(error_code);
	return true;
}

bool lock_artifact_competitors(MYSQL *connection, const player_death_restitution_item &item,
			       unsigned int *error_code)
{
	static const char CURRENT_SQL[] =
		"SELECT item_uid,state FROM item_current_owner WHERE vnum=? AND item_uid<>? "
		"AND state<>2 FOR UPDATE";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, CURRENT_SQL))
		return false;
	int32_t vnum = static_cast<int32_t>(item.artifact_vnum);
	uint64_t uid = item.item_uid;
	MYSQL_BIND parameters[2] = {};
	bind_int(&parameters[0], MYSQL_TYPE_LONG, &vnum, false);
	bind_int(&parameters[1], MYSQL_TYPE_LONGLONG, &uid, true);
	uint64_t competing_uid = 0;
	uint8_t competing_state = 0;
	MYSQL_BIND results[2] = {};
	bind_int(&results[0], MYSQL_TYPE_LONGLONG, &competing_uid, true);
	bind_int(&results[1], MYSQL_TYPE_TINY, &competing_state, true);
	if (mysql_stmt_bind_param(statement, parameters) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0 ||
	    mysql_stmt_bind_result(statement, results) != 0)
		return statement_failure(statement);
	bool found = false;
	while (true)
	{
		const int fetched = mysql_stmt_fetch(statement);
		if (fetched == MYSQL_NO_DATA)
			break;
		if (fetched != 0)
			return statement_failure(statement);
		found = true;
	}
	mysql_stmt_close(statement);
	if (found)
		return mark_stale(error_code);

	static const char CUSTODY_SQL[] =
		"SELECT c.item_uid,c.state,COALESCE(resolved.state,0) "
		"FROM player_death_custody c LEFT JOIN item_current_owner resolved "
		"ON resolved.item_uid=c.item_uid WHERE c.vnum=? AND c.item_uid<>? "
		"AND c.state<>2 AND (resolved.item_uid IS NULL OR resolved.state<>2) FOR UPDATE";
	if (!prepare(&statement, connection, CUSTODY_SQL))
		return false;
	MYSQL_BIND custody_parameters[2] = {};
	bind_int(&custody_parameters[0], MYSQL_TYPE_LONG, &vnum, false);
	bind_int(&custody_parameters[1], MYSQL_TYPE_LONGLONG, &uid, true);
	uint64_t custody_uid = 0;
	uint8_t custody_state = 0, resolved_state = 0;
	MYSQL_BIND custody_results[3] = {};
	bind_int(&custody_results[0], MYSQL_TYPE_LONGLONG, &custody_uid, true);
	bind_int(&custody_results[1], MYSQL_TYPE_TINY, &custody_state, true);
	bind_int(&custody_results[2], MYSQL_TYPE_TINY, &resolved_state, true);
	if (mysql_stmt_bind_param(statement, custody_parameters) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_store_result(statement) != 0 ||
	    mysql_stmt_bind_result(statement, custody_results) != 0)
		return statement_failure(statement);
	found = false;
	while (true)
	{
		const int fetched = mysql_stmt_fetch(statement);
		if (fetched == MYSQL_NO_DATA)
			break;
		if (fetched != 0)
			return statement_failure(statement);
		found = true;
	}
	mysql_stmt_close(statement);
	if (found)
		return mark_stale(error_code);
	return true;
}

bool lock_artifact_authority(MYSQL *connection, const player_death_restitution_item &item,
			     artifact_authority_lock *locked, unsigned int *error_code)
{
	*locked = {};
	if (!lock_artifact_domain(connection, item, locked, error_code))
		return false;
	if (*error_code)
		return true;
	if (!lock_artifact_baseline(connection, item, locked, error_code))
		return false;
	if (*error_code)
		return true;
	if (!lock_artifact_bind(connection, item, locked, error_code))
		return false;
	if (*error_code)
		return true;
	if (!lock_artifact_legacy(connection, item, "artifacts",
				  PLAYER_DEATH_RESTITUTION_ARTIFACT_LEGACY_GOD,
				  &locked->god_projection_present, error_code))
		return false;
	if (*error_code)
		return true;
	if (!lock_artifact_legacy(connection, item, "artifacts_mortal",
				  PLAYER_DEATH_RESTITUTION_ARTIFACT_LEGACY_MORTAL,
				  &locked->mortal_projection_present, error_code))
		return false;
	if (*error_code)
		return true;
	return lock_artifact_competitors(connection, item, error_code);
}

bool insert_receipt(MYSQL *connection, const player_death_restitution_plan &plan,
		    uint16_t candidates, uint16_t delivered, uint16_t unresolved)
{
	static const char SQL[] =
		"INSERT INTO player_death_restitution_receipt(restitution_id,source_pid,death_revision,"
		"recipient_pid,death_operation_id,evidence_digest,plan_digest,status,actor,reason,"
		"candidate_count,delivered_count,unresolved_count,applied_at) VALUES(?,?,?,?,?,?,?,2,?,?,?,?,?,CURRENT_TIMESTAMP(6))";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	unsigned long rid_length = plan.restitution_id.bytes.size();
	unsigned long death_length = plan.death_operation_id.bytes.size();
	unsigned long evidence_length = plan.evidence_digest.size();
	unsigned long digest_length = plan.plan_digest.size();
	unsigned long actor_length = plan.actor.size();
	unsigned long reason_length = plan.reason.size();
	uint32_t source_pid = plan.source_pid, recipient_pid = plan.recipient_pid;
	uint64_t death_revision = plan.death_revision;
	uint16_t candidate_count = candidates, delivered_count = delivered,
		 unresolved_count = unresolved;
	MYSQL_BIND parameters[12] = {};
	bind_blob(&parameters[0], const_cast<uint8_t *>(plan.restitution_id.bytes.data()),
		  &rid_length);
	bind_int(&parameters[1], MYSQL_TYPE_LONG, &source_pid, true);
	bind_int(&parameters[2], MYSQL_TYPE_LONGLONG, &death_revision, true);
	bind_int(&parameters[3], MYSQL_TYPE_LONG, &recipient_pid, true);
	bind_blob(&parameters[4], const_cast<uint8_t *>(plan.death_operation_id.bytes.data()),
		  &death_length);
	bind_blob(&parameters[5], const_cast<uint8_t *>(plan.evidence_digest.data()),
		  &evidence_length);
	bind_blob(&parameters[6], const_cast<uint8_t *>(plan.plan_digest.data()), &digest_length);
	bind_string(&parameters[7], const_cast<char *>(plan.actor.data()), &actor_length);
	bind_string(&parameters[8], const_cast<char *>(plan.reason.data()), &reason_length);
	bind_int(&parameters[9], MYSQL_TYPE_SHORT, &candidate_count, true);
	bind_int(&parameters[10], MYSQL_TYPE_SHORT, &delivered_count, true);
	bind_int(&parameters[11], MYSQL_TYPE_SHORT, &unresolved_count, true);
	const bool ok = mysql_stmt_bind_param(statement, parameters) == 0 &&
			mysql_stmt_execute(statement) == 0 &&
			mysql_stmt_affected_rows(statement) == 1;
	if (!ok)
		last_statement_error = mysql_stmt_errno(statement);
	mysql_stmt_close(statement);
	return ok;
}

bool insert_receipt_item(MYSQL *connection, const player_death_restitution_plan &plan,
			 const player_death_restitution_item &item, uint64_t delivery_epoch)
{
	static const char SQL[] =
		"INSERT INTO player_death_restitution_item(restitution_id,item_uid,source_root_item_uid,"
		"source_parent_item_uid,delivered_root_item_uid,delivered_parent_item_uid,source_item_revision,"
		"delivered_item_revision,vnum,artifact_vnum,disposition,classification,metadata_digest,"
		"metadata_payload,note,artifact_loss_epoch,artifact_source_timer_epoch,"
		"artifact_usable_lifetime_seconds,artifact_delivered_timer_epoch,artifact_timing_basis,"
		"artifact_compensation_reference) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint64_t delivered_revision =
		item.disposition == player_death_restitution_disposition::deliver ?
			item.expected_item_revision + 1 :
			0;
	uint16_t disposition = static_cast<uint16_t>(item.disposition);
	uint64_t delivered_timer_epoch = 0;
	const char *basis = "";
	const char *compensation = "";
	if (item.disposition == player_death_restitution_disposition::deliver && item.artifact_vnum)
	{
		if (!player_death_restitution_artifact_delivery_timer(item, delivery_epoch,
								      &delivered_timer_epoch))
			return false;
		basis = item.artifact_timing_evidence_present ? "historical_loss_remainder" :
								"approved_compensation";
		compensation = item.artifact_timing_uid_approved ? "uid-specific-approval" : "";
	}
	unsigned long rid_length = plan.restitution_id.bytes.size();
	unsigned long class_length = item.classification.size();
	unsigned long metadata_digest_length = item.metadata_digest.size();
	unsigned long metadata_length = item.metadata_payload.size();
	unsigned long note_length = item.note.size();
	unsigned long basis_length = strlen(basis);
	unsigned long compensation_length = strlen(compensation);
	uint32_t artifact_vnum = item.artifact_vnum;
	uint32_t vnum = item.vnum;
	MYSQL_BIND parameters[21] = {};
	using mysql_is_null_type = std::remove_pointer_t<decltype(parameters[12].is_null)>;
	mysql_is_null_type metadata_null = item.metadata_payload.empty();
	bind_blob(&parameters[0], const_cast<uint8_t *>(plan.restitution_id.bytes.data()),
		  &rid_length);
	bind_int(&parameters[1], MYSQL_TYPE_LONGLONG, const_cast<uint64_t *>(&item.item_uid), true);
	bind_int(&parameters[2], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.source_root_item_uid), true);
	bind_int(&parameters[3], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.source_parent_item_uid), true);
	bind_int(&parameters[4], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.delivered_root_item_uid), true);
	bind_int(&parameters[5], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.delivered_parent_item_uid), true);
	bind_int(&parameters[6], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.source_item_revision), true);
	bind_int(&parameters[7], MYSQL_TYPE_LONGLONG, &delivered_revision, true);
	bind_int(&parameters[8], MYSQL_TYPE_LONG, &vnum, false);
	bind_int(&parameters[9], MYSQL_TYPE_LONG, &artifact_vnum, false);
	bind_int(&parameters[10], MYSQL_TYPE_SHORT, &disposition, true);
	bind_string(&parameters[11], const_cast<char *>(item.classification.data()), &class_length);
	bind_blob(&parameters[12],
		  metadata_null ? nullptr : const_cast<uint8_t *>(item.metadata_digest.data()),
		  &metadata_digest_length);
	parameters[12].is_null = &metadata_null;
	bind_blob(&parameters[13],
		  metadata_null ? nullptr : const_cast<uint8_t *>(item.metadata_payload.data()),
		  &metadata_length);
	parameters[13].is_null = &metadata_null;
	bind_string(&parameters[14], const_cast<char *>(item.note.data()), &note_length);
	bind_int(&parameters[15], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.artifact_loss_epoch), true);
	bind_int(&parameters[16], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.artifact_source_timer_epoch), true);
	bind_int(&parameters[17], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.artifact_usable_lifetime_seconds), true);
	bind_int(&parameters[18], MYSQL_TYPE_LONGLONG, &delivered_timer_epoch, true);
	bind_string(&parameters[19], const_cast<char *>(basis), &basis_length);
	bind_string(&parameters[20], const_cast<char *>(compensation), &compensation_length);
	const bool ok = mysql_stmt_bind_param(statement, parameters) == 0 &&
			mysql_stmt_execute(statement) == 0 &&
			mysql_stmt_affected_rows(statement) == 1;
	if (!ok)
		last_statement_error = mysql_stmt_errno(statement);
	mysql_stmt_close(statement);
	return ok;
}

bool insert_player_item(MYSQL *connection, uint32_t recipient_pid,
			player_death_restitution_item_state state, uint64_t parent_id,
			projected_item *inserted)
{
	static const char SQL[] =
		"INSERT INTO player_items(pid,vnum,equip_slot,container_id,quantity,weight,cost,timer,"
		"extra_flags,wear_flags,item_type,value0,value1,value2,value3,value4,value5,value6,value7,"
		"name,short_descr,description,action_descr,bitvector1,bitvector2,bitvector3,bitvector4,"
		"bitvector5,item_material,obj_uid,item_condition) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
	if (!inserted || parent_id > UINT32_MAX ||
	    !player_death_restitution_item_state_valid(state))
		return false;
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint32_t vnum = state.vnum, container = static_cast<uint32_t>(parent_id);
	int16_t equip_slot = state.equip_slot, condition = state.condition;
	uint16_t quantity = state.quantity;
	int32_t weight = state.weight, cost = state.cost, timer = state.timer;
	uint64_t extra_flags = state.extra_flags, item_uid = state.item_uid;
	int32_t wear_flags = state.wear_flags;
	int8_t item_type = state.item_type, material = state.material;
	MYSQL_BIND parameters[31] = {};
	bind_int(&parameters[0], MYSQL_TYPE_LONG, &recipient_pid, true);
	bind_int(&parameters[1], MYSQL_TYPE_LONG, &vnum, false);
	bind_int(&parameters[2], MYSQL_TYPE_SHORT, &equip_slot, false);
	mysql_null_indicator container_null = parent_id == 0;
	bind_int(&parameters[3], MYSQL_TYPE_LONG, &container, true);
	parameters[3].is_null = &container_null;
	bind_int(&parameters[4], MYSQL_TYPE_SHORT, &quantity, true);
	bind_int(&parameters[5], MYSQL_TYPE_LONG, &weight, false);
	bind_int(&parameters[6], MYSQL_TYPE_LONG, &cost, false);
	bind_int(&parameters[7], MYSQL_TYPE_LONG, &timer, false);
	bind_int(&parameters[8], MYSQL_TYPE_LONGLONG, &extra_flags, true);
	bind_int(&parameters[9], MYSQL_TYPE_LONG, &wear_flags, false);
	bind_int(&parameters[10], MYSQL_TYPE_TINY, &item_type, false);
	for (size_t i = 0; i < state.values.size(); ++i)
		bind_int(&parameters[11 + i], MYSQL_TYPE_LONG, &state.values[i], false);
	std::array<unsigned long, 4> lengths = {};
	std::array<mysql_null_indicator, 4> nulls = {};
	for (size_t i = 0; i < state.strings.size(); ++i)
	{
		lengths[i] = state.strings[i].size();
		nulls[i] = !state.string_present[i];
		bind_string(&parameters[19 + i],
			    state.strings[i].empty() ?
				    nullptr :
				    const_cast<uint8_t *>(state.strings[i].data()),
			    &lengths[i], &nulls[i]);
	}
	std::array<mysql_null_indicator, 5> bit_nulls = {};
	for (size_t i = 0; i < state.bitvectors.size(); ++i)
	{
		bit_nulls[i] = !state.bitvector_present[i];
		bind_int(&parameters[23 + i], MYSQL_TYPE_LONGLONG, &state.bitvectors[i], true);
		parameters[23 + i].is_null = &bit_nulls[i];
	}
	bind_int(&parameters[28], MYSQL_TYPE_TINY, &material, false);
	bind_int(&parameters[29], MYSQL_TYPE_LONGLONG, &item_uid, true);
	bind_int(&parameters[30], MYSQL_TYPE_SHORT, &condition, false);
	if (mysql_stmt_bind_param(statement, parameters) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_affected_rows(statement) != 1)
		return statement_failure(statement);
	inserted->id = static_cast<uint32_t>(mysql_insert_id(connection));
	inserted->uid = item_uid;
	mysql_stmt_close(statement);
	if (!inserted->id ||
	    !player_death_restitution_item_state_encode(state, &inserted->state_payload))
		return false;
	SHA256(inserted->state_payload.data(), inserted->state_payload.size(),
	       inserted->state_digest.data());
	return true;
}

bool insert_affects(MYSQL *connection, uint32_t item_id,
		    const player_death_restitution_item_state &state)
{
	static const char SQL[] =
		"INSERT INTO player_item_affects(item_id,location,modifier) VALUES(?,?,?)";
	for (size_t i = 0; i < state.affects.size(); ++i)
	{
		const auto &affect = state.affects[i];
		if (affect.location == 0 && affect.modifier == 0)
			continue;
		bool duplicate = false;
		for (size_t j = 0; j < i; ++j)
			duplicate = duplicate || (state.affects[j].location == affect.location &&
						  state.affects[j].modifier == affect.modifier);
		if (duplicate)
			continue;
		MYSQL_STMT *statement = nullptr;
		if (!prepare(&statement, connection, SQL))
			return false;
		MYSQL_BIND parameters[3] = {};
		bind_int(&parameters[0], MYSQL_TYPE_LONG, &item_id, true);
		bind_int(&parameters[1], MYSQL_TYPE_SHORT, const_cast<int16_t *>(&affect.location));
		bind_int(&parameters[2], MYSQL_TYPE_SHORT, const_cast<int16_t *>(&affect.modifier));
		const bool ok = mysql_stmt_bind_param(statement, parameters) == 0 &&
				mysql_stmt_execute(statement) == 0;
		if (!ok)
			last_statement_error = mysql_stmt_errno(statement);
		mysql_stmt_close(statement);
		if (!ok)
			return false;
	}
	return true;
}

bool insert_descriptions(MYSQL *connection, uint32_t item_id,
			 const player_death_restitution_item_state &state)
{
	static const char SQL[] =
		"INSERT INTO player_item_extra_descr(item_id,keyword,description) VALUES(?,?,?)";
	for (size_t i = 0; i < state.extra_descriptions.size(); ++i)
	{
		const auto &description = state.extra_descriptions[i];
		bool duplicate = false;
		for (size_t j = 0; j < i; ++j)
			duplicate = duplicate ||
				    (state.extra_descriptions[j].keyword == description.keyword &&
				     state.extra_descriptions[j].description ==
					     description.description);
		if (duplicate)
			continue;
		MYSQL_STMT *statement = nullptr;
		if (!prepare(&statement, connection, SQL))
			return false;
		unsigned long keyword_length = description.keyword.size(),
			      text_length = description.description.size();
		MYSQL_BIND parameters[3] = {};
		bind_int(&parameters[0], MYSQL_TYPE_LONG, &item_id, true);
		bind_string(&parameters[1], const_cast<uint8_t *>(description.keyword.data()),
			    &keyword_length);
		bind_string(&parameters[2],
			    description.description.empty() ?
				    nullptr :
				    const_cast<uint8_t *>(description.description.data()),
			    &text_length);
		const bool ok = mysql_stmt_bind_param(statement, parameters) == 0 &&
				mysql_stmt_execute(statement) == 0;
		if (!ok)
			last_statement_error = mysql_stmt_errno(statement);
		mysql_stmt_close(statement);
		if (!ok)
			return false;
	}
	return true;
}

bool insert_delivery(MYSQL *connection, const player_death_restitution_plan &plan,
		     const player_death_restitution_item &item, uint32_t delivered_id)
{
	static const char SQL[] =
		"INSERT INTO player_death_restitution_delivery(item_uid,restitution_id,source_pid,death_revision,"
		"recipient_pid,source_item_revision,delivered_item_revision,delivered_item_id,metadata_digest,original_payload) "
		"VALUES(?,?,?,?,?,?,?,?,?,?)";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint32_t source_pid = plan.source_pid, recipient_pid = plan.recipient_pid;
	uint64_t death_revision = plan.death_revision,
		 delivered_revision = item.expected_item_revision + 1;
	// The delivery record's metadata_digest authenticates the immutable
	// original_payload.  item.metadata_digest belongs to the mutable native
	// metadata_payload (IST1) domain and is intentionally different.
	std::array<uint8_t, SHA256_DIGEST_LENGTH> original_payload_digest = {};
	SHA256(item.original_payload.data(), item.original_payload.size(),
	       original_payload_digest.data());
	unsigned long rid_length = plan.restitution_id.bytes.size(),
		      delivery_digest_length = original_payload_digest.size(),
		      original_length = item.original_payload.size();
	MYSQL_BIND parameters[10] = {};
	bind_int(&parameters[0], MYSQL_TYPE_LONGLONG, const_cast<uint64_t *>(&item.item_uid), true);
	bind_blob(&parameters[1], const_cast<uint8_t *>(plan.restitution_id.bytes.data()),
		  &rid_length);
	bind_int(&parameters[2], MYSQL_TYPE_LONG, &source_pid, true);
	bind_int(&parameters[3], MYSQL_TYPE_LONGLONG, &death_revision, true);
	bind_int(&parameters[4], MYSQL_TYPE_LONG, &recipient_pid, true);
	bind_int(&parameters[5], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.source_item_revision), true);
	bind_int(&parameters[6], MYSQL_TYPE_LONGLONG, &delivered_revision, true);
	bind_int(&parameters[7], MYSQL_TYPE_LONG, &delivered_id, true);
	bind_blob(&parameters[8], original_payload_digest.data(), &delivery_digest_length);
	bind_blob(&parameters[9], const_cast<uint8_t *>(item.original_payload.data()),
		  &original_length);
	const bool ok = mysql_stmt_bind_param(statement, parameters) == 0 &&
			mysql_stmt_execute(statement) == 0;
	if (!ok)
		last_statement_error = mysql_stmt_errno(statement);
	mysql_stmt_close(statement);
	return ok;
}

bool insert_runtime(MYSQL *connection, uint32_t recipient_pid, const projected_item &item)
{
	static const char SQL[] =
		"INSERT INTO player_death_restitution_runtime(item_uid,recipient_pid,state_payload,state_digest) VALUES(?,?,?,?)";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	unsigned long state_length = item.state_payload.size(),
		      digest_length = item.state_digest.size();
	MYSQL_BIND parameters[4] = {};
	bind_int(&parameters[0], MYSQL_TYPE_LONGLONG, const_cast<uint64_t *>(&item.uid), true);
	bind_int(&parameters[1], MYSQL_TYPE_LONG, &recipient_pid, true);
	bind_blob(&parameters[2], const_cast<uint8_t *>(item.state_payload.data()), &state_length);
	bind_blob(&parameters[3], const_cast<uint8_t *>(item.state_digest.data()), &digest_length);
	const bool ok = mysql_stmt_bind_param(statement, parameters) == 0 &&
			mysql_stmt_execute(statement) == 0;
	if (!ok)
		last_statement_error = mysql_stmt_errno(statement);
	mysql_stmt_close(statement);
	return ok;
}

bool increment_owner_revision(MYSQL *connection, uint32_t owner_id, uint64_t expected_revision,
			      uint64_t *revision)
{
	static const char SQL[] =
		"UPDATE item_owner_revision SET revision=revision+1 WHERE owner_type=1 AND owner_id=? AND "
		"owner_context_id=0 AND revision=?";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint64_t expected = expected_revision;
	MYSQL_BIND parameters[2] = {};
	bind_int(&parameters[0], MYSQL_TYPE_LONG, &owner_id, true);
	bind_int(&parameters[1], MYSQL_TYPE_LONGLONG, &expected, true);
	if (mysql_stmt_bind_param(statement, parameters) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_affected_rows(statement) != 1)
		return statement_failure(statement);
	mysql_stmt_close(statement);
	*revision = expected + 1;
	return true;
}

bool update_owner(MYSQL *connection, const player_death_restitution_plan &plan,
		  const player_death_restitution_item &item)
{
	static const char SQL[] =
		"UPDATE item_current_owner SET root_item_uid=?,parent_item_uid=?,owner_type=1,owner_id=?,"
		"owner_context_id=0,item_revision=?,state=1 WHERE item_uid=? AND owner_type=1 AND owner_id=? "
		"AND owner_context_id=0 AND root_item_uid=? AND COALESCE(parent_item_uid,0)=? AND item_revision=? "
		"AND state=? AND vnum=?";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint32_t recipient_pid = plan.recipient_pid, source_pid = plan.source_pid, vnum = item.vnum;
	uint64_t new_item_revision = item.expected_item_revision + 1;
	uint8_t expected_state = item.expected_owner_state;
	MYSQL_BIND parameters[11] = {};
	bind_int(&parameters[0], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.delivered_root_item_uid), true);
	mysql_null_indicator parent_null = item.delivered_parent_item_uid == 0;
	bind_int(&parameters[1], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.delivered_parent_item_uid), true);
	parameters[1].is_null = &parent_null;
	bind_int(&parameters[2], MYSQL_TYPE_LONG, &recipient_pid, true);
	bind_int(&parameters[3], MYSQL_TYPE_LONGLONG, &new_item_revision, true);
	bind_int(&parameters[4], MYSQL_TYPE_LONGLONG, const_cast<uint64_t *>(&item.item_uid), true);
	bind_int(&parameters[5], MYSQL_TYPE_LONG, &source_pid, true);
	bind_int(&parameters[6], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.source_root_item_uid), true);
	bind_int(&parameters[7], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.source_parent_item_uid), true);
	bind_int(&parameters[8], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.expected_item_revision), true);
	bind_int(&parameters[9], MYSQL_TYPE_TINY, &expected_state, true);
	bind_int(&parameters[10], MYSQL_TYPE_LONG, &vnum, false);
	if (mysql_stmt_bind_param(statement, parameters) != 0 ||
	    mysql_stmt_execute(statement) != 0 || mysql_stmt_affected_rows(statement) != 1)
		return statement_failure(statement);
	mysql_stmt_close(statement);
	return true;
}

bool insert_artifact_baseline(MYSQL *connection, const player_death_restitution_item &item,
			      const artifact_authority_lock &locked)
{
	if (locked.baseline_present ||
	    (item.artifact_domain_present && item.artifact_domain_item_uid_present))
		return true;
	static const char SQL[] =
		"INSERT INTO artifact_domain_baseline(vnum,opening_timer_epoch,opening_bind_owner_pid,"
		"opening_bind_timer_epoch,opening_revision) VALUES(?,?,?,?,?)";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	int32_t vnum = static_cast<int32_t>(item.artifact_vnum);
	int64_t opening_timer = static_cast<int64_t>(item.artifact_source_timer_epoch);
	int32_t opening_bind_owner = item.artifact_bind_owner_pid;
	int64_t opening_bind_timer = item.artifact_bind_timer_epoch;
	uint64_t opening_revision = 0;
	MYSQL_BIND parameters[5] = {};
	bind_int(&parameters[0], MYSQL_TYPE_LONG, &vnum, false);
	bind_int(&parameters[1], MYSQL_TYPE_LONGLONG, &opening_timer, false);
	bind_int(&parameters[2], MYSQL_TYPE_LONG, &opening_bind_owner, false);
	bind_int(&parameters[3], MYSQL_TYPE_LONGLONG, &opening_bind_timer, false);
	bind_int(&parameters[4], MYSQL_TYPE_LONGLONG, &opening_revision, true);
	const bool ok = mysql_stmt_bind_param(statement, parameters) == 0 &&
			mysql_stmt_execute(statement) == 0 &&
			mysql_stmt_affected_rows(statement) == 1;
	if (!ok)
		last_statement_error = mysql_stmt_errno(statement);
	mysql_stmt_close(statement);
	return ok;
}

bool update_artifact_domain(MYSQL *connection, const player_death_restitution_plan &plan,
			    const player_death_restitution_item &item,
			    const artifact_authority_lock &locked, uint64_t delivered_timer,
			    unsigned int *error_code)
{
	const uint64_t delivered_revision = item.expected_item_revision + 1;
	const uint8_t target_loc_type = PLAYER_DEATH_RESTITUTION_ARTIFACT_LOCATION_ON_PLAYER;
	if (!locked.domain_present)
	{
		static const char INSERT_SQL[] =
			"INSERT INTO artifact_domain_state(vnum,owned,loc_type,location,timer_epoch,"
			"artifact_type,bind_owner_pid,bind_timer_epoch,item_uid,item_revision,revision) "
			"VALUES(?,1,?,?,?,?,?,?,?,?,1)";
		MYSQL_STMT *statement = nullptr;
		if (!prepare(&statement, connection, INSERT_SQL))
			return false;
		int32_t vnum = static_cast<int32_t>(item.artifact_vnum);
		uint32_t recipient_pid = plan.recipient_pid;
		int64_t timer_epoch = static_cast<int64_t>(delivered_timer);
		int32_t bind_owner = item.artifact_bind_owner_pid;
		int64_t bind_timer = item.artifact_bind_timer_epoch;
		uint64_t item_uid = item.item_uid;
		MYSQL_BIND parameters[9] = {};
		bind_int(&parameters[0], MYSQL_TYPE_LONG, &vnum, false);
		bind_int(&parameters[1], MYSQL_TYPE_TINY, const_cast<uint8_t *>(&target_loc_type),
			 true);
		bind_int(&parameters[2], MYSQL_TYPE_LONG, &recipient_pid, true);
		bind_int(&parameters[3], MYSQL_TYPE_LONGLONG, &timer_epoch, false);
		bind_int(&parameters[4], MYSQL_TYPE_TINY,
			 const_cast<uint8_t *>(&item.artifact_type), true);
		bind_int(&parameters[5], MYSQL_TYPE_LONG, &bind_owner, false);
		bind_int(&parameters[6], MYSQL_TYPE_LONGLONG, &bind_timer, false);
		bind_int(&parameters[7], MYSQL_TYPE_LONGLONG, &item_uid, true);
		bind_int(&parameters[8], MYSQL_TYPE_LONGLONG,
			 const_cast<uint64_t *>(&delivered_revision), true);
		const bool ok = mysql_stmt_bind_param(statement, parameters) == 0 &&
				mysql_stmt_execute(statement) == 0 &&
				mysql_stmt_affected_rows(statement) == 1;
		if (!ok)
			last_statement_error = mysql_stmt_errno(statement);
		mysql_stmt_close(statement);
		return ok;
	}
	const std::string SQL =
		std::string(
			"UPDATE artifact_domain_state SET loc_type=?,location=?,timer_epoch=?,") +
		"item_uid=?,item_revision=?,revision=revision+1 WHERE vnum=? AND owned=1 AND "
		"loc_type=? AND location=? AND timer_epoch=? AND artifact_type=? AND "
		"bind_owner_pid=? AND bind_timer_epoch=? AND " +
		(item.artifact_domain_item_uid_present ? "item_uid=? AND " :
							 "item_uid IS NULL AND ") +
		"COALESCE(item_revision,0)=? AND revision=?";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL.c_str()))
		return false;
	int32_t vnum = static_cast<int32_t>(item.artifact_vnum);
	uint32_t recipient_pid = plan.recipient_pid;
	int64_t timer_epoch = static_cast<int64_t>(delivered_timer);
	uint64_t item_uid = item.item_uid;
	uint64_t delivered_item_revision = delivered_revision;
	int32_t source_location = item.artifact_source_location;
	int64_t source_timer = static_cast<int64_t>(item.artifact_source_timer_epoch);
	int32_t bind_owner = item.artifact_bind_owner_pid;
	int64_t bind_timer = item.artifact_bind_timer_epoch;
	MYSQL_BIND parameters[16] = {};
	unsigned int count = 0;
	bind_int(&parameters[count++], MYSQL_TYPE_TINY, const_cast<uint8_t *>(&target_loc_type),
		 true);
	bind_int(&parameters[count++], MYSQL_TYPE_LONG, &recipient_pid, true);
	bind_int(&parameters[count++], MYSQL_TYPE_LONGLONG, &timer_epoch, false);
	bind_int(&parameters[count++], MYSQL_TYPE_LONGLONG, &item_uid, true);
	bind_int(&parameters[count++], MYSQL_TYPE_LONGLONG, &delivered_item_revision, true);
	bind_int(&parameters[count++], MYSQL_TYPE_LONG, &vnum, false);
	bind_int(&parameters[count++], MYSQL_TYPE_TINY,
		 const_cast<uint8_t *>(&item.artifact_source_location_type), true);
	bind_int(&parameters[count++], MYSQL_TYPE_LONG, &source_location, false);
	bind_int(&parameters[count++], MYSQL_TYPE_LONGLONG, &source_timer, false);
	bind_int(&parameters[count++], MYSQL_TYPE_TINY, const_cast<uint8_t *>(&item.artifact_type),
		 true);
	bind_int(&parameters[count++], MYSQL_TYPE_LONG, &bind_owner, false);
	bind_int(&parameters[count++], MYSQL_TYPE_LONGLONG, &bind_timer, false);
	if (item.artifact_domain_item_uid_present)
		bind_int(&parameters[count++], MYSQL_TYPE_LONGLONG,
			 const_cast<uint64_t *>(&item.artifact_domain_item_uid), true);
	bind_int(&parameters[count++], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.artifact_domain_item_revision), true);
	bind_int(&parameters[count++], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.artifact_domain_revision), true);
	const bool ok = mysql_stmt_bind_param(statement, parameters) == 0 &&
			mysql_stmt_execute(statement) == 0;
	const my_ulonglong affected = ok ? mysql_stmt_affected_rows(statement) : 0;
	if (!ok)
	{
		last_statement_error = mysql_stmt_errno(statement);
		mysql_stmt_close(statement);
		return false;
	}
	mysql_stmt_close(statement);
	if (affected != 1)
		return mark_stale(error_code);
	return true;
}

bool update_artifact_legacy(MYSQL *connection, const player_death_restitution_plan &plan,
			    const player_death_restitution_item &item, const char *table,
			    uint64_t delivered_timer, unsigned int *error_code)
{
	const std::string SQL =
		std::string("UPDATE ") + table +
		" SET locType=3,location=?,timer=FROM_UNIXTIME(?),"
		"lastUpdate=DATE_ADD(CURRENT_TIMESTAMP,INTERVAL 1 SECOND) WHERE vnum=? AND owned='Y' "
		"AND locType=? AND location=? AND COALESCE(UNIX_TIMESTAMP(timer),0)=? AND type=?";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL.c_str()))
		return false;
	uint32_t recipient_pid = plan.recipient_pid;
	int64_t timer_epoch = static_cast<int64_t>(delivered_timer);
	int32_t vnum = static_cast<int32_t>(item.artifact_vnum);
	int32_t source_location = item.artifact_source_location;
	int64_t source_timer = static_cast<int64_t>(item.artifact_source_timer_epoch);
	uint8_t source_loc_type = item.artifact_source_location_type;
	uint8_t artifact_type = item.artifact_type;
	MYSQL_BIND parameters[7] = {};
	bind_int(&parameters[0], MYSQL_TYPE_LONG, &recipient_pid, true);
	bind_int(&parameters[1], MYSQL_TYPE_LONGLONG, &timer_epoch, false);
	bind_int(&parameters[2], MYSQL_TYPE_LONG, &vnum, false);
	bind_int(&parameters[3], MYSQL_TYPE_TINY, &source_loc_type, true);
	bind_int(&parameters[4], MYSQL_TYPE_LONG, &source_location, false);
	bind_int(&parameters[5], MYSQL_TYPE_LONGLONG, &source_timer, false);
	bind_int(&parameters[6], MYSQL_TYPE_TINY, &artifact_type, true);
	const bool ok = mysql_stmt_bind_param(statement, parameters) == 0 &&
			mysql_stmt_execute(statement) == 0;
	const my_ulonglong affected = ok ? mysql_stmt_affected_rows(statement) : 0;
	if (!ok)
	{
		last_statement_error = mysql_stmt_errno(statement);
		mysql_stmt_close(statement);
		return false;
	}
	mysql_stmt_close(statement);
	if (affected != 1)
		return mark_stale(error_code);
	return true;
}

bool apply_artifact_authority(MYSQL *connection, const player_death_restitution_plan &plan,
			      const player_death_restitution_item &item,
			      const artifact_authority_lock &locked, uint64_t delivery_epoch,
			      unsigned int *error_code)
{
	uint64_t delivered_timer = 0;
	if (!player_death_restitution_artifact_delivery_timer(item, delivery_epoch,
							      &delivered_timer))
		return false;
	if (!insert_artifact_baseline(connection, item, locked))
		return false;
	if (!update_artifact_domain(connection, plan, item, locked, delivered_timer, error_code))
		return false;
	if (*error_code)
		return true;
	if (item.artifact_legacy_projection_mask & PLAYER_DEATH_RESTITUTION_ARTIFACT_LEGACY_GOD &&
	    !update_artifact_legacy(connection, plan, item, "artifacts", delivered_timer,
				    error_code))
		return false;
	if (*error_code)
		return true;
	if (item.artifact_legacy_projection_mask &
		    PLAYER_DEATH_RESTITUTION_ARTIFACT_LEGACY_MORTAL &&
	    !update_artifact_legacy(connection, plan, item, "artifacts_mortal", delivered_timer,
				    error_code))
		return false;
	return true;
}

bool insert_ownership_ledger(MYSQL *connection, const critical_command &command,
			     const player_death_restitution_plan &plan,
			     const player_death_restitution_item &item, uint16_t event_index,
			     uint64_t to_owner_revision)
{
	static const char SQL[] =
		"INSERT INTO item_ownership_ledger(operation_id,event_index,item_uid,root_item_uid,parent_item_uid,"
		"from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,to_owner_id,to_owner_context_id,"
		"item_revision,from_owner_revision,to_owner_revision,reason_type,reason_id,source_site) "
		"VALUES(?,?,?,?,?,1,?,0,1,?,0,?,?,?,?,?,?)";
	MYSQL_STMT *statement = nullptr;
	if (!prepare(&statement, connection, SQL))
		return false;
	uint16_t reason_type = static_cast<uint16_t>(item_transfer_reason::death_restitution);
	uint16_t source_site = static_cast<uint16_t>(critical_source_site::recovery);
	uint32_t source_pid = plan.source_pid, recipient_pid = plan.recipient_pid;
	int64_t reason_id = static_cast<int64_t>(plan.death_revision);
	uint64_t item_revision = item.expected_item_revision + 1;
	unsigned long operation_length = command.operation_id.bytes.size();
	MYSQL_BIND parameters[13] = {};
	bind_blob(&parameters[0], const_cast<uint8_t *>(command.operation_id.bytes.data()),
		  &operation_length);
	bind_int(&parameters[1], MYSQL_TYPE_SHORT, &event_index, true);
	bind_int(&parameters[2], MYSQL_TYPE_LONGLONG, const_cast<uint64_t *>(&item.item_uid), true);
	bind_int(&parameters[3], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.delivered_root_item_uid), true);
	mysql_null_indicator parent_null = item.delivered_parent_item_uid == 0;
	bind_int(&parameters[4], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.delivered_parent_item_uid), true);
	parameters[4].is_null = &parent_null;
	bind_int(&parameters[5], MYSQL_TYPE_LONG, &source_pid, true);
	bind_int(&parameters[6], MYSQL_TYPE_LONG, &recipient_pid, true);
	bind_int(&parameters[7], MYSQL_TYPE_LONGLONG, &item_revision, true);
	bind_int(&parameters[8], MYSQL_TYPE_LONGLONG,
		 const_cast<uint64_t *>(&item.expected_owner_revision), true);
	bind_int(&parameters[9], MYSQL_TYPE_LONGLONG, &to_owner_revision, true);
	bind_int(&parameters[10], MYSQL_TYPE_SHORT, &reason_type, true);
	bind_int(&parameters[11], MYSQL_TYPE_LONGLONG, &reason_id, false);
	bind_int(&parameters[12], MYSQL_TYPE_SHORT, &source_site, true);
	// The five literal owner-type/context fields above intentionally keep the
	// statement's parameter list bounded to values that can vary per item.
	const bool ok = mysql_stmt_bind_param(statement, parameters) == 0 &&
			mysql_stmt_execute(statement) == 0 &&
			mysql_stmt_affected_rows(statement) == 1;
	if (!ok)
		last_statement_error = mysql_stmt_errno(statement);
	mysql_stmt_close(statement);
	return ok;
}

} // namespace

bool player_death_restitution_repository_validate_plan(const player_death_restitution_plan &plan,
						       unsigned int *error_code)
{
	if (error_code)
		*error_code = 0;
	if (!player_death_restitution_plan_valid(plan))
	{
		if (error_code)
			*error_code = EINVAL;
		return false;
	}
	return true;
}

bool player_death_restitution_repository_execute(MYSQL *connection, const critical_command &command,
						 player_death_restitution_result *result,
						 unsigned int *result_code, bool *mutation_applied)
{
	if (!connection || !result || !result_code || !mutation_applied)
		return false;
	*result = {};
	*result_code = 0;
	*mutation_applied = false;
	player_death_restitution_plan plan = {};
	if (!player_death_restitution_command_decode_payload(command, &plan))
	{
		*result_code = EINVAL;
		return true;
	}
	result->restitution_id = plan.restitution_id;
	result->source_pid = plan.source_pid;
	result->recipient_pid = plan.recipient_pid;
	result->candidate_count = static_cast<uint16_t>(plan.items.size());
	result->unresolved_count = static_cast<uint16_t>(std::count_if(
		plan.items.begin(), plan.items.end(), [](const auto &item)
		{ return item.disposition != player_death_restitution_disposition::deliver; }));
	result->delivered_count =
		static_cast<uint16_t>(plan.items.size() - result->unresolved_count);
	if (!player_death_restitution_repository_validate_plan(plan, result_code))
	{
		if (!*result_code)
			*result_code = EINVAL;
		return true;
	}
	if (!result->delivered_count)
	{
		*result_code = ENOTSUP;
		return true;
	}
	uint64_t target_revision = 0;
	if (!read_save_revision(connection, plan.recipient_pid, &target_revision, result_code))
		return false;
	if (*result_code || target_revision != plan.expected_recipient_save_revision)
	{
		*result_code = *result_code ? *result_code : ESTALE;
		return true;
	}
	if (!verify_death_evidence(connection, plan, result_code))
		return false;
	if (*result_code)
		return true;
	for (const auto &item : plan.items)
	{
		if (item.disposition != player_death_restitution_disposition::deliver)
			continue;
		if (!verify_custody_evidence(connection, plan, item, result_code))
			return false;
		if (*result_code)
			return true;
	}
	uint64_t source_owner_revision = 0, recipient_owner_revision = 0;
	const bool same_owner = plan.source_pid == plan.recipient_pid;
	if (same_owner)
	{
		if (!read_owner_revision(connection, plan.source_pid,
					 plan.expected_source_owner_revision,
					 &source_owner_revision, result_code))
			return false;
		recipient_owner_revision = source_owner_revision;
	}
	else if (plan.source_pid < plan.recipient_pid)
	{
		if (!read_owner_revision(connection, plan.source_pid,
					 plan.expected_source_owner_revision,
					 &source_owner_revision, result_code))
			return false;
		if (!*result_code && !ensure_owner_revision(connection, plan.recipient_pid))
			return false;
		if (!*result_code && !read_owner_revision(connection, plan.recipient_pid,
							  plan.expected_recipient_owner_revision,
							  &recipient_owner_revision, result_code))
			return false;
	}
	else
	{
		if (!ensure_owner_revision(connection, plan.recipient_pid))
			return false;
		if (!read_owner_revision(connection, plan.recipient_pid,
					 plan.expected_recipient_owner_revision,
					 &recipient_owner_revision, result_code))
			return false;
		if (!*result_code && !read_owner_revision(connection, plan.source_pid,
							  plan.expected_source_owner_revision,
							  &source_owner_revision, result_code))
			return false;
	}
	if (*result_code)
		return true;
	std::vector<size_t> order;
	std::vector<artifact_authority_lock> artifact_locks(plan.items.size());
	for (size_t index = 0; index < plan.items.size(); ++index)
		if (plan.items[index].disposition == player_death_restitution_disposition::deliver)
			order.push_back(index);
	for (size_t index : order)
	{
		owner_row owner = {};
		if (!lock_owner(connection, plan, plan.items[index], &owner, result_code))
			return false;
		if (*result_code)
			return true;
		bool exists = false;
		if (!uid_exists(
			    connection,
			    "SELECT 1 FROM player_death_restitution_delivery WHERE item_uid=? FOR UPDATE",
			    plan.items[index].item_uid, &exists))
			return false;
		if (exists)
		{
			*result_code = EEXIST;
			return true;
		}
		if (!uid_exists(connection, "SELECT 1 FROM player_items WHERE obj_uid=? FOR UPDATE",
				plan.items[index].item_uid, &exists))
			return false;
		if (exists)
		{
			*result_code = EEXIST;
			return true;
		}
		if (plan.items[index].artifact_vnum &&
		    !lock_artifact_authority(connection, plan.items[index], &artifact_locks[index],
					     result_code))
			return false;
		if (*result_code)
			return true;
	}
	// The delivery epoch is the mutation boundary: all evidence and FOR UPDATE
	// fences above have completed, but persistent receipt/projection writes have
	// not started.  It is intentionally not a commit timestamp; the small,
	// unavoidable mutation-to-COMMIT gap remains real elapsed lifetime.
	struct timespec now = {};
	if (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0)
		return false;
	result->delivery_epoch = static_cast<uint64_t>(now.tv_sec);
	if (!insert_receipt(connection, plan, result->candidate_count, result->delivered_count,
			    result->unresolved_count))
		return false;
	std::vector<projected_item> projected;
	projected.reserve(result->delivered_count);
	for (size_t item_index = 0; item_index < plan.items.size(); ++item_index)
	{
		const auto &item = plan.items[item_index];
		if (!insert_receipt_item(connection, plan, item, result->delivery_epoch))
			return false;
		if (item.disposition != player_death_restitution_disposition::deliver)
			continue;
		player_death_restitution_item_state state = {};
		if (!player_death_restitution_item_state_decode(
			    item.metadata_payload.data(), item.metadata_payload.size(), &state))
			return false;
		uint64_t parent_id = 0;
		if (item.delivered_parent_item_uid)
		{
			auto parent = std::find_if(
				projected.begin(), projected.end(), [&](const auto &entry)
				{ return entry.uid == item.delivered_parent_item_uid; });
			if (parent == projected.end())
				return false;
			parent_id = parent->id;
		}
		projected_item inserted = {};
		if (!insert_player_item(connection, plan.recipient_pid, state, parent_id,
					&inserted) ||
		    !insert_affects(connection, inserted.id, state) ||
		    !insert_descriptions(connection, inserted.id, state) ||
		    !insert_delivery(connection, plan, item, inserted.id) ||
		    !insert_runtime(connection, plan.recipient_pid, inserted))
			return false;
		if (item.artifact_vnum &&
		    !apply_artifact_authority(connection, plan, item, artifact_locks[item_index],
					      result->delivery_epoch, result_code))
			return false;
		if (*result_code)
			return true;
		projected.push_back(std::move(inserted));
	}
	uint64_t new_source_owner_revision = 0, new_recipient_owner_revision = 0;
	if (!increment_owner_revision(connection, plan.source_pid,
				      plan.expected_source_owner_revision,
				      &new_source_owner_revision))
		return false;
	if (same_owner)
		new_recipient_owner_revision = new_source_owner_revision;
	else if (!increment_owner_revision(connection, plan.recipient_pid,
					   plan.expected_recipient_owner_revision,
					   &new_recipient_owner_revision))
		return false;
	uint16_t event_index = 0;
	for (const auto &item : plan.items)
	{
		if (item.disposition != player_death_restitution_disposition::deliver)
			continue;
		if (!update_owner(connection, plan, item) ||
		    !insert_ownership_ledger(connection, command, plan, item, event_index++,
					     new_recipient_owner_revision))
			return false;
		result->durable_revision =
			std::max(result->durable_revision, item.expected_item_revision + 1);
	}
	result->durable_revision =
		std::max(result->durable_revision,
			 std::max(new_source_owner_revision, new_recipient_owner_revision));
	result->mutation_applied = true;
	*mutation_applied = true;
	return true;
}

critical_apply_result
player_death_restitution_repository_apply_in_transaction(MYSQL *connection,
							 const critical_command &command)
{
	player_death_restitution_result result = {};
	unsigned int result_code = 0;
	bool mutation_applied = false;
	if (!player_death_restitution_repository_execute(connection, command, &result, &result_code,
							 &mutation_applied))
	{
		const unsigned int error = database_error(connection);
		rollback(connection);
		return failure(error ? error : EIO);
	}
	std::array<uint8_t, PLAYER_DEATH_RESTITUTION_RESULT_BYTES> payload = {};
	if (!player_death_restitution_command_encode_result(result, &payload))
	{
		rollback(connection);
		return failure(EBADMSG);
	}
	if (mutation_applied && !critical_command_repository_insert_outbox_event(
					connection, command.operation_id, 0,
					PLAYER_DEATH_RESTITUTION_OUTBOX_DESTINATION,
					PLAYER_DEATH_RESTITUTION_OUTBOX_EVENT,
					PLAYER_DEATH_RESTITUTION_OUTBOX_PAYLOAD_VERSION,
					payload.data(), payload.size()))
	{
		const unsigned int error = database_error(connection);
		rollback(connection);
		return failure(error ? error : EIO);
	}
	if (!critical_command_repository_finish_inbox(connection, command, result.durable_revision,
						      result_code, payload.data(), payload.size()))
	{
		const unsigned int error = database_error(connection);
		rollback(connection);
		return failure(error ? error : EIO);
	}
	if (!exec_sql(connection, "COMMIT"))
	{
		const unsigned int error = mysql_errno(connection);
		if (!connection_error(error))
			rollback(connection);
		return { connection_error(error) ? critical_apply_outcome::ambiguous_commit :
						   failure(error).outcome,
			 result.durable_revision, error };
	}
	critical_apply_result applied = { result_code ? critical_apply_outcome::terminal_failure :
							critical_apply_outcome::applied,
					  result.durable_revision, result_code };
	applied.result_size = payload.size();
	std::copy(payload.begin(), payload.end(), applied.result_payload.begin());
	return applied;
}
